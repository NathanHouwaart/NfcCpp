/**
 * @file ICardDetector.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Interface for NFC card detection
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
#include "CardInfo.h"

namespace nfc
{

    /**
     * @brief Interface for NFC card detection
     *
     */
    class ICardDetector
    {
    public:
        virtual ~ICardDetector() = default;

        /**
         * @brief Detects an NFC card and retrieves its information
         *
         * @return etl::expected<CardInfo, error::Error> card information on success, Error on failure
         */
        virtual etl::expected<CardInfo, error::Error> detectCard() = 0;

        /**
         * @brief Checks if a card is currently present
         *
         * @return true if a card is present
         * @return false if no card is present
         */
        virtual bool isCardPresent() = 0;
    };

} // namespace nfc