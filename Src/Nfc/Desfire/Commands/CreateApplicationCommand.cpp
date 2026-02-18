/**
 * @file CreateApplicationCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire create application command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/CreateApplicationCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t CREATE_APPLICATION_COMMAND_CODE = 0xCA;
}

CreateApplicationCommand::CreateApplicationCommand(const CreateApplicationCommandOptions& options)
    : options(options)
    , complete(false)
{
}

etl::string_view CreateApplicationCommand::name() const
{
    return "CreateApplication";
}

etl::expected<DesfireRequest, error::Error> CreateApplicationCommand::buildRequest(const DesfireContext& context)
{
    (void)context;

    if (complete)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    uint8_t keySettings2 = 0x00;
    if (!encodeKeySettings2(keySettings2))
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    DesfireRequest request;
    request.commandCode = CREATE_APPLICATION_COMMAND_CODE;
    request.data.clear();

    // AID is transmitted LSB-first in DESFire APDUs
    request.data.push_back(options.aid[0]);
    request.data.push_back(options.aid[1]);
    request.data.push_back(options.aid[2]);
    request.data.push_back(options.keySettings1);
    request.data.push_back(keySettings2);

    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> CreateApplicationCommand::parseResponse(
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

bool CreateApplicationCommand::isComplete() const
{
    return complete;
}

void CreateApplicationCommand::reset()
{
    complete = false;
}

bool CreateApplicationCommand::encodeKeySettings2(uint8_t& out) const
{
    if (options.keyCount == 0 || options.keyCount > 14)
    {
        return false;
    }

    uint8_t keyTypeBits = 0x00;
    switch (options.keyType)
    {
        case DesfireKeyType::DES:
        case DesfireKeyType::DES3_2K:
            keyTypeBits = 0x00;
            break;
        case DesfireKeyType::DES3_3K:
            keyTypeBits = 0x40;
            break;
        case DesfireKeyType::AES:
            keyTypeBits = 0x80;
            break;
        default:
            return false;
    }

    out = static_cast<uint8_t>((options.keyCount & 0x0F) | keyTypeBits);
    return true;
}
