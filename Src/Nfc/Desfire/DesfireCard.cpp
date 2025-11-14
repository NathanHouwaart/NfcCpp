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
#include "Error/DesfireError.h"

using namespace nfc;

DesfireCard::DesfireCard(IApduTransceiver& transceiver)
    : transceiver(transceiver)
    , context()
    , plainPipe(nullptr)
    , macPipe(nullptr)
    , encPipe(nullptr)
{
    // TODO: Initialize pipes
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
    
    // Execute command with potential multi-frame responses
    while (!command.isComplete())
    {
        // Build request
        auto requestResult = command.buildRequest(context);
        if (!requestResult)
        {
            return etl::unexpected(requestResult.error());
        }
        
        DesfireRequest& request = requestResult.value();
        
        // Build DESFire APDU: 0x90 [CMD] 0x00 0x00 [Lc] [Data...] 0x00
        etl::vector<uint8_t, 261> apdu;
        apdu.push_back(0x90);  // CLA: DESFire proprietary
        apdu.push_back(request.commandCode);  // INS: command code
        apdu.push_back(0x00);  // P1
        apdu.push_back(0x00);  // P2
        
        if (!request.data.empty())
        {
            apdu.push_back(static_cast<uint8_t>(request.data.size()));  // Lc: data length
            for (size_t i = 0; i < request.data.size(); ++i)
            {
                apdu.push_back(request.data[i]);
            }
        }
        else
        {
            apdu.push_back(0x00);  // Lc = 0
        }
        
        apdu.push_back(0x00);  // Le: expect response
        
        // Transceive APDU
        auto apduResult = transceiver.transceive(apdu);
        if (!apduResult)
        {
            return etl::unexpected(apduResult.error());
        }
        
        ApduResponse& apduResponse = apduResult.value();
        
        // Check APDU status
        if (!apduResponse.isSuccess())
        {
            // Map DESFire status to error
            // SW1=0x91, SW2=DESFire status code
            if (apduResponse.sw1 == 0x91)
            {
                auto desfireStatus = static_cast<error::DesfireError>(apduResponse.sw2);
                return etl::unexpected(error::Error::fromDesfire(desfireStatus));
            }
            return etl::unexpected(error::Error::fromApdu(error::ApduError::Unknown));
        }
        
        // Parse response
        auto parseResult = command.parseResponse(apduResponse.data, context);
        if (!parseResult)
        {
            return etl::unexpected(parseResult.error());
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
    options.mode = mode;
    options.keyNo = keyNo;
    options.key = key;
    
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
