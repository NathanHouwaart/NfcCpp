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

    // TODO: Match these error codes to actual PN532 error responses
    enum class Pn532Error : uint8_t {
        Ok,
        Timeout,
        CRCError,
        ParityError,
        CollisionError,
        MifareFramingError,
        BufferSizeInsufficient,
        SelftestFail,
        RFBufferOverflow,
        RFFieldTimeout,
        RFProtocolError,
        InvalidAckFrame,
        TemperatureError,
        InternalBufferOverflow,
        InvalidParameter,
        MifareAutError,
        UIDCheckByteError,
        WrongConfig,
        WrongCommand,
        Released,
        OverCurrent,
        MissingDEP,
        SAMerror,
        FrameCheckFailed,
        InvalidResponse,
        NotImplemented,
        Unknown
    };

} // namespace error