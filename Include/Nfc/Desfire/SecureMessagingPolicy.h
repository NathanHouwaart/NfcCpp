/**
 * @file SecureMessagingPolicy.h
 * @brief Shared secure messaging policy helpers for command-level flows
 */

#pragma once

#include "Nfc/Desfire/DesfireContext.h"
#include "Error/Error.h"
#include <cstdint>
#include <etl/expected.h>
#include <etl/vector.h>

namespace nfc
{
    /**
     * @brief Central secure messaging policy for command-level crypto/session rules.
     *
     * MVP scope:
     * - Resolve session cipher family from context
     * - Derive request IV for authenticated plain commands
     * - Verify response CMAC and derive next IV for authenticated plain commands
     * - Apply legacy DES/2K3DES command-boundary IV reset behavior
     */
    class SecureMessagingPolicy
    {
    public:
        enum class LegacySendIvSeedMode : uint8_t
        {
            Zero,
            SessionEncryptedRndB
        };

        struct ValueOperationRequestProtection
        {
            etl::vector<uint8_t, 32> encryptedPayload;
            etl::vector<uint8_t, 16> requestState;
        };

        struct EncryptedPayloadProtection
        {
            etl::vector<uint8_t, 128> encryptedPayload;
            etl::vector<uint8_t, 16> requestState;
            bool updateContextIv = false;
        };

        enum class SessionCipher : uint8_t
        {
            DES,
            DES3_2K,
            DES3_3K,
            AES,
            UNKNOWN
        };

        /**
         * @brief Resolve active session cipher from context.
         */
        static SessionCipher resolveSessionCipher(const DesfireContext& context);

        /**
         * @brief Check whether current session uses legacy DES/2K3DES command-local chaining.
         */
        static bool isLegacyDesOr2KSession(const DesfireContext& context);

        /**
         * @brief Derive request IV/CMAC state for authenticated plain command message.
         *
         * @param context Session context
         * @param plainRequestMessage Full CMAC message bytes (usually INS + command data)
         * @param useZeroIvWhenContextIvMissing Use zero IV fallback when context.iv is missing
         * @return Expected request IV bytes (16 bytes for AES, 8 bytes for ISO 3DES/2K3DES)
         */
        static etl::expected<etl::vector<uint8_t, 16>, error::Error> derivePlainRequestIv(
            const DesfireContext& context,
            const etl::ivector<uint8_t>& plainRequestMessage,
            bool useZeroIvWhenContextIvMissing = false);

        /**
         * @brief Derive next IV from authenticated plain response (status + optional CMAC).
         *
         * @param context Session context
         * @param plainResponse Raw response bytes ([status][optional mac/cmac...])
         * @param requestIv Request IV from derivePlainRequestIv()
         * @param truncatedCmacLength Truncated CMAC length in response (usually 8)
         * @return Expected next IV bytes
         */
        static etl::expected<etl::vector<uint8_t, 16>, error::Error> derivePlainResponseIv(
            const DesfireContext& context,
            const etl::ivector<uint8_t>& plainResponse,
            const etl::ivector<uint8_t>& requestIv,
            size_t truncatedCmacLength = 8U);

        /**
         * @brief Verify/update context IV for authenticated plain command response.
         *
         * No-op when not authenticated or when cipher does not use authenticated-plain CMAC progression.
         */
        static etl::expected<void, error::Error> updateContextIvForPlainCommand(
            DesfireContext& context,
            const etl::ivector<uint8_t>& plainRequestMessage,
            const etl::ivector<uint8_t>& plainResponse,
            size_t truncatedCmacLength = 8U);

        /**
         * @brief Verify authenticated-plain payload MAC/CMAC and update context IV.
         *
         * @param context Session context to update on success
         * @param payloadAndMac Response payload bytes excluding status byte
         * @param statusCode DESFire status byte used in CMAC message
         * @param requestIv Request IV/state derived from derivePlainRequestIv()
         * @param payloadLength Number of plaintext payload bytes at head of payloadAndMac
         * @param truncatedCmacLength Number of trailing CMAC bytes (0, 4, 8, ...)
         */
        static etl::expected<void, error::Error> verifyAuthenticatedPlainPayloadAndUpdateContextIv(
            DesfireContext& context,
            const etl::ivector<uint8_t>& payloadAndMac,
            uint8_t statusCode,
            const etl::ivector<uint8_t>& requestIv,
            size_t payloadLength,
            size_t truncatedCmacLength = 8U);

        /**
         * @brief Verify authenticated-plain payload with automatic MAC-length selection.
         *
         * Attempts verification with supported truncated CMAC lengths in this order: 8, 4, 0.
         * Returns the accepted MAC length on success and updates context IV.
         */
        static etl::expected<size_t, error::Error> verifyAuthenticatedPlainPayloadAutoMacAndUpdateContextIv(
            DesfireContext& context,
            const etl::ivector<uint8_t>& payloadAndMac,
            uint8_t statusCode,
            const etl::ivector<uint8_t>& requestIv,
            size_t payloadLength);

        /**
         * @brief Protect value-operation request payload (Credit/Debit/LimitedCredit).
         *
         * Builds encrypted payload and request state needed for response IV progression.
         */
        static etl::expected<ValueOperationRequestProtection, error::Error> protectValueOperationRequest(
            const DesfireContext& context,
            uint8_t commandCode,
            uint8_t fileNo,
            int32_t value);

        /**
         * @brief Verify and update context IV from value-operation response.
         */
        static etl::expected<void, error::Error> updateContextIvForValueOperationResponse(
            DesfireContext& context,
            const etl::ivector<uint8_t>& response,
            const etl::ivector<uint8_t>& requestState);

        /**
         * @brief Update context IV from encrypted response ciphertext bytes.
         *
         * For AES/ISO 2K3DES/3K3DES sessions this uses the trailing ciphertext block.
         * For legacy DES/2K3DES it applies command-boundary zero-IV policy.
         */
        static etl::expected<void, error::Error> updateContextIvFromEncryptedCiphertext(
            DesfireContext& context,
            const etl::ivector<uint8_t>& ciphertext);

        /**
         * @brief Protect an already-built plaintext payload for encrypted command transport.
         *
         * @param context Session context
         * @param plaintext Block-aligned payload bytes to encrypt
         * @param sessionCipher Session cipher selected by command/session
         * @param useLegacySendMode Use legacy SEND_MODE transform instead of CBC encryption
         * @param legacySeed Legacy SEND_MODE seed behavior
         */
        static etl::expected<EncryptedPayloadProtection, error::Error> protectEncryptedPayload(
            const DesfireContext& context,
            const etl::ivector<uint8_t>& plaintext,
            SessionCipher sessionCipher,
            bool useLegacySendMode,
            LegacySendIvSeedMode legacySeed = LegacySendIvSeedMode::Zero);

        /**
         * @brief Apply encrypted-command response IV progression based on request protection metadata.
         */
        static etl::expected<void, error::Error> updateContextIvForEncryptedCommandResponse(
            DesfireContext& context,
            const etl::ivector<uint8_t>& response,
            const EncryptedPayloadProtection& protection);

        /**
         * @brief DESFire CRC16 helper.
         */
        static uint16_t calculateCrc16(const etl::ivector<uint8_t>& data);

        /**
         * @brief DESFire CRC32 helper.
         */
        static uint32_t calculateCrc32Desfire(const etl::ivector<uint8_t>& data);

        /**
         * @brief Apply legacy DES/2K3DES command-boundary IV reset behavior.
         */
        static void applyLegacyCommandBoundaryIvPolicy(DesfireContext& context);
    };
} // namespace nfc
