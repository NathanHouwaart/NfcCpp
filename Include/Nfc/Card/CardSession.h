/**
 * @file CardSession.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Card session management
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/variant.h>
#include <etl/expected.h>
#include "CardInfo.h"
#include "Nfc/Desfire/DesfireCard.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Nfc/MifareClassic/MifareClassicContext.h"
#include "Nfc/Ultralight/UltralightContext.h"
#include "Nfc/MifareClassic/MifareClassicCard.h"
#include "Nfc/Ultralight/UltralightCard.h"
#include "Error/Error.h"
#include "Nfc/Apdu/IApduTransceiver.h"

namespace nfc
{

    /**
     * @brief Card session class
     * 
     * Manages an active card session with type-specific card and context
     */
    class CardSession
    {
    public:
        using CardVariant = etl::variant<etl::monostate, DesfireCard, MifareClassicCard, UltralightCard>;
        using ContextVariant = etl::variant<etl::monostate, DesfireContext, MifareClassicContext, UltralightContext>;

        /**
         * @brief Get card as specific type
         * 
         * @tparam T Card type
         * @return T* Pointer to card or nullptr
         */
        template<typename T>
        T* getCardAs();

        /**
         * @brief Get card as specific type (const)
         * 
         * @tparam T Card type
         * @return const T* Pointer to card or nullptr
         */
        template<typename T>
        const T* getCardAs() const;

        /**
         * @brief Get context as specific type
         * 
         * @tparam T Context type
         * @return T* Pointer to context or nullptr
         */
        template<typename T>
        T* getContextAs();

        /**
         * @brief Get context as specific type (const)
         * 
         * @tparam T Context type
         * @return const T* Pointer to context or nullptr
         */
        template<typename T>
        const T* getContextAs() const;

        /**
         * @brief Get DESFire card
         * 
         * @return DesfireCard* Pointer to DESFire card or nullptr
         */
        DesfireCard* getDesfireCard();

        /**
         * @brief Get MIFARE Classic card
         * 
         * @return MifareClassicCard* Pointer to MIFARE Classic card or nullptr
         */
        MifareClassicCard* getMifareClassicCard();

        /**
         * @brief Get Ultralight card
         * 
         * @return UltralightCard* Pointer to Ultralight card or nullptr
         */
        UltralightCard* getUltralightCard();

        /**
         * @brief Get DESFire context
         * 
         * @return DesfireContext* Pointer to DESFire context or nullptr
         */
        DesfireContext* getDesfireContext();

        /**
         * @brief Get MIFARE Classic context
         * 
         * @return MifareClassicContext* Pointer to MIFARE Classic context or nullptr
         */
        MifareClassicContext* getClassicContext();

        /**
         * @brief Get Ultralight context
         * 
         * @return UltralightContext* Pointer to Ultralight context or nullptr
         */
        UltralightContext* getUltralightContext();

        /**
         * @brief Reset the session
         * 
         */
        void reset();

        /**
         * @brief Get card type
         * 
         * @return CardType Card type
         */
        CardType getCardType() const;

        /**
         * @brief Get card info
         * 
         * @return const CardInfo& Card information
         */
        const CardInfo& getCardInfo() const;

        /**
         * @brief Create a card session
         * 
         * @param transceiver APDU transceiver
         * @param info Card information
         * @return etl::expected<CardSession, error::Error> Card session or error
         */
        static etl::expected<CardSession, error::Error> create(
            ::IApduTransceiver& transceiver,
            const CardInfo& info);

    private:
        /**
         * @brief Construct a new CardSession
         * 
         * @param info Card information
         */
        explicit CardSession(const CardInfo& info);

        CardInfo info;
        CardVariant card;
        ContextVariant context;
    };

} // namespace nfc
