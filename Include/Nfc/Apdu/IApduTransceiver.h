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

#include "Error/Error.h"
#include "ApduResponse.h"

/**
 * @brief Interface for APDU transceivers
 */
class IApduTransceiver {
public:
    virtual ~IApduTransceiver() = default;

    /**
     * @brief Transmits an APDU command and receives the response
     * 
     * @param apduCommand apdu command to transmit
     * @return etl::expected<ApduResponse, error::Error> apdu response on success, ApduError on failure
     */
    virtual etl::expected<ApduResponse, error::Error> transceive(const etl::ivector<uint8_t>& apdu) = 0;
private: 

    /**
     * @brief Parses raw APDU response data into an ApduResponse object
     * 
     * @param raw raw APDU response data
     * @return etl::expected<ApduResponse, error::Error> apdu response on success, ApduError on failure
     */
    virtual etl::expected<ApduResponse, error::Error> parseApduResponse(const etl::ivector<uint8_t>& raw) = 0;

};