/**
 * @file GetFileSettingsCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get file settings command
 * @version 0.1
 * @date 2026-02-14
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
     * @brief GetFileSettings command
     *
     * Executes DESFire GetFileSettings (INS 0xF5) for one file number.
     */
    class GetFileSettingsCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Known DESFire file types
         */
        enum class FileType : uint8_t
        {
            StandardData = 0x00,
            BackupData = 0x01,
            Value = 0x02,
            LinearRecord = 0x03,
            CyclicRecord = 0x04,
            Unknown = 0xFF
        };

        /**
         * @brief Command stage
         */
        enum class Stage : uint8_t
        {
            Initial,
            Complete
        };

        /**
         * @brief Construct GetFileSettings command
         *
         * @param fileNo Target file number
         */
        explicit GetFileSettingsCommand(uint8_t fileNo);

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
         * @brief Get requested file number
         *
         * @return uint8_t File number
         */
        uint8_t getFileNo() const;

        /**
         * @brief Get parsed file type enum
         *
         * @return FileType File type
         */
        FileType getFileType() const;

        /**
         * @brief Get raw file type value
         *
         * @return uint8_t Raw file type byte
         */
        uint8_t getFileTypeRaw() const;

        /**
         * @brief Get file communication settings byte
         *
         * @return uint8_t Communication settings
         */
        uint8_t getCommunicationSettings() const;

        /**
         * @brief Get read access nibble
         *
         * @return uint8_t Read access key selector nibble
         */
        uint8_t getReadAccess() const;

        /**
         * @brief Get write access nibble
         *
         * @return uint8_t Write access key selector nibble
         */
        uint8_t getWriteAccess() const;

        /**
         * @brief Get read/write access nibble
         *
         * @return uint8_t Read/write access key selector nibble
         */
        uint8_t getReadWriteAccess() const;

        /**
         * @brief Get change access nibble
         *
         * @return uint8_t Change access key selector nibble
         */
        uint8_t getChangeAccess() const;

        /**
         * @brief Check if file size is available (standard/backup files)
         *
         * @return true File size available
         * @return false File size not available
         */
        bool hasFileSize() const;

        /**
         * @brief Get parsed file size
         *
         * @return uint32_t File size in bytes
         */
        uint32_t getFileSize() const;

        /**
         * @brief Check if value-file settings are available
         *
         * @return true Value settings available
         * @return false Value settings not available
         */
        bool hasValueSettings() const;

        /**
         * @brief Get value-file lower limit
         *
         * @return uint32_t Lower limit
         */
        uint32_t getLowerLimit() const;

        /**
         * @brief Get value-file upper limit
         *
         * @return uint32_t Upper limit
         */
        uint32_t getUpperLimit() const;

        /**
         * @brief Get value-file limited credit value
         *
         * @return uint32_t Limited credit value
         */
        uint32_t getLimitedCreditValue() const;

        /**
         * @brief Check if limited credit is enabled
         *
         * @return true Limited credit enabled
         * @return false Limited credit disabled
         */
        bool isLimitedCreditEnabled() const;

        /**
         * @brief Check if free get-value option is enabled (if present)
         *
         * @return true Free get-value enabled
         * @return false Free get-value disabled
         */
        bool isFreeGetValueEnabled() const;

        /**
         * @brief Check if record-file settings are available
         *
         * @return true Record settings available
         * @return false Record settings not available
         */
        bool hasRecordSettings() const;

        /**
         * @brief Get record size
         *
         * @return uint32_t Record size in bytes
         */
        uint32_t getRecordSize() const;

        /**
         * @brief Get maximum record count
         *
         * @return uint32_t Maximum number of records
         */
        uint32_t getMaxRecords() const;

        /**
         * @brief Get current record count
         *
         * @return uint32_t Current number of records
         */
        uint32_t getCurrentRecords() const;

        /**
         * @brief Get raw response payload bytes (without status)
         *
         * @return const etl::vector<uint8_t, 64>& Raw payload bytes
         */
        const etl::vector<uint8_t, 64>& getRawPayload() const;

    private:
        static uint32_t readLe24(const etl::ivector<uint8_t>& payload, size_t offset);
        static uint32_t readLe32(const etl::ivector<uint8_t>& payload, size_t offset);
        static FileType toFileType(uint8_t rawType);
        bool tryDecodeAuthenticatedPayload(
            const etl::ivector<uint8_t>& candidate,
            uint8_t statusCode,
            DesfireContext& context,
            etl::vector<uint8_t, 64>& outPayload);
        bool parsePayload(const etl::ivector<uint8_t>& payload);

        uint8_t fileNo;
        Stage stage;

        FileType fileType;
        uint8_t fileTypeRaw;
        uint8_t communicationSettings;
        uint8_t readAccess;
        uint8_t writeAccess;
        uint8_t readWriteAccess;
        uint8_t changeAccess;

        bool fileSizeValid;
        uint32_t fileSize;

        bool valueSettingsValid;
        uint32_t lowerLimit;
        uint32_t upperLimit;
        uint32_t limitedCreditValue;
        bool limitedCreditEnabled;
        bool freeGetValueEnabled;

        bool recordSettingsValid;
        uint32_t recordSize;
        uint32_t maxRecords;
        uint32_t currentRecords;

        etl::vector<uint8_t, 64> rawPayload;
        etl::vector<uint8_t, 16> requestIv;
        bool hasRequestIv;
    };

} // namespace nfc
