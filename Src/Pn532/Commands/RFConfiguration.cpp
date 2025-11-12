/**
 * @file RFConfiguration.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief RFConfiguration command implementation
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Commands/RFConfiguration.h"
#include "Error/Pn532Error.h"

using namespace error;

namespace pn532
{
    RFConfiguration::RFConfiguration(const RFConfigurationOptions& opts)
        : options(opts)
    {
    }

    etl::string_view RFConfiguration::name() const
    {
        return "RFConfiguration";
    }

    CommandRequest RFConfiguration::buildRequest()
    {
        // Build payload: [CfgItem] [Config Data...]
        etl::vector<uint8_t, 32> payload;
        payload.push_back(static_cast<uint8_t>(options.item));
        
        // Append configuration data
        for (size_t i = 0; i < options.configData.size(); ++i)
        {
            payload.push_back(options.configData[i]);
        }
        
        return createCommandRequest(0x32, payload); // 0x32 = RFConfiguration
    }

    etl::expected<CommandResponse, Error> RFConfiguration::parseResponse(const Pn532ResponseFrame& frame)
    {
        // RFConfiguration returns no data on success (empty payload)
        // The ACK and response frame presence indicates success
        const auto& data = frame.data();
        
        // Response should be empty
        if (!data.empty())
        {
            // If there's data, it might indicate an error or additional info
            // For now, we accept it as valid
        }
        
        return createCommandResponse(frame.getCommandCode(), data);
    }

    bool RFConfiguration::expectsDataFrame() const
    {
        return true;
    }

} // namespace pn532
