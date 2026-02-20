/**
 * @file ChangeKeySettingsCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire change key settings command implementation
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/ChangeKeySettingsCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t CHANGE_KEY_SETTINGS_COMMAND_CODE = 0x54;
}

ChangeKeySettingsCommand::ChangeKeySettingsCommand(const ChangeKeySettingsCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
    , pendingIv()
    , updateContextIv(false)
{
}

etl::string_view ChangeKeySettingsCommand::name() const
{
    return "ChangeKeySettings";
}

etl::expected<DesfireRequest, error::Error> ChangeKeySettingsCommand::buildRequest(const DesfireContext& context)
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
    request.commandCode = CHANGE_KEY_SETTINGS_COMMAND_CODE;
    request.data.clear();
    const etl::vector<uint8_t, 32>& payload = encryptedPayloadResult.value();
    for (size_t i = 0; i < payload.size(); ++i)
    {
        request.data.push_back(payload[i]);
    }
    request.expectedResponseLength = 0U;
    return request;
}

etl::expected<DesfireResult, error::Error> ChangeKeySettingsCommand::parseResponse(
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

    if (!response.empty())
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

    stage = Stage::Complete;
    return result;
}

bool ChangeKeySettingsCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void ChangeKeySettingsCommand::reset()
{
    stage = Stage::Initial;
    pendingIv.clear();
    updateContextIv = false;
}

etl::expected<etl::vector<uint8_t, 32>, error::Error> ChangeKeySettingsCommand::buildEncryptedPayload(
    const DesfireContext& context)
{
    const SecureMessagingPolicy::SessionCipher sessionCipher = resolveSessionCipher(context);
    if (sessionCipher == SecureMessagingPolicy::SessionCipher::UNKNOWN)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    etl::vector<uint8_t, 32> plaintext;
    plaintext.push_back(options.keySettings);
    const bool legacySendMode = useLegacySendMode(sessionCipher);

    if (legacySendMode)
    {
        const uint16_t crc16 = calculateCrc16(plaintext);
        plaintext.push_back(static_cast<uint8_t>(crc16 & 0xFFU));
        plaintext.push_back(static_cast<uint8_t>((crc16 >> 8U) & 0xFFU));
    }
    else
    {
        etl::vector<uint8_t, 8> crcInput;
        crcInput.push_back(CHANGE_KEY_SETTINGS_COMMAND_CODE);
        crcInput.push_back(options.keySettings);
        const uint32_t crc32 = calculateCrc32Desfire(crcInput);
        plaintext.push_back(static_cast<uint8_t>(crc32 & 0xFFU));
        plaintext.push_back(static_cast<uint8_t>((crc32 >> 8U) & 0xFFU));
        plaintext.push_back(static_cast<uint8_t>((crc32 >> 16U) & 0xFFU));
        plaintext.push_back(static_cast<uint8_t>((crc32 >> 24U) & 0xFFU));
    }

    const size_t blockSize = (sessionCipher == SecureMessagingPolicy::SessionCipher::AES) ? 16U : 8U;
    while ((plaintext.size() % blockSize) != 0U)
    {
        plaintext.push_back(0x00);
    }

    auto protectionResult = SecureMessagingPolicy::protectEncryptedPayload(
        context,
        plaintext,
        sessionCipher,
        legacySendMode);
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

SecureMessagingPolicy::SessionCipher ChangeKeySettingsCommand::resolveSessionCipher(const DesfireContext& context) const
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

bool ChangeKeySettingsCommand::useLegacySendMode(SecureMessagingPolicy::SessionCipher cipher) const
{
    if (options.authMode != DesfireAuthMode::LEGACY)
    {
        return false;
    }

    return cipher == SecureMessagingPolicy::SessionCipher::DES ||
           cipher == SecureMessagingPolicy::SessionCipher::DES3_2K;
}

uint16_t ChangeKeySettingsCommand::calculateCrc16(const etl::ivector<uint8_t>& data) const
{
    return SecureMessagingPolicy::calculateCrc16(data);
}

uint32_t ChangeKeySettingsCommand::calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const
{
    return SecureMessagingPolicy::calculateCrc32Desfire(data);
}
