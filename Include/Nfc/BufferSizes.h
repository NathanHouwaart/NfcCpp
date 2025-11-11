/**
 * @file BufferSizes.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Buffer size constants for NFC communication layers
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
    namespace buffer
    {
        // ========================================================================
        // Hardware Layer (PN532/RC522)
        // ========================================================================
        
        /**
         * @brief PN532 maximum frame size
         * 
         * Calculation:
         * - Preamble: 1 byte (0x00)
         * - Start codes: 2 bytes (0x00 0xFF)
         * - Length: 1 byte (LEN)
         * - Length checksum: 1 byte (LCS)
         * - Data payload: 254 bytes maximum
         * - Data checksum: 1 byte (DCS)
         * - Postamble: 1 byte (0x00)
         * Total: 1 + 2 + 1 + 1 + 254 + 1 + 1 = 261 bytes
         */
        constexpr size_t PN532_FRAME_MAX = 261;
        
        /**
         * @brief PN532 maximum data payload size
         * 
         * Maximum data that can be sent in a single PN532 frame
         */
        constexpr size_t PN532_DATA_MAX = 254;
        
        /**
         * @brief PN532 frame overhead
         * 
         * Calculation: Preamble(1) + Start(2) + LEN(1) + LCS(1) + DCS(1) + Postamble(1) = 7 bytes
         */
        constexpr size_t PN532_FRAME_OVERHEAD = 7;
        
        /**
         * @brief RC522 FIFO buffer size
         * 
         * RC522 has a 64-byte FIFO buffer for transceive operations
         */
        constexpr size_t RC522_FIFO_SIZE = 64;
        
        // ========================================================================
        // ISO 14443-4 / ISO 7816-4 APDU Layer
        // ========================================================================
        
        /**
         * @brief Maximum APDU command size
         * 
         * Calculation:
         * - Header: 4 bytes (CLA INS P1 P2)
         * - Lc: 1 byte (data length)
         * - Data: 255 bytes maximum
         * - Le: 1 byte (expected response length)
         * Total: 4 + 1 + 255 + 1 = 261 bytes
         */
        constexpr size_t APDU_COMMAND_MAX = 261;
        
        /**
         * @brief Maximum APDU response size
         * 
         * Calculation:
         * - Data: 256 bytes maximum
         * - Status: 2 bytes (SW1 SW2)
         * Total: 256 + 2 = 258 bytes
         */
        constexpr size_t APDU_RESPONSE_MAX = 258;
        
        /**
         * @brief Maximum APDU data payload (without header/status)
         * 
         * Standard short APDU maximum data size
         */
        constexpr size_t APDU_DATA_MAX = 256;
        
        /**
         * @brief APDU command data maximum (without header)
         * 
         * Maximum data that can be sent in APDU command (excluding CLA INS P1 P2 Lc Le)
         */
        constexpr size_t APDU_COMMAND_DATA_MAX = 255;
        
        /**
         * @brief APDU header size
         * 
         * CLA(1) + INS(1) + P1(1) + P2(1) = 4 bytes
         */
        constexpr size_t APDU_HEADER_SIZE = 4;
        
        /**
         * @brief APDU status word size
         * 
         * SW1(1) + SW2(1) = 2 bytes
         */
        constexpr size_t APDU_STATUS_SIZE = 2;
        
        // ========================================================================
        // DESFire Native Protocol Layer
        // ========================================================================
        
        /**
         * @brief Maximum DESFire command/response frame size
         * 
         * Calculation:
         * - Command code/Status: 1 byte
         * - Data: 255 bytes maximum
         * Total: 1 + 255 = 256 bytes
         */
        constexpr size_t DESFIRE_FRAME_MAX = 256;
        
        /**
         * @brief Maximum DESFire data payload
         * 
         * Maximum data in DESFire frame (excluding command/status byte)
         */
        constexpr size_t DESFIRE_DATA_MAX = 255;
        
        /**
         * @brief DESFire command/status byte size
         */
        constexpr size_t DESFIRE_HEADER_SIZE = 1;
        
        // ========================================================================
        // DESFire Encrypted Layer
        // ========================================================================
        
        /**
         * @brief Maximum DESFire plain data before encryption
         * 
         * Calculation:
         * - DESFire can send 255 bytes total
         * - CMAC takes 8 bytes
         * - Padding may add up to 15 bytes (AES block size)
         * - To safely fit in 255 bytes after encryption: 255 - 8 (CMAC) - 15 (max padding) = 232
         * - But for practical use with DES/3DES (8-byte blocks): 255 - 8 (CMAC) - 7 (max padding) = 240
         * - Conservative estimate for compatibility: 252 bytes allows room for encryption overhead
         */
        constexpr size_t DESFIRE_PLAIN_DATA_MAX = 252;
        
        /**
         * @brief Maximum DESFire encrypted frame size
         * 
         * Calculation:
         * - Status/Command: 1 byte
         * - Encrypted data: 252 bytes
         * - CMAC: 8 bytes
         * Total: 1 + 252 + 8 = 261 bytes (fits in PN532 frame)
         */
        constexpr size_t DESFIRE_ENCRYPTED_MAX = 261;
        
        /**
         * @brief DESFire CMAC size
         * 
         * CMAC is truncated to 8 bytes for DESFire
         */
        constexpr size_t DESFIRE_CMAC_SIZE = 8;
        
        /**
         * @brief DES block size
         * 
         * DES and 3DES use 8-byte blocks
         */
        constexpr size_t DES_BLOCK_SIZE = 8;
        
        /**
         * @brief AES block size
         * 
         * AES uses 16-byte blocks
         */
        constexpr size_t AES_BLOCK_SIZE = 16;
        
        // ========================================================================
        // Cryptographic Keys
        // ========================================================================
        
        /**
         * @brief Maximum key size (3DES 3-key = 24 bytes)
         * 
         * Supports: DES (8), 3DES 2-key (16), 3DES 3-key (24), AES-128 (16)
         */
        constexpr size_t KEY_SIZE_MAX = 24;
        
        /**
         * @brief DES key size
         */
        constexpr size_t KEY_SIZE_DES = 8;
        
        /**
         * @brief 3DES 2-key size
         */
        constexpr size_t KEY_SIZE_3DES_2K = 16;
        
        /**
         * @brief 3DES 3-key size or AES-192
         */
        constexpr size_t KEY_SIZE_3DES_3K = 24;
        
        /**
         * @brief AES-128 key size
         */
        constexpr size_t KEY_SIZE_AES128 = 16;
        
        /**
         * @brief Initialization Vector size
         * 
         * Maximum IV size for AES-128
         */
        constexpr size_t IV_SIZE_MAX = 16;
        
        // ========================================================================
        // DESFire Specific
        // ========================================================================
        
        /**
         * @brief DESFire UID maximum size
         * 
         * DESFire cards can have 4, 7, or 10 byte UIDs
         */
        constexpr size_t DESFIRE_UID_MAX = 10;
        
        /**
         * @brief DESFire Application ID size
         * 
         * AIDs are always 3 bytes
         */
        constexpr size_t DESFIRE_AID_SIZE = 3;
        
        /**
         * @brief DESFire random challenge size
         * 
         * Used in authentication (rndA, rndB)
         */
        constexpr size_t DESFIRE_RND_SIZE = 16;
        
        /**
         * @brief DESFire encrypted response size during authentication
         * 
         * RndA(16) + RndB(16) = 32 bytes
         */
        constexpr size_t DESFIRE_AUTH_RESPONSE_SIZE = 32;
        
        /**
         * @brief DESFire key cryptogram maximum size
         * 
         * Used in ChangeKey command: old key + new key + version + CRC
         */
        constexpr size_t DESFIRE_KEY_CRYPTOGRAM_MAX = 48;
        
    } // namespace buffer
    
} // namespace nfc
