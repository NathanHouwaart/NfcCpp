/**
 * @file ChangeKeyCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Change key command implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Desfire/Commands/ChangeKeyCommand.h"
#include "Nfc/Card/DesfireContext.h"

using namespace nfc;

ChangeKeyCommand::ChangeKeyCommand(const ChangeKeyCommandOptions& options)
    : options(options)
    , stage(Stage::Initial)
{
}

etl::string_view ChangeKeyCommand::name() const
{
    return "ChangeKey";
}

etl::expected<DesfireRequest, error::Error> ChangeKeyCommand::buildRequest(const DesfireContext& context)
{
    // TODO: Implement change key request building
    DesfireRequest request;
    request.commandCode = 0xC4; // CHANGE_KEY command
    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> ChangeKeyCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    // TODO: Implement change key response parsing
    DesfireResult result;
    result.statusCode = 0x00;
    stage = Stage::Complete;
    return result;
}

bool ChangeKeyCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void ChangeKeyCommand::reset()
{
    stage = Stage::Initial;
}

etl::vector<uint8_t, 48> ChangeKeyCommand::buildKeyCryptogram(const DesfireContext& context)
{
    // TODO: Implement key cryptogram building
    etl::vector<uint8_t, 48> cryptogram;
    return cryptogram;
}

size_t ChangeKeyCommand::getKeySize() const
{
    switch (options.newKeyType)
    {
        case DesfireKeyType::DES:
            return 8;
        case DesfireKeyType::DES3_2K:
            return 16;
        case DesfireKeyType::DES3_3K:
        case DesfireKeyType::AES:
            return 24;
        default:
            return 0;
    }
}

etl::pair<etl::vector<uint8_t, 24>, etl::vector<uint8_t, 24>> ChangeKeyCommand::deriveSessionKeys(
    const etl::vector<uint8_t, 24>& masterKey,
    DesfireKeyType keyType)
{
    // TODO: Implement session key derivation
    etl::pair<etl::vector<uint8_t, 24>, etl::vector<uint8_t, 24>> keys;
    return keys;
}
