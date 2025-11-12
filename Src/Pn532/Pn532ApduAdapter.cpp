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
    
    // First, configure SAM to Normal mode (required before card operations)
    auto samResult = driver.setSamConfiguration(0x01);  // 0x01 = Normal mode
    if (!samResult)
    {
        LOG_ERROR("Failed to configure SAM");
        return etl::unexpected(samResult.error());
    }
    
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

    LOG_INFO("Card detected successfully");
    
    // TODO: Extract card info from detected targets
    return {};
}

bool Pn532ApduAdapter::isCardPresent() {
    LOG_INFO("Checking card presence");
    
    // TODO: Implement quick card presence check
    // This could use a simplified detection or cached state
    
    // For now, return false
    LOG_WARN("Card presence check not yet implemented");
    return false;
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
