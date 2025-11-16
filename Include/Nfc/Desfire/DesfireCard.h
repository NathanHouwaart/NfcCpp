/**
 * @file DesfireCard.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire card implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/array.h>
#include <etl/vector.h>
#include <etl/expected.h>
#include "DesfireContext.h"
#include "DesfireAuthMode.h"
#include "Error/Error.h"

namespace nfc
{
    // Forward declarations
    class ISecurePipe;
    class PlainPipe;
    class MacPipe;
    class EncPipe;
    class IDesfireCommand;
    struct DesfireRequest;
    struct DesfireResult;
    class IApduTransceiver;
    class IWire;

    /**
     * @brief DESFire card class
     * 
     * Manages DESFire card operations with different security pipes
     */
    class DesfireCard
    {
    public:
        /**
         * @brief Construct a new DesfireCard
         * 
         * @param transceiver APDU transceiver
         * @param wire Wire strategy for APDU framing (managed by CardManager)
         */
        DesfireCard(IApduTransceiver& transceiver, IWire& wire);

        // /**
        //  * @brief Get the DESFire context
        //  * 
        //  * @return DesfireContext& Reference to the context
        //  */
        // DesfireContext& getContext();

        // /**
        //  * @brief Get the DESFire context (const)
        //  * 
        //  * @return const DesfireContext& Reference to the context
        //  */
        // const DesfireContext& getContext() const;

        /**
         * @brief Execute a DESFire command
         * 
         * @param command Command to execute
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> executeCommand(IDesfireCommand& command);

        /**
         * @brief Select application
         * 
         * @param aid Application ID (3 bytes)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> selectApplication(const etl::array<uint8_t, 3>& aid);

        /**
         * @brief Authenticate with the card
         * 
         * @param keyNo Key number
         * @param key Key data
         * @param mode Authentication mode
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> authenticate(
            uint8_t keyNo,
            const etl::vector<uint8_t, 24>& key,
            DesfireAuthMode mode);

        /**
         * @brief Get real card UID
         * 
         * @return etl::expected<etl::vector<uint8_t, 10>, error::Error> UID or error
         */
        etl::expected<etl::vector<uint8_t, 10>, error::Error> getRealCardUid();

        /**
         * @brief Wrap a DESFire request using the appropriate pipe
         * 
         * @param request Request to wrap
         * @return etl::expected<etl::vector<uint8_t, 261>, error::Error> Wrapped data or error
         */
        etl::expected<etl::vector<uint8_t, 261>, error::Error> wrapRequest(const DesfireRequest& request);

        /**
         * @brief Unwrap a response using the appropriate pipe
         * 
         * @param response Response to unwrap
         * @return etl::expected<DesfireResult, error::Error> Unwrapped result or error
         */
        etl::expected<DesfireResult, error::Error> unwrapResponse(const etl::ivector<uint8_t>& response);

    private:
        IApduTransceiver& transceiver;
        DesfireContext context;
        IWire* wire;  // Wire strategy for APDU framing

        PlainPipe* plainPipe;
        MacPipe* macPipe;
        EncPipe* encPipe;
    };

} // namespace nfc
