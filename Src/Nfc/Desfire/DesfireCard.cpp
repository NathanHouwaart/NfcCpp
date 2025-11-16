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

#include "Nfc/Desfire/DesfireCard.h"
#include "Nfc/Apdu/IApduTransceiver.h"
#include "Nfc/Desfire/ISecurePipe.h"
#include "Nfc/Desfire/PlainPipe.h"
#include "Nfc/Desfire/MacPipe.h"
#include "Nfc/Desfire/EncPipe.h"
#include "Nfc/Desfire/IDesfireCommand.h"
#include "Nfc/Desfire/DesfireRequest.h"
#include "Nfc/Desfire/DesfireResult.h"
#include "Nfc/Desfire/Commands/SelectApplicationCommand.h"
#include "Nfc/Desfire/Commands/AuthenticateCommand.h"
#include "Nfc/Wire/IWire.h"
#include "Nfc/Wire/IsoWire.h"
#include "Error/DesfireError.h"

using namespace nfc;

DesfireCard::DesfireCard(IApduTransceiver& transceiver, IWire& wireRef)
    : transceiver(transceiver)
    , context()
    , wire(&wireRef)
    , plainPipe(nullptr)
    , macPipe(nullptr)
    , encPipe(nullptr)
{
    // TODO: Initialize pipes
    // Wire is now managed by CardManager and passed in
}

// DesfireContext& DesfireCard::getContext()
// {
//     return context;
// }

// const DesfireContext& DesfireCard::getContext() const
// {
//     return context;
// }

etl::expected<void, error::Error> DesfireCard::executeCommand(IDesfireCommand& command)
{
    // Reset command state
    command.reset();
    
    // Multi-stage loop for commands that require multiple frames
    while (!command.isComplete())
    {
        // 1. Build request (PDU format: [CMD][Data...])
        auto requestResult = command.buildRequest(context);
        if (!requestResult)
        {
            return etl::unexpected(requestResult.error());
        }
        
        DesfireRequest& request = requestResult.value();
        
        // 2. Build PDU (Protocol Data Unit)
        etl::vector<uint8_t, 256> pdu;
        pdu.push_back(request.commandCode);
        for (size_t i = 0; i < request.data.size(); ++i)
        {
            pdu.push_back(request.data[i]);
        }
        
        // 3. Wrap PDU into APDU using wire strategy
        etl::vector<uint8_t, 261> apdu = wire->wrap(pdu);
        
        // 4. Transceive APDU (adapter unwraps using configured wire)
        // Returns normalized PDU: [Status][Data...]
        auto pduResult = transceiver.transceive(apdu);
        if (!pduResult)
        {
            return etl::unexpected(pduResult.error());
        }
        
        etl::vector<uint8_t, 256>& unwrappedPdu = pduResult.value();
        
        // 7. Parse response (updates command state)
        auto parseResult = command.parseResponse(unwrappedPdu, context);
        if (!parseResult)
        {
            return etl::unexpected(parseResult.error());
        }
        
        DesfireResult& result = parseResult.value();
        
        // 8. Check for errors (unless it's an additional frame request)
        if (!result.isSuccess() && result.statusCode != 0xAF)
        {
            auto desfireStatus = static_cast<error::DesfireError>(result.statusCode);
            return etl::unexpected(error::Error::fromDesfire(desfireStatus));
        }
    }
    
    return {};
}

etl::expected<void, error::Error> DesfireCard::selectApplication(const etl::array<uint8_t, 3>& aid)
{
    SelectApplicationCommand command(aid);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::authenticate(
    uint8_t keyNo,
    const etl::vector<uint8_t, 24>& key,
    DesfireAuthMode mode)
{
    AuthenticateCommandOptions options;
    options.keyNo = keyNo;
    options.key = key;
    options.mode = mode;
    
    AuthenticateCommand command(options);
    return executeCommand(command);
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
