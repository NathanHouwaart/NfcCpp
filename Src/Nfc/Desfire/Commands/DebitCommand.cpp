/**
 * @file DebitCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire debit command implementation
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/DebitCommand.h"
#include "Nfc/Desfire/Commands/ValueMutationCommandUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t DEBIT_COMMAND_CODE = 0xDC;
}

DebitCommand::DebitCommand(const DebitCommandOptions& options)
    : options(options)
    , complete(false)
    , sessionCipher(valueop_detail::SessionCipher::UNKNOWN)
    , updateContextIv(false)
    , pendingIv()
{
}

etl::string_view DebitCommand::name() const
{
    return "Debit";
}

etl::expected<DesfireRequest, error::Error> DebitCommand::buildRequest(const DesfireContext& context)
{
    if (complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (!value_mutation_detail::validateOptions(options))
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }
    return value_mutation_detail::buildRequest(
        DEBIT_COMMAND_CODE,
        options,
        context,
        sessionCipher,
        updateContextIv,
        pendingIv);
}

etl::expected<DesfireResult, error::Error> DebitCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    auto parseResult = value_mutation_detail::parseResponse(
        response,
        context,
        sessionCipher,
        updateContextIv,
        pendingIv);
    if (!parseResult)
    {
        return etl::unexpected(parseResult.error());
    }

    complete = true;
    return parseResult.value();
}

bool DebitCommand::isComplete() const
{
    return complete;
}

void DebitCommand::reset()
{
    complete = false;
    sessionCipher = valueop_detail::SessionCipher::UNKNOWN;
    updateContextIv = false;
    pendingIv.clear();
}
