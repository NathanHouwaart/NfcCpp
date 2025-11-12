/**
 * @file GetGeneralStatus.cpp
 * @author your name (you@domain.com)
 * @brief GetGeneralStatus command implementation
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Commands/GetGeneralStatus.h"
#include "Error/Pn532Error.h"
#include <cstdio>

using namespace error;

namespace pn532
{
    etl::string<255> GeneralStatus::toString() const
    {
        etl::string<255> result;
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), 
                     "Err: 0x%02X, Field: 0x%02X, NbTg: %u, TG1: [%02X %02X %02X %02X], TG2: [%02X %02X %02X %02X], SAM Status: 0x%02X",
                     err, field, nbTg,
                     tg1[0], tg1[1], tg1[2], tg1[3],
                     tg2[0], tg2[1], tg2[2], tg2[3],
                     samStatus);
        result.assign(buffer);
        return result;
    }

    etl::string_view GetGeneralStatus::name() const
    {
        return "GetGeneralStatus";
    }

    CommandRequest GetGeneralStatus::buildRequest()
    {
        // GetGeneralStatus command has no parameters
        etl::vector<uint8_t, 1> payload;
        return createCommandRequest(0x04, payload); // 0x04 = GetGeneralStatus
    }

    etl::expected<CommandResponse, Error> GetGeneralStatus::parseResponse(const Pn532ResponseFrame& frame)
    {
        // Expected response length is 4 bytes
        constexpr size_t kExpectedLength = 4;

        const auto& data = frame.data();
        
        if (data.size() < kExpectedLength)
        {
            return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
        }

        uint8_t index = 0;

        // Parse general status
        GeneralStatus status;
        status.err = data[index++];
        status.field = data[index++];
        status.nbTg = data[index++];
        
        // If there are tags, parse their status
        for(int i = 0; i < status.nbTg && i < 2; ++i)
        {
            etl::array<uint8_t, 4>& tgStatus = (i == 0) ? status.tg1 : status.tg2;
            for(int j = 0; j < 4; ++j)
            {
                if (index < data.size())
                {
                    tgStatus[j] = data[index++];
                }
                else
                {
                    return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
                }
            }
        }
        status.samStatus = data[index++];

        // Cache the status
        cachedStatus = status;

        return createCommandResponse(frame.getCommandCode(), data);
    }

    bool GetGeneralStatus::expectsDataFrame() const
    {
        return true;
    }

    const GeneralStatus& GetGeneralStatus::getGeneralStatus() const
    {
        return cachedStatus;
    }

} // namespace pn532