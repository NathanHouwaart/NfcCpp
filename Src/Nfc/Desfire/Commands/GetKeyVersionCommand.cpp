/**
 * @file GetKeyVersionCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get key version command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/GetKeyVersionCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t GET_KEY_VERSION_COMMAND_CODE = 0x64;
}

GetKeyVersionCommand::GetKeyVersionCommand(uint8_t keyNo)
    : keyNo(keyNo)
    , stage(Stage::Initial)
    , keyVersion(0x00)
    , rawPayload()
{
}

etl::string_view GetKeyVersionCommand::name() const
{
    return "GetKeyVersion";
}

etl::expected<DesfireRequest, error::Error> GetKeyVersionCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    DesfireRequest request;
    request.commandCode = GET_KEY_VERSION_COMMAND_CODE;
    request.data.clear();
    request.data.push_back(keyNo);
    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> GetKeyVersionCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    (void)context;

    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (response.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    DesfireResult result;
    result.statusCode = response[0];
    result.data.clear();

    if (!result.isSuccess())
    {
        return etl::unexpected(error::Error::fromDesfire(static_cast<error::DesfireError>(result.statusCode)));
    }

    rawPayload.clear();
    for (size_t i = 1; i < response.size(); ++i)
    {
        if (result.data.full() || rawPayload.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }
        const uint8_t value = response[i];
        result.data.push_back(value);
        rawPayload.push_back(value);
    }

    if (rawPayload.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    keyVersion = rawPayload[0];
    stage = Stage::Complete;
    return result;
}

bool GetKeyVersionCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void GetKeyVersionCommand::reset()
{
    stage = Stage::Initial;
    keyVersion = 0x00;
    rawPayload.clear();
}

uint8_t GetKeyVersionCommand::getKeyVersion() const
{
    return keyVersion;
}

const etl::vector<uint8_t, 16>& GetKeyVersionCommand::getRawPayload() const
{
    return rawPayload;
}

