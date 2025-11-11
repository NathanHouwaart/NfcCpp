/**
 * @file EncPipe.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Encryption pipe for DESFire
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
     * @brief Encryption pipe (full encryption)
     * 
     * Encrypts request data and decrypts response data
     */
    class EncPipe : public ISecurePipe
    {
    public:
        /**
         * @brief Wrap a PDU with encryption
         * 
         * @param pdu Plain PDU data
         * @return etl::vector<uint8_t, 261> Encrypted data
         */
        etl::vector<uint8_t, 261> wrap(const etl::ivector<uint8_t>& pdu) override;

        /**
         * @brief Unwrap and decrypt an APDU
         * 
         * @param apdu Encrypted APDU data
         * @return etl::expected<etl::vector<uint8_t, 256>, error::Error> Decrypted data or error
         */
        etl::expected<etl::vector<uint8_t, 256>, error::Error> unwrap(const etl::ivector<uint8_t>& apdu) override;
    };

} // namespace nfc
