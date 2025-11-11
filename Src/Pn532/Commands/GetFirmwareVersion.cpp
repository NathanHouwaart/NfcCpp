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

namespace pn532
{
    etl::string<255> FirmwareInfo::toString() const
    {
        // TODO: Implement string representation
        etl::string<255> result;
        return result;
    }

    etl::string_view GetFirmwareVersion::name() const
    {
        return "GetFirmwareVersion";
    }

    CommandRequest GetFirmwareVersion::buildRequest()
    {
        // TODO: Implement request building
        etl::vector<uint8_t, 1> payload;
        return createCommandRequest(0x02, payload); // 0x02 = GetFirmwareVersion command
    }

    etl::expected<CommandResponse, error::Error> GetFirmwareVersion::parseResponse(const Pn532ResponseFrame& frame)
    {
        // TODO: Implement response parsing
        etl::vector<uint8_t, 1> payload;
        return createCommandResponse(frame.getCommandCode(), payload);
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
