/**
 * @file GetApplicationIdsCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get application IDs command
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "../IDesfireCommand.h"
#include <etl/array.h>
#include <etl/vector.h>

namespace nfc
{
    /**
     * @brief GetApplicationIDs command
     *
     * Executes DESFire GetApplicationIDs (INS 0x6A). The response is one or
     * more 0xAF-chained frames containing AID triplets.
     */
    class GetApplicationIdsCommand : public IDesfireCommand
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

        /**
         * @brief Construct GetApplicationIds command
         */
        GetApplicationIdsCommand();

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
         * @brief Get parsed application IDs
         *
         * AIDs are returned as 3-byte values in on-wire order.
         *
         * @return const etl::vector<etl::array<uint8_t, 3>, 84>& Parsed AIDs
         */
        const etl::vector<etl::array<uint8_t, 3>, 84>& getApplicationIds() const;

        /**
         * @brief Get raw concatenated payload bytes
         *
         * @return const etl::vector<uint8_t, 252>& Raw payload bytes
         */
        const etl::vector<uint8_t, 252>& getRawPayload() const;

    private:
        Stage stage;
        etl::vector<uint8_t, 252> rawPayload;
        etl::vector<etl::array<uint8_t, 3>, 84> applicationIds;
    };

} // namespace nfc

