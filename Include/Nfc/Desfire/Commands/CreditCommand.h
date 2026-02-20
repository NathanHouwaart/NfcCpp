/**
 * @file CreditCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire credit command
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
     * @brief Credit command options
     */
    struct CreditCommandOptions
    {
        uint8_t fileNo;
        int32_t value;
        uint8_t communicationSettings = 0xFFU; // 0x00 plain, 0x01 mac, 0x03 enc, 0xFF auto
    };

    /**
     * @brief Credit command
     *
     * Executes DESFire Credit (INS 0x0C).
     */
    class CreditCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Construct credit command
         *
         * @param options Command options
         */
        explicit CreditCommand(const CreditCommandOptions& options);

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
        CreditCommandOptions options;
        bool complete;
        bool updateContextIv;
        etl::vector<uint8_t, 16> requestState;
    };

} // namespace nfc
