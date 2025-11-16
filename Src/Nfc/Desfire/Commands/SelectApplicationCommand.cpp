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
    
    // Response format from wire: [Status][Data...]
    if (response.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }
    
    // Extract status code
    result.statusCode = response[0];
    
    // Extract data (if any)
    result.data.clear();
    for (size_t i = 1; i < response.size(); ++i)
    {
        result.data.push_back(response[i]);
    }
    
    // Only update context if successful
    if (result.isSuccess())
    {
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
    }
    
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

