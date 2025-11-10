/**
 * @file Rc522Driver.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Placeholder implementation of RC522 driver
 * @version 0.1
 * @date 2025-11-10
 * 
 * @copyright Copyright (c) 2025
 * 
 * @note This is a placeholder - RC522 not implemented
 */

#include "Rc522/Rc522Driver.h"
#include "Utils/Logging.h"

Rc522Driver::Rc522Driver(comms::IHardwareBus& bus)
    : bus(bus) {
    LOG_WARN("Rc522Driver created - not implemented");
}

etl::expected<void, error::Error> Rc522Driver::init() {
    LOG_ERROR("RC522 driver initialization not implemented");
    return etl::unexpected(error::Error::fromRc522(error::Rc522Error::NotImplemented));
}
