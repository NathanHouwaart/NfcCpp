/**
 * @file Pn532ResponseFrame.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief PN532 response frame structure
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/vector.h>
#include <etl/string.h>
#include <cstdint>

namespace pn532
{
    // Forward declaration for friend class
    class Pn532Driver;

    /**
     * @brief Represents a PN532 response frame
     * 
     */
    class Pn532ResponseFrame
    {
    public:
        static constexpr size_t MaxPayloadSize = 255;

        /**
         * @brief Get string representation of the response frame
         * 
         * @return etl::string<256> String representation
         */
        etl::string<256> toString() const;

        /**
         * @brief Get the size of the payload
         * 
         * @return size_t Size of the payload in bytes
         */
        size_t size() const;

        /**
         * @brief Check if the payload is empty
         * 
         * @return bool True if empty, false otherwise
         */
        bool empty() const;

        /**
         * @brief Get reference to the payload data
         * 
         * @return const etl::ivector<uint8_t>& Reference to payload vector
         */
        const etl::ivector<uint8_t>& data() const;

        /**
         * @brief Get the command code
         * 
         * @return uint8_t Command code
         */
        uint8_t getCommandCode() const;

    private:
        /**
         * @brief Construct a new Pn532ResponseFrame object
         * 
         * @param cmd Command code
         * @param payload Payload data
         */
        Pn532ResponseFrame(uint8_t cmd, const etl::ivector<uint8_t>& payload);

        uint8_t commandCode;
        etl::vector<uint8_t, MaxPayloadSize> payload;

        // Friend declaration to allow Pn532Driver to construct response frames
        friend class Pn532Driver;
    };

} // namespace pn532
