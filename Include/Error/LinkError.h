/**
 * @file LinkError.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines link layer error codes
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
     * @brief Link layer error codes
     * 
     */
    enum class LinkError : uint8_t {
        Ok = 0x00,
        Timeout = 0x01,
        CrcError = 0x02,
        ParityError = 0x03,
        Collision = 0x06,
        BufferInsufficient = 0x07,
        RfError = 0x0B,
        AuthenticationError = 0x14,
        CardDisappeared = 0x2B
    };

} // namespace error