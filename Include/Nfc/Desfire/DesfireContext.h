/**
 * @file DesfireContext.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire session context
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/vector.h>
#include <cstdint>

namespace nfc
{
    /**
     * @brief DESFire communication mode
     */
    enum class CommMode : uint8_t
    {
        Plain,      // No security
        MACed,      // MAC only
        Enciphered  // Full encryption
    };

    /**
     * @brief Active authenticated session flavor.
     */
    enum class SessionAuthScheme : uint8_t
    {
        None = 0x00,
        Legacy = 0x0A,
        Iso = 0x1A,
        Aes = 0xAA
    };

    /**
     * @brief DESFire session context
     * 
     * Stores session state including authentication status and session keys
     */
    struct DesfireContext
    {
        bool authenticated = false;
        CommMode commMode = CommMode::Plain;
        SessionAuthScheme authScheme = SessionAuthScheme::None;
        
        etl::vector<uint8_t, 24> sessionKeyEnc;  // Encryption session key
        etl::vector<uint8_t, 24> sessionKeyMac;  // MAC session key
        etl::vector<uint8_t, 16> iv;             // Initialization vector
        etl::vector<uint8_t, 16> sessionEncRndB; // Raw encrypted RndB from last successful authenticate
        
        uint8_t keyNo = 0;                        // Current authenticated key number
        etl::vector<uint8_t, 3> selectedAid;     // Selected application ID
    };

} // namespace nfc
