/**
 * @file CreateStdDataFileCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire create standard data file command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/CreateStdDataFileCommand.h"
#include "Nfc/Desfire/Commands/CreateFileCommandUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t CREATE_STD_DATA_FILE_COMMAND_CODE = 0xCD;
}

CreateStdDataFileCommand::CreateStdDataFileCommand(const CreateStdDataFileCommandOptions& options)
    : options(options)
    , complete(false)
{
}

etl::string_view CreateStdDataFileCommand::name() const
{
    return "CreateStdDataFile";
}

etl::expected<DesfireRequest, error::Error> CreateStdDataFileCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (!validateOptions())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    uint8_t accessLow = 0x00;
    uint8_t accessHigh = 0x00;
    if (!buildAccessRightsBytes(accessLow, accessHigh))
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    return create_file_detail::buildCreateDataFileRequest(
        CREATE_STD_DATA_FILE_COMMAND_CODE,
        options,
        accessLow,
        accessHigh);
}

etl::expected<DesfireResult, error::Error> CreateStdDataFileCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    (void)context;

    auto parseResult = create_file_detail::parseSimpleResponse(response);
    if (!parseResult)
    {
        return etl::unexpected(parseResult.error());
    }

    complete = true;
    return parseResult.value();
}

bool CreateStdDataFileCommand::isComplete() const
{
    return complete;
}

void CreateStdDataFileCommand::reset()
{
    complete = false;
}

bool CreateStdDataFileCommand::validateOptions() const
{
    return create_file_detail::validateDataFileOptions(options);
}

bool CreateStdDataFileCommand::buildAccessRightsBytes(uint8_t& lowByte, uint8_t& highByte) const
{
    return create_file_detail::buildAccessRightsBytes(options, lowByte, highByte);
}
