/**
 * @file ReadDataCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire read data command
 * @version 0.1
 * @date 2026-02-15
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
     * @brief ReadData command options
     */
    struct ReadDataCommandOptions
    {
        uint8_t fileNo;
        uint32_t offset;
        uint32_t length;
        uint16_t chunkSize;
        uint8_t communicationSettings = 0xFFU; // 0x00 plain, 0x01 mac, 0x03 enc, 0xFF auto
    };

    /**
     * @brief ReadData command
     *
     * Reads bytes from a standard/backup data file (INS 0xBD).
     * This implementation uses chunked reads across multiple command cycles
     * to avoid large single-frame responses.
     */
    class ReadDataCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Command stage
         */
        enum class Stage : uint8_t
        {
            Initial,
            ReadChunk,
            AdditionalFrame,
            Complete
        };

        static constexpr size_t MAX_READ_DATA_SIZE = 4096U;
        static constexpr size_t MAX_FRAME_DATA_SIZE = 272U;
        static constexpr size_t MAX_READ_DATA_BUFFER_SIZE = MAX_READ_DATA_SIZE + 64U;
        static constexpr uint16_t DEFAULT_CHUNK_SIZE = 240U;
        static constexpr uint16_t MAX_CHUNK_SIZE = 240U;

        /**
         * @brief Construct ReadData command
         *
         * @param options Command options
         */
        explicit ReadDataCommand(const ReadDataCommandOptions& options);

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
         * @return const etl::vector<uint8_t, MAX_READ_DATA_SIZE>& Read data bytes
         */
        const etl::vector<uint8_t, MAX_READ_DATA_SIZE>& getData() const;

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
        uint16_t effectiveChunkSize() const;
        uint8_t resolveCommunicationSettings(const DesfireContext& context) const;
        SessionCipher resolveSessionCipher(const DesfireContext& context) const;
        bool tryDecodeEncryptedChunk(DesfireContext& context, etl::vector<uint8_t, MAX_FRAME_DATA_SIZE>& outPlainChunk);
        bool decryptPayload(
            const etl::ivector<uint8_t>& ciphertext,
            const DesfireContext& context,
            SessionCipher cipher,
            etl::vector<uint8_t, MAX_FRAME_DATA_SIZE>& plaintext) const;
        void resetProgress();

        ReadDataCommandOptions options;
        Stage stage;
        uint32_t currentOffset;
        uint32_t remainingLength;
        uint32_t lastRequestedChunkLength;
        etl::vector<uint8_t, MAX_FRAME_DATA_SIZE> frameData;
        etl::vector<uint8_t, MAX_READ_DATA_SIZE> data;
        uint8_t activeCommunicationSettings;
        SessionCipher sessionCipher;
        etl::vector<uint8_t, 16> requestIv;
        bool hasRequestIv;
    };

} // namespace nfc
