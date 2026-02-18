/**
 * @file CreateCyclicRecordFileCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire create cyclic record file command
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "../IDesfireCommand.h"

namespace nfc
{
    /**
     * @brief Create cyclic record file command options
     *
     * Payload layout (INS 0xC0):
     * - FileNo (1 byte)
     * - CommunicationSettings (1 byte): 0x00 plain, 0x01 MAC, 0x03 enciphered
     * - AccessRights (2 bytes): [RW|CAR] [R|W]
     * - RecordSize (3 bytes, LSB first)
     * - MaxRecords (3 bytes, LSB first)
     */
    struct CreateCyclicRecordFileCommandOptions
    {
        uint8_t fileNo;
        uint8_t communicationSettings;
        uint8_t readAccess;
        uint8_t writeAccess;
        uint8_t readWriteAccess;
        uint8_t changeAccess;
        uint32_t recordSize;
        uint32_t maxRecords;
    };

    /**
     * @brief Create cyclic record file command
     *
     * Creates a DESFire cyclic record file (INS 0xC0).
     */
    class CreateCyclicRecordFileCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Construct create cyclic record file command
         *
         * @param options Command options
         */
        explicit CreateCyclicRecordFileCommand(const CreateCyclicRecordFileCommandOptions& options);

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
         * @brief Check if complete
         *
         * @return true Command complete
         * @return false Command not complete
         */
        bool isComplete() const override;

        /**
         * @brief Reset state
         */
        void reset() override;

    private:
        bool validateOptions() const;
        bool buildAccessRightsBytes(uint8_t& lowByte, uint8_t& highByte) const;

        CreateCyclicRecordFileCommandOptions options;
        bool complete;
    };

} // namespace nfc

