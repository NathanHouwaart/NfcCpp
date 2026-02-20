/**
 * @file DeleteFileCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire delete file command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/DeleteFileCommand.h"
#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t DELETE_FILE_COMMAND_CODE = 0xDF;
}

DeleteFileCommand::DeleteFileCommand(uint8_t fileNo)
    : fileNo(fileNo)
    , complete(false)
{
}

etl::string_view DeleteFileCommand::name() const
{
    return "DeleteFile";
}

etl::expected<DesfireRequest, error::Error> DeleteFileCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (fileNo > 0x1FU)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    DesfireRequest request;
    request.commandCode = DELETE_FILE_COMMAND_CODE;
    request.data.clear();
    request.data.push_back(fileNo);
    request.expectedResponseLength = 0U;
    return request;
}

etl::expected<DesfireResult, error::Error> DeleteFileCommand::parseResponse(
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
        etl::vector<uint8_t, 2> cmacMessage;
        cmacMessage.push_back(DELETE_FILE_COMMAND_CODE);
        cmacMessage.push_back(fileNo);
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

bool DeleteFileCommand::isComplete() const
{
    return complete;
}

void DeleteFileCommand::reset()
{
    complete = false;
}
