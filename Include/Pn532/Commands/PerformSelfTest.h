/**
 * @file PerformSelfTest.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Perform self test command
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "Pn532/IPn532Command.h"
#include <etl/vector.h>
#include <cstdint>

namespace pn532
{
    /**
     * @brief Self test types
     * 
     */
    enum class TestType : uint8_t
    {
        CommunicationLine = 0x00,
        RomChecksum = 0x01,
        RamIntegrity = 0x02,
        PollingToTarget = 0x04,
        EchoBack = 0x05,
        CardPresence = 0x06,
        AntennaContinuity = 0x07
    };

    /**
     * @brief Self test options
     * 
     */
    struct SelfTestOptions
    {
        TestType test;
        etl::vector<uint8_t, 255> parameters;
        bool verifyEcho;
        uint32_t responseTimeoutMs;

        SelfTestOptions() 
            : test(TestType::CommunicationLine), verifyEcho(false), responseTimeoutMs(1000) {}
    };

    /**
     * @brief Perform self test command
     * 
     */
    class PerformSelfTest : public IPn532Command
    {
    public:
        explicit PerformSelfTest(const SelfTestOptions& opts);

        etl::string_view name() const override;
        CommandRequest buildRequest() override;
        etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) override;
        bool expectsDataFrame() const override;

        /**
         * @brief Make antenna threshold byte
         * 
         * @param highThresholdCode High threshold code
         * @param lowThresholdCode Low threshold code
         * @param useUpperComparator Use upper comparator
         * @param useLowerComparator Use lower comparator
         * @return uint8_t Encoded threshold byte
         */
        static uint8_t makeAntennaThreshold(
            uint8_t highThresholdCode, 
            uint8_t lowThresholdCode,
            bool useUpperComparator, 
            bool useLowerComparator);

    private:
        SelfTestOptions options;
        
        /**
         * @brief Get default timeout for a test type
         * 
         * @param test Test type
         * @return uint32_t Default timeout in milliseconds
         */
        static uint32_t defaultTimeoutFor(TestType test);
    };

} // namespace pn532
