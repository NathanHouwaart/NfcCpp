/**
 * @file CreateValueFileCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire create value file command
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <cstdint>
#include "../IDesfireCommand.h"

namespace nfc
{
    /**
     * @brief Create value file command options
     *
     * Payload layout (INS 0xCC):
     * - FileNo (1 byte)
     * - CommunicationSettings (1 byte): 0x00 plain, 0x01 MAC, 0x03 enciphered
     * - AccessRights (2 bytes): [RW|CAR] [R|W]
     * - LowerLimit (4 bytes, LSB first, signed)
     * - UpperLimit (4 bytes, LSB first, signed)
     * - LimitedCreditValue (4 bytes, LSB first, signed)
     * - ValueOptions (1 byte): bit0 limited credit, bit1 free get value
     */
    struct CreateValueFileCommandOptions
    {
        uint8_t fileNo;
        uint8_t communicationSettings;
        uint8_t readAccess;
        uint8_t writeAccess;
        uint8_t readWriteAccess;
        uint8_t changeAccess;
        int32_t lowerLimit;
        int32_t upperLimit;
        int32_t limitedCreditValue;
        uint8_t valueOptions;
    };

    /**
     * @brief Create value file command
     *
     * Creates a DESFire value file (INS 0xCC).
     */
    class CreateValueFileCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Construct create value file command
         *
         * @param options Command options
         */
        explicit CreateValueFileCommand(const CreateValueFileCommandOptions& options);

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
        void appendLe32(DesfireRequest& request, int32_t value) const;

        CreateValueFileCommandOptions options;
        bool complete;
    };

} // namespace nfc

