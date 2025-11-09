/**
 * @file IHardwareBus.hpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Interface for hardware communication buses
 * @version 0.1
 * @date 2025-11-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

#include <etl/expected.h>
#include <etl/vector.h>

#include "Error/Error.h"
#include "BusPoperties.hpp"


namespace comms {

/**
 * @brief Interface for hardware communication buses
 */
class IHardwareBus {
public:

    // ==============================================================================
    // Initialization and Teardown
    // ==============================================================================
    
    virtual ~IHardwareBus() = default;

    /**
     * @brief Initializes the hardware bus
     * 
     * @return etl::expected<void, Error> void on success, Error of type HardwareError on failure
     */
    virtual etl::expected<void, error::Error> init() = 0;

    // ==============================================================================
    // Open and Close
    // ==============================================================================

    /**
     * @brief Opens the hardware bus

     * @return etl::expected<void, Error> void on success, Error of type HardwareError on failure
     */
    virtual etl::expected<void, error::Error> open() = 0;

    
    /**
     * @brief Closes the hardware bus
     * 
     */
    virtual void close() = 0;

    // ==============================================================================
    // Read and Write
    // ==============================================================================

    /**
     * @brief Writes data to the hardware bus
     * 
     * @param data Data to write
     * @return etl::expected<void, Error> void on success, Error of type HardwareError on failure
     */
    virtual etl::expected<void, error::Error> write(const etl::ivector<uint8_t>& data) = 0;

    /**
     * @brief Reads data from the hardware bus
     * 
     * @param buffer Buffer to store read data
     * @param length Number of bytes to read
     * @return etl::expected<size_t, Error> Number of bytes read on success, Error of type HardwareError on failure
     */
    virtual etl::expected<size_t, error::Error> read(etl::ivector<uint8_t>& buffer, size_t length) = 0;

    /**
     * @brief Flushes the hardware bus buffers
     * 
     * @return etl::expected<void, Error> void on success, Error of type HardwareError on failure
     */
    virtual etl::expected<void, error::Error> flush() = 0;

    /**
     * @brief Checks how many bytes are available to read
     * 
     * @return size_t Number of available bytes
     */
    virtual size_t available() const = 0;


    // ==============================================================================
    // Bus Properties
    // ==============================================================================

    /**
     * @brief Set the Property object
     * 
     * @param property The property to set
     * @param value The value to set
     * @return etl::expected<void, Error> void on success, Error of type HardwareError on failure
     */
    virtual etl::expected<void, error::Error> setProperty(BusProperty property, uint32_t value) = 0;

    /**
     * @brief Get the Property object
     * 
     * @param property The property to get`
     * @return etl::expected<uint32_t, Error> Value of the property on success, Error of type HardwareError on failure
     */
    virtual etl::expected<uint32_t, error::Error> getProperty(BusProperty property) const = 0;

    // ==============================================================================
    // Non virtual helper functions
    // ==============================================================================

    /**
     * @brief Checks if the bus is open
     * 
     * @return true Bus is open
     * @return false Bus is closed
     */
    bool isOpen() const
    {
        return openFlag;
    }

protected:

    /**
     * @brief Set the Open Flag object
     * 
     * @param flag The flag to set
     */
    void setIsOpen(bool flag)
    {
        openFlag = flag;
    }
    
private:

    bool openFlag = false;  // Indicates if the bus is open

};


} // namespace comms