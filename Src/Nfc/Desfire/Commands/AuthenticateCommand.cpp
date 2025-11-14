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
#include <cstring>

using namespace nfc;
using namespace crypto;

AuthenticateCommand::AuthenticateCommand(const AuthenticateCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
    , encryptedChallenge()
    , encryptedResponse()
    , rndA()
    , rndB()
    , currentIv()
{
    // Initialize IV to zeros
    currentIv.resize(8, 0);
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
            // STEP 1: Send authenticate command with key number
            request.commandCode = static_cast<uint8_t>(options.mode);
            request.data.clear();
            request.data.push_back(options.keyNo);
            request.expectedResponseLength = 8; // Expect 8-byte encrypted challenge
            break;
            
        case Stage::ChallengeSent:
            // STEP 7: Send encrypted response (RndA || RndB')
            request.commandCode = 0xAF; // Additional frame
            request.data.clear();
            for (size_t i = 0; i < encryptedResponse.size(); ++i)
            {
                request.data.push_back(encryptedResponse[i]);
            }
            request.expectedResponseLength = 8; // Expect 8-byte encrypted RndA'
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
    result.statusCode = 0x00;
    
    switch (stage)
    {
        case Stage::Initial:
            // STEP 2: Received encrypted RndB from card
            if (response.size() < 8)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }
            
            // Save encrypted challenge
            encryptedChallenge.clear();
            for (size_t i = 0; i < 8; ++i)
            {
                encryptedChallenge.push_back(response[i]);
            }
            
            // STEP 3: Decrypt challenge to get RndB
            decryptChallenge();
            
            // STEP 4: Update IV for ISO mode
            if (options.mode == DesfireAuthMode::ISO)
            {
                // IV = encrypted RndB
                for (size_t i = 0; i < 8; ++i)
                {
                    currentIv[i] = encryptedChallenge[i];
                }
            }
            
            // STEP 5: Rotate RndB right by 1 byte
            rotateRight(rndB, 1);
            
            // STEP 6: Generate RndA
            generateRandom(rndA, 8);
            
            // STEP 7: Build and encrypt response
            generateAuthResponse();
            
            stage = Stage::ChallengeSent;
            result.statusCode = 0xAF; // Additional frame required
            break;
            
        case Stage::ChallengeSent:
            // STEP 11: Received encrypted RndA' from card
            if (response.size() < 8)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }
            
            // STEP 12-14: Verify authentication confirmation
            if (!verifyAuthConfirmation(response))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::AuthenticationFailed));
            }
            
            // STEP 15: Generate session key
            uint8_t sessionKeyRaw[16];
            
            // Rotate RndB back left by 1 byte to get original
            rotateLeft(rndB, 1);
            
            DesFireCrypto::generateSessionKey(rndA.data(), rndB.data(), sessionKeyRaw);
            
            // Store session key in context
            context.sessionKeyEnc.clear();
            for (size_t i = 0; i < 16; ++i)
            {
                context.sessionKeyEnc.push_back(sessionKeyRaw[i]);
            }
            
            // For legacy/ISO, MAC key same as encryption key
            context.sessionKeyMac = context.sessionKeyEnc;
            
            // Reset IV to zeros for future operations
            context.iv.clear();
            context.iv.resize(8, 0);
            
            // Mark as authenticated
            context.authenticated = true;
            context.keyNo = options.keyNo;
            context.commMode = CommMode::Enciphered;
            
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
    currentIv.resize(8, 0);
}

void AuthenticateCommand::generateAuthResponse()
{
    // STEP 7: Concatenate RndA || RndB'
    etl::vector<uint8_t, 16> plainResponse;
    for (size_t i = 0; i < rndA.size(); ++i)
    {
        plainResponse.push_back(rndA[i]);
    }
    for (size_t i = 0; i < rndB.size(); ++i)
    {
        plainResponse.push_back(rndB[i]);
    }
    
    // STEP 8: Encrypt with 3DES CBC
    encryptedResponse.clear();
    encryptedResponse.resize(16);
    
    if (options.mode == DesfireAuthMode::ISO)
    {
        // ISO mode: Use CBC with updated IV
        DesFireCrypto::des3CbcEncrypt(
            plainResponse.data(),
            16,
            options.key.data(),
            currentIv.data(),
            encryptedResponse.data());
        
        // STEP 9: Update IV = last 8 bytes of encrypted response
        for (size_t i = 0; i < 8; ++i)
        {
            currentIv[i] = encryptedResponse[8 + i];
        }
    }
    else
    {
        // Legacy mode: Use CBC with IV=0
        uint8_t zeroIv[8] = {0};
        DesFireCrypto::des3CbcEncrypt(
            plainResponse.data(),
            16,
            options.key.data(),
            zeroIv,
            encryptedResponse.data());
    }
}

bool AuthenticateCommand::verifyAuthConfirmation(const etl::ivector<uint8_t>& response)
{
    // STEP 12: Decrypt RndA'
    etl::vector<uint8_t, 8> decryptedRndA;
    decryptedRndA.resize(8);
    
    if (options.mode == DesfireAuthMode::ISO)
    {
        // ISO mode: Use CBC with current IV
        DesFireCrypto::des3CbcDecrypt(
            response.data(),
            8,
            options.key.data(),
            currentIv.data(),
            decryptedRndA.data());
    }
    else
    {
        // Legacy mode: Use ECB (single block)
        DesFireCrypto::des3Decrypt(
            response.data(),
            options.key.data(),
            decryptedRndA.data());
    }
    
    // STEP 13: Rotate right by 1 to get original RndA
    rotateRight(decryptedRndA, 1);
    
    // STEP 14: Compare with our original RndA
    if (decryptedRndA.size() != rndA.size())
        return false;
    
    for (size_t i = 0; i < rndA.size(); ++i)
    {
        if (decryptedRndA[i] != rndA[i])
            return false;
    }
    
    return true;
}

void AuthenticateCommand::decryptChallenge()
{
    // STEP 3: Decrypt encrypted RndB
    rndB.clear();
    rndB.resize(8);
    
    if (options.mode == DesfireAuthMode::ISO)
    {
        // ISO mode: Use CBC with IV=0
        uint8_t zeroIv[8] = {0};
        DesFireCrypto::des3CbcDecrypt(
            encryptedChallenge.data(),
            8,
            options.key.data(),
            zeroIv,
            rndB.data());
    }
    else
    {
        // Legacy mode: Use ECB (single block)
        DesFireCrypto::des3Decrypt(
            encryptedChallenge.data(),
            options.key.data(),
            rndB.data());
    }
}

void AuthenticateCommand::encrypt(const etl::ivector<uint8_t>& plaintext, etl::vector<uint8_t, 32>& ciphertext)
{
    // Not used in state machine flow, but keeping for potential future use
    ciphertext.clear();
    ciphertext.resize(plaintext.size());
    
    if (options.mode == DesfireAuthMode::ISO)
    {
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
    
    if (options.mode == DesfireAuthMode::ISO)
    {
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
        DesFireCrypto::des3CbcDecrypt(
            ciphertext.data(),
            ciphertext.size(),
            options.key.data(),
            zeroIv,
            plaintext.data());
    }
}
