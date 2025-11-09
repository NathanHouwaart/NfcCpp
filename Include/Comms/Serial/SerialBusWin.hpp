/**
 * @file SerialBusWin.hpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Windows implementation of serial communication bus
 * @version 0.1
 * @date 2025-11-03
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <Windows.h>
#include <stdint.h>

#include <etl/string.h>
#include <etl/expected.h>

#include "ISerialBus.hpp"

namespace comms
{
    namespace serial
    {

        class SerialBusWin : public ISerialBus
        {
        public:
            // ==============================================================================
            // Initialization and Teardown
            // ==============================================================================

            /**
             * @brief Construct a new Serial Communication object
             *
             */
            SerialBusWin(const etl::string<256> &portname, uint32_t baudrate);

            /**
             * @brief Destroy the Serial Bus Win object
             *
             */
            ~SerialBusWin() override;

            /**
             * @brief Initializes the serial bus
             *
             * @return etl::expected<void, Error> void on success, Error on failure
             */
            etl::expected<void, Error> init() override;

            // ==============================================================================
            // Open and Close
            // ==============================================================================

            /**
             * @brief Opens the serial bus
             *
             * @return etl::expected<void, Error> void on success, Error on failure
             */
            etl::expected<void, Error> open() override;

            /**
             * @brief Closes the serial bus
             *
             */
            void close() override;

            // ==============================================================================
            // Read and Write
            // ==============================================================================

            /**
             * @brief Writes data to the hardware bus
             *
             * @param data Data to write
             * @return etl::expected<void, Error> void on success, Error on failure
             */
            etl::expected<void, Error> write(const etl::ivector<uint8_t> &data) override;

            /**
             * @brief Reads data from the hardware bus
             *
             * @param buffer Buffer to store read data
             * @param length Number of bytes to read
             * @return etl::expected<size_t, Error> Number of bytes read on success, Error on failure
             */
            etl::expected<size_t, Error> read(etl::ivector<uint8_t> &buffer, size_t length) override;

            /**
             * @brief Flushes the hardware bus buffers
             *
             * @return etl::expected<void, Error> void on success, Error on failure
             */
            etl::expected<void, Error> flush() override;

            /**
             * @brief Checks how many bytes are available to read
             *
             * @return size_t Number of available bytes
             */
            size_t available() const override;

            // ==============================================================================
            // Bus-specific Properties
            // ==============================================================================

            /**
             * @brief Set the Baud Rate
             *
             * @param baudrate The baud rate to set
             * @return etl::expected<void, Error> void on success, Error on failure
             */
            etl::expected<void, Error> setBaudRate(uint32_t baudrate) override;

            /**
             * @brief Set the Parity
             *
             * @param parity The parity to set
             * @return etl::expected<void, Error> void on success, Error on failure
             */
            etl::expected<void, Error> setParity(Parity parity) override;

            /**
             * @brief Set the Stop Bits
             *
             * @param stopBits The stop bits to set
             * @return etl::expected<void, Error>
             */
            etl::expected<void, Error> setStopBits(StopBits stopBits) override;

            /**
             * @brief Set the Flow Control
             *
             * @param flowControl The flow control to set
             * @return etl::expected<void, Error> void on success, Error on failure
             */
            etl::expected<void, Error> setFlowControl(FlowControl flowControl) override;

            /**
             * @brief Set timeout for read and write operations
             * 
             * @param timeoutMs Timeout in milliseconds
             * @return etl::expected<void, Error> void on success, Error on failure
             */
            etl::expected<void, Error> setTimeout(uint32_t timeoutMs) override;


            // ==============================================================================
            // Bus Properties
            // ==============================================================================

            /**
             * @brief Set the Property object
             *
             * @param property The property to set
             * @param value The value to set
             * @return etl::expected<void, Error> void on success, Error on failure
             */
            etl::expected<void, Error> setProperty(BusProperty property, uint32_t value) override;

            /**
             * @brief Get the Property object
             *
             * @param property The property to get
             * @return etl::expected<uint32_t, Error> Value of the property on success, Error on failure
             */
            etl::expected<uint32_t, Error> getProperty(BusProperty property) const override;

        private:
            HANDLE serialHandler;
            DCB dcbHandler;
            COMMTIMEOUTS serialTimeouts;

            etl::string<256> portName;
            uint32_t baudRate;
        };

    } // namespace serial

} // namespace comms
