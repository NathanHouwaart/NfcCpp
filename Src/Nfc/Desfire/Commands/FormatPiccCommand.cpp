/**
 * @file FormatPiccCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire format PICC command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/FormatPiccCommand.h"
#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t FORMAT_PICC_COMMAND_CODE = 0xFC;
}

FormatPiccCommand::FormatPiccCommand()
    : complete(false)
{
}

etl::string_view FormatPiccCommand::name() const
{
    return "FormatPICC";
}

etl::expected<DesfireRequest, error::Error> FormatPiccCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    DesfireRequest request;
    request.commandCode = FORMAT_PICC_COMMAND_CODE;
    request.data.clear();
    request.expectedResponseLength = 0U;
    return request;
}

etl::expected<DesfireResult, error::Error> FormatPiccCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    if (response.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    DesfireResult result;
    result.statusCode = response[0];
    result.data.clear();
    for (size_t i = 1; i < response.size(); ++i)
    {
        result.data.push_back(response[i]);
    }

    if (result.isSuccess())
    {
        etl::vector<uint8_t, 1> cmacMessage;
        cmacMessage.push_back(FORMAT_PICC_COMMAND_CODE);
        auto ivUpdateResult = SecureMessagingPolicy::updateContextIvForPlainCommand(
            context,
            cmacMessage,
            response,
            8U);
        if (!ivUpdateResult)
        {
            return etl::unexpected(ivUpdateResult.error());
        }
    }

    complete = true;
    return result;
}

bool FormatPiccCommand::isComplete() const
{
    return complete;
}

void FormatPiccCommand::reset()
{
    complete = false;
}
