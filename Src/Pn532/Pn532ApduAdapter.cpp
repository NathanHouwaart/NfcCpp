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
#include "Utils/Logging.h"

using namespace pn532;
using namespace error;

Pn532ApduAdapter::Pn532ApduAdapter(pn532::Pn532Driver& driver)
    : driver(driver) {
    LOG_INFO("Pn532ApduAdapter initialized");
}

etl::expected<ApduResponse, error::Error> Pn532ApduAdapter::transceive(
    const etl::ivector<uint8_t>& apdu) {
    
    LOG_INFO("Transmitting APDU command, length: %zu", apdu.size());
    
    // TODO: Implement actual APDU transmission via PN532
    // This would typically involve:
    // 1. Selecting the card/application
    // 2. Sending the APDU command
    // 3. Receiving the response
    // 4. Parsing the response
    
    // For now, return a placeholder error
    LOG_WARN("APDU transceive not yet implemented");
    return etl::unexpected(error::Error::fromPn532(error::Pn532Error::NotImplemented));
}

etl::expected<CardInfo, error::Error> Pn532ApduAdapter::detectCard() {
    LOG_INFO("Detecting card presence");
    
    // Now detect the card with appropriate timeout
    auto cmd = InListPassiveTarget(
        InListPassiveTargetOptions{
            .maxTargets = 1,
            .target = CardTargetType::TypeA_106kbps,
            .responseTimeoutMs = 5000  // 5 second timeout for card detection
        }
    );

    auto result = driver.executeCommand(cmd);

    if (!result) {
        LOG_WARN("Card detection failed");
        return etl::unexpected(result.error());
    }

    // please fill in detected card info extraction here
    const auto& detectedTargets = cmd.getDetectedTargets();
    if (!detectedTargets.empty()) {
        const auto& target = detectedTargets.front();

        // print hex of UID
        LOG_HEX("INFO", "Detected card UID", target.uid.data(), target.uid.size());
        
        // Create CardInfo with full information including ATS
        CardInfo cardInfo;
        cardInfo.uid = target.uid;
        cardInfo.atqa = target.atqa;
        cardInfo.sak = target.sak;
        cardInfo.ats = target.ats;
        cardInfo.detectType();  // Detect card type based on ATQA/SAK
        
        return cardInfo;
    }

    LOG_WARN("No card detected");
    
    return {};
}

bool Pn532ApduAdapter::isCardPresent() {
    LOG_INFO("Checking if card is present");

    // Quick check: Send InDataExchange with minimal data to target 1
    // If the command succeeds, card is present
    // If it fails (timeout, card disappeared, etc.), card is not present
    InDataExchangeOptions opts;
    opts.targetNumber = 1;  // Target from last detectCard()
    opts.payload = {0x00};  // Minimal payload
    opts.responseTimeoutMs = 500;  // Quick timeout (500ms)
    
    InDataExchange cmd(opts);
    auto result = driver.executeCommand(cmd);
    
    // Command success = card present and responding
    // Command failure = card not present or not responding
    if (result.has_value()) {
        LOG_INFO("Card present (successful exchange)");
        return true;
    }
    else {
        LOG_INFO("Card not present");
        return false;
    }
}

etl::expected<ApduResponse, error::Error> Pn532ApduAdapter::parseApduResponse(
    const etl::ivector<uint8_t>& raw) {
    
    // APDU responses must be at least 2 bytes (SW1 and SW2)
    if (raw.size() < 2) {
        LOG_ERROR("Invalid APDU response: too short (size: %zu)", raw.size());
        return etl::unexpected(error::Error::fromApdu(error::ApduError::Unknown));
    }
    
    ApduResponse response;
    
    // Extract status words (last 2 bytes)
    response.sw1 = raw[raw.size() - 2];
    response.sw2 = raw[raw.size() - 1];
    
    // Extract data (everything except last 2 bytes)
    if (raw.size() > 2) {
        response.data.assign(raw.begin(), raw.begin() + (raw.size() - 2));
    }
    
    LOG_INFO("Parsed APDU response: SW1=0x%02X, SW2=0x%02X, data_length=%zu",
             response.sw1, response.sw2, response.data.size());
    
    return response;
}
