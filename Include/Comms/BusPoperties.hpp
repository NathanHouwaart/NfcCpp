/**
 * @file BusPoperties.hpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Properties for various communication buses
 * @version 0.1
 * @date 2025-11-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

namespace comms {

enum class BusProperty : uint8_t {
    // Serial / UART properties
    BaudRate,       // uint32_t
    Parity,         // enum (None, Even, Odd)
    StopBits,       // enum (One, Two)
    FlowControl,    // enum (None, Hardware, Software)
    Timeout,        // uint32_t in milliseconds

    // SPI properties
    SpiMode,        // enum (Mode0..Mode3)
    BitsPerWord,    // uint8_t
    SpiSpeed,       // uint32_t

    // I2C properties
    I2cAddress,     // uint8_t

    // Generic / future
    BufferSize,     // uint32_t
};

} // namespace comms