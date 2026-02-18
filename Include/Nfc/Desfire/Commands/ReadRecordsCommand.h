/**
 * @file ReadRecordsCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire read records command
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "../IDesfireCommand.h"
#include <etl/vector.h>

namespace nfc
{
    /**
     * @brief ReadRecords command options
     */
    struct ReadRecordsCommandOptions
    {
        uint8_t fileNo;
        uint32_t recordOffset; // record index (API-level)
        uint32_t recordCount; // number of records (API-level)
        uint32_t recordSize; // bytes per record
        uint32_t expectedDataLength; // Expected returned bytes (used to strip trailing CMAC when authenticated)
        uint8_t communicationSettings = 0xFFU; // 0x00 plain, 0x01 mac, 0x03 enc, 0xFF auto
    };

    /**
     * @brief ReadRecords command
     *
     * Reads records from a linear/cyclic record file (INS 0xBB).
     * The payload fields are record-based:
     * - recordOffset: first record index
     * - recordCount: number of records (0 => all from offset)
     */
    class ReadRecordsCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Command stage
         */
        enum class Stage : uint8_t
        {
            Initial,
            AdditionalFrame,
            Complete
        };

        static constexpr size_t MAX_READ_RECORDS_SIZE = 4096U;
        static constexpr size_t MAX_READ_RECORDS_BUFFER_SIZE = MAX_READ_RECORDS_SIZE + 64U;

        /**
         * @brief Construct ReadRecords command
         *
         * @param options Command options
         */
        explicit ReadRecordsCommand(const ReadRecordsCommandOptions& options);

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
         * @brief Get accumulated read data
         *
         * @return const etl::vector<uint8_t, MAX_READ_RECORDS_SIZE>& Read data bytes
         */
        const etl::ivector<uint8_t>& getData() const;

    private:
        enum class SessionCipher : uint8_t
        {
            DES,
            DES3_2K,
            DES3_3K,
            AES,
            UNKNOWN
        };

        static void appendLe24(etl::ivector<uint8_t>& target, uint32_t value);
        bool validateOptions() const;
        uint8_t resolveCommunicationSettings(const DesfireContext& context) const;
        SessionCipher resolveSessionCipher(const DesfireContext& context) const;
        bool deriveAesPlainRequestIv(const DesfireContext& context, etl::vector<uint8_t, 16>& outIv) const;
        bool tryDecodeEncryptedRecords(DesfireContext& context);
        bool decryptPayload(
            const etl::ivector<uint8_t>& ciphertext,
            const DesfireContext& context,
            SessionCipher cipher,
            etl::vector<uint8_t, MAX_READ_RECORDS_BUFFER_SIZE>& plaintext) const;
        bool trimAuthenticatedTrailingMac(const DesfireContext& context);

        ReadRecordsCommandOptions options;
        Stage stage;
        etl::vector<uint8_t, MAX_READ_RECORDS_BUFFER_SIZE> data;
        uint8_t activeCommunicationSettings;
        SessionCipher sessionCipher;
        etl::vector<uint8_t, 16> requestIv;
        bool hasRequestIv;
    };

} // namespace nfc
