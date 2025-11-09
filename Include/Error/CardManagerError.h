/**
 * @file CardManagerError.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines Card Manager specific error codes
 * @version 0.1
 * @date 2025-11-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

namespace error {

    enum class CardManagerError : uint8_t {
        Ok = 0,
        NoCardPresent,
        MultipleCards,
        UnsupportedCardType,
        CardMute,
        AuthenticationRequired,
        OperationFailed,
        InvalidParameter
    };

} // namespace error