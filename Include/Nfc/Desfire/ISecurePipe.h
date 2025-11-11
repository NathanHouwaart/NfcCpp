/**
 * @file ISecurePipe.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Secure pipe interface for DESFire
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/vector.h>
#include <etl/expected.h>
#include "Error/Error.h"

namespace nfc
{
    /**
     * @brief Secure pipe interface
     * 
     * Interface for wrapping and unwrapping DESFire PDUs with different security levels
     */
    class ISecurePipe
    {
    public:
        virtual ~ISecurePipe() = default;

        /**
         * @brief Wrap a PDU with security
         * 
         * @param pdu Plain PDU data
         * @return etl::vector<uint8_t, 261> Wrapped data
         */
        virtual etl::vector<uint8_t, 261> wrap(const etl::ivector<uint8_t>& pdu) = 0;

        /**
         * @brief Unwrap a secured APDU
         * 
         * @param apdu Secured APDU data
         * @return etl::expected<etl::vector<uint8_t, 256>, error::Error> Unwrapped data or error
         */
        virtual etl::expected<etl::vector<uint8_t, 256>, error::Error> unwrap(const etl::ivector<uint8_t>& apdu) = 0;
    };

} // namespace nfc
