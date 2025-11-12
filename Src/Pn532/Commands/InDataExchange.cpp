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
#include "Error/Pn532Error.h"

using namespace error;

namespace pn532
{
    InDataExchange::InDataExchange(const InDataExchangeOptions& opts)
        : options(opts), cachedStatusByte(0xFF)
    {
    }

    etl::string_view InDataExchange::name() const
    {
        return "InDataExchange";
    }

    CommandRequest InDataExchange::buildRequest()
    {
        // Build payload: [Tg] [DataOut...]
        etl::vector<uint8_t, 256> payload;
        payload.push_back(options.targetNumber);
        
        // Add command payload
        for (size_t i = 0; i < options.payload.size(); ++i)
        {
            payload.push_back(options.payload[i]);
        }
        
        return createCommandRequest(0x40, payload, options.responseTimeoutMs); // 0x40 = InDataExchange
    }

    etl::expected<CommandResponse, Error> InDataExchange::parseResponse(const Pn532ResponseFrame& frame)
    {
        const auto& data = frame.data();
        
        // InDataExchange response format: [Status] [DataIn...]
        if (data.empty())
        {
            return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
        }

        // Extract status byte
        cachedStatusByte = data[0];
        
        // Extract response data from card (everything after status byte)
        cachedResponse.clear();
        if (data.size() > 1)
        {
            cachedResponse.assign(data.begin() + 1, data.end());
        }
        
        // Check card-level status and return PN532 error for failures
        // Status byte 0x00 = success, anything else is an error
        // The status byte values directly correspond to Pn532Error enum values
        if (cachedStatusByte != 0x00)
        {
            Pn532Error pn532Error = static_cast<Pn532Error>(cachedStatusByte);
            return etl::unexpected(Error::fromPn532(pn532Error));
        }
        
        return createCommandResponse(frame.getCommandCode(), data);
    }

    bool InDataExchange::expectsDataFrame() const
    {
        return true;
    }

    uint8_t InDataExchange::getStatusByte() const
    {
        return cachedStatusByte;
    }

    Pn532Error InDataExchange::getStatus() const
    {
        return static_cast<Pn532Error>(cachedStatusByte);
    }

    

    bool InDataExchange::isSuccess() const
    {
        return cachedStatusByte == 0x00;
    }

    const etl::ivector<uint8_t>& InDataExchange::getResponseData() const
    {
        return cachedResponse;
    }

} // namespace pn532
