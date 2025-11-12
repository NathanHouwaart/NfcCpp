/**
 * @file SetSerialBaudRate.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief SetSerialBaudRate command for PN532
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
     * @brief Serial baud rate for PN532
     * 
     */
    enum class Pn532Baudrate : uint8_t
    {
        Baud9600 = 0x00,
        Baud19200 = 0x01,
        Baud38400 = 0x02,
        Baud57600 = 0x03,
        Baud115200 = 0x04,
        Baud230400 = 0x05,
        Baud460800 = 0x06,
        Baud921600 = 0x07,
        Baud1288000 = 0x08
    };

    /**
     * @brief SetSerialBaudRate command options
     * 
     */
    struct SetSerialBaudRateOptions
    {
        Pn532Baudrate baudRate = Pn532Baudrate::Baud115200;
    };

    /**
     * @brief SetSerialBaudRate command - Change serial communication speed
     * 
     * This command changes the baud rate for serial communication.
     * Note: After sending this command, the host must also change its baud rate
     * to match the new setting.
     */
    class SetSerialBaudRate : public IPn532Command
    {
    public:
        explicit SetSerialBaudRate(const SetSerialBaudRateOptions& opts);

        etl::string_view name() const override;
        CommandRequest buildRequest() override;
        etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) override;
        bool expectsDataFrame() const override;

    private:
        SetSerialBaudRateOptions options;
    };

} // namespace pn532
