/**
 * @file SAMConfiguration.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief SAMConfiguration command implementation
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Commands/SAMConfiguration.h"
#include "Error/Pn532Error.h"

using namespace error;

namespace pn532
{
    SAMConfiguration::SAMConfiguration(const SAMConfigurationOptions& opts)
        : options(opts)
    {
    }

    etl::string_view SAMConfiguration::name() const
    {
        return "SAMConfiguration";
    }

    CommandRequest SAMConfiguration::buildRequest()
    {
        // Build payload: [Mode] [Timeout] [IRQ]
        etl::vector<uint8_t, 3> payload;
        payload.push_back(static_cast<uint8_t>(options.mode));
        
        // Timeout is only used in Virtual Card mode, but we send it anyway
        payload.push_back(options.timeout);
        
        // IRQ usage (0x00 = not used, 0x01 = used)
        payload.push_back(options.useIRQ ? 0x01 : 0x00);
        
        return createCommandRequest(0x14, payload); // 0x14 = SAMConfiguration
    }

    etl::expected<CommandResponse, Error> SAMConfiguration::parseResponse(const Pn532ResponseFrame& frame)
    {
        // SAMConfiguration returns no data on success (empty payload)
        // The ACK and response frame presence indicates success
        const auto& data = frame.data();
        
        // Response should be empty or contain status
        if (!data.empty())
        {
            // If there's data, it might be an error code
            // For now, we accept it as valid
        }
        
        return createCommandResponse(frame.getCommandCode(), data);
    }

    bool SAMConfiguration::expectsDataFrame() const
    {
        return true;
    }

} // namespace pn532
