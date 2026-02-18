/**
 * @file AuthenticateCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Authenticate command implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Desfire/Commands/AuthenticateCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Utils/DesfireCrypto.h"
#include "Error/DesfireError.h"
#include "Utils/Logging.h"
#include <cppdes/descbc.h>
#include <cppdes/des3cbc.h>
#include <aes.hpp>
#include <cstring>

using namespace nfc;
using namespace crypto;

namespace
{
    bool isDegenerate16ByteDesKey(const etl::ivector<uint8_t>& key)
    {
        if (key.size() != 16)
        {
            return false;
        }

        // Ignore parity/version bit (LSB) when comparing K1 and K2.
        for (size_t i = 0; i < 8; ++i)
        {
            if ((key[i] & 0xFE) != (key[i + 8] & 0xFE))
            {
                return false;
            }
        }

        return true;
    }
}

AuthenticateCommand::AuthenticateCommand(const AuthenticateCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
    , encryptedChallenge()
    , encryptedResponse()
    , rndA()
    , rndB()
    , currentIv()
{
    // DES/3DES keys carry version/parity metadata in bit 0 of each byte.
    // Normalize key bytes for cipher operations so auth is not sensitive to
    // caller-provided parity bit patterns.
    if (this->options.mode != DesfireAuthMode::AES && !this->options.key.empty())
    {
        DesFireCrypto::clearParityBits(this->options.key.data(), this->options.key.size());
    }

    // Initialize IV to zeros
    currentIv.resize(blockSize(), 0);
}

etl::string_view AuthenticateCommand::name() const
{
    return "Authenticate";
}

etl::expected<DesfireRequest, error::Error> AuthenticateCommand::buildRequest(const DesfireContext& context)
{
    DesfireRequest request;
    
    switch (stage)
    {
        case Stage::Initial:
            if (options.mode == DesfireAuthMode::AES && options.key.size() != 16)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
            }
            if (options.mode == DesfireAuthMode::LEGACY && options.key.size() != 8 && options.key.size() != 16)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
            }
            if (options.mode == DesfireAuthMode::ISO && options.key.size() != 8 &&
                options.key.size() != 16 && options.key.size() != 24)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
            }

            // STEP 1: Send authenticate command with key number
            request.commandCode = static_cast<uint8_t>(options.mode);
            request.data.clear();
            request.data.push_back(options.keyNo);
            request.expectedResponseLength = randomSize(); // 8-byte (DES/ISO) or 16-byte (AES) encrypted challenge
            break;
            
        case Stage::ChallengeSent:
            // STEP 7: Send encrypted response (RndA || RndB')
            request.commandCode = 0xAF; // Additional frame
            request.data.clear();
            for (size_t i = 0; i < encryptedResponse.size(); ++i)
            {
                request.data.push_back(encryptedResponse[i]);
            }
            request.expectedResponseLength = randomSize(); // 8-byte (DES/ISO) or 16-byte (AES) encrypted RndA'
            break;
            
        default:
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }
    
    return request;
}

etl::expected<DesfireResult, error::Error> AuthenticateCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    DesfireResult result;
    
    // Response format from wire: [Status][Data...]
    if (response.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }
    
    // Extract status code
    result.statusCode = response[0];
    
    // Check for errors
    if (!result.isSuccess() && !result.isAdditionalFrame())
    {
        return etl::unexpected(error::Error::fromDesfire(static_cast<error::DesfireError>(result.statusCode)));
    }
    
    // Extract data (skip status byte)
    etl::vector<uint8_t, 256> data;
    for (size_t i = 1; i < response.size(); ++i)
    {
        data.push_back(response[i]);
    }
    
    switch (stage)
    {
        case Stage::Initial:
            // STEP 2: Received encrypted RndB from card
            if (data.size() < randomSize())
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }
            
            // Save encrypted challenge
            encryptedChallenge.clear();
            for (size_t i = 0; i < randomSize(); ++i)
            {
                encryptedChallenge.push_back(data[i]);
            }
            
            // STEP 3: Decrypt challenge to get RndB
            // Note: IV is reset to 0 before decryption (per addons code)
            decryptChallenge();
            
            // STEP 4: Rotate RndB left by 1 byte (despite protocol saying "right")
            // Per addons code and actual protocol bytes: [a,b,c,...,h] -> [b,c,...,h,a]
            rotateLeft(rndB, 1);
            
            // STEP 6: Generate RndA
            generateRandom(rndA, randomSize());
            
            // STEP 7: Build and encrypt response
            generateAuthResponse();
            
            stage = Stage::ChallengeSent;
            result.statusCode = 0xAF; // Additional frame required
            break;
            
        case Stage::ChallengeSent:
            // STEP 11: Received encrypted RndA' from card
            if (data.size() < randomSize())
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }
            
            // STEP 12-14: Verify authentication confirmation
            if (!verifyAuthConfirmation(data))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::AuthenticationFailed));
            }
            
            // STEP 15: Generate session key
            uint8_t sessionKeyRaw16[16];
            uint8_t sessionKeyRaw24[24];
            uint8_t sessionKeyRaw8[8];

            // Rotate RndB back right by 1 byte to get original (reverse of left rotation)
            rotateRight(rndB, 1);

            // Store session key in context
            context.sessionKeyEnc.clear();

            if (usesSingleDesSessionKey())
            {
                // DES-style session key: RndA[0..3] || RndB[0..3]
                for (size_t i = 0; i < 4; ++i)
                {
                    sessionKeyRaw8[i] = rndA[i];
                    sessionKeyRaw8[i + 4] = rndB[i];
                }
                DesFireCrypto::clearParityBits(sessionKeyRaw8, sizeof(sessionKeyRaw8));

                for (size_t i = 0; i < sizeof(sessionKeyRaw8); ++i)
                {
                    context.sessionKeyEnc.push_back(sessionKeyRaw8[i]);
                }
            }
            else if (usesThreeKey3DesSessionKey())
            {
                // 3K3DES session key:
                // RndA[0..3]||RndB[0..3]||RndA[6..9]||RndB[6..9]||RndA[12..15]||RndB[12..15]
                for (size_t i = 0; i < 4; ++i)
                {
                    sessionKeyRaw24[i] = rndA[i];
                    sessionKeyRaw24[i + 4] = rndB[i];
                    sessionKeyRaw24[i + 8] = rndA[6 + i];
                    sessionKeyRaw24[i + 12] = rndB[6 + i];
                    sessionKeyRaw24[i + 16] = rndA[12 + i];
                    sessionKeyRaw24[i + 20] = rndB[12 + i];
                }
                DesFireCrypto::clearParityBits(sessionKeyRaw24, sizeof(sessionKeyRaw24));

                for (size_t i = 0; i < sizeof(sessionKeyRaw24); ++i)
                {
                    context.sessionKeyEnc.push_back(sessionKeyRaw24[i]);
                }
            }
            else if (options.mode == DesfireAuthMode::AES)
            {
                // AES session key: RndA[0..3]||RndB[0..3]||RndA[12..15]||RndB[12..15]
                for (size_t i = 0; i < 4; ++i)
                {
                    sessionKeyRaw16[i] = rndA[i];
                    sessionKeyRaw16[i + 4] = rndB[i];
                    sessionKeyRaw16[i + 8] = rndA[12 + i];
                    sessionKeyRaw16[i + 12] = rndB[12 + i];
                }

                for (size_t i = 0; i < sizeof(sessionKeyRaw16); ++i)
                {
                    context.sessionKeyEnc.push_back(sessionKeyRaw16[i]);
                }
            }
            else
            {
                // 2K3DES-style session key: RndA[0..3]||RndB[0..3]||RndA[4..7]||RndB[4..7]
                DesFireCrypto::generateSessionKey(rndA.data(), rndB.data(), sessionKeyRaw16);
                for (size_t i = 0; i < sizeof(sessionKeyRaw16); ++i)
                {
                    context.sessionKeyEnc.push_back(sessionKeyRaw16[i]);
                }
            }

            // For legacy/ISO, MAC key same as encryption key
            context.sessionKeyMac = context.sessionKeyEnc;
            
            // Reset IV to zeros for future operations
            context.iv.clear();
            context.iv.resize(blockSize(), 0);

            // Preserve encrypted RndB from auth stage for legacy DES-chain variants.
            context.sessionEncRndB.clear();
            for (size_t i = 0; i < encryptedChallenge.size(); ++i)
            {
                context.sessionEncRndB.push_back(encryptedChallenge[i]);
            }
            
            // Mark as authenticated
            context.authenticated = true;
            context.keyNo = options.keyNo;
            context.commMode = CommMode::Enciphered;
            switch (options.mode)
            {
                case DesfireAuthMode::LEGACY:
                    context.authScheme = SessionAuthScheme::Legacy;
                    break;
                case DesfireAuthMode::ISO:
                    context.authScheme = SessionAuthScheme::Iso;
                    break;
                case DesfireAuthMode::AES:
                    context.authScheme = SessionAuthScheme::Aes;
                    break;
                default:
                    context.authScheme = SessionAuthScheme::None;
                    break;
            }
            
            stage = Stage::Complete;
            result.statusCode = 0x00; // Success
            break;
            
        default:
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }
    
    return result;
}

bool AuthenticateCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void AuthenticateCommand::reset()
{
    stage = Stage::Initial;
    encryptedChallenge.clear();
    encryptedResponse.clear();
    rndA.clear();
    rndB.clear();
    currentIv.clear();
    currentIv.resize(blockSize(), 0);
}

void AuthenticateCommand::generateAuthResponse()
{
    // STEP 7: Concatenate RndA || RndB'
    etl::vector<uint8_t, 32> plainResponse;
    for (size_t i = 0; i < rndA.size(); ++i)
    {
        plainResponse.push_back(rndA[i]);
    }
    for (size_t i = 0; i < rndB.size(); ++i)
    {
        plainResponse.push_back(rndB[i]);
    }
    
    // STEP 8: Encrypt with DES/3DES/AES CBC
    encryptedResponse.clear();
    encryptedResponse.resize(plainResponse.size());

    if (options.mode == DesfireAuthMode::AES)
    {
        if (options.key.size() < 16 || encryptedChallenge.size() < 16)
        {
            encryptedResponse.clear();
            return;
        }

        for (size_t i = 0; i < plainResponse.size(); ++i)
        {
            encryptedResponse[i] = plainResponse[i];
        }

        uint8_t iv[16] = {0};
        for (size_t i = 0; i < 16; ++i)
        {
            iv[i] = encryptedChallenge[i];
        }

        AES_ctx aesContext;
        AES_init_ctx_iv(&aesContext, options.key.data(), iv);
        AES_CBC_encrypt_buffer(&aesContext, encryptedResponse.data(), encryptedResponse.size());
        return;
    }

    if (options.mode == DesfireAuthMode::LEGACY)
    {
        // Legacy authenticate (0x0A) uses MF3ICD40 "send mode":
        // for each block: Y_i = D_K(X_i XOR Y_{i-1}), Y_-1 = 0.
        uint8_t previous[8] = {0};

        if (usesSingleDesSessionKey())
        {
            if (options.key.size() < 8)
            {
                encryptedResponse.clear();
                return;
            }

            for (size_t offset = 0; offset < plainResponse.size(); offset += 8)
            {
                uint8_t xoredBlock[8];
                for (size_t i = 0; i < 8; ++i)
                {
                    xoredBlock[i] = static_cast<uint8_t>(plainResponse[offset + i] ^ previous[i]);
                }

                uint8_t transformed[8];
                DesFireCrypto::desDecrypt(xoredBlock, options.key.data(), transformed);

                for (size_t i = 0; i < 8; ++i)
                {
                    encryptedResponse[offset + i] = transformed[i];
                    previous[i] = transformed[i];
                }
            }
        }
        else
        {
            if (options.key.size() != 16)
            {
                encryptedResponse.clear();
                return;
            }

            for (size_t offset = 0; offset < plainResponse.size(); offset += 8)
            {
                uint8_t xoredBlock[8];
                for (size_t i = 0; i < 8; ++i)
                {
                    xoredBlock[i] = static_cast<uint8_t>(plainResponse[offset + i] ^ previous[i]);
                }

                uint8_t transformed[8];
                DesFireCrypto::des3Decrypt(xoredBlock, options.key.data(), transformed);

                for (size_t i = 0; i < 8; ++i)
                {
                    encryptedResponse[offset + i] = transformed[i];
                    previous[i] = transformed[i];
                }
            }
        }

        return;
    }

    // ISO mode: CBC chain continues from encrypted RndB; Legacy mode: IV=0
    uint8_t iv[8] = {0};
    if (options.mode == DesfireAuthMode::ISO)
    {
        for (size_t i = 0; i < 8; ++i)
        {
            iv[i] = encryptedChallenge[encryptedChallenge.size() - 8 + i];
        }
    }
    
    if (usesSingleDesSessionKey())
    {
        if (options.key.size() < 8)
        {
            encryptedResponse.clear();
            return;
        }

        DESCBC desCbc(bytesToUint64(options.key.data()), bytesToUint64(iv));
        for (size_t offset = 0; offset < plainResponse.size(); offset += 8)
        {
            const uint64_t block = bytesToUint64(plainResponse.data() + offset);
            const uint64_t cipherBlock = desCbc.encrypt(block);
            uint64ToBytes(cipherBlock, encryptedResponse.data() + offset);
        }
    }
    else if (usesThreeKey3DesSessionKey())
    {
        if (options.key.size() != 24)
        {
            encryptedResponse.clear();
            return;
        }

        DES3CBC des3Cbc(
            bytesToUint64(options.key.data()),
            bytesToUint64(options.key.data() + 8),
            bytesToUint64(options.key.data() + 16),
            bytesToUint64(iv));

        for (size_t offset = 0; offset < plainResponse.size(); offset += 8)
        {
            const uint64_t block = bytesToUint64(plainResponse.data() + offset);
            const uint64_t cipherBlock = des3Cbc.encrypt(block);
            uint64ToBytes(cipherBlock, encryptedResponse.data() + offset);
        }
    }
    else
    {
        if (options.key.size() != 16)
        {
            encryptedResponse.clear();
            return;
        }

        DesFireCrypto::des3CbcEncrypt(
            plainResponse.data(),
            plainResponse.size(),
            options.key.data(),
            iv,
            encryptedResponse.data());
    }
}

bool AuthenticateCommand::verifyAuthConfirmation(const etl::ivector<uint8_t>& response)
{
    // STEP 12: Decrypt RndA'
    etl::vector<uint8_t, 16> decryptedRndA;
    decryptedRndA.resize(randomSize());

    if (options.mode == DesfireAuthMode::AES)
    {
        if (options.key.size() < 16 || response.size() < 16 || encryptedResponse.size() < 16)
        {
            return false;
        }

        for (size_t i = 0; i < 16; ++i)
        {
            decryptedRndA[i] = response[i];
        }

        uint8_t iv[16];
        for (size_t i = 0; i < 16; ++i)
        {
            iv[i] = encryptedResponse[encryptedResponse.size() - 16 + i];
        }

        AES_ctx aesContext;
        AES_init_ctx_iv(&aesContext, options.key.data(), iv);
        AES_CBC_decrypt_buffer(&aesContext, decryptedRndA.data(), decryptedRndA.size());
    }
    else if (options.mode == DesfireAuthMode::ISO)
    {
        // ISO mode: CBC chain continues from last encrypted block we sent
        uint8_t iv[8];
        for (size_t i = 0; i < 8; ++i)
        {
            iv[i] = encryptedResponse[encryptedResponse.size() - 8 + i];
        }

        if (usesSingleDesSessionKey())
        {
            if (options.key.size() < 8)
            {
                return false;
            }

            DESCBC desCbc(bytesToUint64(options.key.data()), bytesToUint64(iv));
            const uint64_t plainBlock = desCbc.decrypt(bytesToUint64(response.data()));
            uint64ToBytes(plainBlock, decryptedRndA.data());
        }
        else if (usesThreeKey3DesSessionKey())
        {
            if (options.key.size() != 24)
            {
                return false;
            }

            DES3CBC des3Cbc(
                bytesToUint64(options.key.data()),
                bytesToUint64(options.key.data() + 8),
                bytesToUint64(options.key.data() + 16),
                bytesToUint64(iv));

            for (size_t offset = 0; offset < randomSize(); offset += 8)
            {
                const uint64_t plainBlock = des3Cbc.decrypt(bytesToUint64(response.data() + offset));
                uint64ToBytes(plainBlock, decryptedRndA.data() + offset);
            }
        }
        else
        {
            if (options.key.size() != 16)
            {
                return false;
            }

            DesFireCrypto::des3CbcDecrypt(
                response.data(),
                8,
                options.key.data(),
                iv,
                decryptedRndA.data());
        }
    }
    else
    {
        // Legacy mode: Use ECB (single block)
        if (usesSingleDesSessionKey())
        {
            if (options.key.size() < 8)
            {
                return false;
            }

            DesFireCrypto::desDecrypt(
                response.data(),
                options.key.data(),
                decryptedRndA.data());
        }
        else
        {
            if (options.key.size() != 16)
            {
                return false;
            }

            DesFireCrypto::des3Decrypt(
                response.data(),
                options.key.data(),
                decryptedRndA.data());
        }
    }
    
    // STEP 13: Rotate right by 1 to get original RndA
    rotateRight(decryptedRndA, 1);
    
    // STEP 14: Compare with our original RndA
    if (decryptedRndA.size() != rndA.size())
    {
        LOG_ERROR("RndA size mismatch");
        return false;
    }
    
    for (size_t i = 0; i < rndA.size(); ++i)
    {
        if (decryptedRndA[i] != rndA[i])
        {
            LOG_ERROR("RndA verification failed at byte %zu: expected 0x%02X, got 0x%02X", i, rndA[i], decryptedRndA[i]);
            return false;
        }
    }
    
    LOG_INFO("Authentication verified successfully!");
    return true;
}

void AuthenticateCommand::decryptChallenge()
{
    // STEP 3: Decrypt encrypted RndB
    rndB.clear();
    rndB.resize(randomSize());

    if (options.mode == DesfireAuthMode::AES)
    {
        if (options.key.size() < 16 || encryptedChallenge.size() < 16)
        {
            rndB.clear();
            return;
        }

        for (size_t i = 0; i < 16; ++i)
        {
            rndB[i] = encryptedChallenge[i];
        }

        uint8_t zeroIv[16] = {0};
        AES_ctx aesContext;
        AES_init_ctx_iv(&aesContext, options.key.data(), zeroIv);
        AES_CBC_decrypt_buffer(&aesContext, rndB.data(), rndB.size());
        return;
    }
    
    if (options.mode == DesfireAuthMode::ISO)
    {
        // ISO mode: Use CBC with IV=0
        uint8_t zeroIv[8] = {0};

        if (usesSingleDesSessionKey())
        {
            if (options.key.size() < 8)
            {
                rndB.clear();
                return;
            }

            DESCBC desCbc(bytesToUint64(options.key.data()), bytesToUint64(zeroIv));
            const uint64_t plainBlock = desCbc.decrypt(bytesToUint64(encryptedChallenge.data()));
            uint64ToBytes(plainBlock, rndB.data());
        }
        else if (usesThreeKey3DesSessionKey())
        {
            if (options.key.size() != 24)
            {
                rndB.clear();
                return;
            }

            DES3CBC des3Cbc(
                bytesToUint64(options.key.data()),
                bytesToUint64(options.key.data() + 8),
                bytesToUint64(options.key.data() + 16),
                bytesToUint64(zeroIv));

            for (size_t offset = 0; offset < randomSize(); offset += 8)
            {
                const uint64_t plainBlock = des3Cbc.decrypt(bytesToUint64(encryptedChallenge.data() + offset));
                uint64ToBytes(plainBlock, rndB.data() + offset);
            }
        }
        else
        {
            if (options.key.size() != 16)
            {
                rndB.clear();
                return;
            }

            DesFireCrypto::des3CbcDecrypt(
                encryptedChallenge.data(),
                randomSize(),
                options.key.data(),
                zeroIv,
                rndB.data());
        }
    }
    else
    {
        // Legacy mode: Use ECB (single block)
        if (usesSingleDesSessionKey())
        {
            if (options.key.size() < 8)
            {
                rndB.clear();
                return;
            }

            DesFireCrypto::desDecrypt(
                encryptedChallenge.data(),
                options.key.data(),
                rndB.data());
        }
        else
        {
            if (options.key.size() != 16)
            {
                rndB.clear();
                return;
            }

            DesFireCrypto::des3Decrypt(
                encryptedChallenge.data(),
                options.key.data(),
                rndB.data());
        }
    }
}

bool AuthenticateCommand::usesSingleDesSessionKey() const
{
    if (options.mode == DesfireAuthMode::AES)
    {
        return false;
    }

    if (options.key.size() == 8)
    {
        return true;
    }

    return isDegenerate16ByteDesKey(options.key);
}

bool AuthenticateCommand::usesThreeKey3DesSessionKey() const
{
    return options.mode == DesfireAuthMode::ISO && options.key.size() == 24;
}

size_t AuthenticateCommand::randomSize() const
{
    if (options.mode == DesfireAuthMode::AES || usesThreeKey3DesSessionKey())
    {
        return 16U;
    }

    return 8U;
}

size_t AuthenticateCommand::blockSize() const
{
    return options.mode == DesfireAuthMode::AES ? 16U : 8U;
}

void AuthenticateCommand::encrypt(const etl::ivector<uint8_t>& plaintext, etl::vector<uint8_t, 32>& ciphertext)
{
    // Not used in state machine flow, but keeping for potential future use
    ciphertext.clear();
    ciphertext.resize(plaintext.size());

    if (options.mode == DesfireAuthMode::AES)
    {
        if (options.key.size() < 16 || currentIv.size() < 16)
        {
            ciphertext.clear();
            return;
        }

        for (size_t i = 0; i < plaintext.size(); ++i)
        {
            ciphertext[i] = plaintext[i];
        }

        AES_ctx aesContext;
        AES_init_ctx_iv(&aesContext, options.key.data(), currentIv.data());
        AES_CBC_encrypt_buffer(&aesContext, ciphertext.data(), ciphertext.size());
        return;
    }

    if (options.mode == DesfireAuthMode::ISO)
    {
        if (currentIv.size() < 8)
        {
            ciphertext.clear();
            return;
        }

        if (usesThreeKey3DesSessionKey())
        {
            if (options.key.size() != 24)
            {
                ciphertext.clear();
                return;
            }

            DES3CBC des3Cbc(
                bytesToUint64(options.key.data()),
                bytesToUint64(options.key.data() + 8),
                bytesToUint64(options.key.data() + 16),
                bytesToUint64(currentIv.data()));

            for (size_t offset = 0; offset < plaintext.size(); offset += 8)
            {
                const uint64_t block = bytesToUint64(plaintext.data() + offset);
                const uint64_t cipherBlock = des3Cbc.encrypt(block);
                uint64ToBytes(cipherBlock, ciphertext.data() + offset);
            }
            return;
        }

        if (options.key.size() != 16)
        {
            ciphertext.clear();
            return;
        }

        DesFireCrypto::des3CbcEncrypt(
            plaintext.data(),
            plaintext.size(),
            options.key.data(),
            currentIv.data(),
            ciphertext.data());
    }
    else
    {
        uint8_t zeroIv[8] = {0};

        if (usesSingleDesSessionKey())
        {
            if (options.key.size() < 8)
            {
                ciphertext.clear();
                return;
            }

            DESCBC desCbc(bytesToUint64(options.key.data()), bytesToUint64(zeroIv));
            for (size_t offset = 0; offset < plaintext.size(); offset += 8)
            {
                const uint64_t block = bytesToUint64(plaintext.data() + offset);
                const uint64_t cipherBlock = desCbc.encrypt(block);
                uint64ToBytes(cipherBlock, ciphertext.data() + offset);
            }
            return;
        }

        if (options.key.size() != 16)
        {
            ciphertext.clear();
            return;
        }

        DesFireCrypto::des3CbcEncrypt(
            plaintext.data(),
            plaintext.size(),
            options.key.data(),
            zeroIv,
            ciphertext.data());
    }
}

void AuthenticateCommand::decrypt(const etl::ivector<uint8_t>& ciphertext, etl::vector<uint8_t, 32>& plaintext)
{
    // Not used in state machine flow, but keeping for potential future use
    plaintext.clear();
    plaintext.resize(ciphertext.size());

    if (options.mode == DesfireAuthMode::AES)
    {
        if (options.key.size() < 16 || currentIv.size() < 16)
        {
            plaintext.clear();
            return;
        }

        for (size_t i = 0; i < ciphertext.size(); ++i)
        {
            plaintext[i] = ciphertext[i];
        }

        AES_ctx aesContext;
        AES_init_ctx_iv(&aesContext, options.key.data(), currentIv.data());
        AES_CBC_decrypt_buffer(&aesContext, plaintext.data(), plaintext.size());
        return;
    }

    if (options.mode == DesfireAuthMode::ISO)
    {
        if (currentIv.size() < 8)
        {
            plaintext.clear();
            return;
        }

        if (usesThreeKey3DesSessionKey())
        {
            if (options.key.size() != 24)
            {
                plaintext.clear();
                return;
            }

            DES3CBC des3Cbc(
                bytesToUint64(options.key.data()),
                bytesToUint64(options.key.data() + 8),
                bytesToUint64(options.key.data() + 16),
                bytesToUint64(currentIv.data()));

            for (size_t offset = 0; offset < ciphertext.size(); offset += 8)
            {
                const uint64_t plainBlock = des3Cbc.decrypt(bytesToUint64(ciphertext.data() + offset));
                uint64ToBytes(plainBlock, plaintext.data() + offset);
            }
            return;
        }

        if (options.key.size() != 16)
        {
            plaintext.clear();
            return;
        }

        DesFireCrypto::des3CbcDecrypt(
            ciphertext.data(),
            ciphertext.size(),
            options.key.data(),
            currentIv.data(),
            plaintext.data());
    }
    else
    {
        uint8_t zeroIv[8] = {0};

        if (usesSingleDesSessionKey())
        {
            if (options.key.size() < 8)
            {
                plaintext.clear();
                return;
            }

            DESCBC desCbc(bytesToUint64(options.key.data()), bytesToUint64(zeroIv));
            for (size_t offset = 0; offset < ciphertext.size(); offset += 8)
            {
                const uint64_t plainBlock = desCbc.decrypt(bytesToUint64(ciphertext.data() + offset));
                uint64ToBytes(plainBlock, plaintext.data() + offset);
            }
            return;
        }

        if (options.key.size() != 16)
        {
            plaintext.clear();
            return;
        }

        DesFireCrypto::des3CbcDecrypt(
            ciphertext.data(),
            ciphertext.size(),
            options.key.data(),
            zeroIv,
            plaintext.data());
    }
}
