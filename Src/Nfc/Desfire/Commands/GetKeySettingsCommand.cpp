/**
 * @file GetKeySettingsCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get key settings command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/GetKeySettingsCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t GET_KEY_SETTINGS_COMMAND_CODE = 0x45;
}

GetKeySettingsCommand::GetKeySettingsCommand()
    : stage(Stage::Initial)
    , keySettings1(0x00)
    , keySettings2(0x00)
    , keyCount(0x00)
    , keyTypeFlags(0x00)
    , rawPayload()
{
}

etl::string_view GetKeySettingsCommand::name() const
{
    return "GetKeySettings";
}

etl::expected<DesfireRequest, error::Error> GetKeySettingsCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    DesfireRequest request;
    request.commandCode = GET_KEY_SETTINGS_COMMAND_CODE;
    request.data.clear();
    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> GetKeySettingsCommand::parseResponse(
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

    if (rawPayload.size() < 2U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    keySettings1 = rawPayload[0];
    keySettings2 = rawPayload[1];
    keyCount = static_cast<uint8_t>(keySettings2 & 0x0FU);
    keyTypeFlags = static_cast<uint8_t>(keySettings2 & 0xC0U);

    stage = Stage::Complete;
    return result;
}

bool GetKeySettingsCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void GetKeySettingsCommand::reset()
{
    stage = Stage::Initial;
    keySettings1 = 0x00;
    keySettings2 = 0x00;
    keyCount = 0x00;
    keyTypeFlags = 0x00;
    rawPayload.clear();
}

uint8_t GetKeySettingsCommand::getKeySettings1() const
{
    return keySettings1;
}

uint8_t GetKeySettingsCommand::getKeySettings2() const
{
    return keySettings2;
}

uint8_t GetKeySettingsCommand::getKeyCount() const
{
    return keyCount;
}

uint8_t GetKeySettingsCommand::getKeyTypeFlags() const
{
    return keyTypeFlags;
}

const etl::vector<uint8_t, 32>& GetKeySettingsCommand::getRawPayload() const
{
    return rawPayload;
}

