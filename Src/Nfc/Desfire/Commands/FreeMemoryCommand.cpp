/**
 * @file FreeMemoryCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire free memory command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/FreeMemoryCommand.h"
#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t FREE_MEMORY_COMMAND_CODE = 0x6E;
}

FreeMemoryCommand::FreeMemoryCommand()
    : stage(Stage::Initial)
    , freeMemoryBytes(0U)
    , rawPayload()
    , requestIv()
    , hasRequestIv(false)
{
}

etl::string_view FreeMemoryCommand::name() const
{
    return "FreeMemory";
}

etl::expected<DesfireRequest, error::Error> FreeMemoryCommand::buildRequest(const DesfireContext& context)
{
    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    requestIv.clear();
    hasRequestIv = false;
    if (context.authenticated)
    {
        etl::vector<uint8_t, 1> cmacMessage;
        cmacMessage.push_back(FREE_MEMORY_COMMAND_CODE);

        auto requestIvResult = SecureMessagingPolicy::derivePlainRequestIv(context, cmacMessage);
        if (requestIvResult)
        {
            const auto& derivedIv = requestIvResult.value();
            for (size_t i = 0; i < derivedIv.size(); ++i)
            {
                requestIv.push_back(derivedIv[i]);
            }
            hasRequestIv = true;
        }
    }

    DesfireRequest request;
    request.commandCode = FREE_MEMORY_COMMAND_CODE;
    request.data.clear();
    request.expectedResponseLength = 0U;
    return request;
}

etl::expected<DesfireResult, error::Error> FreeMemoryCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
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

    etl::vector<uint8_t, 16> candidate;
    for (size_t i = 1U; i < response.size(); ++i)
    {
        if (candidate.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }
        candidate.push_back(response[i]);
    }

    etl::vector<uint8_t, 3> payload;
    bool decoded = false;

    if (context.authenticated && hasRequestIv)
    {
        auto verifyResult = SecureMessagingPolicy::verifyAuthenticatedPlainPayloadAutoMacAndUpdateContextIv(
            context,
            candidate,
            result.statusCode,
            requestIv,
            3U);
        if (!verifyResult)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
        }

        for (size_t i = 0U; i < 3U; ++i)
        {
            payload.push_back(candidate[i]);
        }
        decoded = true;
    }

    if (!decoded)
    {
        size_t payloadLength = candidate.size();
        if (payloadLength != 3U)
        {
            if (context.authenticated && payloadLength > 3U)
            {
                if (payloadLength >= 11U && (payloadLength - 8U) == 3U)
                {
                    payloadLength -= 8U;
                }
                else if (payloadLength >= 7U && (payloadLength - 4U) == 3U)
                {
                    payloadLength -= 4U;
                }
            }
        }

        if (payloadLength != 3U)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
        }

        for (size_t i = 0U; i < 3U; ++i)
        {
            payload.push_back(candidate[i]);
        }
    }

    rawPayload.clear();
    result.data.clear();
    for (size_t i = 0U; i < payload.size(); ++i)
    {
        rawPayload.push_back(payload[i]);
        result.data.push_back(payload[i]);
    }

    freeMemoryBytes = parseLe24(rawPayload);
    stage = Stage::Complete;
    return result;
}

bool FreeMemoryCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void FreeMemoryCommand::reset()
{
    stage = Stage::Initial;
    freeMemoryBytes = 0U;
    rawPayload.clear();
    requestIv.clear();
    hasRequestIv = false;
}

uint32_t FreeMemoryCommand::getFreeMemoryBytes() const
{
    return freeMemoryBytes;
}

const etl::vector<uint8_t, 16>& FreeMemoryCommand::getRawPayload() const
{
    return rawPayload;
}

uint32_t FreeMemoryCommand::parseLe24(const etl::ivector<uint8_t>& payload)
{
    return static_cast<uint32_t>(payload[0]) |
           (static_cast<uint32_t>(payload[1]) << 8U) |
           (static_cast<uint32_t>(payload[2]) << 16U);
}
