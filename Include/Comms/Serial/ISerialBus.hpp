/**
 * @file ISerialBus.hpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Interface for serial communication buses
 * @version 0.1
 * @date 2025-11-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

#include "../IHardwareBus.hpp"


namespace comms {

    namespace serial {

        using namespace error;
        
        /**
         * @brief Parity options for serial communication
         */
        enum class Parity : uint8_t {
            None,
            Even,
            Odd
        };

        /**
         * @brief Stop bits options for serial communication
         */
        enum class StopBits : uint8_t {
            One,
            Two
        };

        /**
         * @brief Flow control options for serial communication
         */
        enum class FlowControl : uint8_t {
            None,
            Hardware,
            Software
        };

        /**
         * @brief Interface for serial communication buses
         */
        class ISerialBus : public IHardwareBus {
        public:
            virtual ~ISerialBus() = default;

            // ==============================================================================
            // Serial-specific Properties
            // ==============================================================================

            /**
             * @brief Set the Baud Rate
             * 
             * @param baudrate The baud rate to set
             * @return etl::expected<void, Error> void on success, Error of type HardwareError of type HardwareError onfailure
             */
            virtual etl::expected<void, Error> setBaudRate(uint32_t baudrate) = 0;

            /**
             * @brief Set the Parity object
             * 
             * @param parity The parity to set
             * @return etl::expected<void, Error> void on success, Error of type HardwareError onfailure
             */
            virtual etl::expected<void, Error> setParity(Parity parity) = 0;

            /**
             * @brief Set the Stop Bits object
             * 
             * @param stopBits The stop bits to set
             * @return etl::expected<void, Error> 
             */
            virtual etl::expected<void, Error> setStopBits(StopBits stopBits) = 0;

            /**
             * @brief Set the Flow Control object
             * 
             * @param flowControl The flow control to set
             * @return etl::expected<void, Error> void on success, Error of type HardwareError onfailure
             */
            virtual etl::expected<void, Error> setFlowControl(FlowControl flowControl) = 0;

            /**
             * @brief Set timeout for read and write operations
             * 
             */
            virtual etl::expected<void, Error> setTimeout(uint32_t timeout_ms) = 0;
        };

    }

} // namespace comms