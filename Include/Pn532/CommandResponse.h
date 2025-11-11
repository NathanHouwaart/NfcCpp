/**
 * @file CommandResponse.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief PN532 command response structure
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
    class IPn532Command;

    /**
     * @brief Represents a PN532 command response
     * 
     */
    class CommandResponse
    {
    public:
        static constexpr size_t MaxPayloadSize = 300;

        /**
         * @brief Get string representation of the command response
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
         * @brief Construct a new CommandResponse object
         * 
         * @param cmd Command code
         * @param payload Payload data
         */
        CommandResponse(uint8_t cmd, const etl::ivector<uint8_t>& payload);

        uint8_t cmd;
        etl::vector<uint8_t, MaxPayloadSize> payload;
    
    friend class IPn532Command;
    };

} // namespace pn532
