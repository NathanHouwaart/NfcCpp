/**
 * @file IPn532Command.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Interface for PN532 commands
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/string_view.h>
#include <etl/expected.h>
#include "Pn532/Pn532ResponseFrame.h"
#include "Pn532/CommandRequest.h"
#include "Pn532/CommandResponse.h"
#include "Error/Error.h"

namespace pn532
{
    /**
     * @brief Interface for PN532 commands
     * 
     */
    class IPn532Command
    {
    public:
        virtual ~IPn532Command() = default;

        /**
         * @brief Get the name of the command
         * 
         * @return etl::string_view Command name
         */
        virtual etl::string_view name() const = 0;

        /**
         * @brief Build the command request
         * 
         * @return CommandRequest The command request to send
         */
        virtual CommandRequest buildRequest() = 0;

        /**
         * @brief Parse the response frame
         * 
         * @param frame Response frame from PN532
         * @return etl::expected<CommandResponse, error::Error> Parsed response or error
         */
        virtual etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) = 0;

        /**
         * @brief Check if the command expects a data frame response
         * 
         * @return bool True if data frame is expected
         */
        virtual bool expectsDataFrame() const = 0;
    
    protected:

        /**
         * @brief Factory method to create CommandRequest
         * 
         */
        static CommandRequest createCommandRequest(uint8_t cmd, const etl::ivector<uint8_t>& payload, uint32_t timeout = 1000)
        {
            return CommandRequest(cmd, payload, timeout);
        }

        /**
         * @brief Factory method to create CommandResponse
         * 
         */
        static CommandResponse createCommandResponse(uint8_t cmd, const etl::ivector<uint8_t>& payload)
        {
            return CommandResponse(cmd, payload);
        }
    };

} // namespace pn532
;