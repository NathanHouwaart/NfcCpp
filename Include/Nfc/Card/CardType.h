/**
 * @file CardType.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines NFC card types
 * @version 0.1
 * @date 2025-11-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <cstdint>

enum class CardType : uint8_t {
    Unknown = 0,
    MifareDesfire,
    MifareUltralight,
    MifareClassic,
    Ntag213_215_216,
    FeliCa,
    ISO14443_4_Generic
};