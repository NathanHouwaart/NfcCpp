/**
 * @file CommandResponse.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief PN532 command response implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/CommandResponse.h"

namespace pn532
{
    CommandResponse::CommandResponse(uint8_t command, const etl::ivector<uint8_t>& payloadData)
        : cmd(command), payload(payloadData.begin(), payloadData.end())
    {
        // TODO: Initialize command response
    }

    etl::string<256> CommandResponse::toString() const
    {
        // TODO: Implement string representation
        etl::string<256> result;
        return result;
    }

    size_t CommandResponse::size() const
    {
        // TODO: Implement size calculation
        return payload.size();
    }

    bool CommandResponse::empty() const
    {
        // TODO: Implement empty check
        return payload.empty();
    }

    const etl::ivector<uint8_t>& CommandResponse::data() const
    {
        // TODO: Implement data access
        return payload;
    }

    uint8_t CommandResponse::getCommandCode() const
    {
        // TODO: Implement command code getter
        return cmd;
    }

} // namespace pn532
