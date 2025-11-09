/**
 * @file Rc522Error.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines RC522 specific error codes
 * @version 0.1
 * @date 2025-11-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

namespace error {

    // TODO: Match these error codes to actual PN532 error responses
    enum class Rc522Error : uint8_t {
        Ok = 0,
        Timeout,
        FifoOverflow,
        CrcError,
        ParityError,
        Collision,
        AuthError,
        FrameError,
        ProtocolError,
    };

} // namespace error