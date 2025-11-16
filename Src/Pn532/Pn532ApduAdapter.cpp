/**
 * @file Pn532ApduAdapter.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Implementation of PN532 APDU adapter
 * @version 0.1
 * @date 2025-11-10
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "Pn532/Pn532ApduAdapter.h"
#include "Pn532/Commands/InListPassiveTarget.h"
#include "Pn532/Commands/InDataExchange.h"
#include "Pn532/Pn532Driver.h"
#include "Nfc/Wire/IWire.h"
#include "Utils/Logging.h"

using namespace error;
using namespace nfc;

namespace pn532
{

    Pn532ApduAdapter::Pn532ApduAdapter(Pn532Driver &driver)
        : driver(driver), activeWire(nullptr)
    {
        LOG_INFO("Pn532ApduAdapter initialized");
    }

    void Pn532ApduAdapter::setWire(IWire& wire)
    {
        activeWire = &wire;
        LOG_INFO("Wire protocol configured for adapter");
    }

    etl::expected<etl::vector<uint8_t, buffer::APDU_DATA_MAX>, error::Error> Pn532ApduAdapter::transceive(
        const etl::ivector<uint8_t> &apdu)
    {
        if (!activeWire)
        {
            LOG_ERROR("Wire not configured - call setWire() first");
            return etl::unexpected(error::Error::fromCardManager(error::CardManagerError::NoCardPresent));
        }

        LOG_INFO("Transmitting APDU command, length: %zu", apdu.size());
        LOG_HEX("DEBUG", "APDU TX", apdu.data(), apdu.size());

        // Prepare InDataExchange command with APDU payload
        InDataExchangeOptions opts;
        opts.targetNumber = 1;  // Target from last detectCard()
        opts.responseTimeoutMs = 5000;  // 5 second timeout
        
        // Copy APDU into payload
        opts.payload.clear();
        for (const uint8_t byte : apdu)
        {
            opts.payload.push_back(byte);
        }

        InDataExchange cmd(opts);
        auto result = driver.executeCommand(cmd);

        if (!result)
        {
            LOG_ERROR("InDataExchange failed");
            return etl::unexpected(result.error());
        }

        // Check if the exchange was successful at PN532 level
        if (!cmd.isSuccess())
        {
            LOG_ERROR("InDataExchange status error: 0x%02X", cmd.getStatusByte());
            return etl::unexpected(error::Error::fromPn532(cmd.getStatus()));
        }

        // Get raw card response and unwrap using configured wire protocol
        const auto& responseData = cmd.getResponseData();
        LOG_HEX("DEBUG", "Card RX (raw)", responseData.data(), responseData.size());
        
        // Wire unwraps protocol-specific framing to normalized PDU: [Status][Data...]
        auto pduResult = activeWire->unwrap(responseData);
        if (!pduResult)
        {
            LOG_ERROR("Wire unwrap failed");
            return etl::unexpected(pduResult.error());
        }
        
        const auto& pdu = pduResult.value();
        LOG_HEX("DEBUG", "PDU (unwrapped)", pdu.data(), pdu.size());
        
        return pdu;
    }

    etl::expected<CardInfo, error::Error> Pn532ApduAdapter::detectCard()
    {
        LOG_INFO("Detecting card presence");

        // Now detect the card with appropriate timeout
        auto cmd = InListPassiveTarget(
            InListPassiveTargetOptions{
                .maxTargets = 1,
                .target = CardTargetType::TypeA_106kbps,
                .responseTimeoutMs = 5000 // 5 second timeout for card detection
            });

        auto result = driver.executeCommand(cmd);

        if (!result)
        {
            LOG_WARN("Card detection failed");
            return etl::unexpected(result.error());
        }

        // please fill in detected card info extraction here
        const auto &detectedTargets = cmd.getDetectedTargets();
        if (!detectedTargets.empty())
        {
            const auto &target = detectedTargets.front();

            // print hex of UID
            LOG_HEX("INFO", "Detected card UID", target.uid.data(), target.uid.size());

            // Create CardInfo with full information including ATS
            CardInfo cardInfo;
            cardInfo.uid = target.uid;
            cardInfo.atqa = target.atqa;
            cardInfo.sak = target.sak;
            cardInfo.ats = target.ats;
            cardInfo.detectType(); // Detect card type based on ATQA/SAK

            return cardInfo;
        }

        LOG_WARN("No card detected");

        return {};
    }

    bool Pn532ApduAdapter::isCardPresent()
    {
        LOG_INFO("Checking if card is present");

        // Quick check: Send InDataExchange with minimal data to target 1
        // If the command succeeds, card is present
        // If it fails (timeout, card disappeared, etc.), card is not present
        InDataExchangeOptions opts;
        opts.targetNumber = 1;        // Target from last detectCard()
        opts.payload = {0x00};        // Minimal payload
        opts.responseTimeoutMs = 500; // Quick timeout (500ms)

        InDataExchange cmd(opts);
        auto result = driver.executeCommand(cmd);

        // Command success = card present and responding
        // Command failure = card not present or not responding
        if (result.has_value())
        {
            LOG_INFO("Card present (successful exchange)");
            return true;
        }
        else
        {
            LOG_INFO("Card not present");
            return false;
        }
    }

} // namespace pn532