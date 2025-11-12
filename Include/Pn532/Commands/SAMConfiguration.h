/**
 * @file SAMConfiguration.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief SAMConfiguration command for PN532
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
     * @brief SAM (Security Access Module) modes
     * 
     */
    enum class SamMode : uint8_t
    {
        Normal = 0x01,           // Normal mode - SAM not used
        VirtualCard = 0x02,      // Virtual card mode
        WiredCard = 0x03,        // Wired card mode
        DualCard = 0x04          // Dual card mode
    };

    /**
     * @brief SAMConfiguration command options
     * 
     */
    struct SAMConfigurationOptions
    {
        SamMode mode = SamMode::Normal;
        uint8_t timeout = 0x00;      // Timeout (only for Virtual Card mode), 0x00 = no timeout
        bool useIRQ = false;         // Use IRQ pin (optional, default false)
    };

    /**
     * @brief SAMConfiguration command - Configure the SAM
     * 
     * This command is used to configure the Security Access Module (SAM).
     * For typical NFC operations, Normal mode (0x01) is used.
     */
    class SAMConfiguration : public IPn532Command
    {
    public:
        explicit SAMConfiguration(const SAMConfigurationOptions& opts);
        SAMConfiguration(SamMode mode); // Convenience constructor

        etl::string_view name() const override;
        CommandRequest buildRequest() override;
        etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) override;
        bool expectsDataFrame() const override;

    private:
        SAMConfigurationOptions options;
    };

} // namespace pn532
