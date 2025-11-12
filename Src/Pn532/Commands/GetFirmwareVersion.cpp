/**
 * @file GetFirmwareVersion.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Get firmware version command implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Commands/GetFirmwareVersion.h"
#include "Error/Pn532Error.h"
#include <cstdio>

using namespace error;

namespace pn532
{
    etl::string<255> FirmwareInfo::toString() const
    {
        etl::string<255> result;
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), 
                     "IC: 0x%02X, Version: 0x%02X, Revision: 0x%02X, Support: 0x%02X",
                     ic, ver, rev, support);
        result.assign(buffer);
        return result;
    }

    etl::string_view GetFirmwareVersion::name() const
    {
        return "GetFirmwareVersion";
    }

    CommandRequest GetFirmwareVersion::buildRequest()
    {
        // GetFirmwareVersion command has no parameters
        etl::vector<uint8_t, 1> payload;
        return createCommandRequest(0x02, payload); // 0x02 = GetFirmwareVersion
    }

    etl::expected<CommandResponse, Error> GetFirmwareVersion::parseResponse(const Pn532ResponseFrame& frame)
    {
        // Expected response: [IC] [Ver] [Rev] [Support]
        constexpr size_t kExpectedLength = 4;
        constexpr size_t kIndexIc = 0;
        constexpr size_t kIndexVersion = 1;
        constexpr size_t kIndexRevision = 2;
        constexpr size_t kIndexSupport = 3;

        const auto& data = frame.data();
        
        if (data.size() != kExpectedLength)
        {
            return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
        }

        // Parse firmware info
        cachedInfo.ic = data[kIndexIc];
        cachedInfo.ver = data[kIndexVersion];
        cachedInfo.rev = data[kIndexRevision];
        cachedInfo.support = data[kIndexSupport];

        // Return response with raw payload
        return createCommandResponse(frame.getCommandCode(), data);
    }

    bool GetFirmwareVersion::expectsDataFrame() const
    {
        return true;
    }

    const FirmwareInfo& GetFirmwareVersion::getFirmwareInfo() const
    {
        return cachedInfo;
    }

} // namespace pn532
