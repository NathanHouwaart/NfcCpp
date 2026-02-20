/**
 * @file SecureMessagingPolicy.cpp
 * @brief Shared secure messaging policy helpers for command-level flows
 */

#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include "Detail/ValueOperationCryptoUtils.h"
#include "Utils/DesfireCrypto.h"
#include "Error/DesfireError.h"
#include <cppdes/des3.h>
#include <cppdes/descbc.h>
#include <cppdes/des3cbc.h>
#include <aes.hpp>

using namespace nfc;

namespace
{
    valueop_detail::SessionCipher toValueOpCipher(SecureMessagingPolicy::SessionCipher cipher)
    {
        switch (cipher)
        {
            case SecureMessagingPolicy::SessionCipher::DES:
                return valueop_detail::SessionCipher::DES;
            case SecureMessagingPolicy::SessionCipher::DES3_2K:
                return valueop_detail::SessionCipher::DES3_2K;
            case SecureMessagingPolicy::SessionCipher::DES3_3K:
                return valueop_detail::SessionCipher::DES3_3K;
            case SecureMessagingPolicy::SessionCipher::AES:
                return valueop_detail::SessionCipher::AES;
            default:
                return valueop_detail::SessionCipher::UNKNOWN;
        }
    }

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

SecureMessagingPolicy::SessionCipher SecureMessagingPolicy::resolveSessionCipher(const DesfireContext& context)
{
    switch (valueop_detail::resolveSessionCipher(context))
    {
        case valueop_detail::SessionCipher::DES:
            return SessionCipher::DES;
        case valueop_detail::SessionCipher::DES3_2K:
            return SessionCipher::DES3_2K;
        case valueop_detail::SessionCipher::DES3_3K:
            return SessionCipher::DES3_3K;
        case valueop_detail::SessionCipher::AES:
            return SessionCipher::AES;
        default:
            return SessionCipher::UNKNOWN;
    }
}

bool SecureMessagingPolicy::isLegacyDesOr2KSession(const DesfireContext& context)
{
    const valueop_detail::SessionCipher cipher = valueop_detail::resolveSessionCipher(context);
    return valueop_detail::useLegacyDesCryptoMode(context, cipher);
}

etl::expected<etl::vector<uint8_t, 16>, error::Error> SecureMessagingPolicy::derivePlainRequestIv(
    const DesfireContext& context,
    const etl::ivector<uint8_t>& plainRequestMessage,
    bool useZeroIvWhenContextIvMissing)
{
    if (plainRequestMessage.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    const SessionCipher cipher = resolveSessionCipher(context);
    etl::vector<uint8_t, 16> requestIv;
    bool ok = false;

    if (cipher == SessionCipher::AES)
    {
        ok = valueop_detail::deriveAesPlainRequestIv(
            context,
            plainRequestMessage,
            requestIv,
            useZeroIvWhenContextIvMissing);
    }
    else if (
        cipher == SessionCipher::DES3_3K ||
        (cipher == SessionCipher::DES3_2K && !isLegacyDesOr2KSession(context)))
    {
        ok = valueop_detail::deriveTktdesPlainRequestIv(
            context,
            plainRequestMessage,
            requestIv,
            useZeroIvWhenContextIvMissing);
    }
    else
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (!ok)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    return requestIv;
}

etl::expected<etl::vector<uint8_t, 16>, error::Error> SecureMessagingPolicy::derivePlainResponseIv(
    const DesfireContext& context,
    const etl::ivector<uint8_t>& plainResponse,
    const etl::ivector<uint8_t>& requestIv,
    size_t truncatedCmacLength)
{
    const SessionCipher cipher = resolveSessionCipher(context);
    if (cipher == SessionCipher::AES)
    {
        return valueop_detail::deriveAesPlainResponseIv(
            plainResponse,
            context,
            requestIv,
            truncatedCmacLength);
    }

    if (
        cipher == SessionCipher::DES3_3K ||
        (cipher == SessionCipher::DES3_2K && !isLegacyDesOr2KSession(context)))
    {
        return valueop_detail::deriveTktdesPlainResponseIv(
            plainResponse,
            context,
            requestIv,
            truncatedCmacLength);
    }

    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
}

etl::expected<void, error::Error> SecureMessagingPolicy::updateContextIvForPlainCommand(
    DesfireContext& context,
    const etl::ivector<uint8_t>& plainRequestMessage,
    const etl::ivector<uint8_t>& plainResponse,
    size_t truncatedCmacLength)
{
    if (!context.authenticated || context.sessionKeyEnc.empty())
    {
        return {};
    }

    const SessionCipher cipher = resolveSessionCipher(context);
    if (cipher == SessionCipher::UNKNOWN)
    {
        return {};
    }

    if (cipher == SessionCipher::DES || (cipher == SessionCipher::DES3_2K && isLegacyDesOr2KSession(context)))
    {
        applyLegacyCommandBoundaryIvPolicy(context);
        return {};
    }

    auto requestIvResult = derivePlainRequestIv(context, plainRequestMessage);
    if (!requestIvResult)
    {
        return etl::unexpected(requestIvResult.error());
    }

    auto nextIvResult = derivePlainResponseIv(
        context,
        plainResponse,
        requestIvResult.value(),
        truncatedCmacLength);
    if (!nextIvResult)
    {
        return etl::unexpected(nextIvResult.error());
    }

    valueop_detail::setContextIv(context, nextIvResult.value());
    return {};
}

etl::expected<void, error::Error> SecureMessagingPolicy::verifyAuthenticatedPlainPayloadAndUpdateContextIv(
    DesfireContext& context,
    const etl::ivector<uint8_t>& payloadAndMac,
    uint8_t statusCode,
    const etl::ivector<uint8_t>& requestIv,
    size_t payloadLength,
    size_t truncatedCmacLength)
{
    if (payloadLength > payloadAndMac.size())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    if ((payloadAndMac.size() - payloadLength) != truncatedCmacLength)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    if (!context.authenticated || context.sessionKeyEnc.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    const SessionCipher cipher = resolveSessionCipher(context);
    if (cipher == SessionCipher::UNKNOWN)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    etl::vector<uint8_t, 16> nextIv;
    bool verified = false;

    if (cipher == SessionCipher::AES)
    {
        verified = valueop_detail::verifyAesAuthenticatedPlainPayload(
            payloadAndMac,
            statusCode,
            context,
            requestIv,
            payloadLength,
            truncatedCmacLength,
            nextIv);
    }
    else if (cipher == SessionCipher::DES3_3K || (cipher == SessionCipher::DES3_2K && !isLegacyDesOr2KSession(context)))
    {
        verified = valueop_detail::verifyTktdesAuthenticatedPlainPayload(
            payloadAndMac,
            statusCode,
            context,
            requestIv,
            payloadLength,
            truncatedCmacLength,
            nextIv);
    }
    else
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (!verified)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::IntegrityError));
    }

    valueop_detail::setContextIv(context, nextIv);
    return {};
}

etl::expected<size_t, error::Error> SecureMessagingPolicy::verifyAuthenticatedPlainPayloadAutoMacAndUpdateContextIv(
    DesfireContext& context,
    const etl::ivector<uint8_t>& payloadAndMac,
    uint8_t statusCode,
    const etl::ivector<uint8_t>& requestIv,
    size_t payloadLength)
{
    if (payloadLength > payloadAndMac.size())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    const size_t receivedMacLength = payloadAndMac.size() - payloadLength;
    constexpr size_t macCandidates[3] = {8U, 4U, 0U};
    for (size_t i = 0U; i < 3U; ++i)
    {
        const size_t candidateMacLength = macCandidates[i];
        if (receivedMacLength != candidateMacLength)
        {
            continue;
        }

        auto verifyResult = verifyAuthenticatedPlainPayloadAndUpdateContextIv(
            context,
            payloadAndMac,
            statusCode,
            requestIv,
            payloadLength,
            candidateMacLength);
        if (verifyResult)
        {
            return candidateMacLength;
        }
    }

    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
}

etl::expected<SecureMessagingPolicy::ValueOperationRequestProtection, error::Error>
SecureMessagingPolicy::protectValueOperationRequest(
    const DesfireContext& context,
    uint8_t commandCode,
    uint8_t fileNo,
    int32_t value)
{
    if (!context.authenticated || context.sessionKeyEnc.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::AuthenticationError));
    }

    const SessionCipher cipher = resolveSessionCipher(context);
    if (cipher == SessionCipher::UNKNOWN)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    ValueOperationRequestProtection protection;
    const auto encryptedPayloadResult = valueop_detail::buildEncryptedValuePayload(
        commandCode,
        fileNo,
        value,
        context,
        toValueOpCipher(cipher),
        protection.requestState);
    if (!encryptedPayloadResult)
    {
        return etl::unexpected(encryptedPayloadResult.error());
    }

    const auto& encryptedPayload = encryptedPayloadResult.value();
    for (size_t i = 0; i < encryptedPayload.size(); ++i)
    {
        protection.encryptedPayload.push_back(encryptedPayload[i]);
    }

    return protection;
}

etl::expected<void, error::Error> SecureMessagingPolicy::updateContextIvForValueOperationResponse(
    DesfireContext& context,
    const etl::ivector<uint8_t>& response,
    const etl::ivector<uint8_t>& requestState)
{
    if (response.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    if (!context.authenticated || context.sessionKeyEnc.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    const SessionCipher cipher = resolveSessionCipher(context);
    if (cipher == SessionCipher::UNKNOWN)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (cipher == SessionCipher::DES || (cipher == SessionCipher::DES3_2K && isLegacyDesOr2KSession(context)))
    {
        applyLegacyCommandBoundaryIvPolicy(context);
        return {};
    }

    if (cipher == SessionCipher::AES)
    {
        auto nextIvResult = valueop_detail::deriveAesResponseIvForValueOperation(
            response,
            context,
            requestState);
        if (!nextIvResult)
        {
            return etl::unexpected(nextIvResult.error());
        }

        valueop_detail::setContextIv(context, nextIvResult.value());
        return {};
    }

    if (cipher == SessionCipher::DES3_3K || cipher == SessionCipher::DES3_2K)
    {
        auto nextIvResult = valueop_detail::deriveTktdesResponseIvForValueOperation(
            response,
            context,
            requestState);
        if (!nextIvResult)
        {
            return etl::unexpected(nextIvResult.error());
        }

        valueop_detail::setContextIv(context, nextIvResult.value());
        return {};
    }

    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
}

etl::expected<void, error::Error> SecureMessagingPolicy::updateContextIvFromEncryptedCiphertext(
    DesfireContext& context,
    const etl::ivector<uint8_t>& ciphertext)
{
    if (!context.authenticated || context.sessionKeyEnc.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    const SessionCipher cipher = resolveSessionCipher(context);
    if (cipher == SessionCipher::UNKNOWN)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (cipher == SessionCipher::DES || (cipher == SessionCipher::DES3_2K && isLegacyDesOr2KSession(context)))
    {
        applyLegacyCommandBoundaryIvPolicy(context);
        return {};
    }

    const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
    if (ciphertext.size() < blockSize)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    etl::vector<uint8_t, 16> nextIv;
    for (size_t i = ciphertext.size() - blockSize; i < ciphertext.size(); ++i)
    {
        nextIv.push_back(ciphertext[i]);
    }
    valueop_detail::setContextIv(context, nextIv);
    return {};
}

etl::expected<SecureMessagingPolicy::EncryptedPayloadProtection, error::Error>
SecureMessagingPolicy::protectEncryptedPayload(
    const DesfireContext& context,
    const etl::ivector<uint8_t>& plaintext,
    SessionCipher sessionCipher,
    bool useLegacySendMode,
    LegacySendIvSeedMode legacySeed)
{
    if (plaintext.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    if (sessionCipher == SessionCipher::UNKNOWN)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    const size_t blockSize = (sessionCipher == SessionCipher::AES) ? 16U : 8U;
    if ((plaintext.size() % blockSize) != 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
    }

    EncryptedPayloadProtection protection;

    if (!useLegacySendMode)
    {
        etl::vector<uint8_t, 16> iv;
        if (context.iv.empty())
        {
            iv.resize(blockSize, 0x00U);
        }
        else
        {
            if (context.iv.size() != blockSize)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            for (size_t i = 0U; i < context.iv.size(); ++i)
            {
                iv.push_back(context.iv[i]);
            }
        }

        if (sessionCipher == SessionCipher::AES)
        {
            if (context.sessionKeyEnc.size() < 16U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            if (plaintext.size() > protection.encryptedPayload.max_size())
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
            }
            for (size_t i = 0U; i < plaintext.size(); ++i)
            {
                protection.encryptedPayload.push_back(plaintext[i]);
            }

            AES_ctx aesContext;
            AES_init_ctx_iv(&aesContext, context.sessionKeyEnc.data(), iv.data());
            AES_CBC_encrypt_buffer(
                &aesContext,
                protection.encryptedPayload.data(),
                protection.encryptedPayload.size());
        }
        else if (sessionCipher == SessionCipher::DES)
        {
            if (context.sessionKeyEnc.size() < 8U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            DESCBC desCbc(crypto::bytesToUint64(context.sessionKeyEnc.data()), crypto::bytesToUint64(iv.data()));
            for (size_t offset = 0U; offset < plaintext.size(); offset += 8U)
            {
                const uint64_t block = crypto::bytesToUint64(plaintext.data() + offset);
                const uint64_t cipherBlock = desCbc.encrypt(block);
                uint8_t blockBytes[8];
                crypto::uint64ToBytes(cipherBlock, blockBytes);

                if ((protection.encryptedPayload.size() + 8U) > protection.encryptedPayload.max_size())
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
                }

                for (size_t i = 0U; i < 8U; ++i)
                {
                    protection.encryptedPayload.push_back(blockBytes[i]);
                }
            }
        }
        else if (sessionCipher == SessionCipher::DES3_2K)
        {
            if (context.sessionKeyEnc.size() < 16U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            DES3CBC des3Cbc(
                crypto::bytesToUint64(context.sessionKeyEnc.data()),
                crypto::bytesToUint64(context.sessionKeyEnc.data() + 8U),
                crypto::bytesToUint64(context.sessionKeyEnc.data()),
                crypto::bytesToUint64(iv.data()));

            for (size_t offset = 0U; offset < plaintext.size(); offset += 8U)
            {
                const uint64_t block = crypto::bytesToUint64(plaintext.data() + offset);
                const uint64_t cipherBlock = des3Cbc.encrypt(block);
                uint8_t blockBytes[8];
                crypto::uint64ToBytes(cipherBlock, blockBytes);

                if ((protection.encryptedPayload.size() + 8U) > protection.encryptedPayload.max_size())
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
                }

                for (size_t i = 0U; i < 8U; ++i)
                {
                    protection.encryptedPayload.push_back(blockBytes[i]);
                }
            }
        }
        else if (sessionCipher == SessionCipher::DES3_3K)
        {
            if (context.sessionKeyEnc.size() < 24U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            DES3CBC des3Cbc(
                crypto::bytesToUint64(context.sessionKeyEnc.data()),
                crypto::bytesToUint64(context.sessionKeyEnc.data() + 8U),
                crypto::bytesToUint64(context.sessionKeyEnc.data() + 16U),
                crypto::bytesToUint64(iv.data()));

            for (size_t offset = 0U; offset < plaintext.size(); offset += 8U)
            {
                const uint64_t block = crypto::bytesToUint64(plaintext.data() + offset);
                const uint64_t cipherBlock = des3Cbc.encrypt(block);
                uint8_t blockBytes[8];
                crypto::uint64ToBytes(cipherBlock, blockBytes);

                if ((protection.encryptedPayload.size() + 8U) > protection.encryptedPayload.max_size())
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
                }

                for (size_t i = 0U; i < 8U; ++i)
                {
                    protection.encryptedPayload.push_back(blockBytes[i]);
                }
            }
        }
        else
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        protection.requestState.clear();
        for (size_t i = protection.encryptedPayload.size() - blockSize; i < protection.encryptedPayload.size(); ++i)
        {
            protection.requestState.push_back(protection.encryptedPayload[i]);
        }
        protection.updateContextIv = true;
        return protection;
    }

    if (blockSize != 8U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    etl::vector<uint8_t, 8> previousBlock;
    previousBlock.resize(8U, 0x00U);

    if (legacySeed == LegacySendIvSeedMode::SessionEncryptedRndB)
    {
        if (context.sessionEncRndB.size() < 8U)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        for (size_t i = 0U; i < 8U; ++i)
        {
            previousBlock[i] = context.sessionEncRndB[i];
        }
    }
    else if (!context.iv.empty())
    {
        if (context.iv.size() != 8U || !isAllZero(context.iv))
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }
    }

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
            crypto::DesFireCrypto::desDecrypt(xoredBlock, context.sessionKeyEnc.data(), transformedBlock);
        }
        else if (sessionCipher == SessionCipher::DES3_2K)
        {
            if (context.sessionKeyEnc.size() < 16U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }
            crypto::DesFireCrypto::des3Decrypt(xoredBlock, context.sessionKeyEnc.data(), transformedBlock);
        }
        else if (sessionCipher == SessionCipher::DES3_3K)
        {
            if (context.sessionKeyEnc.size() < 24U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            DES3 des3(
                crypto::bytesToUint64(context.sessionKeyEnc.data()),
                crypto::bytesToUint64(context.sessionKeyEnc.data() + 8U),
                crypto::bytesToUint64(context.sessionKeyEnc.data() + 16U));
            const uint64_t decrypted = des3.decrypt(crypto::bytesToUint64(xoredBlock));
            crypto::uint64ToBytes(decrypted, transformedBlock);
        }
        else
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        if ((protection.encryptedPayload.size() + 8U) > protection.encryptedPayload.max_size())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }

        for (size_t i = 0U; i < 8U; ++i)
        {
            protection.encryptedPayload.push_back(transformedBlock[i]);
            previousBlock[i] = transformedBlock[i];
        }
    }

    protection.requestState.clear();
    protection.updateContextIv = false;
    return protection;
}

etl::expected<void, error::Error> SecureMessagingPolicy::updateContextIvForEncryptedCommandResponse(
    DesfireContext& context,
    const etl::ivector<uint8_t>& response,
    const EncryptedPayloadProtection& protection)
{
    if (!protection.updateContextIv)
    {
        applyLegacyCommandBoundaryIvPolicy(context);
        return {};
    }

    return updateContextIvForValueOperationResponse(context, response, protection.requestState);
}

uint16_t SecureMessagingPolicy::calculateCrc16(const etl::ivector<uint8_t>& data)
{
    return valueop_detail::calculateCrc16(data);
}

uint32_t SecureMessagingPolicy::calculateCrc32Desfire(const etl::ivector<uint8_t>& data)
{
    return valueop_detail::calculateCrc32Desfire(data);
}

void SecureMessagingPolicy::applyLegacyCommandBoundaryIvPolicy(DesfireContext& context)
{
    if (!isLegacyDesOr2KSession(context))
    {
        return;
    }

    etl::vector<uint8_t, 8> zeroIv;
    zeroIv.resize(8U, 0x00U);
    valueop_detail::setContextIv(context, zeroIv);
}
