/**
 * @file Pn532ApduAdapter.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Adapter for PN532 driver to APDU transceiver interface
 * @version 0.1
 * @date 2025-11-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "Nfc/Apdu/IApduTransceiver.h"
#include "Nfc/Apdu/ApduResponse.h"
#include "Nfc/Card/ICardDetector.h"
#include "Nfc/Card/CardInfo.h"
#include "Pn532/Pn532Driver.h"
#include "Error/Error.h"

#include <etl/vector.h>
#include <etl/expected.h>

/**
 * @brief Adapter that wraps Pn532Driver to provide APDU and card detection interfaces
 * 
 * This adapter implements both IApduTransceiver and ICardDetector interfaces,
 * allowing the PN532 driver to be used for APDU communication and card detection
 * in a standardized way.
 */
class Pn532ApduAdapter : public IApduTransceiver, public ICardDetector {
public:
    /**
     * @brief Construct a new Pn532ApduAdapter
     * @param driver Reference to the PN532 driver instance
     */
    explicit Pn532ApduAdapter(Pn532Driver& driver);
    
    /**
     * @brief Destroy the Pn532ApduAdapter
     */
    virtual ~Pn532ApduAdapter() = default;

    // IApduTransceiver interface implementation
    
    /**
     * @brief Transmit an APDU command and receive the response
     * @param apdu The APDU command to send
     * @return Expected ApduResponse on success, Error on failure
     */
    etl::expected<ApduResponse, error::Error> transceive(
        const etl::ivector<uint8_t>& apdu
    ) override;

    // ICardDetector interface implementation
    
    /**
     * @brief Detect and identify a card
     * @return Expected CardInfo on success, Error on failure
     */
    etl::expected<CardInfo, error::Error> detectCard() override;
    
    /**
     * @brief Check if a card is present
     * @return true if a card is detected, false otherwise
     */
    bool isCardPresent() override;

private:
    /**
     * @brief Parse raw response data into ApduResponse structure
     * @param raw Raw response bytes from the card
     * @return Expected ApduResponse on success, Error on failure
     */
    etl::expected<ApduResponse, error::Error> parseApduResponse(
        const etl::ivector<uint8_t>& raw
    );

    Pn532Driver& driver;  ///< Reference to the PN532 driver
};
