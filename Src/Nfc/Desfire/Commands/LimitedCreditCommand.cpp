/**
 * @file LimitedCreditCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire limited credit command implementation
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/LimitedCreditCommand.h"
#include "Nfc/Desfire/Commands/ValueMutationCommandUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t LIMITED_CREDIT_COMMAND_CODE = 0x1C;
}

LimitedCreditCommand::LimitedCreditCommand(const LimitedCreditCommandOptions& options)
    : options(options)
    , complete(false)
    , sessionCipher(valueop_detail::SessionCipher::UNKNOWN)
    , updateContextIv(false)
    , pendingIv()
{
}

etl::string_view LimitedCreditCommand::name() const
{
    return "LimitedCredit";
}

etl::expected<DesfireRequest, error::Error> LimitedCreditCommand::buildRequest(const DesfireContext& context)
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
        LIMITED_CREDIT_COMMAND_CODE,
        options,
        context,
        sessionCipher,
        updateContextIv,
        pendingIv);
}

etl::expected<DesfireResult, error::Error> LimitedCreditCommand::parseResponse(
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

bool LimitedCreditCommand::isComplete() const
{
    return complete;
}

void LimitedCreditCommand::reset()
{
    complete = false;
    sessionCipher = valueop_detail::SessionCipher::UNKNOWN;
    updateContextIv = false;
    pendingIv.clear();
}
