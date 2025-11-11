/**
 * @file IsoWire.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief ISO 14443-4 wire protocol
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "IWire.h"

namespace nfc
{
    /**
     * @brief ISO 14443-4 wire protocol implementation
     * 
     * ISO mode wraps DESFire commands in ISO 7816-4 APDUs
     */
    class IsoWire : public IWire
    {
    public:
        IsoWire() = default;

        /**
         * @brief Wrap a PDU into ISO 7816-4 APDU format
         * 
         * @param pdu PDU to wrap
         * @return etl::vector<uint8_t, 261> Wrapped APDU
         */
        etl::vector<uint8_t, 261> wrap(const etl::ivector<uint8_t>& pdu) override;

        /**
         * @brief Unwrap ISO 7816-4 APDU response
         * 
         * @param apdu APDU response to unwrap
         * @return etl::expected<etl::vector<uint8_t, 256>, error::Error> Unwrapped PDU or error
         */
        etl::expected<etl::vector<uint8_t, 256>, error::Error> unwrap(const etl::ivector<uint8_t>& apdu) override;
    };

} // namespace nfc
