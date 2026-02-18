/**
 * @file GetVersionCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get version command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/GetVersionCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t GET_VERSION_COMMAND_CODE = 0x60;
    constexpr uint8_t ADDITIONAL_FRAME_COMMAND_CODE = 0xAF;
}

GetVersionCommand::GetVersionCommand()
    : stage(Stage::Initial)
    , versionData()
{
}

etl::string_view GetVersionCommand::name() const
{
    return "GetVersion";
}

etl::expected<DesfireRequest, error::Error> GetVersionCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    DesfireRequest request;
    request.data.clear();
    request.expectedResponseLength = 0;

    switch (stage)
    {
        case Stage::Initial:
            request.commandCode = GET_VERSION_COMMAND_CODE;
            return request;

        case Stage::AdditionalFrame:
            request.commandCode = ADDITIONAL_FRAME_COMMAND_CODE;
            return request;

        case Stage::Complete:
        default:
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }
}

etl::expected<DesfireResult, error::Error> GetVersionCommand::parseResponse(
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

    if (!result.isSuccess() && !result.isAdditionalFrame())
    {
        return etl::unexpected(error::Error::fromDesfire(static_cast<error::DesfireError>(result.statusCode)));
    }

    for (size_t i = 1; i < response.size(); ++i)
    {
        if (result.data.full() || versionData.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }

        result.data.push_back(response[i]);
        versionData.push_back(response[i]);
    }

    if (result.isAdditionalFrame())
    {
        stage = Stage::AdditionalFrame;
    }
    else
    {
        stage = Stage::Complete;
    }

    return result;
}

bool GetVersionCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void GetVersionCommand::reset()
{
    stage = Stage::Initial;
    versionData.clear();
}

const etl::vector<uint8_t, 96>& GetVersionCommand::getVersionData() const
{
    return versionData;
}

