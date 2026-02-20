/**
 * @file ChangeKeyCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Change key command implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Desfire/Commands/ChangeKeyCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Utils/DesfireCrypto.h"
#include "Error/DesfireError.h"

using namespace nfc;
using namespace crypto;

namespace
{
    constexpr uint8_t CHANGE_KEY_COMMAND_CODE = 0xC4;

    enum class KeyFamily : uint8_t
    {
        DesOr2K,
        ThreeK,
        Aes,
        Unknown
    };

    KeyFamily keyFamilyFromKeyType(DesfireKeyType keyType)
    {
        switch (keyType)
        {
            case DesfireKeyType::DES:
            case DesfireKeyType::DES3_2K:
                return KeyFamily::DesOr2K;
            case DesfireKeyType::DES3_3K:
                return KeyFamily::ThreeK;
            case DesfireKeyType::AES:
                return KeyFamily::Aes;
            default:
                return KeyFamily::Unknown;
        }
    }

}

ChangeKeyCommand::ChangeKeyCommand(const ChangeKeyCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
    , pendingIv()
    , updateContextIv(false)
    , sameKeyChange(false)
    , effectiveKeyNo(0x00)
{
}

etl::string_view ChangeKeyCommand::name() const
{
    return "ChangeKey";
}

etl::expected<DesfireRequest, error::Error> ChangeKeyCommand::buildRequest(const DesfireContext& context)
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

    auto cryptogramResult = buildKeyCryptogram(context);
    if (!cryptogramResult)
    {
        return etl::unexpected(cryptogramResult.error());
    }

    const etl::vector<uint8_t, 48>& encryptedCryptogram = cryptogramResult.value();

    DesfireRequest request;
    request.commandCode = CHANGE_KEY_COMMAND_CODE;
    request.data.clear();
    request.data.push_back(effectiveKeyNo);
    for (size_t i = 0; i < encryptedCryptogram.size(); ++i)
    {
        request.data.push_back(encryptedCryptogram[i]);
    }
    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> ChangeKeyCommand::parseResponse(
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
    for (size_t i = 1; i < response.size(); ++i)
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

    if (sameKeyChange)
    {
        // After changing the authenticated key, the session is no longer valid.
        context.authenticated = false;
        context.commMode = CommMode::Plain;
        context.authScheme = SessionAuthScheme::None;
        context.sessionKeyEnc.clear();
        context.sessionKeyMac.clear();
        context.iv.clear();
        context.iv.resize(8, 0x00);
        context.sessionEncRndB.clear();
    }

    stage = Stage::Complete;
    return result;
}

bool ChangeKeyCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void ChangeKeyCommand::reset()
{
    stage = Stage::Initial;
    pendingIv.clear();
    updateContextIv = false;
    sameKeyChange = false;
    effectiveKeyNo = 0x00;
}

etl::expected<etl::vector<uint8_t, 48>, error::Error> ChangeKeyCommand::buildKeyCryptogram(const DesfireContext& context)
{
    effectiveKeyNo = options.keyNo;

    const bool piccSelected = context.selectedAid.size() == 3 &&
        context.selectedAid[0] == 0x00 &&
        context.selectedAid[1] == 0x00 &&
        context.selectedAid[2] == 0x00;

    if (piccSelected && (effectiveKeyNo & 0x0F) != 0x00)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    // PICC master key change uses key number flags to encode target key type.
    if (piccSelected)
    {
        if (options.newKeyType == DesfireKeyType::DES3_3K)
        {
            effectiveKeyNo = static_cast<uint8_t>(effectiveKeyNo | 0x40);
        }
        else if (options.newKeyType == DesfireKeyType::AES)
        {
            effectiveKeyNo = static_cast<uint8_t>(effectiveKeyNo | 0x80);
        }
    }

    const SecureMessagingPolicy::SessionCipher sessionCipher = resolveSessionCipher(context);
    if (sessionCipher == SecureMessagingPolicy::SessionCipher::UNKNOWN)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    // Per DESFire behavior, application key type is fixed when the application is
    // created. Only PICC master key type may be changed via ChangeKey flags.
    if (context.selectedAid.size() == 3 && !piccSelected)
    {
        KeyFamily currentFamily = KeyFamily::Unknown;
        switch (sessionCipher)
        {
            case SecureMessagingPolicy::SessionCipher::DES:
            case SecureMessagingPolicy::SessionCipher::DES3_2K:
                currentFamily = KeyFamily::DesOr2K;
                break;
            case SecureMessagingPolicy::SessionCipher::DES3_3K:
                currentFamily = KeyFamily::ThreeK;
                break;
            case SecureMessagingPolicy::SessionCipher::AES:
                currentFamily = KeyFamily::Aes;
                break;
            default:
                break;
        }

        const KeyFamily requestedFamily = keyFamilyFromKeyType(options.newKeyType);
        if (currentFamily != KeyFamily::Unknown &&
            requestedFamily != KeyFamily::Unknown &&
            currentFamily != requestedFamily)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
        }
    }

    auto newKeyMaterialResult = normalizeKeyMaterial(options.newKey, options.newKeyType);
    if (!newKeyMaterialResult)
    {
        return etl::unexpected(newKeyMaterialResult.error());
    }

    const etl::vector<uint8_t, 24>& newKeyMaterial = newKeyMaterialResult.value();

    const bool sameKey = ((effectiveKeyNo & 0x0F) == (context.keyNo & 0x0F));
    sameKeyChange = sameKey;

    etl::vector<uint8_t, 24> keyDataForCrypto;
    if (sameKey)
    {
        for (size_t i = 0; i < newKeyMaterial.size(); ++i)
        {
            keyDataForCrypto.push_back(newKeyMaterial[i]);
        }
    }
    else
    {
        if (!options.oldKey.has_value())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
        }

        const DesfireKeyType oldKeyType = (options.oldKeyType == DesfireKeyType::UNKNOWN) ?
            options.newKeyType : options.oldKeyType;

        auto oldKeyMaterialResult = normalizeKeyMaterial(options.oldKey.value(), oldKeyType);
        if (!oldKeyMaterialResult)
        {
            return etl::unexpected(oldKeyMaterialResult.error());
        }

        const etl::vector<uint8_t, 24>& oldKeyMaterial = oldKeyMaterialResult.value();
        if (oldKeyMaterial.size() != newKeyMaterial.size())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
        }

        for (size_t i = 0; i < newKeyMaterial.size(); ++i)
        {
            keyDataForCrypto.push_back(static_cast<uint8_t>(newKeyMaterial[i] ^ oldKeyMaterial[i]));
        }
    }

    etl::vector<uint8_t, 25> keyStream;
    for (size_t i = 0; i < keyDataForCrypto.size(); ++i)
    {
        keyStream.push_back(keyDataForCrypto[i]);
    }
    if (options.newKeyType == DesfireKeyType::AES)
    {
        keyStream.push_back(options.newKeyVersion);
    }

    etl::vector<uint8_t, 33> plaintextCryptogram;
    for (size_t i = 0; i < keyStream.size(); ++i)
    {
        plaintextCryptogram.push_back(keyStream[i]);
    }

    if (options.authMode == DesfireAuthMode::LEGACY)
    {
        const uint16_t crcCrypto = calculateCrc16(keyStream);
        plaintextCryptogram.push_back(static_cast<uint8_t>(crcCrypto & 0xFFU));
        plaintextCryptogram.push_back(static_cast<uint8_t>((crcCrypto >> 8U) & 0xFFU));
    }
    else
    {
        etl::vector<uint8_t, 27> crcInput;
        crcInput.push_back(CHANGE_KEY_COMMAND_CODE);
        crcInput.push_back(effectiveKeyNo);
        for (size_t i = 0; i < keyStream.size(); ++i)
        {
            crcInput.push_back(keyStream[i]);
        }
        const uint32_t crcCrypto = calculateCrc32Desfire(crcInput);
        plaintextCryptogram.push_back(static_cast<uint8_t>(crcCrypto & 0xFFU));
        plaintextCryptogram.push_back(static_cast<uint8_t>((crcCrypto >> 8U) & 0xFFU));
        plaintextCryptogram.push_back(static_cast<uint8_t>((crcCrypto >> 16U) & 0xFFU));
        plaintextCryptogram.push_back(static_cast<uint8_t>((crcCrypto >> 24U) & 0xFFU));
    }

    if (!sameKey)
    {
        if (options.authMode == DesfireAuthMode::LEGACY)
        {
            const uint16_t crcNewKey = calculateCrc16(newKeyMaterial);
            plaintextCryptogram.push_back(static_cast<uint8_t>(crcNewKey & 0xFFU));
            plaintextCryptogram.push_back(static_cast<uint8_t>((crcNewKey >> 8U) & 0xFFU));
        }
        else
        {
            const uint32_t crcNewKey = calculateCrc32Desfire(newKeyMaterial);
            plaintextCryptogram.push_back(static_cast<uint8_t>(crcNewKey & 0xFFU));
            plaintextCryptogram.push_back(static_cast<uint8_t>((crcNewKey >> 8U) & 0xFFU));
            plaintextCryptogram.push_back(static_cast<uint8_t>((crcNewKey >> 16U) & 0xFFU));
            plaintextCryptogram.push_back(static_cast<uint8_t>((crcNewKey >> 24U) & 0xFFU));
        }
    }

    const size_t blockSize = (sessionCipher == SecureMessagingPolicy::SessionCipher::AES) ? 16U : 8U;

    etl::vector<uint8_t, 48> paddedCryptogram;
    for (size_t i = 0; i < plaintextCryptogram.size(); ++i)
    {
        paddedCryptogram.push_back(plaintextCryptogram[i]);
    }
    while ((paddedCryptogram.size() % blockSize) != 0U)
    {
        paddedCryptogram.push_back(0x00);
    }

    SecureMessagingPolicy::LegacySendIvSeedMode legacySeed = SecureMessagingPolicy::LegacySendIvSeedMode::Zero;
    if (options.legacyIvMode == ChangeKeyLegacyIvMode::SessionEncryptedRndB)
    {
        legacySeed = SecureMessagingPolicy::LegacySendIvSeedMode::SessionEncryptedRndB;
    }

    auto protectionResult = SecureMessagingPolicy::protectEncryptedPayload(
        context,
        paddedCryptogram,
        sessionCipher,
        useLegacySendMode(sessionCipher),
        legacySeed);
    if (!protectionResult)
    {
        return etl::unexpected(protectionResult.error());
    }

    const auto& protection = protectionResult.value();
    if (protection.encryptedPayload.size() > 48U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
    }

    etl::vector<uint8_t, 48> encrypted;
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

size_t ChangeKeyCommand::getKeySize(DesfireKeyType keyType) const
{
    switch (keyType)
    {
        case DesfireKeyType::DES:
            return 8;
        case DesfireKeyType::DES3_2K:
            return 16;
        case DesfireKeyType::DES3_3K:
            return 24;
        case DesfireKeyType::AES:
            return 16;
        default:
            return 0;
    }
}

etl::expected<etl::vector<uint8_t, 24>, error::Error> ChangeKeyCommand::normalizeKeyMaterial(
    const etl::ivector<uint8_t>& keyData,
    DesfireKeyType keyType) const
{
    etl::vector<uint8_t, 24> normalized;

    if (keyType == DesfireKeyType::DES)
    {
        if (keyData.size() == 8)
        {
            etl::vector<uint8_t, 8> singleDesKey;
            for (size_t i = 0; i < 8; ++i)
            {
                singleDesKey.push_back(keyData[i]);
            }

            // DESFire stores DES key-version/parity metadata in bit 0.
            // Normalize LSBs to 0 so DES operations are not sensitive to caller parity.
            DesFireCrypto::clearParityBits(singleDesKey.data(), singleDesKey.size());

            // ChangeKey DES payload uses K1||K1 material (16 bytes) for crypto framing.
            for (size_t i = 0; i < 8; ++i)
            {
                normalized.push_back(singleDesKey[i]);
            }
            for (size_t i = 0; i < 8; ++i)
            {
                normalized.push_back(singleDesKey[i]);
            }
            return normalized;
        }

        if (keyData.size() == 16)
        {
            // Accept 16-byte legacy input only when both halves represent the
            // same DES key (optionally with different parity/version bits).
            for (size_t i = 0; i < 8; ++i)
            {
                if ((keyData[i] & 0xFEU) != (keyData[i + 8] & 0xFEU))
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
                }
            }

            // Keep parity/version handling consistent for DES-key payloads.
            for (size_t i = 0; i < 8; ++i)
            {
                normalized.push_back(keyData[i]);
            }
            DesFireCrypto::clearParityBits(normalized.data(), normalized.size());
            // Mirror normalized first half into second half to keep canonical K1||K1 form.
            for (size_t i = 0; i < 8; ++i)
            {
                normalized.push_back(normalized[i]);
            }
            return normalized;
        }

        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    const size_t requiredKeySize = getKeySize(keyType);
    if (requiredKeySize == 0 || keyData.size() != requiredKeySize)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    for (size_t i = 0; i < requiredKeySize; ++i)
    {
        normalized.push_back(keyData[i]);
    }

    return normalized;
}

SecureMessagingPolicy::SessionCipher ChangeKeyCommand::resolveSessionCipher(const DesfireContext& context) const
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

        // Prefer the live session when user-provided key type conflicts
        // (for example ISO auth with degenerate 2K3DES key deriving DES session material).
        return inferredCipher;
    }

    return inferredCipher;
}

uint16_t ChangeKeyCommand::calculateCrc16(const etl::ivector<uint8_t>& data) const
{
    return SecureMessagingPolicy::calculateCrc16(data);
}

uint32_t ChangeKeyCommand::calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const
{
    return SecureMessagingPolicy::calculateCrc32Desfire(data);
}

bool ChangeKeyCommand::useLegacySendMode(SecureMessagingPolicy::SessionCipher cipher) const
{
    if (options.authMode != DesfireAuthMode::LEGACY)
    {
        return false;
    }

    return cipher == SecureMessagingPolicy::SessionCipher::DES ||
           cipher == SecureMessagingPolicy::SessionCipher::DES3_2K ||
           cipher == SecureMessagingPolicy::SessionCipher::DES3_3K;
}
