/**
 * @file ApduError.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines APDU error codes
 * @version 0.1
 * @date 2025-11-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

namespace error {

    enum class ApduError : uint8_t {
        Ok, 
        WrongLength,
        SecurityStatusNotSatisfied,
        ConditionsNotSatisfied,
        FileNotFound,
        WrongP1P2,
        Unknown
    };

} // namespace error