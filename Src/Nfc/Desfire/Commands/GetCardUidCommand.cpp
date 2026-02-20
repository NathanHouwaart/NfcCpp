/**
 * @file GetCardUidCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get card UID command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/GetCardUidCommand.h"
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
    constexpr uint8_t GET_CARD_UID_COMMAND_CODE = 0x51;
    constexpr size_t UID_LENGTH = 7U;
}

GetCardUidCommand::GetCardUidCommand()
    : stage(Stage::Initial)
    , uid()
{
}

etl::string_view GetCardUidCommand::name() const
{
    return "GetCardUID";
}

etl::expected<DesfireRequest, error::Error> GetCardUidCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    DesfireRequest request;
    request.commandCode = GET_CARD_UID_COMMAND_CODE;
    request.data.clear();
    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> GetCardUidCommand::parseResponse(
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

    for (size_t i = 1; i < response.size(); ++i)
    {
        result.data.push_back(response[i]);
    }

    uid.clear();

    bool parsed = false;
    if (context.authenticated && !context.sessionKeyEnc.empty())
    {
        parsed = tryDecodeEncryptedUid(result.data, context, uid);
    }

    // Fallback for plain or integrity-bytes-tailed responses.
    if (!parsed)
    {
        if (result.data.size() < UID_LENGTH)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
        }

        for (size_t i = 0; i < UID_LENGTH; ++i)
        {
            uid.push_back(result.data[i]);
        }
    }

    stage = Stage::Complete;
    return result;
}

bool GetCardUidCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void GetCardUidCommand::reset()
{
    stage = Stage::Initial;
    uid.clear();
}

const etl::vector<uint8_t, 10>& GetCardUidCommand::getUid() const
{
    return uid;
}

GetCardUidCommand::SessionCipher GetCardUidCommand::resolveSessionCipher(const DesfireContext& context) const
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

uint16_t GetCardUidCommand::calculateCrc16(const etl::ivector<uint8_t>& data) const
{
    return SecureMessagingPolicy::calculateCrc16(data);
}

uint32_t GetCardUidCommand::calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const
{
    return SecureMessagingPolicy::calculateCrc32Desfire(data);
}

bool GetCardUidCommand::tryDecodeEncryptedUid(
    const etl::ivector<uint8_t>& payload,
    DesfireContext& context,
    etl::vector<uint8_t, 10>& outUid)
{
    const SessionCipher cipher = resolveSessionCipher(context);
    if (cipher == SessionCipher::UNKNOWN)
    {
        return false;
    }

    const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
    if (payload.size() < blockSize)
    {
        return false;
    }

    // Try full length first, then trimmed variants for potential trailing MAC bytes.
    const size_t trimCandidates[4] = {0U, 8U, 4U, 2U};
    for (size_t trimIndex = 0; trimIndex < 4; ++trimIndex)
    {
        const size_t trim = trimCandidates[trimIndex];
        if (payload.size() <= trim)
        {
            continue;
        }

        const size_t candidateLen = payload.size() - trim;
        if ((candidateLen % blockSize) != 0U)
        {
            continue;
        }

        etl::vector<uint8_t, 32> ciphertext;
        for (size_t i = 0; i < candidateLen; ++i)
        {
            ciphertext.push_back(payload[i]);
        }

        etl::vector<uint8_t, 32> plaintext;
        if (!decryptPayload(ciphertext, context, cipher, plaintext))
        {
            continue;
        }

        if (plaintext.size() < UID_LENGTH)
        {
            continue;
        }

        bool crcOk = false;
        if (cipher == SessionCipher::AES || cipher == SessionCipher::DES3_3K)
        {
            if (plaintext.size() >= UID_LENGTH + 4U)
            {
                etl::vector<uint8_t, 8> crcData;
                for (size_t i = 0; i < UID_LENGTH; ++i)
                {
                    crcData.push_back(plaintext[i]);
                }
                // CRC32R behavior: CRC over data + success status 0x00.
                crcData.push_back(0x00);
                const uint32_t expectedCrc = calculateCrc32Desfire(crcData);
                const uint32_t receivedCrc =
                    static_cast<uint32_t>(plaintext[UID_LENGTH]) |
                    (static_cast<uint32_t>(plaintext[UID_LENGTH + 1]) << 8) |
                    (static_cast<uint32_t>(plaintext[UID_LENGTH + 2]) << 16) |
                    (static_cast<uint32_t>(plaintext[UID_LENGTH + 3]) << 24);
                crcOk = (expectedCrc == receivedCrc);
            }
        }
        else
        {
            if (plaintext.size() >= UID_LENGTH + 2U)
            {
                etl::vector<uint8_t, 7> crcData;
                for (size_t i = 0; i < UID_LENGTH; ++i)
                {
                    crcData.push_back(plaintext[i]);
                }
                const uint16_t expectedCrc = calculateCrc16(crcData);
                const uint16_t receivedCrc =
                    static_cast<uint16_t>(plaintext[UID_LENGTH]) |
                    (static_cast<uint16_t>(plaintext[UID_LENGTH + 1]) << 8);
                crcOk = (expectedCrc == receivedCrc);
            }
        }

        if (!crcOk)
        {
            continue;
        }

        outUid.clear();
        for (size_t i = 0; i < UID_LENGTH; ++i)
        {
            outUid.push_back(plaintext[i]);
        }

        auto ivUpdateResult = SecureMessagingPolicy::updateContextIvFromEncryptedCiphertext(context, ciphertext);
        if (!ivUpdateResult)
        {
            return false;
        }

        return true;
    }

    return false;
}

bool GetCardUidCommand::decryptPayload(
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
        iv.resize(blockSize, 0x00);
    }

    if (cipher == SessionCipher::AES)
    {
        if (context.sessionKeyEnc.size() < 16)
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
        if (context.sessionKeyEnc.size() < 8)
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
        if (context.sessionKeyEnc.size() < 16)
        {
            return false;
        }

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
        return true;
    }

    if (cipher == SessionCipher::DES3_3K)
    {
        if (context.sessionKeyEnc.size() < 24)
        {
            return false;
        }

        DES3CBC des3Cbc(
            bytesToUint64(context.sessionKeyEnc.data()),
            bytesToUint64(context.sessionKeyEnc.data() + 8),
            bytesToUint64(context.sessionKeyEnc.data() + 16),
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
