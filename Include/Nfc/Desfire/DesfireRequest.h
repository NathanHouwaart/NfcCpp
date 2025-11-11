/**
 * @file DesfireRequest.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire request structure
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/vector.h>
#include <cstdint>
#include <cstddef>

namespace nfc
{
    /**
     * @brief DESFire request structure
     * 
     * Contains command code, request data, and expected response length
     */
    struct DesfireRequest
    {
        uint8_t commandCode;
        etl::vector<uint8_t, 252> data;
        size_t expectedResponseLength;
    };

} // namespace nfc
