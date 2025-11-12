/**
 * @file Pn532Error.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines PN532 specific error codes
 * @version 0.1
 * @date 2025-11-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

namespace error {

    /**
     * @brief PN532 error codes matching InDataExchange status byte values
     * These values correspond to the status byte returned by InDataExchange command
     */
    enum class Pn532Error : uint8_t {
        // Success/OK codes (not from InDataExchange)
        Ok = 0x00,
        
        // InDataExchange status byte codes
        Timeout = 0x01,                     // The target has not answered
        CRCError = 0x02,                    // CRC error detected by CIU
        ParityError = 0x03,                 // Parity error detected by CIU
        BitCountError = 0x04,               // Wrong bit count during anti-collision
        MifareFramingError = 0x05,          // Framing error during Mifare operation
        CollisionError = 0x06,              // Abnormal bit collision during anti-collision at 106 kbps
        BufferSizeInsufficient = 0x07,      // Insufficient communication buffer
        RFBufferOverflow = 0x09,            // RF buffer overflow
        RFFieldTimeout = 0x0A,              // RF field has not been switched on
        RFProtocolError = 0x0B,             // RF protocol error
        TemperatureError = 0x0D,            // Overheating - temperature sensor detected overheating
        InternalBufferOverflow = 0x0E,      // Internal buffer overflow
        InvalidParameter = 0x10,            // Invalid parameter
        CommandNotSupported = 0x12,         // Command not supported
        DataFormatError = 0x13,             // Wrong data format
        MifareAutError = 0x14,              // Authentication error (Mifare)
        UIDCheckByteError = 0x23,           // Wrong UID check byte
        InvalidDeviceState = 0x25,          // Invalid device state
        OperationNotAllowed = 0x26,         // Operation not allowed
        CommandNotAcceptable = 0x27,        // Command not acceptable
        Released = 0x29,                    // Target has been released
        CardExchanged = 0x2A,               // Card has been exchanged
        CardDisappeared = 0x2B,             // Card has disappeared
        Nfcid3Mismatch = 0x2C,              // NFCID3 initiator/target mismatch
        OverCurrent = 0x2D,                 // Over current
        MissingDEP = 0x2E,                  // NAD missing in DEP frame
        
        // Additional non-InDataExchange errors (use high values to avoid conflicts)
        SelftestFail = 0xF0,
        InvalidAckFrame = 0xF1,
        WrongConfig = 0xF2,
        WrongCommand = 0xF3,
        SAMerror = 0xF4,
        FrameCheckFailed = 0xF5,
        InvalidResponse = 0xF6,
        NotImplemented = 0xF7,
        Unknown = 0xFF
    };

} // namespace error