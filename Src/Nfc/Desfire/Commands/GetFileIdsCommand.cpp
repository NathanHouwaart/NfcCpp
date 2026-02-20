/**
 * @file GetFileIdsCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get file IDs command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/GetFileIdsCommand.h"
#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t GET_FILE_IDS_COMMAND_CODE = 0x6F;
    constexpr uint8_t MAX_FILE_NO = 0x1F;

    bool isPlausibleFileIdPayload(const etl::ivector<uint8_t>& payload, size_t length)
    {
        if (length > 32U || length > payload.size())
        {
            return false;
        }

        for (size_t i = 0; i < length; ++i)
        {
            if (payload[i] > MAX_FILE_NO)
            {
                return false;
            }
        }

        return true;
    }
}

GetFileIdsCommand::GetFileIdsCommand()
    : stage(Stage::Initial)
    , rawPayload()
    , fileIds()
    , requestIv()
    , hasRequestIv(false)
{
}

etl::string_view GetFileIdsCommand::name() const
{
    return "GetFileIDs";
}

etl::expected<DesfireRequest, error::Error> GetFileIdsCommand::buildRequest(const DesfireContext& context)
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
        cmacMessage.push_back(GET_FILE_IDS_COMMAND_CODE);

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

    DesfireRequest request;
    request.commandCode = GET_FILE_IDS_COMMAND_CODE;
    request.data.clear();
    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> GetFileIdsCommand::parseResponse(
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

    rawPayload.clear();
    for (size_t i = 1; i < response.size(); ++i)
    {
        if (rawPayload.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }
        rawPayload.push_back(response[i]);
    }

    size_t dataLength = rawPayload.size();
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
            if (!isPlausibleFileIdPayload(rawPayload, candidateLength))
            {
                continue;
            }

            auto verifyResult = SecureMessagingPolicy::verifyAuthenticatedPlainPayloadAutoMacAndUpdateContextIv(
                context,
                rawPayload,
                result.statusCode,
                requestIv,
                candidateLength);
            if (verifyResult)
            {
                dataLength = candidateLength;
                verified = true;
                break;
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
        if (!isPlausibleFileIdPayload(rawPayload, dataLength))
        {
            if (dataLength >= 8U && isPlausibleFileIdPayload(rawPayload, dataLength - 8U))
            {
                dataLength -= 8U;
            }
            else if (dataLength >= 4U && isPlausibleFileIdPayload(rawPayload, dataLength - 4U))
            {
                dataLength -= 4U;
            }
            else
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }
        }
    }
    else if (!isPlausibleFileIdPayload(rawPayload, dataLength))
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    fileIds.clear();
    for (size_t i = 0; i < dataLength; ++i)
    {
        if (result.data.full() || fileIds.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }
        const uint8_t value = rawPayload[i];
        result.data.push_back(value);
        fileIds.push_back(value);
    }

    stage = Stage::Complete;
    return result;
}

bool GetFileIdsCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void GetFileIdsCommand::reset()
{
    stage = Stage::Initial;
    rawPayload.clear();
    fileIds.clear();
    requestIv.clear();
    hasRequestIv = false;
}

const etl::vector<uint8_t, 32>& GetFileIdsCommand::getFileIds() const
{
    return fileIds;
}

const etl::vector<uint8_t, 40>& GetFileIdsCommand::getRawPayload() const
{
    return rawPayload;
}
