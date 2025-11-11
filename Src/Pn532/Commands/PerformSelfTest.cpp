/**
 * @file PerformSelfTest.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Perform self test command implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Commands/PerformSelfTest.h"

namespace pn532
{
    PerformSelfTest::PerformSelfTest(const SelfTestOptions& opts)
        : options(opts)
    {
    }

    etl::string_view PerformSelfTest::name() const
    {
        return "PerformSelfTest";
    }

    CommandRequest PerformSelfTest::buildRequest()
    {
        // TODO: Implement request building
        etl::vector<uint8_t, 256> payload;
        payload.push_back(static_cast<uint8_t>(options.test));
        // Add parameters
        for (auto param : options.parameters)
        {
            payload.push_back(param);
        }
        return createCommandRequest(0x00, payload, options.responseTimeoutMs); // 0x00 = Diagnose command
    }

    etl::expected<CommandResponse, error::Error> PerformSelfTest::parseResponse(const Pn532ResponseFrame& frame)
    {
        // TODO: Implement response parsing
        etl::vector<uint8_t, 1> payload;
        return createCommandResponse(frame.getCommandCode(), payload);
    }

    bool PerformSelfTest::expectsDataFrame() const
    {
        return true;
    }

    uint8_t PerformSelfTest::makeAntennaThreshold(
        uint8_t highThresholdCode, 
        uint8_t lowThresholdCode,
        bool useUpperComparator, 
        bool useLowerComparator)
    {
        // TODO: Implement antenna threshold encoding
        uint8_t result = 0;
        result |= (highThresholdCode & 0x03) << 6;
        result |= (lowThresholdCode & 0x03) << 4;
        result |= (useUpperComparator ? 0x08 : 0x00);
        result |= (useLowerComparator ? 0x04 : 0x00);
        return result;
    }

} // namespace pn532
