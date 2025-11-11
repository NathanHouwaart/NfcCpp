/**
 * @file InDataExchange.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief InDataExchange command for APDU communication
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
     * @brief InDataExchange command options
     * 
     */
    struct InDataExchangeOptions
    {
        uint8_t targetNumber;
        etl::vector<uint8_t, 255> payload;
        uint32_t responseTimeoutMs;

        InDataExchangeOptions() 
            : targetNumber(0x01), responseTimeoutMs(1000) {}
    };

    /**
     * @brief InDataExchange command
     * 
     */
    class InDataExchange : public IPn532Command
    {
    public:
        explicit InDataExchange(const InDataExchangeOptions& opts);

        etl::string_view name() const override;
        CommandRequest buildRequest() override;
        etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) override;
        bool expectsDataFrame() const override;

        /**
         * @brief Get the status byte from the response
         * 
         * @return uint8_t Status byte
         */
        uint8_t getStatusByte() const;

    private:
        InDataExchangeOptions options;
        uint8_t cachedStatusByte;
    };

} // namespace pn532
