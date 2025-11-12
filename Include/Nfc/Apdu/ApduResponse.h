/**
 * @file ApduResponse.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines APDU response structure
 * @version 0.1
 * @date 2025-11-09
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <etl/vector.h>
#include <cstdint>

namespace nfc
{

    class ApduResponse
    {
    public:
        etl::vector<uint8_t, 258> data; // Maximum APDU response size is 256 bytes + 2 bytes for SW1 and SW2
        uint8_t sw1;
        uint8_t sw2;

        ApduResponse() : sw1(0), sw2(0) {}

        ApduResponse(const etl::vector<uint8_t, 258> &responseData, uint8_t status1, uint8_t status2)
            : data(responseData), sw1(status1), sw2(status2) {}

        bool isSuccess() const
        {
            return (sw1 == 0x90 && sw2 == 0x00);
        }

        uint16_t getStatusWord() const
        {
            return (static_cast<uint16_t>(sw1) << 8) | sw2;
        }
    };

} // namespace nfc