/**
 * @file DeleteApplicationCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire delete application command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/DeleteApplicationCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t DELETE_APPLICATION_COMMAND_CODE = 0xDA;
}

DeleteApplicationCommand::DeleteApplicationCommand(const etl::array<uint8_t, 3>& aid)
    : aid(aid)
    , complete(false)
{
}

etl::string_view DeleteApplicationCommand::name() const
{
    return "DeleteApplication";
}

etl::expected<DesfireRequest, error::Error> DeleteApplicationCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    DesfireRequest request;
    request.commandCode = DELETE_APPLICATION_COMMAND_CODE;
    request.data.clear();

    // AID is transmitted LSB-first in DESFire APDUs
    request.data.push_back(aid[0]);
    request.data.push_back(aid[1]);
    request.data.push_back(aid[2]);

    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> DeleteApplicationCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    (void)context;

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

    complete = true;
    return result;
}

bool DeleteApplicationCommand::isComplete() const
{
    return complete;
}

void DeleteApplicationCommand::reset()
{
    complete = false;
}

