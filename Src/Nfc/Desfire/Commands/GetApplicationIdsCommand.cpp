/**
 * @file GetApplicationIdsCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get application IDs command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/GetApplicationIdsCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t GET_APPLICATION_IDS_COMMAND_CODE = 0x6A;
    constexpr uint8_t ADDITIONAL_FRAME_COMMAND_CODE = 0xAF;
}

GetApplicationIdsCommand::GetApplicationIdsCommand()
    : stage(Stage::Initial)
    , rawPayload()
    , applicationIds()
{
}

etl::string_view GetApplicationIdsCommand::name() const
{
    return "GetApplicationIDs";
}

etl::expected<DesfireRequest, error::Error> GetApplicationIdsCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    DesfireRequest request;
    request.data.clear();
    request.expectedResponseLength = 0;

    switch (stage)
    {
        case Stage::Initial:
            request.commandCode = GET_APPLICATION_IDS_COMMAND_CODE;
            return request;

        case Stage::AdditionalFrame:
            request.commandCode = ADDITIONAL_FRAME_COMMAND_CODE;
            return request;

        case Stage::Complete:
        default:
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }
}

etl::expected<DesfireResult, error::Error> GetApplicationIdsCommand::parseResponse(
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

    const size_t frameDataOffset = 1U;
    size_t frameDataLength = response.size() - frameDataOffset;

    // In authenticated sessions the final response may carry a trailing MAC/CMAC.
    // Current secure pipe integration does not strip it, so trim here when needed.
    if (result.isSuccess() && context.authenticated)
    {
        const size_t combinedLength = rawPayload.size() + frameDataLength;
        if ((combinedLength % 3U) != 0U)
        {
            if (frameDataLength >= 8U && ((combinedLength - 8U) % 3U) == 0U)
            {
                frameDataLength -= 8U;
            }
            else if (frameDataLength >= 4U && ((combinedLength - 4U) % 3U) == 0U)
            {
                frameDataLength -= 4U;
            }
        }
    }

    for (size_t i = 0; i < frameDataLength; ++i)
    {
        if (result.data.full() || rawPayload.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }

        const uint8_t value = response[frameDataOffset + i];
        result.data.push_back(value);
        rawPayload.push_back(value);
    }

    if (result.isAdditionalFrame())
    {
        stage = Stage::AdditionalFrame;
        return result;
    }

    // Final frame received: payload must be whole AID triplets.
    if ((rawPayload.size() % 3U) != 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    applicationIds.clear();
    for (size_t offset = 0; offset < rawPayload.size(); offset += 3U)
    {
        if (applicationIds.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }

        etl::array<uint8_t, 3> aid = {rawPayload[offset], rawPayload[offset + 1], rawPayload[offset + 2]};
        applicationIds.push_back(aid);
    }

    stage = Stage::Complete;
    return result;
}

bool GetApplicationIdsCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void GetApplicationIdsCommand::reset()
{
    stage = Stage::Initial;
    rawPayload.clear();
    applicationIds.clear();
}

const etl::vector<etl::array<uint8_t, 3>, 84>& GetApplicationIdsCommand::getApplicationIds() const
{
    return applicationIds;
}

const etl::vector<uint8_t, 252>& GetApplicationIdsCommand::getRawPayload() const
{
    return rawPayload;
}
