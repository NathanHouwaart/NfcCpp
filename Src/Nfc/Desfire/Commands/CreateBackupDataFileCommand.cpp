/**
 * @file CreateBackupDataFileCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire create backup data file command implementation
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/CreateBackupDataFileCommand.h"
#include "Nfc/Desfire/Commands/CreateFileCommandUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t CREATE_BACKUP_DATA_FILE_COMMAND_CODE = 0xCB;
}

CreateBackupDataFileCommand::CreateBackupDataFileCommand(const CreateBackupDataFileCommandOptions& options)
    : options(options)
    , complete(false)
{
}

etl::string_view CreateBackupDataFileCommand::name() const
{
    return "CreateBackupDataFile";
}

etl::expected<DesfireRequest, error::Error> CreateBackupDataFileCommand::buildRequest(const DesfireContext& context)
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
        CREATE_BACKUP_DATA_FILE_COMMAND_CODE,
        options,
        accessLow,
        accessHigh);
}

etl::expected<DesfireResult, error::Error> CreateBackupDataFileCommand::parseResponse(
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

bool CreateBackupDataFileCommand::isComplete() const
{
    return complete;
}

void CreateBackupDataFileCommand::reset()
{
    complete = false;
}

bool CreateBackupDataFileCommand::validateOptions() const
{
    return create_file_detail::validateDataFileOptions(options);
}

bool CreateBackupDataFileCommand::buildAccessRightsBytes(uint8_t& lowByte, uint8_t& highByte) const
{
    return create_file_detail::buildAccessRightsBytes(options, lowByte, highByte);
}
