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
#include "Nfc/Desfire/SecureMessagingPolicy.h"
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
    , requestIv()
    , hasRequestIv(false)
{
}

etl::string_view GetApplicationIdsCommand::name() const
{
    return "GetApplicationIDs";
}

etl::expected<DesfireRequest, error::Error> GetApplicationIdsCommand::buildRequest(const DesfireContext& context)
{
    DesfireRequest request;
    request.data.clear();
    request.expectedResponseLength = 0;

    switch (stage)
    {
        case Stage::Initial:
            requestIv.clear();
            hasRequestIv = false;
            if (context.authenticated)
            {
                etl::vector<uint8_t, 1> cmacMessage;
                cmacMessage.push_back(GET_APPLICATION_IDS_COMMAND_CODE);

                auto requestIvResult = SecureMessagingPolicy::derivePlainRequestIv(context, cmacMessage, true);
                if (requestIvResult)
                {
                    const auto& derivedIv = requestIvResult.value();
                    for (size_t i = 0U; i < derivedIv.size(); ++i)
                    {
                        requestIv.push_back(derivedIv[i]);
                    }
                    hasRequestIv = true;
                }
            }

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
    const size_t frameDataLength = response.size() - frameDataOffset;
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

    size_t payloadLength = rawPayload.size();
    if (context.authenticated && hasRequestIv)
    {
        bool verified = false;
        constexpr size_t macCandidates[3] = {8U, 4U, 0U};
        for (size_t macIndex = 0U; macIndex < 3U; ++macIndex)
        {
            const size_t macLength = macCandidates[macIndex];
            if (rawPayload.size() < macLength)
            {
                continue;
            }

            const size_t candidateLength = rawPayload.size() - macLength;
            if ((candidateLength % 3U) == 0U)
            {
                auto verifyResult = SecureMessagingPolicy::verifyAuthenticatedPlainPayloadAutoMacAndUpdateContextIv(
                    context,
                    rawPayload,
                    result.statusCode,
                    requestIv,
                    candidateLength);
                if (verifyResult)
                {
                    payloadLength = candidateLength;
                    verified = true;
                    break;
                }
            }
        }

        if (!verified)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
        }
    }
    else if (context.authenticated)
    {
        // Compatibility fallback for legacy-auth sessions where authenticated-plain IV/CMAC verification is unavailable.
        if ((payloadLength % 3U) != 0U)
        {
            if (payloadLength >= 8U && ((payloadLength - 8U) % 3U) == 0U)
            {
                payloadLength -= 8U;
            }
            else if (payloadLength >= 4U && ((payloadLength - 4U) % 3U) == 0U)
            {
                payloadLength -= 4U;
            }
        }
    }

    // Final payload must be whole AID triplets.
    if ((payloadLength % 3U) != 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    applicationIds.clear();
    for (size_t offset = 0; offset < payloadLength; offset += 3U)
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
    requestIv.clear();
    hasRequestIv = false;
}

const etl::vector<etl::array<uint8_t, 3>, 84>& GetApplicationIdsCommand::getApplicationIds() const
{
    return applicationIds;
}

const etl::vector<uint8_t, 252>& GetApplicationIdsCommand::getRawPayload() const
{
    return rawPayload;
}
