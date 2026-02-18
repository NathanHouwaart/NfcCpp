/**
 * @file CreateCyclicRecordFileCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire create cyclic record file command implementation
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/CreateCyclicRecordFileCommand.h"
#include "Nfc/Desfire/Commands/CreateFileCommandUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t CREATE_CYCLIC_RECORD_FILE_COMMAND_CODE = 0xC0;
}

CreateCyclicRecordFileCommand::CreateCyclicRecordFileCommand(const CreateCyclicRecordFileCommandOptions& options)
    : options(options)
    , complete(false)
{
}

etl::string_view CreateCyclicRecordFileCommand::name() const
{
    return "CreateCyclicRecordFile";
}

etl::expected<DesfireRequest, error::Error> CreateCyclicRecordFileCommand::buildRequest(const DesfireContext& context)
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

    return create_file_detail::buildCreateRecordFileRequest(
        CREATE_CYCLIC_RECORD_FILE_COMMAND_CODE,
        options,
        accessLow,
        accessHigh);
}

etl::expected<DesfireResult, error::Error> CreateCyclicRecordFileCommand::parseResponse(
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

bool CreateCyclicRecordFileCommand::isComplete() const
{
    return complete;
}

void CreateCyclicRecordFileCommand::reset()
{
    complete = false;
}

bool CreateCyclicRecordFileCommand::validateOptions() const
{
    return create_file_detail::validateRecordFileOptions(options);
}

bool CreateCyclicRecordFileCommand::buildAccessRightsBytes(uint8_t& lowByte, uint8_t& highByte) const
{
    return create_file_detail::buildAccessRightsBytes(options, lowByte, highByte);
}
