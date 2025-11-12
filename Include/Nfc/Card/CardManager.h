/**
 * @file CardManager.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Card manager for NFC operations
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/optional.h>
#include <etl/expected.h>
#include "Nfc/Apdu/IApduTransceiver.h"
#include "Nfc/Card/ICardDetector.h"
#include "Nfc/Wire/IWire.h"
#include "Nfc/Wire/NativeWire.h"
#include "Nfc/Wire/IsoWire.h"
#include "Nfc/Wire/WireKind.h"
#include "CardInfo.h"
#include "CardSession.h"
#include "ReaderCapabilities.h"
#include "Error/Error.h"

namespace nfc
{
    /**
     * @brief Card manager for NFC operations
     * 
     * Manages card detection, session creation, and wire protocol selection
     */
    class CardManager
    {
    public:
        /**
         * @brief Construct a new CardManager
         * 
         * @param transceiver APDU transceiver reference
         * @param detector Card detector reference
         * @param capabilities Reader capabilities
         */
        CardManager(
            ::IApduTransceiver& transceiver,
            ::ICardDetector& detector,
            const ReaderCapabilities& capabilities);

        /**
         * @brief Set wire protocol
         * 
         * @param wire Wire kind to use
         */
        void setWire(WireKind wire);

        /**
         * @brief Get current wire kind
         * 
         * @return WireKind Current wire protocol
         */
        WireKind getWireKind() const;

        /**
         * @brief Detect card
         * 
         * @return etl::expected<CardInfo, error::Error> Card info or error
         */
        etl::expected<CardInfo, error::Error> detectCard();

        /**
         * @brief Check if card is present
         * 
         * @return bool True if card is present
         */
        bool isCardPresent();

        /**
         * @brief Create a card session
         * 
         * @return etl::expected<CardSession*, error::Error> Pointer to session or error
         */
        etl::expected<CardSession*, error::Error> createSession();

        /**
         * @brief Get active session
         * 
         * @return etl::optional<CardSession*> Pointer to session if active
         */
        etl::optional<CardSession*> getActiveSession();

        /**
         * @brief Clear active session
         * 
         */
        void clearSession();

        /**
         * @brief Get maximum APDU size
         * 
         * @return size_t Maximum APDU size
         */
        size_t getMaxApduSize() const;

        /**
         * @brief Get reader capabilities
         * 
         * @return const ReaderCapabilities& Reader capabilities
         */
        const ReaderCapabilities& getCapabilities() const;

    private:
        ::IApduTransceiver& transceiver;
        ::ICardDetector& detector;
        ReaderCapabilities capabilities;

        NativeWire nativeWire;
        IsoWire isoWire;
        IWire* activeWire;
        WireKind activeWireKind;

        etl::optional<CardInfo> currentCardInfo;
        etl::optional<CardSession> activeSession;
    };

} // namespace nfc
