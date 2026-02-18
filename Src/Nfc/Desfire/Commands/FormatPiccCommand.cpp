/**
 * @file FormatPiccCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire format PICC command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/FormatPiccCommand.h"
#include "Nfc/Desfire/Commands/ValueOperationCryptoUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t FORMAT_PICC_COMMAND_CODE = 0xFC;
}

FormatPiccCommand::FormatPiccCommand()
    : complete(false)
{
}

etl::string_view FormatPiccCommand::name() const
{
    return "FormatPICC";
}

etl::expected<DesfireRequest, error::Error> FormatPiccCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    DesfireRequest request;
    request.commandCode = FORMAT_PICC_COMMAND_CODE;
    request.data.clear();
    request.expectedResponseLength = 0U;
    return request;
}

etl::expected<DesfireResult, error::Error> FormatPiccCommand::parseResponse(
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

    // In AES authenticated sessions, FormatPICC is a plain command with
    // response CMAC. Keep IV progression aligned for follow-up commands.
    if (result.isSuccess() &&
        context.authenticated &&
        context.iv.size() == 16U &&
        context.sessionKeyEnc.size() >= 16U)
    {
        etl::vector<uint8_t, 16> requestIv;
        etl::vector<uint8_t, 1> cmacMessage;
        cmacMessage.push_back(FORMAT_PICC_COMMAND_CODE);
        if (!valueop_detail::deriveAesPlainRequestIv(context, cmacMessage, requestIv))
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
        }

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

bool FormatPiccCommand::isComplete() const
{
    return complete;
}

void FormatPiccCommand::reset()
{
    complete = false;
}
