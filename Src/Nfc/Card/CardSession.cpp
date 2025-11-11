/**
 * @file CardSession.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Card session implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Card/CardSession.h"

namespace nfc
{
    CardSession::CardSession(const CardInfo& cardInfo)
        : info(cardInfo)
    {
    }

    template<typename T>
    T* CardSession::getCardAs()
    {
        // TODO: Implement type-safe variant access
        return nullptr;
    }

    template<typename T>
    const T* CardSession::getCardAs() const
    {
        // TODO: Implement type-safe variant access
        return nullptr;
    }

    template<typename T>
    T* CardSession::getContextAs()
    {
        // TODO: Implement type-safe variant access
        return nullptr;
    }

    template<typename T>
    const T* CardSession::getContextAs() const
    {
        // TODO: Implement type-safe variant access
        return nullptr;
    }

    DesfireCard* CardSession::getDesfireCard()
    {
        // TODO: Implement
        return nullptr;
    }

    MifareClassicCard* CardSession::getMifareClassicCard()
    {
        // TODO: Implement
        return nullptr;
    }

    UltralightCard* CardSession::getUltralightCard()
    {
        // TODO: Implement
        return nullptr;
    }

    DesfireContext* CardSession::getDesfireContext()
    {
        // TODO: Implement
        return nullptr;
    }

    MifareClassicContext* CardSession::getClassicContext()
    {
        // TODO: Implement
        return nullptr;
    }

    UltralightContext* CardSession::getUltralightContext()
    {
        // TODO: Implement
        return nullptr;
    }

    void CardSession::reset()
    {
        // TODO: Implement session reset
    }

    CardType CardSession::getCardType() const
    {
        return info.type;
    }

    const CardInfo& CardSession::getCardInfo() const
    {
        return info;
    }

    etl::expected<CardSession, error::Error> CardSession::create(
        IApduTransceiver& transceiver,
        const CardInfo& info)
    {
        // TODO: Implement session creation based on card type
        CardSession session(info);
        
        // Create appropriate card and context based on type
        switch (info.type)
        {
            case CardType::MifareDesfire:
                // TODO: Create DESFire card and context
                break;
            case CardType::MifareClassic:
                // TODO: Create MIFARE Classic card and context
                break;
            case CardType::MifareUltralight:
            case CardType::Ntag213_215_216:
                // TODO: Create Ultralight card and context
                break;
            default:
                return etl::unexpected(error::Error::fromCardManager(error::CardManagerError::UnsupportedCardType));
        }

        return session;
    }

} // namespace nfc
