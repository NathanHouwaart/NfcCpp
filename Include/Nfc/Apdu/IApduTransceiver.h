/**
 * @file ApduTransceiver.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Defines APDU transceiver interface
 * @version 0.1
 * @date 2025-11-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/expected.h>
#include <etl/vector.h>

#include "Error/ApduError.h"
#include "ApduResponse.h"

/**
 * @brief Interface for APDU transceivers
 */
class ApduTransceiver {
public:
    virtual ~ApduTransceiver() = default;

    /**
     * @brief Transmits an APDU command and receives the response
     * 
     * @param apduCommand apdu command to transmit
     * @return etl::expected<ApduResponse, error::ApduError> apdu response on success, ApduError on failure
     */
    etl::expected<ApduResponse, error::ApduError> transmitApdu(const etl::ivector<uint8_t>& apduCommand) = 0;
private: 

    /**
     * @brief Parses raw APDU response data into an ApduResponse object
     * 
     * @param raw raw APDU response data
     * @return etl::expected<ApduResponse, error::ApduError> apdu response on success, ApduError on failure
     */
    etl::expected<ApduResponse, error::ApduError> parseApduResponse(const etl::ivector<uint8_t>& raw) = 0;

}