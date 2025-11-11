/**
 * @file PlainPipe.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Plain (no security) pipe for DESFire
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
     * @brief Plain pipe (no security)
     * 
     * Passes data through without any encryption or MAC
     */
    class PlainPipe : public ISecurePipe
    {
    public:
        /**
         * @brief Wrap a PDU (pass through)
         * 
         * @param pdu Plain PDU data
         * @return etl::vector<uint8_t, 261> Same data
         */
        etl::vector<uint8_t, 261> wrap(const etl::ivector<uint8_t>& pdu) override;

        /**
         * @brief Unwrap an APDU (pass through)
         * 
         * @param apdu APDU data
         * @return etl::expected<etl::vector<uint8_t, 256>, error::Error> Same data or error
         */
        etl::expected<etl::vector<uint8_t, 256>, error::Error> unwrap(const etl::ivector<uint8_t>& apdu) override;
    };

} // namespace nfc
