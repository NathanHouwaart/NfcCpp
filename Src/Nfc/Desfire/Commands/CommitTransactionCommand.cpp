/**
 * @file CommitTransactionCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire commit transaction command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/CommitTransactionCommand.h"
#include "Nfc/Desfire/Commands/ValueOperationCryptoUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t COMMIT_TRANSACTION_COMMAND_CODE = 0xC7;
}

CommitTransactionCommand::CommitTransactionCommand()
    : complete(false)
{
}

etl::string_view CommitTransactionCommand::name() const
{
    return "CommitTransaction";
}

etl::expected<DesfireRequest, error::Error> CommitTransactionCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    DesfireRequest request;
    request.commandCode = COMMIT_TRANSACTION_COMMAND_CODE;
    request.data.clear();
    request.expectedResponseLength = 0U;
    return request;
}

etl::expected<DesfireResult, error::Error> CommitTransactionCommand::parseResponse(
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

    // In AES/3K3DES authenticated sessions, CommitTransaction is a plain
    // command with response CMAC. Keep IV progression aligned for the next
    // command.
    if (result.isSuccess() &&
        context.authenticated &&
        !context.sessionKeyEnc.empty())
    {
        const valueop_detail::SessionCipher cipher = valueop_detail::resolveSessionCipher(context);
        etl::vector<uint8_t, 1> cmacMessage;
        cmacMessage.push_back(COMMIT_TRANSACTION_COMMAND_CODE);

        if (cipher == valueop_detail::SessionCipher::AES)
        {
            etl::vector<uint8_t, 16> requestIv;
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
        else if (
            cipher == valueop_detail::SessionCipher::DES3_3K ||
            (cipher == valueop_detail::SessionCipher::DES3_2K &&
             !valueop_detail::useLegacyDesCryptoMode(context, cipher)))
        {
            etl::vector<uint8_t, 16> requestIv;
            if (!valueop_detail::deriveTktdesPlainRequestIv(context, cmacMessage, requestIv))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
            }

            auto nextIvResult = valueop_detail::deriveTktdesPlainResponseIv(response, context, requestIv, 8U);
            if (!nextIvResult)
            {
                return etl::unexpected(nextIvResult.error());
            }

            valueop_detail::setContextIv(context, nextIvResult.value());
        }
    }

    complete = true;
    return result;
}

bool CommitTransactionCommand::isComplete() const
{
    return complete;
}

void CommitTransactionCommand::reset()
{
    complete = false;
}
