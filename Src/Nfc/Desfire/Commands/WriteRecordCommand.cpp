/**
 * @file WriteRecordCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire write record command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/WriteRecordCommand.h"
#include "Nfc/Desfire/Commands/ValueOperationCryptoUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Utils/DesfireCrypto.h"
#include "Error/DesfireError.h"
#include <cppdes/descbc.h>
#include <cppdes/des3cbc.h>
#include <aes.hpp>

using namespace nfc;
using namespace crypto;

namespace
{
    constexpr uint8_t WRITE_RECORD_COMMAND_CODE = 0x3B;
    constexpr uint16_t WRITE_RECORD_HEADER_LENGTH = 7U; // fileNo + byteOffset(3) + byteLength(3)
    constexpr uint16_t MAX_DESFIRE_REQUEST_DATA = 252U;
}

WriteRecordCommand::WriteRecordCommand(const WriteRecordCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
    , currentIndex(0U)
    , lastChunkByteLength(0U)
    , activeCommunicationSettings(0x00U)
    , sessionCipher(SessionCipher::UNKNOWN)
    , pendingIv()
    , updateContextIv(false)
    , legacyDesCryptoMode(false)
{
    resetProgress();
}

etl::string_view WriteRecordCommand::name() const
{
    return "WriteRecord";
}

etl::expected<DesfireRequest, error::Error> WriteRecordCommand::buildRequest(const DesfireContext& context)
{
    if (stage == Stage::Complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (stage == Stage::Initial)
    {
        if (!validateOptions())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
        }

        activeCommunicationSettings = resolveCommunicationSettings(context);
        if (activeCommunicationSettings == 0x01U)
        {
            // MAC-only secure messaging is not implemented in this command path yet.
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        sessionCipher = SessionCipher::UNKNOWN;
        pendingIv.clear();
        updateContextIv = false;
        legacyDesCryptoMode = false;
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

            legacyDesCryptoMode =
                (context.authScheme == SessionAuthScheme::Legacy) &&
                (sessionCipher == SessionCipher::DES || sessionCipher == SessionCipher::DES3_2K);
        }

        resetProgress();
        stage = Stage::Writing;
    }

    if (options.data == nullptr || currentIndex >= options.data->size())
    {
        stage = Stage::Complete;
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    const size_t remainingBytes = options.data->size() - currentIndex;

    const uint16_t plainChunkBudget = effectivePlainChunkBudget();
    if (plainChunkBudget == 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
    }

    uint32_t chunkByteLength = static_cast<uint32_t>(remainingBytes);
    if (chunkByteLength > plainChunkBudget)
    {
        chunkByteLength = plainChunkBudget;
    }

    const uint64_t currentByteOffset64 = static_cast<uint64_t>(options.offset) + static_cast<uint64_t>(currentIndex);
    if (currentByteOffset64 > 0x00FFFFFFULL)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
    }
    const uint32_t currentByteOffset = static_cast<uint32_t>(currentByteOffset64);

    DesfireRequest request;
    request.commandCode = WRITE_RECORD_COMMAND_CODE;
    request.data.clear();
    request.expectedResponseLength = 0U;

    request.data.push_back(options.fileNo);
    appendLe24(request.data, currentByteOffset);
    appendLe24(request.data, chunkByteLength);

    if (activeCommunicationSettings == 0x03U)
    {
        auto encryptedPayloadResult = buildEncryptedChunkPayload(context, currentByteOffset, chunkByteLength);
        if (!encryptedPayloadResult)
        {
            return etl::unexpected(encryptedPayloadResult.error());
        }

        const auto& encryptedPayload = encryptedPayloadResult.value();
        if ((request.data.size() + encryptedPayload.size()) > request.data.max_size())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }

        for (size_t i = 0U; i < encryptedPayload.size(); ++i)
        {
            request.data.push_back(encryptedPayload[i]);
        }
    }
    else
    {
        for (uint32_t i = 0U; i < chunkByteLength; ++i)
        {
            request.data.push_back((*options.data)[currentIndex + static_cast<size_t>(i)]);
        }
        updateContextIv = false;
        pendingIv.clear();
    }

    lastChunkByteLength = chunkByteLength;
    return request;
}

etl::expected<DesfireResult, error::Error> WriteRecordCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    if (stage != Stage::Writing)
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

    for (size_t i = 1U; i < response.size(); ++i)
    {
        if (result.data.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }
        result.data.push_back(response[i]);
    }

    if (updateContextIv && activeCommunicationSettings == 0x03U)
    {
        if (sessionCipher == SessionCipher::AES)
        {
            auto nextIvResult = valueop_detail::deriveAesResponseIvForValueOperation(response, context, pendingIv);
            if (!nextIvResult)
            {
                return etl::unexpected(nextIvResult.error());
            }

            context.iv.clear();
            const auto& nextIv = nextIvResult.value();
            for (size_t i = 0; i < nextIv.size(); ++i)
            {
                context.iv.push_back(nextIv[i]);
            }
        }
        else if (sessionCipher == SessionCipher::DES3_3K)
        {
            auto nextIvResult = valueop_detail::deriveTktdesResponseIvForValueOperation(response, context, pendingIv);
            if (!nextIvResult)
            {
                return etl::unexpected(nextIvResult.error());
            }

            context.iv.clear();
            const auto& nextIv = nextIvResult.value();
            for (size_t i = 0; i < nextIv.size(); ++i)
            {
                context.iv.push_back(nextIv[i]);
            }
        }
        else if (sessionCipher == SessionCipher::DES3_2K && !legacyDesCryptoMode)
        {
            auto nextIvResult = valueop_detail::deriveTktdesResponseIvForValueOperation(response, context, pendingIv);
            if (!nextIvResult)
            {
                return etl::unexpected(nextIvResult.error());
            }

            context.iv.clear();
            const auto& nextIv = nextIvResult.value();
            for (size_t i = 0; i < nextIv.size(); ++i)
            {
                context.iv.push_back(nextIv[i]);
            }
        }
        else if (sessionCipher == SessionCipher::DES || (sessionCipher == SessionCipher::DES3_2K && legacyDesCryptoMode))
        {
            // Legacy DES/2K3DES secure messaging uses command-local chaining.
            // Keep IV reset between commands.
            context.iv.clear();
            context.iv.resize(8U, 0x00U);
        }
        else
        {
            context.iv.clear();
            for (size_t i = 0; i < pendingIv.size(); ++i)
            {
                context.iv.push_back(pendingIv[i]);
            }
        }
    }

    if (options.data == nullptr || lastChunkByteLength == 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if ((currentIndex + static_cast<size_t>(lastChunkByteLength)) > options.data->size())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    currentIndex += static_cast<size_t>(lastChunkByteLength);
    lastChunkByteLength = 0U;

    if (currentIndex >= options.data->size())
    {
        stage = Stage::Complete;
    }
    else
    {
        stage = Stage::Writing;
    }

    return result;
}

bool WriteRecordCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void WriteRecordCommand::reset()
{
    stage = Stage::Initial;
    activeCommunicationSettings = 0x00U;
    sessionCipher = SessionCipher::UNKNOWN;
    pendingIv.clear();
    updateContextIv = false;
    legacyDesCryptoMode = false;
    resetProgress();
}

void WriteRecordCommand::appendLe24(etl::ivector<uint8_t>& target, uint32_t value)
{
    target.push_back(static_cast<uint8_t>(value & 0xFFU));
    target.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    target.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
}

void WriteRecordCommand::appendLe16(etl::ivector<uint8_t>& target, uint16_t value)
{
    target.push_back(static_cast<uint8_t>(value & 0xFFU));
    target.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
}

void WriteRecordCommand::appendLe32(etl::ivector<uint8_t>& target, uint32_t value)
{
    target.push_back(static_cast<uint8_t>(value & 0xFFU));
    target.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    target.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
    target.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
}

bool WriteRecordCommand::validateOptions() const
{
    if (options.fileNo > 0x1FU)
    {
        return false;
    }

    if (options.data == nullptr || options.data->empty())
    {
        return false;
    }

    if (options.recordSize == 0U || options.recordSize > 0x00FFFFFFU)
    {
        return false;
    }

    if (options.offset >= options.recordSize)
    {
        return false;
    }

    const uint64_t endWithinRecord =
        static_cast<uint64_t>(options.offset) + static_cast<uint64_t>(options.data->size());
    if (endWithinRecord > static_cast<uint64_t>(options.recordSize))
    {
        return false;
    }

    const uint64_t endExclusive = static_cast<uint64_t>(options.offset) + static_cast<uint64_t>(options.data->size());
    if (endExclusive > 0x01000000ULL)
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

    if (effectiveChunkSize() == 0U)
    {
        return false;
    }

    return true;
}

uint16_t WriteRecordCommand::effectiveChunkSize() const
{
    const uint16_t requestedChunkSize = (options.chunkSize == 0U) ? DEFAULT_CHUNK_SIZE : options.chunkSize;
    return (requestedChunkSize < MAX_CHUNK_SIZE) ? requestedChunkSize : MAX_CHUNK_SIZE;
}

uint8_t WriteRecordCommand::resolveCommunicationSettings(const DesfireContext& context) const
{
    if (options.communicationSettings != 0xFFU)
    {
        return options.communicationSettings;
    }

    return context.authenticated ? 0x03U : 0x00U;
}

WriteRecordCommand::SessionCipher WriteRecordCommand::resolveSessionCipher(const DesfireContext& context) const
{
    if (context.iv.size() == 16U && context.sessionKeyEnc.size() >= 16U)
    {
        return SessionCipher::AES;
    }

    if (context.sessionKeyEnc.size() == 8U)
    {
        return SessionCipher::DES;
    }
    if (context.sessionKeyEnc.size() == 16U)
    {
        return SessionCipher::DES3_2K;
    }
    if (context.sessionKeyEnc.size() == 24U)
    {
        return SessionCipher::DES3_3K;
    }

    return SessionCipher::UNKNOWN;
}

uint16_t WriteRecordCommand::effectivePlainChunkBudget() const
{
    const uint16_t requestedBudget = effectiveChunkSize();
    if (activeCommunicationSettings != 0x03U)
    {
        return requestedBudget;
    }

    if (sessionCipher == SessionCipher::UNKNOWN)
    {
        return 0U;
    }

    const uint16_t blockSize = (sessionCipher == SessionCipher::AES) ? 16U : 8U;
    const uint16_t crcLength =
        (sessionCipher == SessionCipher::DES || (sessionCipher == SessionCipher::DES3_2K && legacyDesCryptoMode))
            ? 2U
            : 4U;

    if (MAX_DESFIRE_REQUEST_DATA <= WRITE_RECORD_HEADER_LENGTH)
    {
        return 0U;
    }

    const uint16_t maxCiphertextLength = static_cast<uint16_t>(MAX_DESFIRE_REQUEST_DATA - WRITE_RECORD_HEADER_LENGTH);
    const uint16_t alignedCiphertextLength = static_cast<uint16_t>(
        (maxCiphertextLength / blockSize) * blockSize);

    if (alignedCiphertextLength <= crcLength)
    {
        return 0U;
    }

    const uint16_t securePlainBudget = static_cast<uint16_t>(alignedCiphertextLength - crcLength);
    return (securePlainBudget < requestedBudget) ? securePlainBudget : requestedBudget;
}

etl::expected<etl::vector<uint8_t, WriteRecordCommand::MAX_CHUNK_SIZE + 32U>, error::Error>
WriteRecordCommand::buildEncryptedChunkPayload(
    const DesfireContext& context,
    uint32_t chunkByteOffset,
    uint32_t chunkByteLength)
{
    etl::vector<uint8_t, MAX_CHUNK_SIZE + 32U> plaintext;
    for (uint32_t i = 0U; i < chunkByteLength; ++i)
    {
        plaintext.push_back((*options.data)[currentIndex + static_cast<size_t>(i)]);
    }

    if (sessionCipher == SessionCipher::DES || (sessionCipher == SessionCipher::DES3_2K && legacyDesCryptoMode))
    {
        const uint16_t crc16 = valueop_detail::calculateCrc16(plaintext);
        appendLe16(plaintext, crc16);
    }
    else
    {
        etl::vector<uint8_t, MAX_CHUNK_SIZE + 16U> crcInput;
        crcInput.push_back(WRITE_RECORD_COMMAND_CODE);
        crcInput.push_back(options.fileNo);
        appendLe24(crcInput, chunkByteOffset);
        appendLe24(crcInput, chunkByteLength);
        for (size_t i = 0U; i < plaintext.size(); ++i)
        {
            crcInput.push_back(plaintext[i]);
        }

        const uint32_t crc32 = valueop_detail::calculateCrc32Desfire(crcInput);
        appendLe32(plaintext, crc32);
    }

    const size_t blockSize = (sessionCipher == SessionCipher::AES) ? 16U : 8U;
    while ((plaintext.size() % blockSize) != 0U)
    {
        plaintext.push_back(0x00U);
    }

    return encryptPayload(plaintext, context, sessionCipher);
}

etl::expected<etl::vector<uint8_t, WriteRecordCommand::MAX_CHUNK_SIZE + 32U>, error::Error>
WriteRecordCommand::encryptPayload(
    const etl::ivector<uint8_t>& plaintext,
    const DesfireContext& context,
    SessionCipher cipher)
{
    const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
    if ((plaintext.size() % blockSize) != 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
    }

    etl::vector<uint8_t, MAX_CHUNK_SIZE + 32U> encrypted;

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

        for (size_t i = 0; i < context.iv.size(); ++i)
        {
            iv.push_back(context.iv[i]);
        }
    }

    if (cipher == SessionCipher::AES)
    {
        if (context.sessionKeyEnc.size() < 16U)
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

        // Legacy single-DES "send mode":
        // C_i = D_K(P_i XOR C_{i-1}), C_-1 = 0.
        uint8_t previousBlock[8] = {0};
        for (size_t offset = 0; offset < plaintext.size(); offset += 8U)
        {
            uint8_t xoredBlock[8];
            for (size_t i = 0; i < 8U; ++i)
            {
                xoredBlock[i] = static_cast<uint8_t>(plaintext[offset + i] ^ previousBlock[i]);
            }

            uint8_t transformedBlock[8];
            DesFireCrypto::desDecrypt(
                xoredBlock,
                context.sessionKeyEnc.data(),
                transformedBlock);

            for (size_t i = 0; i < 8U; ++i)
            {
                encrypted.push_back(transformedBlock[i]);
                previousBlock[i] = transformedBlock[i];
            }
        }
    }
    else if (cipher == SessionCipher::DES3_2K)
    {
        if (context.sessionKeyEnc.size() < 16U)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

        if (legacyDesCryptoMode)
        {
            // Legacy 2K3DES "send mode":
            // C_i = D_3DES(P_i XOR C_{i-1}), C_-1 = 0.
            uint8_t previousBlock[8] = {0};
            for (size_t offset = 0; offset < plaintext.size(); offset += 8U)
            {
                uint8_t xoredBlock[8];
                for (size_t i = 0; i < 8U; ++i)
                {
                    xoredBlock[i] = static_cast<uint8_t>(plaintext[offset + i] ^ previousBlock[i]);
                }

                uint8_t transformedBlock[8];
                DesFireCrypto::des3Decrypt(
                    xoredBlock,
                    context.sessionKeyEnc.data(),
                    transformedBlock);

                for (size_t i = 0; i < 8U; ++i)
                {
                    encrypted.push_back(transformedBlock[i]);
                    previousBlock[i] = transformedBlock[i];
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

void WriteRecordCommand::resetProgress()
{
    currentIndex = 0U;
    lastChunkByteLength = 0U;
}
