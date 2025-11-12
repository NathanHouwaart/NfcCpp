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
        return etl::get_if<T>(&card);
    }

    template<typename T>
    const T* CardSession::getCardAs() const
    {
        return etl::get_if<T>(&card);
    }

    template<typename T>
    T* CardSession::getContextAs()
    {
        return etl::get_if<T>(&context);
    }

    template<typename T>
    const T* CardSession::getContextAs() const
    {
        return etl::get_if<T>(&context);
    }

    // Explicit template instantiations
    template DesfireCard* CardSession::getCardAs<DesfireCard>();
    template const DesfireCard* CardSession::getCardAs<DesfireCard>() const;
    template MifareClassicCard* CardSession::getCardAs<MifareClassicCard>();
    template const MifareClassicCard* CardSession::getCardAs<MifareClassicCard>() const;
    template UltralightCard* CardSession::getCardAs<UltralightCard>();
    template const UltralightCard* CardSession::getCardAs<UltralightCard>() const;

    template DesfireContext* CardSession::getContextAs<DesfireContext>();
    template const DesfireContext* CardSession::getContextAs<DesfireContext>() const;
    template MifareClassicContext* CardSession::getContextAs<MifareClassicContext>();
    template const MifareClassicContext* CardSession::getContextAs<MifareClassicContext>() const;
    template UltralightContext* CardSession::getContextAs<UltralightContext>();
    template const UltralightContext* CardSession::getContextAs<UltralightContext>() const;

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
        CardSession session(info);
        
        // Create appropriate card and context based on type
        switch (info.type)
        {
            case CardType::MifareDesfire:
                // Create DESFire card with the transceiver
                session.card.emplace<DesfireCard>(transceiver);
                // Context is created within DesfireCard
                break;
                
            case CardType::MifareClassic:
                // Create MIFARE Classic card (placeholder for now)
                session.card.emplace<MifareClassicCard>();
                break;
                
            case CardType::MifareUltralight:
            case CardType::Ntag213_215_216:
                // Create Ultralight card (placeholder for now)
                session.card.emplace<UltralightCard>();
                break;
                
            default:
                return etl::unexpected(error::Error::fromCardManager(error::CardManagerError::UnsupportedCardType));
        }

        return session;
    }

} // namespace nfc
