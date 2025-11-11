/**
 * @file WireKind.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Wire protocol kind enumeration
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
     * @brief Wire protocol kinds
     * 
     */
    enum class WireKind : uint8_t
    {
        Native,  ///< Native DESFire protocol
        Iso      ///< ISO 14443-4 / ISO 7816-4 wrapped protocol
    };

} // namespace nfc
