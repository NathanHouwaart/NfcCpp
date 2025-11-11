/**
 * @file ReaderCapabilities.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Reader capabilities implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Card/ReaderCapabilities.h"

namespace nfc
{
    ReaderCapabilities ReaderCapabilities::pn532()
    {
        return ReaderCapabilities{
            .maxApduSize = 264,           // PN532 supports extended APDUs
            .supportsIso14443_4 = true,
            .supportsMifareClassic = true,
            .supportsFeliCa = true,
            .supportsNfcDep = true
        };
    }

    ReaderCapabilities ReaderCapabilities::rc522()
    {
        return ReaderCapabilities{
            .maxApduSize = 256,           // RC522 standard APDU size
            .supportsIso14443_4 = true,
            .supportsMifareClassic = true,
            .supportsFeliCa = false,
            .supportsNfcDep = false
        };
    }

} // namespace nfc
