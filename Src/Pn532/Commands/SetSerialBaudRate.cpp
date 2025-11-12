/**
 * @file SetSerialBaudRate.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief SetSerialBaudRate command implementation
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Commands/SetSerialBaudRate.h"
#include "Error/Pn532Error.h"

using namespace error;

namespace pn532
{
    SetSerialBaudRate::SetSerialBaudRate(const SetSerialBaudRateOptions& opts)
        : options(opts)
    {
    }

    etl::string_view SetSerialBaudRate::name() const
    {
        return "SetSerialBaudRate";
    }

    CommandRequest SetSerialBaudRate::buildRequest()
    {
        // Build payload: [BR] (baud rate code)
        etl::vector<uint8_t, 1> payload;
        payload.push_back(static_cast<uint8_t>(options.baudRate));
        
        return createCommandRequest(0x10, payload); // 0x10 = SetSerialBaudRate
    }

    etl::expected<CommandResponse, Error> SetSerialBaudRate::parseResponse(const Pn532ResponseFrame& frame)
    {
        // SetSerialBaudRate returns no data on success (empty payload)
        const auto& data = frame.data();
        
        // Response should be empty
        if (!data.empty())
        {
            // If there's data, it might indicate an error
        }
        
        return createCommandResponse(frame.getCommandCode(), data);
    }

    bool SetSerialBaudRate::expectsDataFrame() const
    {
        return true;
    }

} // namespace pn532
