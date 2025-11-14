/**
 * @file SelectApplicationCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire select application command
 * @version 0.1
 * @date 2025-11-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "../IDesfireCommand.h"
#include <etl/array.h>

namespace nfc
{
    /**
     * @brief Select application command
     * 
     * Selects a DESFire application by its Application ID (AID).
     * AID 0x000000 selects the PICC (master) application.
     */
    class SelectApplicationCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Construct select application command
         * 
         * @param aid Application ID (3 bytes)
         */
        explicit SelectApplicationCommand(const etl::array<uint8_t, 3>& aid);

        /**
         * @brief Get command name
         * 
         * @return etl::string_view Command name
         */
        etl::string_view name() const override;

        /**
         * @brief Build the request
         * 
         * @param context DESFire context
         * @return etl::expected<DesfireRequest, error::Error> Request or error
         */
        etl::expected<DesfireRequest, error::Error> buildRequest(const DesfireContext& context) override;

        /**
         * @brief Parse the response
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
         * @return true Command completed (always true for select application)
         * @return false More frames needed
         */
        bool isComplete() const override;

        /**
         * @brief Reset command state
         */
        void reset() override;

    private:
        etl::array<uint8_t, 3> aid;
        bool complete;
    };

} // namespace nfc

