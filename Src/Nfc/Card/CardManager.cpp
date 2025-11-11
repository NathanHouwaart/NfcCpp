/**
 * @file CardManager.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Card manager implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Card/CardManager.h"

namespace nfc
{
    CardManager::CardManager(
        IApduTransceiver& transceiverRef,
        ICardDetector& detectorRef,
        const ReaderCapabilities& caps)
        : transceiver(transceiverRef)
        , detector(detectorRef)
        , capabilities(caps)
        , nativeWire()
        , isoWire()
        , activeWire(&nativeWire)
        , activeWireKind(WireKind::Native)
    {
    }

    void CardManager::setWire(WireKind wire)
    {
        activeWireKind = wire;
        
        switch (wire)
        {
            case WireKind::Native:
                activeWire = &nativeWire;
                break;
            case WireKind::Iso:
                activeWire = &isoWire;
                break;
        }
    }

    WireKind CardManager::getWireKind() const
    {
        return activeWireKind;
    }

    etl::expected<CardInfo, error::Error> CardManager::detectCard()
    {
        // TODO: Implement card detection
        auto result = detector.detectCard();
        
        if (result.has_value())
        {
            currentCardInfo = result.value();
            return result.value();
        }
        
        return etl::unexpected(result.error());
    }

    bool CardManager::isCardPresent()
    {
        return detector.isCardPresent();
    }

    etl::expected<CardSession*, error::Error> CardManager::createSession()
    {
        // TODO: Implement session creation
        
        // Check if we have card info
        if (!currentCardInfo.has_value())
        {
            auto detectResult = detectCard();
            if (!detectResult.has_value())
            {
                return etl::unexpected(detectResult.error());
            }
        }

        // Create session
        auto sessionResult = CardSession::create(transceiver, currentCardInfo.value());
        
        if (!sessionResult.has_value())
        {
            return etl::unexpected(sessionResult.error());
        }

        activeSession = sessionResult.value();
        return &activeSession.value();
    }

    etl::optional<CardSession*> CardManager::getActiveSession()
    {
        if (activeSession.has_value())
        {
            return &activeSession.value();
        }
        return etl::nullopt;
    }

    void CardManager::clearSession()
    {
        activeSession.reset();
        currentCardInfo.reset();
    }

    size_t CardManager::getMaxApduSize() const
    {
        return capabilities.maxApduSize;
    }

    const ReaderCapabilities& CardManager::getCapabilities() const
    {
        return capabilities;
    }

} // namespace nfc
