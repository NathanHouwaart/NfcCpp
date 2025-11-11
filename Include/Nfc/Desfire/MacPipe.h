/**
 * @file MacPipe.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief MAC pipe for DESFire
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "ISecurePipe.h"

namespace nfc
{
    /**
     * @brief MAC pipe (adds/verifies MAC)
     * 
     * Adds CMAC for request authentication and verifies response MAC
     */
    class MacPipe : public ISecurePipe
    {
    public:
        /**
         * @brief Wrap a PDU with MAC
         * 
         * @param pdu Plain PDU data
         * @return etl::vector<uint8_t, 261> Data with MAC appended
         */
        etl::vector<uint8_t, 261> wrap(const etl::ivector<uint8_t>& pdu) override;

        /**
         * @brief Unwrap an APDU and verify MAC
         * 
         * @param apdu APDU data with MAC
         * @return etl::expected<etl::vector<uint8_t, 256>, error::Error> Data without MAC or error
         */
        etl::expected<etl::vector<uint8_t, 256>, error::Error> unwrap(const etl::ivector<uint8_t>& apdu) override;
    };

} // namespace nfc
