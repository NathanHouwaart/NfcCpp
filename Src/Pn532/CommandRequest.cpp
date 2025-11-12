/**
 * @file CommandRequest.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief PN532 command request implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/CommandRequest.h"

namespace pn532
{
    CommandRequest::CommandRequest(uint8_t cmd, const etl::ivector<uint8_t>& payloadData, uint32_t timeout, bool expectsData)
        : commandCode(cmd), payload(payloadData.begin(), payloadData.end()), responseTimeoutMs(timeout), expectsData(expectsData)
    {
        // TODO: Initialize command request
    }

    etl::string<256> CommandRequest::toString() const
    {
        // TODO: Implement string representation
        etl::string<256> result;
        return result;
    }

    size_t CommandRequest::size() const
    {
        // TODO: Implement size calculation
        return payload.size();
    }

    bool CommandRequest::empty() const
    {
        // TODO: Implement empty check
        return payload.empty();
    }

    uint32_t CommandRequest::timeoutMs() const
    {
        // TODO: Implement timeout getter
        return responseTimeoutMs;
    }

    const etl::ivector<uint8_t>& CommandRequest::data() const
    {
        // TODO: Implement data access
        return payload;
    }

    uint8_t CommandRequest::getCommandCode() const
    {
        // TODO: Implement command code getter
        return commandCode;
    }

    bool CommandRequest::expectsDataFrame() const
    {
        return expectsData;
    }

} // namespace pn532
