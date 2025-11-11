/**
 * @file ReaderCapabilities.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Reader capabilities structure
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <cstddef>

namespace nfc
{
    /**
     * @brief Reader capabilities structure
     * 
     * Describes the capabilities of an NFC reader
     */
    struct ReaderCapabilities
    {
        size_t maxApduSize;              ///< Maximum APDU size supported
        bool supportsIso14443_4;         ///< Supports ISO 14443-4 protocol
        bool supportsMifareClassic;      ///< Supports MIFARE Classic
        bool supportsFeliCa;             ///< Supports FeliCa
        bool supportsNfcDep;             ///< Supports NFC-DEP (P2P)

        /**
         * @brief Create capabilities for PN532 reader
         * 
         * @return ReaderCapabilities PN532 capabilities
         */
        static ReaderCapabilities pn532();

        /**
         * @brief Create capabilities for RC522 reader
         * 
         * @return ReaderCapabilities RC522 capabilities
         */
        static ReaderCapabilities rc522();
    };

} // namespace nfc
