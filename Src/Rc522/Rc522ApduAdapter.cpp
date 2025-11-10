/**
 * @file Rc522ApduAdapter.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Placeholder implementation of RC522 APDU adapter
 * @version 0.1
 * @date 2025-11-10
 * 
 * @copyright Copyright (c) 2025
 * 
 * @note This is a placeholder - RC522 driver not implemented
 */

#include "Rc522/Rc522ApduAdapter.h"
#include "Rc522/Rc522Driver.h"
#include "Utils/Logging.h"

Rc522ApduAdapter::Rc522ApduAdapter(Rc522Driver& driver)
    : driver(driver) {
    LOG_WARN("Rc522ApduAdapter created - RC522 not implemented");
}

etl::expected<ApduResponse, error::Error> Rc522ApduAdapter::transceive(
    const etl::ivector<uint8_t>& apdu) {
    
    LOG_ERROR("RC522 APDU transceive not implemented");
    return etl::unexpected(error::Error::fromRc522(error::Rc522Error::NotImplemented));
}

etl::expected<CardInfo, error::Error> Rc522ApduAdapter::detectCard() {
    LOG_ERROR("RC522 card detection not implemented");
    return etl::unexpected(error::Error::fromRc522(error::Rc522Error::NotImplemented));
}

bool Rc522ApduAdapter::isCardPresent() {
    LOG_ERROR("RC522 card presence check not implemented");
    return false;
}
