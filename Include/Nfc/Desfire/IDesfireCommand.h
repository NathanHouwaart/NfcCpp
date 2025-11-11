/**
 * @file IDesfireCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire command interface
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/string_view.h>
#include <etl/expected.h>
#include "DesfireRequest.h"
#include "DesfireResult.h"
#include "Error/Error.h"

namespace nfc
{
    // Forward declaration
    struct DesfireContext;

    /**
     * @brief DESFire command interface
     * 
     * Interface for all DESFire commands
     */
    class IDesfireCommand
    {
    public:
        virtual ~IDesfireCommand() = default;

        /**
         * @brief Get command name
         * 
         * @return etl::string_view Command name
         */
        virtual etl::string_view name() const = 0;

        /**
         * @brief Build the request
         * 
         * @param context DESFire context
         * @return etl::expected<DesfireRequest, error::Error> Request or error
         */
        virtual etl::expected<DesfireRequest, error::Error> buildRequest(const DesfireContext& context) = 0;

        /**
         * @brief Parse the response
         * 
         * @param response Response data
         * @param context DESFire context (mutable for state updates)
         * @return etl::expected<DesfireResult, error::Error> Result or error
         */
        virtual etl::expected<DesfireResult, error::Error> parseResponse(
            const etl::ivector<uint8_t>& response,
            DesfireContext& context) = 0;

        /**
         * @brief Check if command is complete
         * 
         * @return true Command completed
         * @return false More frames needed
         */
        virtual bool isComplete() const = 0;

        /**
         * @brief Reset command state
         */
        virtual void reset() = 0;
    };

} // namespace nfc
