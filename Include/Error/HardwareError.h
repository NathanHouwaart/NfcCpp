/**
 * @file HardwareError.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines hardware error codes for communication buses
 * @version 0.1
 * @date 2025-11-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

namespace error {

    /**
     * @brief Hardware communication error codes
     * 
     */
    enum class HardwareError : uint8_t {
        Ok = 0,
        Timeout,
        Nack,
        BusError,
        BufferOverflow,
        DeviceNotFound,
        WriteFailed,
        ReadFailed,
        InvalidConfiguration,
        NotSupported,
        Unknown
    };

} // namespace error