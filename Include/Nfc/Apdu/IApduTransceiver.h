/**
 * @file ApduTransceiver.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines APDU transceiver interface
 * @version 0.1
 * @date 2025-11-09
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <etl/expected.h>
#include <etl/vector.h>

#include "Error/Error.h"
#include "Nfc/BufferSizes.h"

namespace nfc
{
    class IWire; // Forward declaration

    /**
     * @brief Interface for APDU transceivers
     * 
     * The adapter is configured with a Wire protocol at session start via setWire().
     * All subsequent transceive calls use that protocol to interpret card responses.
     * Returns normalized PDU format: [Status][Data...]
     */
    class IApduTransceiver
    {
    public:
        virtual ~IApduTransceiver() = default;

        /**
         * @brief Configure the wire protocol for the current card session
         * 
         * Must be called after card detection and before transceive operations.
         * The wire protocol determines how card responses are interpreted.
         *
         * @param wire Wire protocol (Native or ISO) based on card capabilities
         */
        virtual void setWire(IWire& wire) = 0;

        /**
         * @brief Transmits data to card and receives normalized PDU response
         *
         * Uses the Wire protocol configured via setWire() to interpret responses.
         * Returns PDU format: [Status][Data...] where Status is DESFire status byte.
         *
         * @param apdu Command data to transmit
         * @return etl::expected<etl::vector<uint8_t, buffer::APDU_DATA_MAX>, error::Error> 
         *         PDU response or error
         */
        virtual etl::expected<etl::vector<uint8_t, buffer::APDU_DATA_MAX>, error::Error> transceive(
            const etl::ivector<uint8_t> &apdu) = 0;
    };

} // namespace nfc