/**
 * @file Pn532Constants.h
 * @author your name (you@domain.com)
 * @brief PN532 constants and definitions
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <cstdint>

namespace pn532 {
namespace protocol {

    // Frame markers
    constexpr uint8_t PREAMBLE = 0x00;
    constexpr uint8_t START_CODE_1 = 0x00;
    constexpr uint8_t START_CODE_2 = 0xFF;
    constexpr uint8_t POSTAMBLE = 0x00;

    // Direction bytes (TFI - Transport Frame Identifier)
    constexpr uint8_t TFI_HOST_TO_DEVICE = 0xD4;  // PC → PN532
    constexpr uint8_t TFI_DEVICE_TO_HOST = 0xD5;  // PN532 → PC

    // Command/Response offset
    constexpr uint8_t RESPONSE_CODE_OFFSET = 0x01;

    // ACK/NACK frames
    constexpr uint8_t ACK_PACKET_CODE = 0x00;
    constexpr uint8_t NACK_PACKET_CODE = 0xFF;

} // namespace protocol
} // namespace pn532