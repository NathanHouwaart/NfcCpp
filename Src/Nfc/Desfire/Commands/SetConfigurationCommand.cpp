/**
 * @file SetConfigurationCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire SetConfiguration command implementation
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/SetConfigurationCommand.h"
#include "Nfc/Desfire/Commands/ValueOperationCryptoUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Utils/DesfireCrypto.h"
#include "Error/DesfireError.h"
#include <cppdes/des.h>
#include <cppdes/des3.h>
#include <cppdes/descbc.h>
#include <cppdes/des3cbc.h>
#include <aes.hpp>

using namespace nfc;
using namespace crypto;

namespace
{
    constexpr uint8_t SET_CONFIGURATION_COMMAND_CODE = 0x5C;

    bool isAllZero(const etl::ivector<uint8_t>& data)
    {
        for (size_t i = 0; i < data.size(); ++i)
        {
            if (data[i] != 0x00U)
            {
                return false;
            }
        }
        return true;
    }
}

SetConfigurationCommand::SetConfigurationCommand(const SetConfigurationCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
    , pendingIv()
    , updateContextIv(false)
{
}

etl::string_view SetConfigurationCommand::name() const
{
    return "SetConfiguration";
}

etl::expected<DesfireRequest, error::Error> SetConfigurationCommand::buildRequest(const DesfireContext& context)
{
    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (!context.authenticated)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::AuthenticationError));
    }

    if (context.sessionKeyEnc.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    auto encryptedPayloadResult = buildEncryptedPayload(context);
    if (!encryptedPayloadResult)
    {
        return etl::unexpected(encryptedPayloadResult.error());
    }

    DesfireRequest request;
    request.commandCode = SET_CONFIGURATION_COMMAND_CODE;
    request.data.clear();
    request.data.push_back(static_cast<uint8_t>(options.subcommand));

    const etl::vector<uint8_t, 64>& payload = encryptedPayloadResult.value();
    for (size_t i = 0; i < payload.size(); ++i)
    {
        request.data.push_back(payload[i]);
    }
    request.expectedResponseLength = 0U;
    return request;
}

etl::expected<DesfireResult, error::Error> SetConfigurationCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (response.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    DesfireResult result;
    result.statusCode = response[0];
    result.data.clear();
    for (size_t i = 1U; i < response.size(); ++i)
    {
        result.data.push_back(response[i]);
    }

    if (!result.isSuccess())
    {
        return etl::unexpected(error::Error::fromDesfire(static_cast<error::DesfireError>(result.statusCode)));
    }

    if (updateContextIv)
    {
        context.iv.clear();
        for (size_t i = 0; i < pendingIv.size(); ++i)
        {
            context.iv.push_back(pendingIv[i]);
        }
    }

    stage = Stage::Complete;
    return result;
}

bool SetConfigurationCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void SetConfigurationCommand::reset()
{
    stage = Stage::Initial;
    pendingIv.clear();
    updateContextIv = false;
}

etl::expected<etl::vector<uint8_t, 64>, error::Error> SetConfigurationCommand::buildEncryptedPayload(
    const DesfireContext& context)
{
    const SessionCipher sessionCipher = resolveSessionCipher(context);
    if (sessionCipher == SessionCipher::UNKNOWN)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    etl::vector<uint8_t, 64> plaintext;

    switch (options.subcommand)
    {
        case SetConfigurationSubcommand::PiccConfiguration:
            plaintext.push_back(options.piccConfiguration);
            break;

        case SetConfigurationSubcommand::AtsConfiguration:
            if (options.ats.empty())
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
            }
            if (options.ats.size() > 20U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
            }
            for (size_t i = 0; i < options.ats.size(); ++i)
            {
                plaintext.push_back(options.ats[i]);
            }
            break;

        default:
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    etl::vector<uint8_t, 64> crcInput;
    crcInput.push_back(SET_CONFIGURATION_COMMAND_CODE);
    crcInput.push_back(static_cast<uint8_t>(options.subcommand));
    for (size_t i = 0; i < plaintext.size(); ++i)
    {
        crcInput.push_back(plaintext[i]);
    }

    const uint32_t crc32 = calculateCrc32Desfire(crcInput);
    valueop_detail::appendLe32(plaintext, crc32);

    const size_t blockSize = (sessionCipher == SessionCipher::AES) ? 16U : 8U;
    while ((plaintext.size() % blockSize) != 0U)
    {
        plaintext.push_back(0x00U);
    }

    return encryptPayload(plaintext, context, sessionCipher);
}

etl::expected<etl::vector<uint8_t, 64>, error::Error> SetConfigurationCommand::encryptPayload(
    const etl::ivector<uint8_t>& plaintext,
    const DesfireContext& context,
    SessionCipher cipher)
{
    if (plaintext.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
    if ((plaintext.size() % blockSize) != 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
    }

    etl::vector<uint8_t, 64> encrypted;

    if (useLegacySendMode(cipher))
    {
        etl::vector<uint8_t, 8> previousBlock;
        previousBlock.resize(8, 0x00);

        if (!context.iv.empty())
        {
            if (context.iv.size() != 8U || !isAllZero(context.iv))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }
        }

        for (size_t offset = 0; offset < plaintext.size(); offset += 8U)
        {
            uint8_t xoredBlock[8];
            for (size_t i = 0; i < 8U; ++i)
            {
                xoredBlock[i] = static_cast<uint8_t>(plaintext[offset + i] ^ previousBlock[i]);
            }

            uint8_t transformedBlock[8];
            if (cipher == SessionCipher::DES)
            {
                if (context.sessionKeyEnc.size() < 8U)
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
                }
                DesFireCrypto::desDecrypt(xoredBlock, context.sessionKeyEnc.data(), transformedBlock);
            }
            else if (cipher == SessionCipher::DES3_2K)
            {
                if (context.sessionKeyEnc.size() < 16U)
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
                }
                DesFireCrypto::des3Decrypt(xoredBlock, context.sessionKeyEnc.data(), transformedBlock);
            }
            else
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            for (size_t i = 0; i < 8U; ++i)
            {
                encrypted.push_back(transformedBlock[i]);
                previousBlock[i] = transformedBlock[i];
            }
        }

        updateContextIv = false;
        pendingIv.clear();
        return encrypted;
    }

    etl::vector<uint8_t, 16> iv;
    if (context.iv.empty())
    {
        iv.resize(blockSize, 0x00);
    }
    else
    {
        if (context.iv.size() != blockSize)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        for (size_t i = 0; i < context.iv.size(); ++i)
        {
            iv.push_back(context.iv[i]);
        }
    }

    if (cipher == SessionCipher::AES)
    {
        if (context.sessionKeyEnc.size() != 16U)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        for (size_t i = 0; i < plaintext.size(); ++i)
        {
            encrypted.push_back(plaintext[i]);
        }

        AES_ctx aesContext;
        AES_init_ctx_iv(&aesContext, context.sessionKeyEnc.data(), iv.data());
        AES_CBC_encrypt_buffer(&aesContext, encrypted.data(), encrypted.size());
    }
    else if (cipher == SessionCipher::DES)
    {
        if (context.sessionKeyEnc.size() < 8U)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        DESCBC desCbc(bytesToUint64(context.sessionKeyEnc.data()), bytesToUint64(iv.data()));
        for (size_t offset = 0; offset < plaintext.size(); offset += 8U)
        {
            const uint64_t block = bytesToUint64(plaintext.data() + offset);
            const uint64_t cipherBlock = desCbc.encrypt(block);
            uint8_t blockBytes[8];
            uint64ToBytes(cipherBlock, blockBytes);
            for (size_t i = 0; i < 8U; ++i)
            {
                encrypted.push_back(blockBytes[i]);
            }
        }
    }
    else if (cipher == SessionCipher::DES3_2K)
    {
        if (context.sessionKeyEnc.size() < 16U)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        DES3CBC des3Cbc(
            bytesToUint64(context.sessionKeyEnc.data()),
            bytesToUint64(context.sessionKeyEnc.data() + 8U),
            bytesToUint64(context.sessionKeyEnc.data()),
            bytesToUint64(iv.data()));

        for (size_t offset = 0; offset < plaintext.size(); offset += 8U)
        {
            const uint64_t block = bytesToUint64(plaintext.data() + offset);
            const uint64_t cipherBlock = des3Cbc.encrypt(block);
            uint8_t blockBytes[8];
            uint64ToBytes(cipherBlock, blockBytes);
            for (size_t i = 0; i < 8U; ++i)
            {
                encrypted.push_back(blockBytes[i]);
            }
        }
    }
    else if (cipher == SessionCipher::DES3_3K)
    {
        if (context.sessionKeyEnc.size() < 24U)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        DES3CBC des3Cbc(
            bytesToUint64(context.sessionKeyEnc.data()),
            bytesToUint64(context.sessionKeyEnc.data() + 8U),
            bytesToUint64(context.sessionKeyEnc.data() + 16U),
            bytesToUint64(iv.data()));

        for (size_t offset = 0; offset < plaintext.size(); offset += 8U)
        {
            const uint64_t block = bytesToUint64(plaintext.data() + offset);
            const uint64_t cipherBlock = des3Cbc.encrypt(block);
            uint8_t blockBytes[8];
            uint64ToBytes(cipherBlock, blockBytes);
            for (size_t i = 0; i < 8U; ++i)
            {
                encrypted.push_back(blockBytes[i]);
            }
        }
    }
    else
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    pendingIv.clear();
    for (size_t i = encrypted.size() - blockSize; i < encrypted.size(); ++i)
    {
        pendingIv.push_back(encrypted[i]);
    }
    updateContextIv = true;
    return encrypted;
}

SetConfigurationCommand::SessionCipher SetConfigurationCommand::resolveSessionCipher(const DesfireContext& context) const
{
    SessionCipher requestedCipher = SessionCipher::UNKNOWN;
    switch (options.sessionKeyType)
    {
        case DesfireKeyType::DES:
            requestedCipher = SessionCipher::DES;
            break;
        case DesfireKeyType::DES3_2K:
            requestedCipher = SessionCipher::DES3_2K;
            break;
        case DesfireKeyType::DES3_3K:
            requestedCipher = SessionCipher::DES3_3K;
            break;
        case DesfireKeyType::AES:
            requestedCipher = SessionCipher::AES;
            break;
        default:
            break;
    }

    if (requestedCipher == SessionCipher::UNKNOWN && options.authMode == DesfireAuthMode::AES)
    {
        requestedCipher = SessionCipher::AES;
    }

    SessionCipher inferredCipher = SessionCipher::UNKNOWN;
    switch (valueop_detail::resolveSessionCipher(context))
    {
        case valueop_detail::SessionCipher::DES:
            inferredCipher = SessionCipher::DES;
            break;
        case valueop_detail::SessionCipher::DES3_2K:
            inferredCipher = SessionCipher::DES3_2K;
            break;
        case valueop_detail::SessionCipher::DES3_3K:
            inferredCipher = SessionCipher::DES3_3K;
            break;
        case valueop_detail::SessionCipher::AES:
            inferredCipher = SessionCipher::AES;
            break;
        default:
            break;
    }

    if (requestedCipher != SessionCipher::UNKNOWN)
    {
        if (inferredCipher == SessionCipher::UNKNOWN || inferredCipher == requestedCipher)
        {
            return requestedCipher;
        }

        return inferredCipher;
    }

    return inferredCipher;
}

bool SetConfigurationCommand::useLegacySendMode(SessionCipher cipher) const
{
    if (options.authMode != DesfireAuthMode::LEGACY)
    {
        return false;
    }

    return cipher == SessionCipher::DES || cipher == SessionCipher::DES3_2K;
}

uint32_t SetConfigurationCommand::calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const
{
    return valueop_detail::calculateCrc32Desfire(data);
}
