/**
 * @file Rc522Driver.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Placeholder for RC522 NFC reader driver
 * @version 0.1
 * @date 2025-11-10
 * 
 * @copyright Copyright (c) 2025
 * 
 * @note This is a placeholder to demonstrate system extensibility
 *       RC522 implementation is not included in this version
 */

#pragma once

#include "Comms/IHardwareBus.hpp"
#include "Error/Error.h"

#include <etl/expected.h>

/**
 * @brief Placeholder driver for RC522 NFC reader
 * 
 * This class serves as a placeholder to show how the system can be extended
 * to support different NFC reader hardware beyond the PN532.
 * 
 * The RC522 is a popular low-cost NFC reader IC that operates at 13.56 MHz.
 * 
 * @note To implement RC522 support:
 *       - Define RC522 command set
 *       - Implement register operations
 *       - Add card detection and communication
 *       - Implement MIFARE Classic operations
 *       - Add error handling specific to RC522
 */
class Rc522Driver {
public:
    /**
     * @brief Construct a new Rc522Driver
     * @param bus Reference to the hardware communication bus
     */
    explicit Rc522Driver(comms::IHardwareBus& bus);
    
    /**
     * @brief Destroy the Rc522Driver
     */
    ~Rc522Driver() = default;

    /**
     * @brief Initialize the RC522 reader
     * @return Expected void on success, Error on failure
     * @note Not implemented - placeholder only
     */
    etl::expected<void, error::Error> init();
    
    // TODO: Add RC522-specific methods:
    // - Card detection
    // - Authentication
    // - Block read/write
    // - Register operations
    // - Antenna control
    // - etc.

private:
    comms::IHardwareBus& bus;  ///< Hardware communication bus
};
