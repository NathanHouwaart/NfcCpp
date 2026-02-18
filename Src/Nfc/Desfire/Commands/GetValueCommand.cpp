/**
 * @file GetValueCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get value command implementation
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/GetValueCommand.h"
#include "Nfc/Desfire/Commands/ValueOperationCryptoUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Utils/DesfireCrypto.h"
#include "Error/DesfireError.h"
#include <cppdes/des3cbc.h>
#include <aes.hpp>

using namespace nfc;
using namespace crypto;

namespace
{
    constexpr uint8_t GET_VALUE_COMMAND_CODE = 0x6C;

    bool deriveGetValuePlainRequestIv(const DesfireContext& context, uint8_t fileNo, uint8_t* outIv)
    {
        if (!outIv)
        {
            return false;
        }

        etl::vector<uint8_t, 2> message;
        message.push_back(GET_VALUE_COMMAND_CODE);
        message.push_back(fileNo);

        etl::vector<uint8_t, 16> derivedIv;
        if (!valueop_detail::deriveAesPlainRequestIv(context, message, derivedIv, true))
        {
            return false;
        }

        for (size_t i = 0U; i < 16U; ++i)
        {
            outIv[i] = derivedIv[i];
        }
        return true;
    }

    bool deriveGetValueTktdesRequestIv(const DesfireContext& context, uint8_t fileNo, uint8_t* outIv)
    {
        if (!outIv)
        {
            return false;
        }

        etl::vector<uint8_t, 2> message;
        message.push_back(GET_VALUE_COMMAND_CODE);
        message.push_back(fileNo);

        etl::vector<uint8_t, 16> derivedIv;
        if (!valueop_detail::deriveTktdesPlainRequestIv(context, message, derivedIv, true))
        {
            return false;
        }

        for (size_t i = 0U; i < 8U; ++i)
        {
            outIv[i] = derivedIv[i];
        }
        return true;
    }
}

GetValueCommand::GetValueCommand(uint8_t fileNo)
    : fileNo(fileNo)
    , stage(Stage::Initial)
    , value(0)
    , rawPayload()
{
}

etl::string_view GetValueCommand::name() const
{
    return "GetValue";
}

etl::expected<DesfireRequest, error::Error> GetValueCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (fileNo > 0x1FU)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    DesfireRequest request;
    request.commandCode = GET_VALUE_COMMAND_CODE;
    request.data.clear();
    request.data.push_back(fileNo);
    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> GetValueCommand::parseResponse(
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

    if (!result.isSuccess())
    {
        return etl::unexpected(error::Error::fromDesfire(static_cast<error::DesfireError>(result.statusCode)));
    }

    etl::vector<uint8_t, 32> payload;
    for (size_t i = 1; i < response.size(); ++i)
    {
        if (payload.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }
        payload.push_back(response[i]);
    }

    etl::vector<uint8_t, 4> valueBytes;
    bool decoded = false;

    if (context.authenticated && !context.sessionKeyEnc.empty())
    {
        decoded = tryDecodeEncryptedValue(payload, context, valueBytes);
    }

    // Fallback path for plain responses (and plain+trailing MAC/CMAC variants).
    if (!decoded)
    {
        if (context.authenticated && !context.sessionKeyEnc.empty())
        {
            const SessionCipher cipher = resolveSessionCipher(context);
            const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
            if ((cipher == SessionCipher::AES ||
                 cipher == SessionCipher::DES3_3K ||
                 (cipher == SessionCipher::DES3_2K && context.authScheme == SessionAuthScheme::Iso)) &&
                payload.size() >= blockSize &&
                (payload.size() % blockSize) == 0U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }
        }

        size_t payloadLength = payload.size();
        if (payloadLength != 4U)
        {
            if (context.authenticated && payloadLength > 4U)
            {
                const size_t extraLength = payloadLength - 4U;
                if (extraLength == 8U || extraLength == 4U)
                {
                    payloadLength = 4U;
                }
            }
        }

        if (payloadLength != 4U)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
        }

        for (size_t i = 0; i < 4U; ++i)
        {
            valueBytes.push_back(payload[i]);
        }
    }

    rawPayload.clear();
    result.data.clear();
    for (size_t i = 0; i < valueBytes.size(); ++i)
    {
        if (rawPayload.full() || result.data.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }
        rawPayload.push_back(valueBytes[i]);
        result.data.push_back(valueBytes[i]);
    }

    value = parseLe32(rawPayload);
    stage = Stage::Complete;
    return result;
}

bool GetValueCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void GetValueCommand::reset()
{
    stage = Stage::Initial;
    value = 0;
    rawPayload.clear();
}

int32_t GetValueCommand::getValue() const
{
    return value;
}

const etl::vector<uint8_t, 16>& GetValueCommand::getRawPayload() const
{
    return rawPayload;
}

int32_t GetValueCommand::parseLe32(const etl::ivector<uint8_t>& payload)
{
    const uint32_t raw = static_cast<uint32_t>(payload[0]) |
                         (static_cast<uint32_t>(payload[1]) << 8U) |
                         (static_cast<uint32_t>(payload[2]) << 16U) |
                         (static_cast<uint32_t>(payload[3]) << 24U);
    return static_cast<int32_t>(raw);
}

GetValueCommand::SessionCipher GetValueCommand::resolveSessionCipher(const DesfireContext& context) const
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

uint16_t GetValueCommand::calculateCrc16(const etl::ivector<uint8_t>& data) const
{
    return valueop_detail::calculateCrc16(data);
}

uint32_t GetValueCommand::calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const
{
    return valueop_detail::calculateCrc32Desfire(data);
}

bool GetValueCommand::tryDecodeEncryptedValue(
    const etl::ivector<uint8_t>& payload,
    DesfireContext& context,
    etl::vector<uint8_t, 4>& outValueBytes)
{
    const SessionCipher cipher = resolveSessionCipher(context);
    if (cipher == SessionCipher::UNKNOWN)
    {
        return false;
    }
    const bool legacyDesCryptoMode =
        (context.authScheme == SessionAuthScheme::Legacy) &&
        (cipher == SessionCipher::DES || cipher == SessionCipher::DES3_2K);

    const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
    if (payload.size() < blockSize)
    {
        return false;
    }

    uint8_t plainRequestIv[16] = {0};
    bool hasDerivedRequestIv = false;
    if (cipher == SessionCipher::AES)
    {
        hasDerivedRequestIv = deriveGetValuePlainRequestIv(context, fileNo, plainRequestIv);
    }
    else if (
        cipher == SessionCipher::DES3_3K ||
        (cipher == SessionCipher::DES3_2K && !legacyDesCryptoMode))
    {
        hasDerivedRequestIv = deriveGetValueTktdesRequestIv(context, fileNo, plainRequestIv);
    }

    const size_t trimCandidates[4] = {0U, 8U, 4U, 2U};
    for (size_t trimIndex = 0; trimIndex < 4; ++trimIndex)
    {
        const size_t trim = trimCandidates[trimIndex];
        if (payload.size() <= trim)
        {
            continue;
        }

        const size_t candidateLen = payload.size() - trim;
        if (candidateLen == 0U || (candidateLen % blockSize) != 0U)
        {
            continue;
        }

        etl::vector<uint8_t, 32> ciphertext;
        for (size_t i = 0; i < candidateLen; ++i)
        {
            ciphertext.push_back(payload[i]);
        }

        const size_t ivAttemptCount = hasDerivedRequestIv ? 2U : 1U;
        for (size_t ivAttempt = 0U; ivAttempt < ivAttemptCount; ++ivAttempt)
        {
            DesfireContext decodeContext = context;
            if (ivAttempt == 1U)
            {
                decodeContext.iv.clear();
                for (size_t i = 0; i < blockSize; ++i)
                {
                    decodeContext.iv.push_back(plainRequestIv[i]);
                }
            }

            etl::vector<uint8_t, 32> plaintext;
            if (!decryptPayload(ciphertext, decodeContext, cipher, plaintext))
            {
                continue;
            }

            if (plaintext.size() < 4U)
            {
                continue;
            }

            bool crcOk = false;

            // New-authenticated variants use CRC32; support both with and without status-byte inclusion.
            if (plaintext.size() >= 8U)
            {
                etl::vector<uint8_t, 5> crcDataWithStatus;
                crcDataWithStatus.push_back(plaintext[0]);
                crcDataWithStatus.push_back(plaintext[1]);
                crcDataWithStatus.push_back(plaintext[2]);
                crcDataWithStatus.push_back(plaintext[3]);
                crcDataWithStatus.push_back(0x00U);

                const uint32_t expectedCrcWithStatus = calculateCrc32Desfire(crcDataWithStatus);

                etl::vector<uint8_t, 4> crcDataNoStatus;
                crcDataNoStatus.push_back(plaintext[0]);
                crcDataNoStatus.push_back(plaintext[1]);
                crcDataNoStatus.push_back(plaintext[2]);
                crcDataNoStatus.push_back(plaintext[3]);
                const uint32_t expectedCrcNoStatus = calculateCrc32Desfire(crcDataNoStatus);

                const uint32_t receivedCrc =
                    static_cast<uint32_t>(plaintext[4]) |
                    (static_cast<uint32_t>(plaintext[5]) << 8U) |
                    (static_cast<uint32_t>(plaintext[6]) << 16U) |
                    (static_cast<uint32_t>(plaintext[7]) << 24U);

                crcOk = (receivedCrc == expectedCrcWithStatus) || (receivedCrc == expectedCrcNoStatus);
            }

            // Legacy variants may use CRC16.
            if (!crcOk && plaintext.size() >= 6U)
            {
                etl::vector<uint8_t, 5> crcDataWithStatus;
                crcDataWithStatus.push_back(plaintext[0]);
                crcDataWithStatus.push_back(plaintext[1]);
                crcDataWithStatus.push_back(plaintext[2]);
                crcDataWithStatus.push_back(plaintext[3]);
                crcDataWithStatus.push_back(0x00U);
                const uint16_t expectedCrcWithStatus = calculateCrc16(crcDataWithStatus);

                etl::vector<uint8_t, 4> crcDataNoStatus;
                crcDataNoStatus.push_back(plaintext[0]);
                crcDataNoStatus.push_back(plaintext[1]);
                crcDataNoStatus.push_back(plaintext[2]);
                crcDataNoStatus.push_back(plaintext[3]);
                const uint16_t expectedCrcNoStatus = calculateCrc16(crcDataNoStatus);

                const uint16_t receivedCrc =
                    static_cast<uint16_t>(plaintext[4]) |
                    (static_cast<uint16_t>(plaintext[5]) << 8U);

                crcOk = (receivedCrc == expectedCrcWithStatus) || (receivedCrc == expectedCrcNoStatus);
            }

            if (!crcOk)
            {
                continue;
            }

            outValueBytes.clear();
            for (size_t i = 0; i < 4U; ++i)
            {
                outValueBytes.push_back(plaintext[i]);
            }

            if (cipher == SessionCipher::DES || (cipher == SessionCipher::DES3_2K && legacyDesCryptoMode))
            {
                // Legacy DES/2K3DES secure messaging uses command-local chaining.
                // Keep IV reset between commands.
                context.iv.clear();
                context.iv.resize(8U, 0x00U);
            }
            else if (context.iv.size() == blockSize && candidateLen >= blockSize)
            {
                // Keep CBC IV progression for secure chained sessions.
                context.iv.clear();
                for (size_t i = candidateLen - blockSize; i < candidateLen; ++i)
                {
                    context.iv.push_back(payload[i]);
                }
            }

            return true;
        }
    }

    return false;
}

bool GetValueCommand::decryptPayload(
    const etl::ivector<uint8_t>& ciphertext,
    const DesfireContext& context,
    SessionCipher cipher,
    etl::vector<uint8_t, 32>& plaintext) const
{
    const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
    if ((ciphertext.size() % blockSize) != 0U)
    {
        return false;
    }

    plaintext.clear();
    for (size_t i = 0; i < ciphertext.size(); ++i)
    {
        plaintext.push_back(ciphertext[i]);
    }

    etl::vector<uint8_t, 16> iv;
    if (context.iv.size() >= blockSize)
    {
        for (size_t i = 0; i < blockSize; ++i)
        {
            iv.push_back(context.iv[i]);
        }
    }
    else
    {
        iv.resize(blockSize, 0x00U);
    }

    if (cipher == SessionCipher::AES)
    {
        if (context.sessionKeyEnc.size() < 16U)
        {
            return false;
        }

        AES_ctx aesContext;
        AES_init_ctx_iv(&aesContext, context.sessionKeyEnc.data(), iv.data());
        AES_CBC_decrypt_buffer(&aesContext, plaintext.data(), plaintext.size());
        return true;
    }

    if (cipher == SessionCipher::DES)
    {
        if (context.sessionKeyEnc.size() < 8U)
        {
            return false;
        }

        // Legacy single-DES "receive mode":
        // P_i = D_K(C_i) XOR C_{i-1}, C_-1 = 0.
        uint8_t previousCipherBlock[8] = {0};
        for (size_t offset = 0; offset < ciphertext.size(); offset += 8U)
        {
            uint8_t decryptedBlock[8];
            DesFireCrypto::desDecrypt(
                ciphertext.data() + offset,
                context.sessionKeyEnc.data(),
                decryptedBlock);

            for (size_t i = 0U; i < 8U; ++i)
            {
                plaintext[offset + i] = static_cast<uint8_t>(decryptedBlock[i] ^ previousCipherBlock[i]);
                previousCipherBlock[i] = ciphertext[offset + i];
            }
        }
        return true;
    }

    if (cipher == SessionCipher::DES3_2K)
    {
        if (context.sessionKeyEnc.size() < 16U)
        {
            return false;
        }

        const bool legacyDesCryptoMode =
            (context.authScheme == SessionAuthScheme::Legacy) && (cipher == SessionCipher::DES3_2K);
        if (legacyDesCryptoMode)
        {
            // Legacy 2K3DES "receive mode":
            // P_i = D_3DES(C_i) XOR C_{i-1}, C_-1 = 0.
            uint8_t previousCipherBlock[8] = {0};
            for (size_t offset = 0; offset < ciphertext.size(); offset += 8U)
            {
                uint8_t decryptedBlock[8];
                DesFireCrypto::des3Decrypt(
                    ciphertext.data() + offset,
                    context.sessionKeyEnc.data(),
                    decryptedBlock);

                for (size_t i = 0U; i < 8U; ++i)
                {
                    plaintext[offset + i] = static_cast<uint8_t>(decryptedBlock[i] ^ previousCipherBlock[i]);
                    previousCipherBlock[i] = ciphertext[offset + i];
                }
            }
        }
        else
        {
            DES3CBC des3Cbc(
                bytesToUint64(context.sessionKeyEnc.data()),
                bytesToUint64(context.sessionKeyEnc.data() + 8U),
                bytesToUint64(context.sessionKeyEnc.data()),
                bytesToUint64(iv.data()));

            for (size_t offset = 0; offset < ciphertext.size(); offset += 8U)
            {
                const uint64_t plainBlock = des3Cbc.decrypt(bytesToUint64(ciphertext.data() + offset));
                uint64ToBytes(plainBlock, plaintext.data() + offset);
            }
        }
        return true;
    }

    if (cipher == SessionCipher::DES3_3K)
    {
        if (context.sessionKeyEnc.size() < 24U)
        {
            return false;
        }

        DES3CBC des3Cbc(
            bytesToUint64(context.sessionKeyEnc.data()),
            bytesToUint64(context.sessionKeyEnc.data() + 8U),
            bytesToUint64(context.sessionKeyEnc.data() + 16U),
            bytesToUint64(iv.data()));

        for (size_t offset = 0; offset < ciphertext.size(); offset += 8U)
        {
            const uint64_t plainBlock = des3Cbc.decrypt(bytesToUint64(ciphertext.data() + offset));
            uint64ToBytes(plainBlock, plaintext.data() + offset);
        }
        return true;
    }

    return false;
}
