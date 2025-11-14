/**
 * @file SelectApplicationCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Select application command implementation
 * @version 0.1
 * @date 2025-11-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Desfire/Commands/SelectApplicationCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

SelectApplicationCommand::SelectApplicationCommand(const etl::array<uint8_t, 3>& aid)
    : aid(aid)
    , complete(false)
{
}

etl::string_view SelectApplicationCommand::name() const
{
    return "SelectApplication";
}

etl::expected<DesfireRequest, error::Error> SelectApplicationCommand::buildRequest(const DesfireContext& context)
{
    DesfireRequest request;
    
    // SelectApplication command code is 0x5A
    request.commandCode = 0x5A;
    
    // Payload is the 3-byte AID (LSB first)
    request.data.clear();
    for (size_t i = 0; i < 3; ++i)
    {
        request.data.push_back(aid[i]);
    }
    
    // Expect only status byte in response
    request.expectedResponseLength = 0;
    
    return request;
}

etl::expected<DesfireResult, error::Error> SelectApplicationCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    DesfireResult result;
    
    // Response should be empty (only status code is sent separately)
    // Status 0x00 = success, application selected
    result.statusCode = 0x00;
    result.data.clear();
    
    // Update context with selected AID
    context.selectedAid.clear();
    for (size_t i = 0; i < 3; ++i)
    {
        context.selectedAid.push_back(aid[i]);
    }
    
    // Clear authentication state when changing applications
    context.authenticated = false;
    context.commMode = CommMode::Plain;
    context.keyNo = 0;
    context.sessionKeyEnc.clear();
    context.sessionKeyMac.clear();
    context.iv.clear();
    context.iv.resize(8, 0);
    
    complete = true;
    
    return result;
}

bool SelectApplicationCommand::isComplete() const
{
    return complete;
}

void SelectApplicationCommand::reset()
{
    complete = false;
}

