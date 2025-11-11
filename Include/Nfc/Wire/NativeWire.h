/**
 * @file NativeWire.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Native wire protocol (DESFire native mode)
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
     * @brief Native wire protocol implementation
     * 
     * Native mode sends DESFire commands directly without ISO wrapping
     */
    class NativeWire : public IWire
    {
    public:
        NativeWire() = default;

        /**
         * @brief Wrap a PDU into native format
         * 
         * @param pdu PDU to wrap
         * @return etl::vector<uint8_t, 261> Wrapped data
         */
        etl::vector<uint8_t, 261> wrap(const etl::ivector<uint8_t>& pdu) override;

        /**
         * @brief Unwrap native format response
         * 
         * @param apdu Response to unwrap
         * @return etl::expected<etl::vector<uint8_t, 256>, error::Error> Unwrapped PDU or error
         */
        etl::expected<etl::vector<uint8_t, 256>, error::Error> unwrap(const etl::ivector<uint8_t>& apdu) override;
    };

} // namespace nfc
