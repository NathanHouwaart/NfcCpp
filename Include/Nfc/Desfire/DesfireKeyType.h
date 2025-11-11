/**
 * @file DesfireKeyType.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire key type enumeration
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
     * @brief DESFire key types
     */
    enum class DesfireKeyType : uint8_t
    {
        AES,
        DES,
        DES3_2K,
        DES3_3K,
        UNKNOWN
    };

} // namespace nfc
