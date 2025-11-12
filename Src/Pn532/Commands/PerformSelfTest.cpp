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
#include "Error/Pn532Error.h"
#include <algorithm>

using namespace error;

namespace pn532
{
    PerformSelfTest::PerformSelfTest(const SelfTestOptions& opts)
        : options(opts)
    {
        // Set default timeout if not specified
        if (options.responseTimeoutMs == 0)
        {
            options.responseTimeoutMs = defaultTimeoutFor(options.test);
        }
    }

    etl::string_view PerformSelfTest::name() const
    {
        return "PerformSelfTest";
    }

    CommandRequest PerformSelfTest::buildRequest()
    {
        // Build payload: [NumTst] [Parameters...]
        etl::vector<uint8_t, 256> payload;
        payload.push_back(static_cast<uint8_t>(options.test));
        
        // Add test-specific parameters
        for (size_t i = 0; i < options.parameters.size(); ++i)
        {
            payload.push_back(options.parameters[i]);
        }
        
        // EchoBack test (0x05) never returns a data frame, only ACK
        bool expectsData = (options.test != TestType::EchoBack);
        
        return createCommandRequest(0x00, payload, options.responseTimeoutMs, expectsData); // 0x00 = Diagnose
    }

    etl::expected<CommandResponse, Error> PerformSelfTest::parseResponse(const Pn532ResponseFrame& frame)
    {
        const auto& data = frame.data();
        
        // Validate response based on test type
        switch (options.test)
        {
        case TestType::CommunicationLine:
            {
                // Echo test: response should be [NumTst] [Parameters...]
                const size_t expectedSize = 1 + options.parameters.size();
                
                if (data.size() != expectedSize)
                {
                    return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
                }
                
                if (options.verifyEcho)
                {
                    // Verify NumTst echoed back
                    if (data[0] != static_cast<uint8_t>(options.test))
                    {
                        return etl::unexpected(Error::fromPn532(Pn532Error::FrameCheckFailed));
                    }
                    
                    // Verify parameters echoed back
                    if (!options.parameters.empty())
                    {
                        bool parametersMatch = std::equal(
                            options.parameters.begin(),
                            options.parameters.end(),
                            data.begin() + 1
                        );
                        
                        if (!parametersMatch)
                        {
                            return etl::unexpected(Error::fromPn532(Pn532Error::FrameCheckFailed));
                        }
                    }
                }
            }
            break;
            
        case TestType::EchoBack:
            // EchoBack test never returns a response frame
            if (!data.empty())
            {
                return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
            }
            break;
            
        default:
            // Other tests should return at least some data
            if (data.empty())
            {
                return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
            }
            break;
        }
        
        return createCommandResponse(frame.getCommandCode(), data);
    }

    bool PerformSelfTest::expectsDataFrame() const
    {
        // EchoBack test (0x05) never returns a data frame
        return options.test != TestType::EchoBack;
    }

    uint8_t PerformSelfTest::makeAntennaThreshold(
        uint8_t highThresholdCode, 
        uint8_t lowThresholdCode,
        bool useUpperComparator, 
        bool useLowerComparator)
    {
        // Encode antenna threshold according to PN532 specification
        // Bit layout: [andet_bot][andet_up][low_thr1][low_thr0][high_thr1][high_thr0][andet_en]
        
        highThresholdCode &= 0x03;  // 2 bits
        lowThresholdCode &= 0x03;   // 2 bits
        
        uint8_t result = 0;
        
        if (useLowerComparator)
        {
            result |= (1u << 6);  // andet_bot
        }
        
        if (useUpperComparator)
        {
            result |= (1u << 5);  // andet_up
        }
        
        result |= (lowThresholdCode << 3);   // bits 4..3
        result |= (highThresholdCode << 1);  // bits 2..1
        result |= (1u << 0);                 // andet_en (enable)
        
        return result;
    }

    uint32_t PerformSelfTest::defaultTimeoutFor(TestType test)
    {
        switch (test)
        {
        case TestType::CommunicationLine:
            return 500;      // Quick echo
        case TestType::RomChecksum:
        case TestType::RamIntegrity:
            return 2000;     // Memory tests
        case TestType::PollingToTarget:
        case TestType::EchoBack:
            return 4000;     // RF operations
        case TestType::CardPresence:
        case TestType::AntennaContinuity:
            return 2000;     // Analog measurements
        default:
            return 5000;
        }
    }

} // namespace pn532
