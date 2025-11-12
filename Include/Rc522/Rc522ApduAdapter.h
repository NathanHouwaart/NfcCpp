/**
 * @file Rc522ApduAdapter.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Adapter for RC522 driver to APDU transceiver interface
 * @version 0.1
 * @date 2025-11-10
 *
 * @copyright Copyright (c) 2025
 *
 * @note This is a placeholder implementation to demonstrate system extensibility
 *       RC522 driver implementation is not included in this version
 */

#pragma once

#include "Nfc/Apdu/IApduTransceiver.h"
#include "Nfc/Apdu/ApduResponse.h"
#include "Nfc/Card/ICardDetector.h"
#include "Nfc/Card/CardInfo.h"
#include "Error/Error.h"

#include <etl/vector.h>
#include <etl/expected.h>

// Forward declaration of RC522 driver (not implemented)
class Rc522Driver;

using namespace nfc;

namespace rc522
{

    /**
     * @brief Adapter that wraps Rc522Driver to provide APDU and card detection interfaces
     *
     * This is a placeholder class to demonstrate how the system can be extended
     * to support different NFC reader hardware. The RC522 implementation is not
     * included in this version.
     *
     * @note To implement RC522 support:
     *       1. Implement Rc522Driver class
     *       2. Implement the methods in this adapter
     *       3. Add RC522-specific error handling
     *       4. Test with actual RC522 hardware
     */
    class Rc522ApduAdapter : public IApduTransceiver, public ICardDetector
    {
    public:
        /**
         * @brief Construct a new Rc522ApduAdapter
         * @param driver Reference to the RC522 driver instance
         * @note RC522 driver not implemented - this is a placeholder
         */
        explicit Rc522ApduAdapter(Rc522Driver &driver);

        /**
         * @brief Destroy the Rc522ApduAdapter
         */
        virtual ~Rc522ApduAdapter() = default;

        // IApduTransceiver interface implementation

        /**
         * @brief Transmit an APDU command and receive the response
         * @param apdu The APDU command to send
         * @return Expected ApduResponse on success, Error on failure
         * @note Not implemented - placeholder only
         */
        etl::expected<ApduResponse, error::Error> transceive(
            const etl::ivector<uint8_t> &apdu) override;

        // ICardDetector interface implementation

        /**
         * @brief Detect and identify a card
         * @return Expected CardInfo on success, Error on failure
         * @note Not implemented - placeholder only
         */
        etl::expected<CardInfo, error::Error> detectCard() override;

        /**
         * @brief Check if a card is present
         * @return true if a card is detected, false otherwise
         * @note Not implemented - placeholder only
         */
        bool isCardPresent() override;

    private:
        Rc522Driver &driver; ///< Reference to the RC522 driver (not implemented)
    };

} // namespace rc522