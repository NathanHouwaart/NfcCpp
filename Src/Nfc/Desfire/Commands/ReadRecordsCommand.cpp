/**
 * @file ReadRecordsCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire read records command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/ReadRecordsCommand.h"
#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Utils/DesfireCrypto.h"
#include "Error/DesfireError.h"
#include <cppdes/des3cbc.h>
#include <aes.hpp>

using namespace nfc;
using namespace crypto;

namespace
{
    constexpr uint8_t READ_RECORDS_COMMAND_CODE = 0xBB;
    constexpr uint8_t ADDITIONAL_FRAME_COMMAND_CODE = 0xAF;
}

ReadRecordsCommand::ReadRecordsCommand(const ReadRecordsCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
    , data()
    , activeCommunicationSettings(0x00U)
    , sessionCipher(SessionCipher::UNKNOWN)
    , requestIv()
    , hasRequestIv(false)
{
}

etl::string_view ReadRecordsCommand::name() const
{
    return "ReadRecords";
}

etl::expected<DesfireRequest, error::Error> ReadRecordsCommand::buildRequest(const DesfireContext& context)
{
    DesfireRequest request;
    request.data.clear();
    request.expectedResponseLength = 0U;

    switch (stage)
    {
        case Stage::Initial:
        {
            if (!validateOptions())
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
            }

            data.clear();
            activeCommunicationSettings = resolveCommunicationSettings(context);
            sessionCipher = SessionCipher::UNKNOWN;
            requestIv.clear();
            hasRequestIv = false;

            if (context.authenticated && !context.sessionKeyEnc.empty())
            {
                sessionCipher = resolveSessionCipher(context);
            }

            if (activeCommunicationSettings == 0x03U)
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

                const bool shouldDeriveRequestIv =
                    context.authenticated &&
                    ((sessionCipher == SessionCipher::AES) ||
                     (sessionCipher == SessionCipher::DES3_3K) ||
                     (sessionCipher == SessionCipher::DES3_2K && context.authScheme == SessionAuthScheme::Iso));

                if (shouldDeriveRequestIv)
                {
                    etl::vector<uint8_t, 8> cmacMessage;
                    cmacMessage.push_back(READ_RECORDS_COMMAND_CODE);
                    cmacMessage.push_back(options.fileNo);
                    appendLe24(cmacMessage, options.recordOffset);
                    appendLe24(cmacMessage, options.recordCount);

                    auto requestIvResult = SecureMessagingPolicy::derivePlainRequestIv(context, cmacMessage, true);
                    if (!requestIvResult)
                    {
                        return etl::unexpected(requestIvResult.error());
                    }

                    requestIv.clear();
                    const auto& derivedIv = requestIvResult.value();
                    for (size_t i = 0U; i < derivedIv.size(); ++i)
                    {
                        requestIv.push_back(derivedIv[i]);
                    }
                    hasRequestIv = true;
                }
            }
            else if (context.authenticated &&
                     ((sessionCipher == SessionCipher::AES) ||
                      (sessionCipher == SessionCipher::DES3_3K) ||
                      (sessionCipher == SessionCipher::DES3_2K && context.authScheme == SessionAuthScheme::Iso)))
            {
                etl::vector<uint8_t, 8> cmacMessage;
                cmacMessage.push_back(READ_RECORDS_COMMAND_CODE);
                cmacMessage.push_back(options.fileNo);
                appendLe24(cmacMessage, options.recordOffset);
                appendLe24(cmacMessage, options.recordCount);

                auto requestIvResult = SecureMessagingPolicy::derivePlainRequestIv(context, cmacMessage, true);
                if (!requestIvResult)
                {
                    return etl::unexpected(requestIvResult.error());
                }

                requestIv.clear();
                const auto& derivedIv = requestIvResult.value();
                for (size_t i = 0U; i < derivedIv.size(); ++i)
                {
                    requestIv.push_back(derivedIv[i]);
                }
                hasRequestIv = true;
            }

            request.commandCode = READ_RECORDS_COMMAND_CODE;
            request.data.push_back(options.fileNo);
            appendLe24(request.data, options.recordOffset);
            appendLe24(request.data, options.recordCount);
            return request;
        }

        case Stage::AdditionalFrame:
            request.commandCode = ADDITIONAL_FRAME_COMMAND_CODE;
            return request;

        case Stage::Complete:
        default:
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }
}

etl::expected<DesfireResult, error::Error> ReadRecordsCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    if (stage != Stage::Initial && stage != Stage::AdditionalFrame)
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

    if (!result.isSuccess() && !result.isAdditionalFrame())
    {
        return etl::unexpected(error::Error::fromDesfire(static_cast<error::DesfireError>(result.statusCode)));
    }

    for (size_t i = 1U; i < response.size(); ++i)
    {
        if (result.data.full() || data.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }

        const uint8_t value = response[i];
        result.data.push_back(value);
        data.push_back(value);
    }

    if (result.isAdditionalFrame())
    {
        stage = Stage::AdditionalFrame;
        return result;
    }

    if (activeCommunicationSettings == 0x03U)
    {
        if (!tryDecodeEncryptedRecords(context))
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
        }
    }
    else if (context.authenticated && hasRequestIv)
    {
        const size_t expected = static_cast<size_t>(options.expectedDataLength);
        auto verifyResult = SecureMessagingPolicy::verifyAuthenticatedPlainPayloadAutoMacAndUpdateContextIv(
            context,
            data,
            result.statusCode,
            requestIv,
            expected);
        if (!verifyResult)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
        }

        if (data.size() != expected)
        {
            etl::vector<uint8_t, MAX_READ_RECORDS_BUFFER_SIZE> plainData;
            for (size_t i = 0U; i < expected; ++i)
            {
                if (plainData.full())
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
                }
                plainData.push_back(data[i]);
            }
            data = plainData;
        }
    }
    else if (!trimAuthenticatedTrailingMac(context))
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    stage = Stage::Complete;
    return result;
}

bool ReadRecordsCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void ReadRecordsCommand::reset()
{
    stage = Stage::Initial;
    data.clear();
    activeCommunicationSettings = 0x00U;
    sessionCipher = SessionCipher::UNKNOWN;
    requestIv.clear();
    hasRequestIv = false;
}

const etl::ivector<uint8_t>& ReadRecordsCommand::getData() const
{
    return data;
}

void ReadRecordsCommand::appendLe24(etl::ivector<uint8_t>& target, uint32_t value)
{
    target.push_back(static_cast<uint8_t>(value & 0xFFU));
    target.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    target.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
}

bool ReadRecordsCommand::validateOptions() const
{
    if (options.fileNo > 0x1FU)
    {
        return false;
    }

    if (options.recordCount > 0x00FFFFFFU)
    {
        return false;
    }

    if (options.recordOffset > 0x00FFFFFFU)
    {
        return false;
    }

    if (options.expectedDataLength == 0U || options.expectedDataLength > MAX_READ_RECORDS_SIZE)
    {
        return false;
    }

    if (options.recordSize == 0U || options.recordSize > 0x00FFFFFFU)
    {
        return false;
    }

    if ((options.expectedDataLength % options.recordSize) != 0U)
    {
        return false;
    }

    const uint64_t expectedByCount =
        static_cast<uint64_t>(options.recordCount) * static_cast<uint64_t>(options.recordSize);
    if (expectedByCount != static_cast<uint64_t>(options.expectedDataLength))
    {
        return false;
    }

    if (options.communicationSettings != 0x00U &&
        options.communicationSettings != 0x01U &&
        options.communicationSettings != 0x03U &&
        options.communicationSettings != 0xFFU)
    {
        return false;
    }

    return true;
}

uint8_t ReadRecordsCommand::resolveCommunicationSettings(const DesfireContext& context) const
{
    if (options.communicationSettings != 0xFFU)
    {
        return options.communicationSettings;
    }

    return context.authenticated ? 0x03U : 0x00U;
}

ReadRecordsCommand::SessionCipher ReadRecordsCommand::resolveSessionCipher(const DesfireContext& context) const
{
    switch (SecureMessagingPolicy::resolveSessionCipher(context))
    {
        case SecureMessagingPolicy::SessionCipher::DES:
            return SessionCipher::DES;
        case SecureMessagingPolicy::SessionCipher::DES3_2K:
            return SessionCipher::DES3_2K;
        case SecureMessagingPolicy::SessionCipher::DES3_3K:
            return SessionCipher::DES3_3K;
        case SecureMessagingPolicy::SessionCipher::AES:
            return SessionCipher::AES;
        default:
            return SessionCipher::UNKNOWN;
    }
}

bool ReadRecordsCommand::tryDecodeEncryptedRecords(DesfireContext& context)
{
    const size_t expectedLength = static_cast<size_t>(options.expectedDataLength);
    if (expectedLength > MAX_READ_RECORDS_SIZE || data.size() == 0U)
    {
        return false;
    }

    const size_t blockSize = (sessionCipher == SessionCipher::AES) ? 16U : 8U;
    if (data.size() < blockSize)
    {
        return false;
    }

    const size_t trimCandidates[4] = {0U, 8U, 4U, 2U};
    for (size_t trimIndex = 0U; trimIndex < 4U; ++trimIndex)
    {
        const size_t trim = trimCandidates[trimIndex];
        if (data.size() <= trim)
        {
            continue;
        }

        const size_t candidateLength = data.size() - trim;
        if ((candidateLength % blockSize) != 0U)
        {
            continue;
        }

        etl::vector<uint8_t, MAX_READ_RECORDS_BUFFER_SIZE> ciphertext;
        for (size_t i = 0U; i < candidateLength; ++i)
        {
            if (ciphertext.full())
            {
                return false;
            }
            ciphertext.push_back(data[i]);
        }

        const size_t ivAttempts = hasRequestIv ? 2U : 1U;
        for (size_t ivAttempt = 0U; ivAttempt < ivAttempts; ++ivAttempt)
        {
            DesfireContext decodeContext = context;
            if (ivAttempt == 1U)
            {
                decodeContext.iv.clear();
                for (size_t i = 0; i < requestIv.size(); ++i)
                {
                    decodeContext.iv.push_back(requestIv[i]);
                }
            }

            etl::vector<uint8_t, MAX_READ_RECORDS_BUFFER_SIZE> plaintext;
            if (!decryptPayload(ciphertext, decodeContext, sessionCipher, plaintext))
            {
                continue;
            }

            const uint8_t crcCandidates[2] = {4U, 2U};
            for (size_t crcIndex = 0U; crcIndex < 2U; ++crcIndex)
            {
                const size_t crcLength = crcCandidates[crcIndex];
                if (plaintext.size() < (expectedLength + crcLength))
                {
                    continue;
                }

                bool paddingOk = true;
                for (size_t i = expectedLength + crcLength; i < plaintext.size(); ++i)
                {
                    const uint8_t byte = plaintext[i];
                    if (byte != 0x00U && !(byte == 0x80U && i == (expectedLength + crcLength)))
                    {
                        paddingOk = false;
                        break;
                    }
                }
                if (!paddingOk)
                {
                    continue;
                }

                bool crcOk = false;
                if (crcLength == 4U)
                {
                    uint32_t received =
                        static_cast<uint32_t>(plaintext[expectedLength]) |
                        (static_cast<uint32_t>(plaintext[expectedLength + 1U]) << 8U) |
                        (static_cast<uint32_t>(plaintext[expectedLength + 2U]) << 16U) |
                        (static_cast<uint32_t>(plaintext[expectedLength + 3U]) << 24U);

                    etl::vector<uint8_t, MAX_READ_RECORDS_SIZE + 1U> crcInput;
                    for (size_t i = 0U; i < expectedLength; ++i)
                    {
                        crcInput.push_back(plaintext[i]);
                    }
                    const uint32_t expectedNoStatus = SecureMessagingPolicy::calculateCrc32Desfire(crcInput);
                    crcInput.push_back(0x00U);
                    const uint32_t expectedWithStatus = SecureMessagingPolicy::calculateCrc32Desfire(crcInput);

                    etl::vector<uint8_t, MAX_READ_RECORDS_SIZE + 16U> crcInputWithHeader;
                    crcInputWithHeader.push_back(READ_RECORDS_COMMAND_CODE);
                    crcInputWithHeader.push_back(options.fileNo);
                    appendLe24(crcInputWithHeader, options.recordOffset);
                    appendLe24(crcInputWithHeader, options.recordCount);
                    for (size_t i = 0U; i < expectedLength; ++i)
                    {
                        crcInputWithHeader.push_back(plaintext[i]);
                    }
                    const uint32_t expectedWithHeaderNoStatus =
                        SecureMessagingPolicy::calculateCrc32Desfire(crcInputWithHeader);
                    crcInputWithHeader.push_back(0x00U);
                    const uint32_t expectedWithHeaderStatus =
                        SecureMessagingPolicy::calculateCrc32Desfire(crcInputWithHeader);

                    crcOk =
                        (received == expectedNoStatus) ||
                        (received == expectedWithStatus) ||
                        (received == expectedWithHeaderNoStatus) ||
                        (received == expectedWithHeaderStatus);
                }
                else
                {
                    uint16_t received =
                        static_cast<uint16_t>(plaintext[expectedLength]) |
                        (static_cast<uint16_t>(plaintext[expectedLength + 1U]) << 8U);

                    etl::vector<uint8_t, MAX_READ_RECORDS_SIZE + 1U> crcInput;
                    for (size_t i = 0U; i < expectedLength; ++i)
                    {
                        crcInput.push_back(plaintext[i]);
                    }
                    const uint16_t expectedNoStatus = SecureMessagingPolicy::calculateCrc16(crcInput);
                    crcInput.push_back(0x00U);
                    const uint16_t expectedWithStatus = SecureMessagingPolicy::calculateCrc16(crcInput);
                    crcOk = (received == expectedNoStatus) || (received == expectedWithStatus);
                }

                if (!crcOk)
                {
                    continue;
                }

                data.clear();
                for (size_t i = 0U; i < expectedLength; ++i)
                {
                    if (data.full())
                    {
                        return false;
                    }
                    data.push_back(plaintext[i]);
                }

                if (candidateLength >= blockSize)
                {
                    auto ivUpdateResult = SecureMessagingPolicy::updateContextIvFromEncryptedCiphertext(
                        context,
                        ciphertext);
                    if (!ivUpdateResult)
                    {
                        return false;
                    }
                }

                return true;
            }
        }
    }

    return false;
}

bool ReadRecordsCommand::decryptPayload(
    const etl::ivector<uint8_t>& ciphertext,
    const DesfireContext& context,
    SessionCipher cipher,
    etl::vector<uint8_t, MAX_READ_RECORDS_BUFFER_SIZE>& plaintext) const
{
    const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
    if ((ciphertext.size() % blockSize) != 0U)
    {
        return false;
    }

    plaintext.clear();
    for (size_t i = 0; i < ciphertext.size(); ++i)
    {
        if (plaintext.full())
        {
            return false;
        }
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

        if (context.authScheme == SessionAuthScheme::Legacy)
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

bool ReadRecordsCommand::trimAuthenticatedTrailingMac(const DesfireContext& context)
{
    const size_t expected = static_cast<size_t>(options.expectedDataLength);
    if (data.size() == expected)
    {
        return true;
    }

    if (data.size() < expected)
    {
        return false;
    }

    // In authenticated sessions current secure pipe path leaves trailing MAC/CMAC bytes in response payload.
    if (!context.authenticated)
    {
        return false;
    }

    const size_t excess = data.size() - expected;
    if (excess != 8U && excess != 4U)
    {
        return false;
    }

    for (size_t i = 0; i < excess; ++i)
    {
        data.pop_back();
    }

    return data.size() == expected;
}
