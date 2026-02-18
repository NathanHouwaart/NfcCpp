/**
 * @file ValueOperationCryptoUtils.h
 * @brief Internal helper utilities for DESFire value operation secure messaging
 */

#pragma once

#include "Nfc/Desfire/DesfireContext.h"
#include "Error/Error.h"
#include "Error/DesfireError.h"
#include "Utils/DesfireCrypto.h"
#include <etl/expected.h>
#include <etl/vector.h>
#include <cppdes/descbc.h>
#include <cppdes/des3cbc.h>
#include <aes.hpp>

namespace nfc
{
    namespace valueop_detail
    {
        constexpr uint8_t AES_CMAC_RB = 0x87U;
        constexpr uint8_t TKTDES_CMAC_RB = 0x1BU;

        enum class SessionCipher : uint8_t
        {
            DES,
            DES3_2K,
            DES3_3K,
            AES,
            UNKNOWN
        };

        inline SessionCipher resolveSessionCipher(const DesfireContext& context)
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

        inline bool useLegacyDesCryptoMode(const DesfireContext& context, SessionCipher cipher)
        {
            return (context.authScheme == SessionAuthScheme::Legacy) &&
                   (cipher == SessionCipher::DES || cipher == SessionCipher::DES3_2K);
        }

        inline uint16_t calculateCrc16(const etl::ivector<uint8_t>& data)
        {
            uint16_t crc = 0x6363U;
            for (size_t i = 0; i < data.size(); ++i)
            {
                uint8_t value = static_cast<uint8_t>(data[i] ^ static_cast<uint8_t>(crc & 0x00FFU));
                value = static_cast<uint8_t>(value ^ static_cast<uint8_t>(value << 4U));
                crc = static_cast<uint16_t>(
                    (crc >> 8U) ^
                    (static_cast<uint16_t>(value) << 8U) ^
                    (static_cast<uint16_t>(value) << 3U) ^
                    (static_cast<uint16_t>(value) >> 4U));
            }
            return crc;
        }

        inline uint32_t calculateCrc32Desfire(const etl::ivector<uint8_t>& data)
        {
            uint32_t crc = 0xFFFFFFFFU;
            for (size_t i = 0; i < data.size(); ++i)
            {
                crc ^= static_cast<uint32_t>(data[i]);
                for (size_t bit = 0; bit < 8U; ++bit)
                {
                    if ((crc & 0x01U) != 0U)
                    {
                        crc = (crc >> 1U) ^ 0xEDB88320U;
                    }
                    else
                    {
                        crc >>= 1U;
                    }
                }
            }

            const uint32_t standardCrc32 = crc ^ 0xFFFFFFFFU;
            return ~standardCrc32;
        }

        inline void xorBlock16(const uint8_t* a, const uint8_t* b, uint8_t* out)
        {
            for (size_t i = 0; i < 16U; ++i)
            {
                out[i] = static_cast<uint8_t>(a[i] ^ b[i]);
            }
        }

        inline void xorBlock8(const uint8_t* a, const uint8_t* b, uint8_t* out)
        {
            for (size_t i = 0; i < 8U; ++i)
            {
                out[i] = static_cast<uint8_t>(a[i] ^ b[i]);
            }
        }

        inline void leftShiftOneBit16(const uint8_t* input, uint8_t* output)
        {
            uint8_t overflow = 0x00U;
            for (int i = 15; i >= 0; --i)
            {
                const uint8_t byte = input[i];
                output[i] = static_cast<uint8_t>((byte << 1U) | overflow);
                overflow = static_cast<uint8_t>((byte & 0x80U) ? 0x01U : 0x00U);
            }
        }

        inline void leftShiftOneBit8(const uint8_t* input, uint8_t* output)
        {
            uint8_t overflow = 0x00U;
            for (int i = 7; i >= 0; --i)
            {
                const uint8_t byte = input[i];
                output[i] = static_cast<uint8_t>((byte << 1U) | overflow);
                overflow = static_cast<uint8_t>((byte & 0x80U) ? 0x01U : 0x00U);
            }
        }

        inline bool encryptTktdesBlock(
            const uint8_t* key,
            size_t keyLength,
            const uint8_t* input,
            uint8_t* output)
        {
            if (!key || !input || !output)
            {
                return false;
            }

            if (keyLength != 16U && keyLength != 24U)
            {
                return false;
            }

            const uint64_t k1 = crypto::bytesToUint64(key);
            const uint64_t k2 = crypto::bytesToUint64(key + 8U);
            const uint64_t k3 = (keyLength == 24U) ? crypto::bytesToUint64(key + 16U) : k1;
            constexpr uint64_t zeroIv = 0U;

            DES3CBC des3Cbc(k1, k2, k3, zeroIv);
            const uint64_t cipherBlock = des3Cbc.encrypt(crypto::bytesToUint64(input));
            crypto::uint64ToBytes(cipherBlock, output);
            return true;
        }

        inline bool generateTktdesCmacSubkeys(
            const uint8_t* key,
            size_t keyLength,
            uint8_t* k1,
            uint8_t* k2)
        {
            if (!key || !k1 || !k2)
            {
                return false;
            }

            uint8_t l[8] = {0};
            const uint8_t zeroBlock[8] = {0};
            if (!encryptTktdesBlock(key, keyLength, zeroBlock, l))
            {
                return false;
            }

            leftShiftOneBit8(l, k1);
            if ((l[0] & 0x80U) != 0U)
            {
                k1[7] ^= TKTDES_CMAC_RB;
            }

            leftShiftOneBit8(k1, k2);
            if ((k1[0] & 0x80U) != 0U)
            {
                k2[7] ^= TKTDES_CMAC_RB;
            }

            return true;
        }

        inline void padCmacBlock8(const uint8_t* source, size_t sourceLength, uint8_t* outBlock)
        {
            for (size_t i = 0; i < 8U; ++i)
            {
                outBlock[i] = 0x00U;
            }

            if (source != nullptr)
            {
                for (size_t i = 0; i < sourceLength; ++i)
                {
                    outBlock[i] = source[i];
                }
            }

            outBlock[sourceLength] = 0x80U;
        }

        inline bool calculateTktdesCmac(
            const uint8_t* key,
            size_t keyLength,
            const uint8_t* initialIv,
            const uint8_t* message,
            size_t messageLength,
            uint8_t* outCmac)
        {
            if (!key || !initialIv || !outCmac)
            {
                return false;
            }

            if (messageLength > 0U && !message)
            {
                return false;
            }

            uint8_t k1[8];
            uint8_t k2[8];
            if (!generateTktdesCmacSubkeys(key, keyLength, k1, k2))
            {
                return false;
            }

            size_t blockCount = (messageLength + 7U) / 8U;
            bool lastBlockComplete = (messageLength != 0U) && ((messageLength % 8U) == 0U);
            if (blockCount == 0U)
            {
                blockCount = 1U;
                lastBlockComplete = false;
            }

            uint8_t mLast[8];
            if (lastBlockComplete)
            {
                const size_t lastOffset = (blockCount - 1U) * 8U;
                xorBlock8(message + lastOffset, k1, mLast);
            }
            else
            {
                const size_t lastOffset = (blockCount - 1U) * 8U;
                const size_t lastLength = messageLength - lastOffset;
                uint8_t padded[8];
                const uint8_t* lastPtr = (lastLength == 0U) ? nullptr : (message + lastOffset);
                padCmacBlock8(lastPtr, lastLength, padded);
                xorBlock8(padded, k2, mLast);
            }

            uint8_t x[8];
            for (size_t i = 0; i < 8U; ++i)
            {
                x[i] = initialIv[i];
            }

            uint8_t y[8];
            for (size_t blockIndex = 0U; blockIndex + 1U < blockCount; ++blockIndex)
            {
                const uint8_t* blockPtr = message + (blockIndex * 8U);
                xorBlock8(x, blockPtr, y);
                if (!encryptTktdesBlock(key, keyLength, y, x))
                {
                    return false;
                }
            }

            xorBlock8(x, mLast, y);
            return encryptTktdesBlock(key, keyLength, y, outCmac);
        }

        inline void aesEncryptBlock(const uint8_t* key, const uint8_t* input, uint8_t* output)
        {
            uint8_t block[16];
            for (size_t i = 0; i < 16U; ++i)
            {
                block[i] = input[i];
            }

            AES_ctx aesContext;
            AES_init_ctx(&aesContext, key);
            AES_ECB_encrypt(&aesContext, block);

            for (size_t i = 0; i < 16U; ++i)
            {
                output[i] = block[i];
            }
        }

        inline void generateAesCmacSubkeys(const uint8_t* key, uint8_t* k1, uint8_t* k2)
        {
            uint8_t l[16] = {0};
            uint8_t zeroBlock[16] = {0};
            aesEncryptBlock(key, zeroBlock, l);

            leftShiftOneBit16(l, k1);
            if ((l[0] & 0x80U) != 0U)
            {
                k1[15] ^= AES_CMAC_RB;
            }

            leftShiftOneBit16(k1, k2);
            if ((k1[0] & 0x80U) != 0U)
            {
                k2[15] ^= AES_CMAC_RB;
            }
        }

        inline void padCmacBlock(const uint8_t* source, size_t sourceLength, uint8_t* outBlock)
        {
            for (size_t i = 0; i < 16U; ++i)
            {
                outBlock[i] = 0x00U;
            }

            for (size_t i = 0; i < sourceLength; ++i)
            {
                outBlock[i] = source[i];
            }
            outBlock[sourceLength] = 0x80U;
        }

        inline bool calculateAesCmac(
            const uint8_t* key,
            const uint8_t* initialIv,
            const uint8_t* message,
            size_t messageLength,
            uint8_t* outCmac)
        {
            if (!key || !initialIv || !message || !outCmac)
            {
                return false;
            }

            uint8_t k1[16];
            uint8_t k2[16];
            generateAesCmacSubkeys(key, k1, k2);

            size_t blockCount = (messageLength + 15U) / 16U;
            bool lastBlockComplete = (messageLength != 0U) && ((messageLength % 16U) == 0U);
            if (blockCount == 0U)
            {
                blockCount = 1U;
                lastBlockComplete = false;
            }

            uint8_t mLast[16];
            if (lastBlockComplete)
            {
                const size_t lastOffset = (blockCount - 1U) * 16U;
                xorBlock16(message + lastOffset, k1, mLast);
            }
            else
            {
                const size_t lastOffset = (blockCount - 1U) * 16U;
                const size_t lastLength = messageLength - lastOffset;
                uint8_t padded[16];
                padCmacBlock(message + lastOffset, lastLength, padded);
                xorBlock16(padded, k2, mLast);
            }

            uint8_t x[16];
            for (size_t i = 0; i < 16U; ++i)
            {
                x[i] = initialIv[i];
            }

            uint8_t y[16];
            for (size_t blockIndex = 0; blockIndex + 1U < blockCount; ++blockIndex)
            {
                const uint8_t* blockPtr = message + (blockIndex * 16U);
                xorBlock16(x, blockPtr, y);
                aesEncryptBlock(key, y, x);
            }

            xorBlock16(x, mLast, y);
            aesEncryptBlock(key, y, outCmac);
            return true;
        }

        inline etl::expected<etl::vector<uint8_t, 16>, error::Error> deriveAesResponseIvForValueOperation(
            const etl::ivector<uint8_t>& response,
            const DesfireContext& context,
            const etl::ivector<uint8_t>& requestIv)
        {
            if (response.empty())
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }

            if (context.sessionKeyEnc.size() < 16U || requestIv.size() != 16U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            // Value operations return [status] with optional 8-byte truncated CMAC.
            if (response.size() != 1U && response.size() != 9U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }

            const uint8_t statusByte = response[0];
            uint8_t cmac[16] = {0};
            if (!calculateAesCmac(
                    context.sessionKeyEnc.data(),
                    requestIv.data(),
                    &statusByte,
                    1U,
                    cmac))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            if (response.size() == 9U)
            {
                for (size_t i = 0; i < 8U; ++i)
                {
                    if (response[1U + i] != cmac[i])
                    {
                        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::IntegrityError));
                    }
                }
            }

            etl::vector<uint8_t, 16> nextIv;
            for (size_t i = 0; i < 16U; ++i)
            {
                nextIv.push_back(cmac[i]);
            }
            return nextIv;
        }

        inline void appendLe24(etl::ivector<uint8_t>& out, uint32_t value)
        {
            out.push_back(static_cast<uint8_t>(value & 0xFFU));
            out.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
            out.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
        }

        inline bool deriveAesPlainRequestIv(
            const DesfireContext& context,
            const etl::ivector<uint8_t>& message,
            etl::vector<uint8_t, 16>& outIv,
            bool useZeroIvWhenContextIvMissing = false)
        {
            if (context.sessionKeyEnc.size() < 16U || message.empty())
            {
                return false;
            }

            uint8_t initialIv[16] = {0};
            if (context.iv.size() >= 16U)
            {
                for (size_t i = 0; i < 16U; ++i)
                {
                    initialIv[i] = context.iv[i];
                }
            }
            else if (!useZeroIvWhenContextIvMissing)
            {
                return false;
            }

            uint8_t cmac[16] = {0};
            if (!calculateAesCmac(
                    context.sessionKeyEnc.data(),
                    initialIv,
                    message.data(),
                    message.size(),
                    cmac))
            {
                return false;
            }

            outIv.clear();
            for (size_t i = 0; i < 16U; ++i)
            {
                outIv.push_back(cmac[i]);
            }

            return true;
        }

        inline bool deriveTktdesPlainRequestIv(
            const DesfireContext& context,
            const etl::ivector<uint8_t>& message,
            etl::vector<uint8_t, 16>& outIv,
            bool useZeroIvWhenContextIvMissing = false)
        {
            if (message.empty())
            {
                return false;
            }

            const size_t keyLength = context.sessionKeyEnc.size();
            if (keyLength != 16U && keyLength != 24U)
            {
                return false;
            }

            uint8_t initialIv[8] = {0};
            if (context.iv.size() >= 8U)
            {
                for (size_t i = 0; i < 8U; ++i)
                {
                    initialIv[i] = context.iv[i];
                }
            }
            else if (!useZeroIvWhenContextIvMissing)
            {
                return false;
            }

            uint8_t cmac[8] = {0};
            if (!calculateTktdesCmac(
                    context.sessionKeyEnc.data(),
                    keyLength,
                    initialIv,
                    message.data(),
                    message.size(),
                    cmac))
            {
                return false;
            }

            outIv.clear();
            for (size_t i = 0; i < 8U; ++i)
            {
                outIv.push_back(cmac[i]);
            }

            return true;
        }

        inline etl::expected<etl::vector<uint8_t, 16>, error::Error> deriveTktdesPlainResponseIv(
            const etl::ivector<uint8_t>& response,
            const DesfireContext& context,
            const etl::ivector<uint8_t>& requestIv,
            size_t truncatedCmacLength)
        {
            if (response.empty())
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }

            if (truncatedCmacLength > 8U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
            }

            const size_t keyLength = context.sessionKeyEnc.size();
            if ((keyLength != 16U && keyLength != 24U) || requestIv.size() != 8U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            if (response.size() != 1U && response.size() != (1U + truncatedCmacLength))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }

            const uint8_t statusByte = response[0];
            uint8_t cmac[8] = {0};
            if (!calculateTktdesCmac(
                    context.sessionKeyEnc.data(),
                    keyLength,
                    requestIv.data(),
                    &statusByte,
                    1U,
                    cmac))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            if (response.size() > 1U)
            {
                for (size_t i = 0; i < truncatedCmacLength; ++i)
                {
                    if (response[1U + i] != cmac[i])
                    {
                        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::IntegrityError));
                    }
                }
            }

            etl::vector<uint8_t, 16> nextIv;
            for (size_t i = 0; i < 8U; ++i)
            {
                nextIv.push_back(cmac[i]);
            }
            return nextIv;
        }

        inline etl::expected<etl::vector<uint8_t, 16>, error::Error> deriveTktdesResponseIvForValueOperation(
            const etl::ivector<uint8_t>& response,
            const DesfireContext& context,
            const etl::ivector<uint8_t>& requestIv)
        {
            // Value operations return [status] with optional 8-byte truncated CMAC.
            return deriveTktdesPlainResponseIv(response, context, requestIv, 8U);
        }

        inline etl::expected<etl::vector<uint8_t, 16>, error::Error> deriveAesPlainResponseIv(
            const etl::ivector<uint8_t>& response,
            const DesfireContext& context,
            const etl::ivector<uint8_t>& requestIv,
            size_t truncatedCmacLength)
        {
            if (response.empty())
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }

            if (truncatedCmacLength > 16U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
            }

            if (context.sessionKeyEnc.size() < 16U || requestIv.size() != 16U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            if (response.size() != 1U && response.size() != (1U + truncatedCmacLength))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }

            const uint8_t statusByte = response[0];
            uint8_t cmac[16] = {0};
            if (!calculateAesCmac(
                    context.sessionKeyEnc.data(),
                    requestIv.data(),
                    &statusByte,
                    1U,
                    cmac))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            if (response.size() > 1U)
            {
                for (size_t i = 0; i < truncatedCmacLength; ++i)
                {
                    if (response[1U + i] != cmac[i])
                    {
                        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::IntegrityError));
                    }
                }
            }

            etl::vector<uint8_t, 16> nextIv;
            for (size_t i = 0; i < 16U; ++i)
            {
                nextIv.push_back(cmac[i]);
            }
            return nextIv;
        }

        inline bool verifyAesAuthenticatedPlainPayload(
            const etl::ivector<uint8_t>& payloadAndMac,
            uint8_t statusCode,
            const DesfireContext& context,
            const etl::ivector<uint8_t>& requestIv,
            size_t payloadLength,
            size_t truncatedCmacLength,
            etl::vector<uint8_t, 16>& outNextIv)
        {
            if (context.sessionKeyEnc.size() < 16U || requestIv.size() != 16U)
            {
                return false;
            }

            if (truncatedCmacLength > 16U)
            {
                return false;
            }

            if (payloadAndMac.size() != (payloadLength + truncatedCmacLength))
            {
                return false;
            }

            etl::vector<uint8_t, 256> cmacMessage;
            if ((payloadLength + 1U) > cmacMessage.max_size())
            {
                return false;
            }

            for (size_t i = 0U; i < payloadLength; ++i)
            {
                cmacMessage.push_back(payloadAndMac[i]);
            }
            cmacMessage.push_back(statusCode);

            uint8_t cmac[16] = {0};
            if (!calculateAesCmac(
                    context.sessionKeyEnc.data(),
                    requestIv.data(),
                    cmacMessage.data(),
                    cmacMessage.size(),
                    cmac))
            {
                return false;
            }

            for (size_t i = 0U; i < truncatedCmacLength; ++i)
            {
                if (payloadAndMac[payloadLength + i] != cmac[i])
                {
                    return false;
                }
            }

            outNextIv.clear();
            for (size_t i = 0U; i < 16U; ++i)
            {
                outNextIv.push_back(cmac[i]);
            }
            return true;
        }

        inline bool verifyTktdesAuthenticatedPlainPayload(
            const etl::ivector<uint8_t>& payloadAndMac,
            uint8_t statusCode,
            const DesfireContext& context,
            const etl::ivector<uint8_t>& requestIv,
            size_t payloadLength,
            size_t truncatedCmacLength,
            etl::vector<uint8_t, 16>& outNextIv)
        {
            const size_t keyLength = context.sessionKeyEnc.size();
            if ((keyLength != 16U && keyLength != 24U) || requestIv.size() != 8U)
            {
                return false;
            }

            if (truncatedCmacLength > 8U)
            {
                return false;
            }

            if (payloadAndMac.size() != (payloadLength + truncatedCmacLength))
            {
                return false;
            }

            etl::vector<uint8_t, 256> cmacMessage;
            if ((payloadLength + 1U) > cmacMessage.max_size())
            {
                return false;
            }

            for (size_t i = 0U; i < payloadLength; ++i)
            {
                cmacMessage.push_back(payloadAndMac[i]);
            }
            cmacMessage.push_back(statusCode);

            uint8_t cmac[8] = {0};
            if (!calculateTktdesCmac(
                    context.sessionKeyEnc.data(),
                    keyLength,
                    requestIv.data(),
                    cmacMessage.data(),
                    cmacMessage.size(),
                    cmac))
            {
                return false;
            }

            for (size_t i = 0U; i < truncatedCmacLength; ++i)
            {
                if (payloadAndMac[payloadLength + i] != cmac[i])
                {
                    return false;
                }
            }

            outNextIv.clear();
            for (size_t i = 0U; i < 8U; ++i)
            {
                outNextIv.push_back(cmac[i]);
            }
            return true;
        }

        inline void setContextIv(DesfireContext& context, const etl::ivector<uint8_t>& newIv)
        {
            context.iv.clear();
            for (size_t i = 0; i < newIv.size(); ++i)
            {
                context.iv.push_back(newIv[i]);
            }
        }

        inline void appendLe16(etl::ivector<uint8_t>& out, uint16_t value)
        {
            out.push_back(static_cast<uint8_t>(value & 0xFFU));
            out.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
        }

        inline void appendLe32(etl::ivector<uint8_t>& out, uint32_t value)
        {
            out.push_back(static_cast<uint8_t>(value & 0xFFU));
            out.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
            out.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
            out.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
        }

        inline etl::expected<etl::vector<uint8_t, 32>, error::Error> encryptPayload(
            const etl::ivector<uint8_t>& plaintext,
            const DesfireContext& context,
            SessionCipher cipher,
            etl::vector<uint8_t, 16>& pendingIv)
        {
            using namespace crypto;

            const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
            if ((plaintext.size() % blockSize) != 0U)
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
            }

            etl::vector<uint8_t, 32> encrypted;

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

                if (useLegacyDesCryptoMode(context, cipher))
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

            return encrypted;
        }

        inline etl::expected<etl::vector<uint8_t, 32>, error::Error> buildEncryptedValuePayload(
            uint8_t commandCode,
            uint8_t fileNo,
            int32_t value,
            const DesfireContext& context,
            SessionCipher cipher,
            etl::vector<uint8_t, 16>& pendingIv)
        {
            etl::vector<uint8_t, 32> plaintext;
            const bool legacyDesMode = useLegacyDesCryptoMode(context, cipher);

            const uint32_t rawValue = static_cast<uint32_t>(value);
            appendLe32(plaintext, rawValue);

            if (cipher == SessionCipher::DES || (cipher == SessionCipher::DES3_2K && legacyDesMode))
            {
                // Legacy CRC16 is calculated only over the encrypted parameter bytes (value).
                etl::vector<uint8_t, 4> crcData;
                appendLe32(crcData, rawValue);
                const uint16_t crc16 = calculateCrc16(crcData);
                appendLe16(plaintext, crc16);
            }
            else
            {
                // AES/3K3DES CRC32 over INS + fileNo + value bytes.
                etl::vector<uint8_t, 8> crcData;
                crcData.push_back(commandCode);
                crcData.push_back(fileNo);
                appendLe32(crcData, rawValue);
                const uint32_t crc32 = calculateCrc32Desfire(crcData);
                appendLe32(plaintext, crc32);
            }

            const size_t blockSize = (cipher == SessionCipher::AES) ? 16U : 8U;
            while ((plaintext.size() % blockSize) != 0U)
            {
                plaintext.push_back(0x00U);
            }

            return encryptPayload(plaintext, context, cipher, pendingIv);
        }
    } // namespace valueop_detail
} // namespace nfc
