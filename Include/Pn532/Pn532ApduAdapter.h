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
#include "Error/Error.h"

#include <etl/vector.h>
#include <etl/expected.h>

using namespace nfc;

namespace pn532
{
    class Pn532Driver;  // Forward declaration


    /**
     * @brief Adapter that wraps Pn532Driver to provide APDU and card detection interfaces
     *
     * This adapter implements both IApduTransceiver and ICardDetector interfaces,
     * allowing the PN532 driver to be used for APDU communication and card detection
     * in a standardized way.
     */
    class Pn532ApduAdapter : public IApduTransceiver, public ICardDetector
    {
    public:
        /**
         * @brief Construct a new Pn532ApduAdapter
         * @param driver Reference to the PN532 driver instance
         */
        explicit Pn532ApduAdapter(Pn532Driver &driver);

        /**
         * @brief Destroy the Pn532ApduAdapter
         */
        virtual ~Pn532ApduAdapter() = default;

        // IApduTransceiver interface implementation

        /**
         * @brief Configure wire protocol for current card session
         * @param wire Wire protocol (Native or ISO)
         */
        void setWire(IWire& wire) override;

        /**
         * @brief Transmit data to card and receive PDU response
         * @param apdu Command data to send
         * @return Expected PDU [Status][Data...] on success, Error on failure
         */
        etl::expected<etl::vector<uint8_t, buffer::APDU_DATA_MAX>, error::Error> transceive(
            const etl::ivector<uint8_t> &apdu) override;

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
        Pn532Driver &driver;
        IWire* activeWire;  // Current wire protocol for card session
    };

} // namespace pn532