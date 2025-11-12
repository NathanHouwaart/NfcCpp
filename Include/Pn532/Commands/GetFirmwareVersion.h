/**
 * @file GetFirmwareVersion.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Get firmware version command
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "Pn532/IPn532Command.h"
#include <etl/string.h>
#include <cstdint>

namespace pn532
{
    /**
     * @brief Firmware information structure
     * 
     */
    struct FirmwareInfo
    {
        uint8_t ic;
        uint8_t ver;
        uint8_t rev;
        uint8_t support;

        /**
         * @brief Get string representation
         * 
         * @return etl::string<255> String representation
         */
        etl::string<255> toString() const;
    };

    /**
     * @brief Get firmware version command
     * 
     */
    class GetFirmwareVersion : public IPn532Command
    {
    public:
        GetFirmwareVersion() = default;

        etl::string_view name() const override;
        CommandRequest buildRequest() override;
        etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) override;
        bool expectsDataFrame() const override;

        /**
         * @brief Get the cached firmware info
         * 
         * @return const FirmwareInfo& Firmware info
         */
        const FirmwareInfo& getFirmwareInfo() const;

    private:
        FirmwareInfo cachedInfo;
    };

} // namespace pn532
