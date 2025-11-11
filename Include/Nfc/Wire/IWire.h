/**
 * @file IWire.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Interface for wire protocol wrappers (ISO vs Native)
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/vector.h>
#include <etl/expected.h>
#include <cstdint>
#include "Error/Error.h"

namespace nfc
{
    /**
     * @brief Interface for wire protocol wrappers
     * 
     * Wires handle the wrapping/unwrapping of PDUs to/from APDUs
     * for different communication protocols (Native DESFire vs ISO 14443-4)
     */
    class IWire
    {
    public:
        virtual ~IWire() = default;

        /**
         * @brief Wrap a PDU (Protocol Data Unit) into an APDU
         * 
         * @param pdu PDU to wrap
         * @return etl::vector<uint8_t, 261> Wrapped APDU (max 256 bytes + 5 byte header)
         */
        virtual etl::vector<uint8_t, 261> wrap(const etl::ivector<uint8_t>& pdu) = 0;

        /**
         * @brief Unwrap an APDU response to get the PDU
         * 
         * @param apdu APDU response to unwrap
         * @return etl::expected<etl::vector<uint8_t, 256>, error::Error> Unwrapped PDU or error
         */
        virtual etl::expected<etl::vector<uint8_t, 256>, error::Error> unwrap(const etl::ivector<uint8_t>& apdu) = 0;
    };

} // namespace nfc
