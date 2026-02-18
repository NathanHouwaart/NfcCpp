/**
 * @file WriteRecordCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire write record command
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
     * @brief WriteRecord command options
     */
    struct WriteRecordCommandOptions
    {
        uint8_t fileNo;
        uint32_t offset; // byte offset within record
        uint32_t recordSize; // record size in bytes (for boundary validation)
        const etl::ivector<uint8_t>* data;
        uint16_t chunkSize;
        uint8_t communicationSettings = 0xFFU; // 0x00 plain, 0x01 mac, 0x03 enc, 0xFF auto
    };

    /**
     * @brief WriteRecord command
     *
     * Writes records to a linear/cyclic record file (INS 0x3B).
     * WriteRecord payload fields are byte-based:
     * - offset: byte offset within the record under construction
     * - length: number of payload bytes
     *
     * This implementation supports chunked transfer for larger payloads while
     * advancing the byte offset for each chunk.
     */
    class WriteRecordCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Command stage
         */
        enum class Stage : uint8_t
        {
            Initial,
            Writing,
            Complete
        };

        static constexpr uint16_t DEFAULT_CHUNK_SIZE = 240U;
        static constexpr uint16_t MAX_CHUNK_SIZE = 240U;

        /**
         * @brief Construct WriteRecord command
         *
         * @param options Command options
         */
        explicit WriteRecordCommand(const WriteRecordCommandOptions& options);

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
        static void appendLe16(etl::ivector<uint8_t>& target, uint16_t value);
        static void appendLe32(etl::ivector<uint8_t>& target, uint32_t value);
        bool validateOptions() const;
        uint16_t effectiveChunkSize() const;
        uint8_t resolveCommunicationSettings(const DesfireContext& context) const;
        SessionCipher resolveSessionCipher(const DesfireContext& context) const;
        uint16_t effectivePlainChunkBudget() const;
        etl::expected<etl::vector<uint8_t, MAX_CHUNK_SIZE + 32U>, error::Error> buildEncryptedChunkPayload(
            const DesfireContext& context,
            uint32_t chunkByteOffset,
            uint32_t chunkByteLength);
        etl::expected<etl::vector<uint8_t, MAX_CHUNK_SIZE + 32U>, error::Error> encryptPayload(
            const etl::ivector<uint8_t>& plaintext,
            const DesfireContext& context,
            SessionCipher cipher);
        void resetProgress();

        WriteRecordCommandOptions options;
        Stage stage;
        size_t currentIndex;
        uint32_t lastChunkByteLength;
        uint8_t activeCommunicationSettings;
        SessionCipher sessionCipher;
        etl::vector<uint8_t, 16> pendingIv;
        bool updateContextIv;
        bool legacyDesCryptoMode;
    };

} // namespace nfc
