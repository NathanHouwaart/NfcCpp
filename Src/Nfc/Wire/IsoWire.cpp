/**
 * @file IsoWire.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief ISO 14443-4 wire protocol implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Wire/IsoWire.h"

using namespace nfc;
using namespace nfc::buffer;

etl::vector<uint8_t, APDU_COMMAND_MAX> IsoWire::wrap(const etl::ivector<uint8_t>& pdu)
{
    // ISO 7816-4 APDU wrapping
    // Format: CLA INS P1 P2 [Lc Data] [Le]
    // For DESFire: CLA=0x90, INS=command, P1=0x00, P2=0x00
    etl::vector<uint8_t, APDU_COMMAND_MAX> apdu;
    
    if (pdu.empty())
    {
        return apdu;
    }

    apdu.push_back(0x90);           // CLA (DESFire class)
    apdu.push_back(pdu[0]);         // INS (command code from PDU)
    apdu.push_back(0x00);           // P1
    apdu.push_back(0x00);           // P2
    
    if (pdu.size() > 1)
    {
        apdu.push_back(static_cast<uint8_t>(pdu.size() - 1)); // Lc (data length)
        for (size_t i = 1; i < pdu.size(); ++i)
        {
            apdu.push_back(pdu[i]);  // Data
        }
    }
    
    apdu.push_back(0x00);           // Le (expected response length, 0 = max 256)
    
    return apdu;
}

etl::expected<etl::vector<uint8_t, APDU_DATA_MAX>, error::Error> IsoWire::unwrap(const etl::ivector<uint8_t>& apdu)
{
    // ISO 7816-4 APDU unwrapping
    // Input format: [Data...][SW1][SW2]
    // Output format: [Status][Data...] where Status is the DESFire status code
    
    if (apdu.size() < APDU_STATUS_SIZE)
    {
        return etl::unexpected(error::Error::fromApdu(error::ApduError::WrongLength));
    }

    // Last 2 bytes are SW1 SW2
    uint8_t sw1 = apdu[apdu.size() - 2];
    uint8_t sw2 = apdu[apdu.size() - 1];

    // ISO status mapping to DESFire status:
    // 0x90 0x00 -> 0x00 (success)
    // 0x91 0xXX -> 0xXX (DESFire status like 0xAF, 0xAE, etc.)
    if (sw1 != 0x90 && sw1 != 0x91)
    {
        // Unexpected ISO status word
        return etl::unexpected(error::Error::fromApdu(error::ApduError::Unknown));
    }

    etl::vector<uint8_t, APDU_DATA_MAX> result;
    
    // Map ISO status to DESFire status byte
    result.push_back(sw2);  // 0x00 for success, or DESFire status code
    
    // Extract data (everything except last 2 status bytes)
    for (size_t i = 0; i < apdu.size() - APDU_STATUS_SIZE; ++i)
    {
        result.push_back(apdu[i]);
    }
    
    return result;
}
