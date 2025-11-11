/**
 * @file DesfireCard.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire card implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Card/DesfireCard.h"
#include "Nfc/Apdu/IApduTransceiver.h"
#include "Nfc/Desfire/ISecurePipe.h"
#include "Nfc/Desfire/PlainPipe.h"
#include "Nfc/Desfire/MacPipe.h"
#include "Nfc/Desfire/EncPipe.h"
#include "Nfc/Desfire/IDesfireCommand.h"
#include "Nfc/Desfire/DesfireRequest.h"
#include "Nfc/Desfire/DesfireResult.h"

namespace nfc
{
    DesfireCard::DesfireCard(IApduTransceiver& transceiver)
        : transceiver(transceiver)
        , context()
        , plainPipe(nullptr)
        , macPipe(nullptr)
        , encPipe(nullptr)
    {
        // TODO: Initialize pipes
    }

    DesfireContext& DesfireCard::getContext()
    {
        return context;
    }

    const DesfireContext& DesfireCard::getContext() const
    {
        return context;
    }

    etl::expected<void, error::Error> DesfireCard::executeCommand(IDesfireCommand& command)
    {
        // TODO: Implement command execution
        return etl::unexpected(error::Error::fromHardware(error::HardwareError::NotSupported));
    }

    etl::expected<void, error::Error> DesfireCard::selectApplication(const etl::array<uint8_t, 3>& aid)
    {
        // TODO: Implement application selection
        return etl::unexpected(error::Error::fromHardware(error::HardwareError::NotSupported));
    }

    etl::expected<void, error::Error> DesfireCard::authenticate(
        uint8_t keyNo,
        const etl::vector<uint8_t, 24>& key,
        DesfireAuthMode mode)
    {
        // TODO: Implement authentication
        return etl::unexpected(error::Error::fromHardware(error::HardwareError::NotSupported));
    }

    etl::expected<etl::vector<uint8_t, 10>, error::Error> DesfireCard::getRealCardUid()
    {
        // TODO: Implement get real UID
        return etl::unexpected(error::Error::fromHardware(error::HardwareError::NotSupported));
    }

    etl::expected<etl::vector<uint8_t, 261>, error::Error> DesfireCard::wrapRequest(const DesfireRequest& request)
    {
        // TODO: Implement request wrapping based on current communication mode
        return etl::unexpected(error::Error::fromHardware(error::HardwareError::NotSupported));
    }

    etl::expected<DesfireResult, error::Error> DesfireCard::unwrapResponse(const etl::ivector<uint8_t>& response)
    {
        // TODO: Implement response unwrapping based on current communication mode
        return etl::unexpected(error::Error::fromHardware(error::HardwareError::NotSupported));
    }

} // namespace nfc
