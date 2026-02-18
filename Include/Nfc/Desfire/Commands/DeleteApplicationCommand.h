/**
 * @file DeleteApplicationCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire delete application command
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "../IDesfireCommand.h"
#include <etl/array.h>

namespace nfc
{
    /**
     * @brief Delete application command
     *
     * Deletes a DESFire application (INS 0xDA) using payload AID(3).
     */
    class DeleteApplicationCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Construct delete application command
         *
         * @param aid Application ID (3 bytes)
         */
        explicit DeleteApplicationCommand(const etl::array<uint8_t, 3>& aid);

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
        etl::array<uint8_t, 3> aid;
        bool complete;
    };

} // namespace nfc

