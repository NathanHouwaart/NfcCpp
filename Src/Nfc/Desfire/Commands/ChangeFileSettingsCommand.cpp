/**
 * @file ChangeFileSettingsCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire change file settings command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/ChangeFileSettingsCommand.h"
#include "Nfc/Desfire/Commands/ValueOperationCryptoUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Utils/DesfireCrypto.h"
#include "Error/DesfireError.h"

using namespace nfc;
using namespace crypto;

namespace
{
    constexpr uint8_t CHANGE_FILE_SETTINGS_COMMAND_CODE = 0x5FU;

    bool isAllZero(const etl::ivector<uint8_t>& data)
    {
        for (size_t i = 0U; i < data.size(); ++i)
        {
            if (data[i] != 0x00U)
            {
                return false;
            }
        }
        return true;
    }

}

ChangeFileSettingsCommand::ChangeFileSettingsCommand(const ChangeFileSettingsCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
    , activeCommandCommunicationSettings(0x00U)
    , sessionCipher(SessionCipher::UNKNOWN)
    , pendingIv()
    , updateContextIv(false)
    , requestIv()
    , hasRequestIv(false)
{
}

etl::string_view ChangeFileSettingsCommand::name() const
{
    return "ChangeFileSettings";
}

etl::expected<DesfireRequest, error::Error> ChangeFileSettingsCommand::buildRequest(const DesfireContext& context)
{
    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (!validateOptions())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    uint8_t access1 = 0x00U;
    uint8_t access2 = 0x00U;
    if (!buildAccessRightsBytes(access1, access2))
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    activeCommandCommunicationSettings = resolveCommandCommunicationSettings(context);
    if (activeCommandCommunicationSettings == 0x01U)
    {
        // MAC-only secure messaging command path is not implemented yet.
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    pendingIv.clear();
    updateContextIv = false;
    requestIv.clear();
    hasRequestIv = false;
    sessionCipher = SessionCipher::UNKNOWN;

    DesfireRequest request;
    request.commandCode = CHANGE_FILE_SETTINGS_COMMAND_CODE;
    request.data.clear();
    request.expectedResponseLength = 0U;
    request.data.push_back(options.fileNo);

    if (activeCommandCommunicationSettings == 0x03U)
    {
        if (!context.authenticated || context.sessionKeyEnc.empty())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::AuthenticationError));
        }

        sessionCipher = resolveSessionCipher(context);
        if (sessionCipher == SessionCipher::UNKNOWN)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        auto encryptedPayloadResult = buildEncryptedPayload(context, access1, access2);
        if (!encryptedPayloadResult)
        {
            return etl::unexpected(encryptedPayloadResult.error());
        }

        const auto& encryptedPayload = encryptedPayloadResult.value();
        for (size_t i = 0U; i < encryptedPayload.size(); ++i)
        {
            request.data.push_back(encryptedPayload[i]);
        }
    }
    else
    {
        request.data.push_back(options.communicationSettings);
        request.data.push_back(access1);
        request.data.push_back(access2);

        if (context.authenticated && context.iv.size() == 16U && context.sessionKeyEnc.size() >= 16U)
        {
            hasRequestIv = deriveAesPlainRequestIv(context, access1, access2, requestIv);
        }
    }

    return request;
}

etl::expected<DesfireResult, error::Error> ChangeFileSettingsCommand::parseResponse(
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

    if (result.isSuccess() && activeCommandCommunicationSettings == 0x03U && updateContextIv)
    {
        if (sessionCipher == SessionCipher::AES)
        {
            auto nextIvResult = valueop_detail::deriveAesResponseIvForValueOperation(response, context, pendingIv);
            if (!nextIvResult)
            {
                return etl::unexpected(nextIvResult.error());
            }
            valueop_detail::setContextIv(context, nextIvResult.value());
        }
        else
        {
            valueop_detail::setContextIv(context, pendingIv);
        }
    }
    else if (result.isSuccess() && activeCommandCommunicationSettings == 0x00U && hasRequestIv)
    {
        auto nextIvResult = valueop_detail::deriveAesResponseIvForValueOperation(response, context, requestIv);
        if (!nextIvResult)
        {
            return etl::unexpected(nextIvResult.error());
        }
        valueop_detail::setContextIv(context, nextIvResult.value());
    }

    stage = Stage::Complete;
    return result;
}

bool ChangeFileSettingsCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void ChangeFileSettingsCommand::reset()
{
    stage = Stage::Initial;
    activeCommandCommunicationSettings = 0x00U;
    sessionCipher = SessionCipher::UNKNOWN;
    pendingIv.clear();
    updateContextIv = false;
    requestIv.clear();
    hasRequestIv = false;
}

bool ChangeFileSettingsCommand::validateOptions() const
{
    if (options.fileNo > 0x1FU)
    {
        return false;
    }

    if (options.communicationSettings != 0x00U &&
        options.communicationSettings != 0x01U &&
        options.communicationSettings != 0x03U)
    {
        return false;
    }

    if (options.readAccess > 0x0FU ||
        options.writeAccess > 0x0FU ||
        options.readWriteAccess > 0x0FU ||
        options.changeAccess > 0x0FU)
    {
        return false;
    }

    if (options.commandCommunicationSettings != 0x00U &&
        options.commandCommunicationSettings != 0x01U &&
        options.commandCommunicationSettings != 0x03U &&
        options.commandCommunicationSettings != 0xFFU)
    {
        return false;
    }

    return true;
}

bool ChangeFileSettingsCommand::buildAccessRightsBytes(uint8_t& access1, uint8_t& access2) const
{
    if (options.readAccess > 0x0FU ||
        options.writeAccess > 0x0FU ||
        options.readWriteAccess > 0x0FU ||
        options.changeAccess > 0x0FU)
    {
        return false;
    }

    access1 = static_cast<uint8_t>((options.readWriteAccess << 4U) | (options.changeAccess & 0x0FU));
    access2 = static_cast<uint8_t>((options.readAccess << 4U) | (options.writeAccess & 0x0FU));
    return true;
}

uint8_t ChangeFileSettingsCommand::resolveCommandCommunicationSettings(const DesfireContext& context) const
{
    if (options.commandCommunicationSettings != 0xFFU)
    {
        return options.commandCommunicationSettings;
    }

    return context.authenticated ? 0x03U : 0x00U;
}

ChangeFileSettingsCommand::SessionCipher ChangeFileSettingsCommand::resolveSessionCipher(const DesfireContext& context) const
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
        case DesfireKeyType::UNKNOWN:
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
        case valueop_detail::SessionCipher::AES:
            inferredCipher = SessionCipher::AES;
            break;
        case valueop_detail::SessionCipher::DES:
            inferredCipher = SessionCipher::DES;
            break;
        case valueop_detail::SessionCipher::DES3_2K:
            inferredCipher = SessionCipher::DES3_2K;
            break;
        case valueop_detail::SessionCipher::DES3_3K:
            inferredCipher = SessionCipher::DES3_3K;
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

bool ChangeFileSettingsCommand::useLegacySendMode(SessionCipher cipher) const
{
    if (options.authMode != DesfireAuthMode::LEGACY)
    {
        return false;
    }

    return cipher == SessionCipher::DES || cipher == SessionCipher::DES3_2K;
}

bool ChangeFileSettingsCommand::deriveAesPlainRequestIv(
    const DesfireContext& context,
    uint8_t access1,
    uint8_t access2,
    etl::vector<uint8_t, 16>& outIv) const
{
    etl::vector<uint8_t, 5> message;
    message.push_back(CHANGE_FILE_SETTINGS_COMMAND_CODE);
    message.push_back(options.fileNo);
    message.push_back(options.communicationSettings);
    message.push_back(access1);
    message.push_back(access2);
    return valueop_detail::deriveAesPlainRequestIv(context, message, outIv);
}

etl::expected<etl::vector<uint8_t, 32>, error::Error> ChangeFileSettingsCommand::buildEncryptedPayload(
    const DesfireContext& context,
    uint8_t access1,
    uint8_t access2)
{
    etl::vector<uint8_t, 32> plaintext;
    plaintext.push_back(options.communicationSettings);
    plaintext.push_back(access1);
    plaintext.push_back(access2);

    if (useLegacySendMode(sessionCipher))
    {
        const uint16_t crc16 = valueop_detail::calculateCrc16(plaintext);
        valueop_detail::appendLe16(plaintext, crc16);
    }
    else
    {
        etl::vector<uint8_t, 8> crcInput;
        crcInput.push_back(CHANGE_FILE_SETTINGS_COMMAND_CODE);
        crcInput.push_back(options.fileNo);
        crcInput.push_back(options.communicationSettings);
        crcInput.push_back(access1);
        crcInput.push_back(access2);

        const uint32_t crc32 = valueop_detail::calculateCrc32Desfire(crcInput);
        valueop_detail::appendLe32(plaintext, crc32);
    }

    const size_t blockSize = (sessionCipher == SessionCipher::AES) ? 16U : 8U;
    while ((plaintext.size() % blockSize) != 0U)
    {
        plaintext.push_back(0x00U);
    }

    if (useLegacySendMode(sessionCipher))
    {
        if (!context.iv.empty())
        {
            if (context.iv.size() != 8U || !isAllZero(context.iv))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }
        }

        etl::vector<uint8_t, 32> encrypted;
        etl::vector<uint8_t, 8> previousBlock;
        previousBlock.resize(8U, 0x00U);

        for (size_t offset = 0U; offset < plaintext.size(); offset += 8U)
        {
            uint8_t xoredBlock[8];
            for (size_t i = 0U; i < 8U; ++i)
            {
                xoredBlock[i] = static_cast<uint8_t>(plaintext[offset + i] ^ previousBlock[i]);
            }

            uint8_t transformedBlock[8];
            if (sessionCipher == SessionCipher::DES)
            {
                if (context.sessionKeyEnc.size() < 8U)
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
                }
                DesFireCrypto::desDecrypt(xoredBlock, context.sessionKeyEnc.data(), transformedBlock);
            }
            else if (sessionCipher == SessionCipher::DES3_2K)
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

            for (size_t i = 0U; i < 8U; ++i)
            {
                encrypted.push_back(transformedBlock[i]);
                previousBlock[i] = transformedBlock[i];
            }
        }

        pendingIv.clear();
        updateContextIv = false;
        return encrypted;
    }

    valueop_detail::SessionCipher valueOpCipher = valueop_detail::SessionCipher::UNKNOWN;
    switch (sessionCipher)
    {
        case SessionCipher::DES:
            valueOpCipher = valueop_detail::SessionCipher::DES;
            break;
        case SessionCipher::DES3_2K:
            valueOpCipher = valueop_detail::SessionCipher::DES3_2K;
            break;
        case SessionCipher::DES3_3K:
            valueOpCipher = valueop_detail::SessionCipher::DES3_3K;
            break;
        case SessionCipher::AES:
            valueOpCipher = valueop_detail::SessionCipher::AES;
            break;
        case SessionCipher::UNKNOWN:
        default:
            valueOpCipher = valueop_detail::SessionCipher::UNKNOWN;
            break;
    }

    auto encryptedResult = valueop_detail::encryptPayload(
        plaintext,
        context,
        valueOpCipher,
        pendingIv);
    if (!encryptedResult)
    {
        return etl::unexpected(encryptedResult.error());
    }

    updateContextIv = true;
    return encryptedResult.value();
}
