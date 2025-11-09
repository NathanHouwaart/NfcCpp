/**
 * @file DesfireError.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines Desfire specific error codes
 * @version 0.1
 * @date 2025-11-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

namespace error {

    enum class DesfireError : uint8_t {
        Ok = 0x00,
        NoChanges = 0x0C,
        OutOfEeprom = 0x0E,
        IllegalCommand = 0x1C,
        IntegrityError = 0x1E,
        NoSuchKey = 0x40,
        LengthError = 0x7E,
        PermissionDenied = 0x9D,
        ParameterError = 0x9E,
        ApplicationNotFound = 0xA0,
        ApplIntegrityError = 0xA1,
        AuthenticationError = 0xAE,
        AdditionalFrame = 0xAF,
        BoundaryError = 0xBE,
        PiccIntegrityError = 0xC1,
        CommandAborted = 0xCA,
        PiccDisabled = 0xCD,
        CountError = 0xCE,
        DuplicateError = 0xDE,
        EepromError = 0xEE,
        FileNotFound = 0xF0,
        FileIntegrityError = 0xF1
    };

} // namespace error