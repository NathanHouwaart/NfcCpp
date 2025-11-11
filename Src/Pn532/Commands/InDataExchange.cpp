/**
 * @file InDataExchange.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief InDataExchange command implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Commands/InDataExchange.h"

namespace pn532
{
    InDataExchange::InDataExchange(const InDataExchangeOptions& opts)
        : options(opts), cachedStatusByte(0)
    {
    }

    etl::string_view InDataExchange::name() const
    {
        return "InDataExchange";
    }

    CommandRequest InDataExchange::buildRequest()
    {
        // TODO: Implement request building
        etl::vector<uint8_t, 256> payload;
        payload.push_back(options.targetNumber);
        
        // Add command payload
        for (auto byte : options.payload)
        {
            payload.push_back(byte);
        }
        
        return createCommandRequest(0x40, payload, options.responseTimeoutMs); // 0x40 = InDataExchange command
    }

    etl::expected<CommandResponse, error::Error> InDataExchange::parseResponse(const Pn532ResponseFrame& frame)
    {
        // TODO: Implement response parsing
        // First byte should be status byte
        etl::vector<uint8_t, 1> payload;
        return createCommandResponse(frame.getCommandCode(), payload);
    }

    bool InDataExchange::expectsDataFrame() const
    {
        return true;
    }

    uint8_t InDataExchange::getStatusByte() const
    {
        return cachedStatusByte;
    }

} // namespace pn532
