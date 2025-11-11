/**
 * @file DesfireAuthMode.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire authentication mode enumeration
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <cstdint>

namespace nfc
{
    /**
     * @brief DESFire authentication mode
     */
    enum class DesfireAuthMode : uint8_t
    {
        LEGACY = 0x0A,
        ISO = 0x1A,
        AES = 0xAA
    };

} // namespace nfc
