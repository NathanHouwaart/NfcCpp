/**
 * @file DesfireResult.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire result structure
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
     * @brief DESFire result structure
     * 
     * Contains status code and response data
     */
    struct DesfireResult
    {
        uint8_t statusCode;
        etl::vector<uint8_t, 256> data;

        /**
         * @brief Check if the result is a success
         * 
         * @return true Success (0x00)
         * @return false Error or additional frame
         */
        bool isSuccess() const
        {
            return statusCode == 0x00;
        }

        /**
         * @brief Check if additional frame is required
         * 
         * @return true Additional frame needed (0xAF)
         * @return false Complete response
         */
        bool isAdditionalFrame() const
        {
            return statusCode == 0xAF;
        }
    };

} // namespace nfc
