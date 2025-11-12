/**
 * @file RFConfiguration.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief RFConfiguration command for PN532
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "Pn532/IPn532Command.h"
#include <cstdint>

namespace pn532
{
    /**
     * @brief RF Configuration items
     * 
     */
    enum class RFConfigItem : uint8_t
    {
        RFField = 0x01,              // RF field configuration
        VariousTimings = 0x02,       // Various timings
        MaxRetries = 0x05,           // Max retries for communication
        AnalogSettings106kA = 0x0A,  // Analog settings for 106 kbps Type A
        AnalogSettings212_424k = 0x0B // Analog settings for 212/424 kbps
    };

    /**
     * @brief RFConfiguration command options
     * 
     */
    struct RFConfigurationOptions
    {
        RFConfigItem item = RFConfigItem::MaxRetries;
        etl::vector<uint8_t, 16> configData;  // Configuration data (item-specific)
    };

    /**
     * @brief RFConfiguration command - Configure RF parameters
     * 
     * This command is used to configure various RF parameters including:
     * - Max retries for ATR_REQ, PSL_REQ, and passive target activation
     * - RF field settings
     * - Timing parameters
     */
    class RFConfiguration : public IPn532Command
    {
    public:
        explicit RFConfiguration(const RFConfigurationOptions& opts);

        etl::string_view name() const override;
        CommandRequest buildRequest() override;
        etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) override;
        bool expectsDataFrame() const override;

    private:
        RFConfigurationOptions options;
    };

} // namespace pn532
