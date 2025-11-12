/**
 * @file CardInfo.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines NFC card information structure
 * @version 0.1
 * @date 2025-11-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/vector.h>
#include <etl/variant.h>
#include <etl/string.h>

#include <cstdint>

#include "CardType.h"

namespace nfc
{

   struct CardInfo {
      etl::vector<uint8_t, 10> uid;    // Unique Identifier of the card
      uint16_t atqa;                   // ATQA value
      uint8_t  sak;                    // SAK value
      etl::vector<uint8_t, 32> ats;    // ATS (Answer To Select) data, if applicable
      CardType type;                   // Detected card type

      CardType detectType();

      etl::string<255> toString() const;
   };

} // namespace nfc