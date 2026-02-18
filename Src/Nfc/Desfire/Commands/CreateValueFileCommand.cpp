/**
 * @file CreateValueFileCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire create value file command implementation
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/CreateValueFileCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t CREATE_VALUE_FILE_COMMAND_CODE = 0xCC;
    constexpr uint8_t SUPPORTED_VALUE_OPTIONS_MASK = 0x03U;
}

CreateValueFileCommand::CreateValueFileCommand(const CreateValueFileCommandOptions& options)
    : options(options)
    , complete(false)
{
}

etl::string_view CreateValueFileCommand::name() const
{
    return "CreateValueFile";
}

etl::expected<DesfireRequest, error::Error> CreateValueFileCommand::buildRequest(const DesfireContext& context)
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

    DesfireRequest request;
    request.commandCode = CREATE_VALUE_FILE_COMMAND_CODE;
    request.data.clear();

    request.data.push_back(options.fileNo);
    request.data.push_back(options.communicationSettings);
    request.data.push_back(accessLow);
    request.data.push_back(accessHigh);
    appendLe32(request, options.lowerLimit);
    appendLe32(request, options.upperLimit);
    appendLe32(request, options.limitedCreditValue);
    request.data.push_back(options.valueOptions);

    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> CreateValueFileCommand::parseResponse(
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

    complete = true;
    return result;
}

bool CreateValueFileCommand::isComplete() const
{
    return complete;
}

void CreateValueFileCommand::reset()
{
    complete = false;
}

bool CreateValueFileCommand::validateOptions() const
{
    if (options.fileNo > 0x1FU)
    {
        return false;
    }

    if (options.communicationSettings != 0x00 &&
        options.communicationSettings != 0x01 &&
        options.communicationSettings != 0x03)
    {
        return false;
    }

    if (options.readAccess > 0x0FU ||
        options.writeAccess > 0x0FU ||
        options.readWriteAccess > 0x0FU ||
        options.changeAccess > 0x0FU)
    {
        return false;
    }

    if (options.lowerLimit > options.upperLimit)
    {
        return false;
    }

    if (options.limitedCreditValue < options.lowerLimit || options.limitedCreditValue > options.upperLimit)
    {
        return false;
    }

    if ((options.valueOptions & static_cast<uint8_t>(~SUPPORTED_VALUE_OPTIONS_MASK)) != 0U)
    {
        return false;
    }

    return true;
}

bool CreateValueFileCommand::buildAccessRightsBytes(uint8_t& lowByte, uint8_t& highByte) const
{
    if (options.readAccess > 0x0FU ||
        options.writeAccess > 0x0FU ||
        options.readWriteAccess > 0x0FU ||
        options.changeAccess > 0x0FU)
    {
        return false;
    }

    // Access rights on wire are [RW|CAR] [R|W].
    lowByte = static_cast<uint8_t>(((options.readWriteAccess & 0x0FU) << 4U) | (options.changeAccess & 0x0FU));
    highByte = static_cast<uint8_t>(((options.readAccess & 0x0FU) << 4U) | (options.writeAccess & 0x0FU));
    return true;
}

void CreateValueFileCommand::appendLe32(DesfireRequest& request, int32_t value) const
{
    const uint32_t raw = static_cast<uint32_t>(value);
    request.data.push_back(static_cast<uint8_t>(raw & 0xFFU));
    request.data.push_back(static_cast<uint8_t>((raw >> 8U) & 0xFFU));
    request.data.push_back(static_cast<uint8_t>((raw >> 16U) & 0xFFU));
    request.data.push_back(static_cast<uint8_t>((raw >> 24U) & 0xFFU));
}

