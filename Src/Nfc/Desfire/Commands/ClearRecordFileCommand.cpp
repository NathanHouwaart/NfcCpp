/**
 * @file ClearRecordFileCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire clear record file command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/ClearRecordFileCommand.h"
#include "Nfc/Desfire/Commands/ValueOperationCryptoUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t CLEAR_RECORD_FILE_COMMAND_CODE = 0xEB;
}

ClearRecordFileCommand::ClearRecordFileCommand(uint8_t fileNo)
    : fileNo(fileNo)
    , complete(false)
    , requestIv()
    , hasRequestIv(false)
{
}

etl::string_view ClearRecordFileCommand::name() const
{
    return "ClearRecordFile";
}

etl::expected<DesfireRequest, error::Error> ClearRecordFileCommand::buildRequest(const DesfireContext& context)
{
    if (complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (fileNo > 0x1FU)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    requestIv.clear();
    hasRequestIv = false;
    if (context.authenticated && context.iv.size() == 16U && context.sessionKeyEnc.size() >= 16U)
    {
        etl::vector<uint8_t, 2> cmacMessage;
        cmacMessage.push_back(CLEAR_RECORD_FILE_COMMAND_CODE);
        cmacMessage.push_back(fileNo);
        hasRequestIv = valueop_detail::deriveAesPlainRequestIv(context, cmacMessage, requestIv);
    }

    DesfireRequest request;
    request.commandCode = CLEAR_RECORD_FILE_COMMAND_CODE;
    request.data.clear();
    request.data.push_back(fileNo);
    request.expectedResponseLength = 0U;
    return request;
}

etl::expected<DesfireResult, error::Error> ClearRecordFileCommand::parseResponse(
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

    if (result.isSuccess() && hasRequestIv)
    {
        auto nextIvResult = valueop_detail::deriveAesPlainResponseIv(response, context, requestIv, 8U);
        if (!nextIvResult)
        {
            return etl::unexpected(nextIvResult.error());
        }

        valueop_detail::setContextIv(context, nextIvResult.value());
    }

    complete = true;
    return result;
}

bool ClearRecordFileCommand::isComplete() const
{
    return complete;
}

void ClearRecordFileCommand::reset()
{
    complete = false;
    requestIv.clear();
    hasRequestIv = false;
}
