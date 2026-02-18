/**
 * @file GetValueCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get value command
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <cstdint>
#include <etl/vector.h>
#include "../IDesfireCommand.h"

namespace nfc
{
    /**
     * @brief GetValue command
     *
     * Executes DESFire GetValue (INS 0x6C) for one value file.
     */
    class GetValueCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Command stage
         */
        enum class Stage : uint8_t
        {
            Initial,
            Complete
        };

        /**
         * @brief Construct GetValue command
         *
         * @param fileNo Target value file number
         */
        explicit GetValueCommand(uint8_t fileNo);

        /**
         * @brief Get command name
         *
         * @return etl::string_view Command name
         */
        etl::string_view name() const override;

        /**
         * @brief Build request
         *
         * @param context DESFire context
         * @return etl::expected<DesfireRequest, error::Error> Request or error
         */
        etl::expected<DesfireRequest, error::Error> buildRequest(const DesfireContext& context) override;

        /**
         * @brief Parse response
         *
         * @param response Response data
         * @param context DESFire context
         * @return etl::expected<DesfireResult, error::Error> Result or error
         */
        etl::expected<DesfireResult, error::Error> parseResponse(
            const etl::ivector<uint8_t>& response,
            DesfireContext& context) override;

        /**
         * @brief Check if command is complete
         *
         * @return true Command completed
         * @return false More frames needed
         */
        bool isComplete() const override;

        /**
         * @brief Reset command state
         */
        void reset() override;

        /**
         * @brief Get parsed signed value
         *
         * @return int32_t Parsed value
         */
        int32_t getValue() const;

        /**
         * @brief Get raw response payload bytes (without status)
         *
         * @return const etl::vector<uint8_t, 16>& Raw payload bytes
         */
        const etl::vector<uint8_t, 16>& getRawPayload() const;

    private:
        enum class SessionCipher : uint8_t
        {
            DES,
            DES3_2K,
            DES3_3K,
            AES,
            UNKNOWN
        };

        SessionCipher resolveSessionCipher(const DesfireContext& context) const;
        uint16_t calculateCrc16(const etl::ivector<uint8_t>& data) const;
        uint32_t calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const;

        bool tryDecodeEncryptedValue(
            const etl::ivector<uint8_t>& payload,
            DesfireContext& context,
            etl::vector<uint8_t, 4>& outValueBytes);

        bool decryptPayload(
            const etl::ivector<uint8_t>& ciphertext,
            const DesfireContext& context,
            SessionCipher cipher,
            etl::vector<uint8_t, 32>& plaintext) const;

        static int32_t parseLe32(const etl::ivector<uint8_t>& payload);

        uint8_t fileNo;
        Stage stage;
        int32_t value;
        etl::vector<uint8_t, 16> rawPayload;
    };

} // namespace nfc
