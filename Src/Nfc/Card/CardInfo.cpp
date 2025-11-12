/**
 * @file CardInfo.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Implementation of CardInfo methods
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Card/CardInfo.h"
#include <cstdio>
#include <etl/string.h>

CardType CardInfo::detectType()
{
    // Detect card type based on ATQA and SAK values
    // Reference: NFC Forum Type Tags and ISO14443 specifications
    // Note: ATQA is stored in little-endian format (as received from PN532)
    
    // DESFire cards: ATQA = 0x4403 (LE) or 0x4400 (LE), SAK = 0x20
    if ((atqa == 0x4403 || atqa == 0x4400) && sak == 0x20)
    {
        type = CardType::MifareDesfire;
        return type;
    }
    
    // MIFARE Ultralight: ATQA = 0x4400 (LE), SAK = 0x00
    if (atqa == 0x4400 && sak == 0x00)
    {
        type = CardType::MifareUltralight;
        return type;
    }
    
    // NTAG213/215/216: ATQA = 0x4400 (LE), SAK = 0x00 (with ATS)
    // Note: Similar to Ultralight, but can be distinguished by ATS presence
    if (atqa == 0x4400 && sak == 0x00 && !ats.empty())
    {
        type = CardType::Ntag213_215_216;
        return type;
    }
    
    // MIFARE Classic 1K: ATQA = 0x0400 (LE), SAK = 0x08
    if (atqa == 0x0400 && sak == 0x08)
    {
        type = CardType::MifareClassic;
        return type;
    }
    
    // MIFARE Classic 4K: ATQA = 0x0200 (LE), SAK = 0x18
    if (atqa == 0x0200 && sak == 0x18)
    {
        type = CardType::MifareClassic;
        return type;
    }
    
    // ISO14443-4 compliant cards (generic): SAK bit 6 set (0x20)
    if ((sak & 0x20) != 0)
    {
        type = CardType::ISO14443_4_Generic;
        return type;
    }
    
    // FeliCa: ATQA = 0x0100 (LE), SAK = 0x01
    if (atqa == 0x0100 && sak == 0x01)
    {
        type = CardType::FeliCa;
        return type;
    }
    
    // Unknown card type
    type = CardType::Unknown;
    return type;
}

etl::string<255> CardInfo::toString() const
{
    etl::string<255> result;
    char buffer[256];
    
    // Format card type
    const char* typeStr = "Unknown";
    switch (type)
    {
        case CardType::MifareDesfire:
            typeStr = "MIFARE DESFire";
            break;
        case CardType::MifareUltralight:
            typeStr = "MIFARE Ultralight";
            break;
        case CardType::MifareClassic:
            typeStr = "MIFARE Classic";
            break;
        case CardType::Ntag213_215_216:
            typeStr = "NTAG213/215/216";
            break;
        case CardType::FeliCa:
            typeStr = "FeliCa";
            break;
        case CardType::ISO14443_4_Generic:
            typeStr = "ISO14443-4 Generic";
            break;
        default:
            typeStr = "Unknown";
            break;
    }
    
    // Format UID as hex string
    char uidStr[32] = {0};
    char* uidPtr = uidStr;
    for (size_t i = 0; i < uid.size(); ++i)
    {
        uidPtr += std::snprintf(uidPtr, sizeof(uidStr) - (uidPtr - uidStr), "%02X", uid[i]);
        if (i < uid.size() - 1)
        {
            uidPtr += std::snprintf(uidPtr, sizeof(uidStr) - (uidPtr - uidStr), " ");
        }
    }
    
    // Format ATS as hex string if present
    char atsStr[128] = {0};
    if (!ats.empty())
    {
        char* atsPtr = atsStr;
        for (size_t i = 0; i < ats.size(); ++i)
        {
            atsPtr += std::snprintf(atsPtr, sizeof(atsStr) - (atsPtr - atsStr), "%02X", ats[i]);
            if (i < ats.size() - 1)
            {
                atsPtr += std::snprintf(atsPtr, sizeof(atsStr) - (atsPtr - atsStr), " ");
            }
        }
    }
    else
    {
        std::snprintf(atsStr, sizeof(atsStr), "None");
    }
    
    // Build final string
    // Note: ATQA is displayed in big-endian format (conventional notation)
    // even though it's stored in little-endian format internally
    uint16_t atqaDisplay = ((atqa & 0xFF) << 8) | ((atqa >> 8) & 0xFF);
    
    std::snprintf(buffer, sizeof(buffer),
                 "Card Type: %s\n"
                 "UID: %s (%zu bytes)\n"
                 "ATQA: 0x%04X\n"
                 "SAK: 0x%02X\n"
                 "ATS: %s",
                 typeStr,
                 uidStr, uid.size(),
                 atqaDisplay,
                 sak,
                 atsStr);
    
    result.assign(buffer);
    return result;
}
