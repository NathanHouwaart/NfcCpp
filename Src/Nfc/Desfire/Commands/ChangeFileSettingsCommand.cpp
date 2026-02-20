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
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t CHANGE_FILE_SETTINGS_COMMAND_CODE = 0x5FU;
}

ChangeFileSettingsCommand::ChangeFileSettingsCommand(const ChangeFileSettingsCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
    , activeCommandCommunicationSettings(0x00U)
    , sessionCipher(SecureMessagingPolicy::SessionCipher::UNKNOWN)
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
    sessionCipher = SecureMessagingPolicy::SessionCipher::UNKNOWN;

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
        if (sessionCipher == SecureMessagingPolicy::SessionCipher::UNKNOWN)
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

        if (context.authenticated && !context.sessionKeyEnc.empty())
        {
            etl::vector<uint8_t, 5> message;
            message.push_back(CHANGE_FILE_SETTINGS_COMMAND_CODE);
            message.push_back(options.fileNo);
            message.push_back(options.communicationSettings);
            message.push_back(access1);
            message.push_back(access2);

            auto requestIvResult = SecureMessagingPolicy::derivePlainRequestIv(context, message);
            if (requestIvResult)
            {
                requestIv.clear();
                for (size_t i = 0U; i < requestIvResult.value().size(); ++i)
                {
                    requestIv.push_back(requestIvResult.value()[i]);
                }
                hasRequestIv = true;
            }
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

    if (result.isSuccess() && activeCommandCommunicationSettings == 0x03U)
    {
        SecureMessagingPolicy::EncryptedPayloadProtection protection;
        for (size_t i = 0U; i < pendingIv.size(); ++i)
        {
            protection.requestState.push_back(pendingIv[i]);
        }
        protection.updateContextIv = updateContextIv;

        auto ivUpdateResult = SecureMessagingPolicy::updateContextIvForEncryptedCommandResponse(
            context,
            response,
            protection);
        if (!ivUpdateResult)
        {
            return etl::unexpected(ivUpdateResult.error());
        }
    }
    else if (result.isSuccess() && activeCommandCommunicationSettings == 0x00U && hasRequestIv)
    {
        auto nextIvResult = SecureMessagingPolicy::derivePlainResponseIv(context, response, requestIv);
        if (!nextIvResult)
        {
            return etl::unexpected(nextIvResult.error());
        }

        context.iv.clear();
        for (size_t i = 0U; i < nextIvResult.value().size(); ++i)
        {
            context.iv.push_back(nextIvResult.value()[i]);
        }
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
    sessionCipher = SecureMessagingPolicy::SessionCipher::UNKNOWN;
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

SecureMessagingPolicy::SessionCipher ChangeFileSettingsCommand::resolveSessionCipher(const DesfireContext& context) const
{
    SecureMessagingPolicy::SessionCipher requestedCipher = SecureMessagingPolicy::SessionCipher::UNKNOWN;
    switch (options.sessionKeyType)
    {
        case DesfireKeyType::DES:
            requestedCipher = SecureMessagingPolicy::SessionCipher::DES;
            break;
        case DesfireKeyType::DES3_2K:
            requestedCipher = SecureMessagingPolicy::SessionCipher::DES3_2K;
            break;
        case DesfireKeyType::DES3_3K:
            requestedCipher = SecureMessagingPolicy::SessionCipher::DES3_3K;
            break;
        case DesfireKeyType::AES:
            requestedCipher = SecureMessagingPolicy::SessionCipher::AES;
            break;
        case DesfireKeyType::UNKNOWN:
        default:
            break;
    }

    if (requestedCipher == SecureMessagingPolicy::SessionCipher::UNKNOWN && options.authMode == DesfireAuthMode::AES)
    {
        requestedCipher = SecureMessagingPolicy::SessionCipher::AES;
    }

    const auto inferredCipher = SecureMessagingPolicy::resolveSessionCipher(context);

    if (requestedCipher != SecureMessagingPolicy::SessionCipher::UNKNOWN)
    {
        if (inferredCipher == SecureMessagingPolicy::SessionCipher::UNKNOWN || inferredCipher == requestedCipher)
        {
            return requestedCipher;
        }

        return inferredCipher;
    }

    return inferredCipher;
}

bool ChangeFileSettingsCommand::useLegacySendMode(SecureMessagingPolicy::SessionCipher cipher) const
{
    if (options.authMode != DesfireAuthMode::LEGACY)
    {
        return false;
    }

    return cipher == SecureMessagingPolicy::SessionCipher::DES ||
           cipher == SecureMessagingPolicy::SessionCipher::DES3_2K;
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
        const uint16_t crc16 = SecureMessagingPolicy::calculateCrc16(plaintext);
        plaintext.push_back(static_cast<uint8_t>(crc16 & 0xFFU));
        plaintext.push_back(static_cast<uint8_t>((crc16 >> 8U) & 0xFFU));
    }
    else
    {
        etl::vector<uint8_t, 8> crcInput;
        crcInput.push_back(CHANGE_FILE_SETTINGS_COMMAND_CODE);
        crcInput.push_back(options.fileNo);
        crcInput.push_back(options.communicationSettings);
        crcInput.push_back(access1);
        crcInput.push_back(access2);

        const uint32_t crc32 = SecureMessagingPolicy::calculateCrc32Desfire(crcInput);
        plaintext.push_back(static_cast<uint8_t>(crc32 & 0xFFU));
        plaintext.push_back(static_cast<uint8_t>((crc32 >> 8U) & 0xFFU));
        plaintext.push_back(static_cast<uint8_t>((crc32 >> 16U) & 0xFFU));
        plaintext.push_back(static_cast<uint8_t>((crc32 >> 24U) & 0xFFU));
    }

    const size_t blockSize = (sessionCipher == SecureMessagingPolicy::SessionCipher::AES) ? 16U : 8U;
    while ((plaintext.size() % blockSize) != 0U)
    {
        plaintext.push_back(0x00U);
    }

    auto protectionResult = SecureMessagingPolicy::protectEncryptedPayload(
        context,
        plaintext,
        sessionCipher,
        useLegacySendMode(sessionCipher));
    if (!protectionResult)
    {
        return etl::unexpected(protectionResult.error());
    }

    const auto& protection = protectionResult.value();
    if (protection.encryptedPayload.size() > 32U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
    }

    etl::vector<uint8_t, 32> encrypted;
    for (size_t i = 0U; i < protection.encryptedPayload.size(); ++i)
    {
        encrypted.push_back(protection.encryptedPayload[i]);
    }

    pendingIv.clear();
    for (size_t i = 0U; i < protection.requestState.size(); ++i)
    {
        pendingIv.push_back(protection.requestState[i]);
    }
    updateContextIv = protection.updateContextIv;

    return encrypted;
}
