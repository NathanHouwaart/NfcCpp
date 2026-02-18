/**
 * @file DesfireCard.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire card implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Desfire/DesfireCard.h"
#include "Nfc/Apdu/IApduTransceiver.h"
#include "Nfc/Desfire/ISecurePipe.h"
#include "Nfc/Desfire/PlainPipe.h"
#include "Nfc/Desfire/MacPipe.h"
#include "Nfc/Desfire/EncPipe.h"
#include "Nfc/Desfire/IDesfireCommand.h"
#include "Nfc/Desfire/DesfireRequest.h"
#include "Nfc/Desfire/DesfireResult.h"
#include "Nfc/Desfire/Commands/SelectApplicationCommand.h"
#include "Nfc/Desfire/Commands/AuthenticateCommand.h"
#include "Nfc/Desfire/Commands/GetVersionCommand.h"
#include "Nfc/Desfire/Commands/FormatPiccCommand.h"
#include "Nfc/Desfire/Commands/FreeMemoryCommand.h"
#include "Nfc/Desfire/Commands/GetApplicationIdsCommand.h"
#include "Nfc/Desfire/Commands/GetFileIdsCommand.h"
#include "Nfc/Desfire/Commands/GetFileSettingsCommand.h"
#include "Nfc/Desfire/Commands/GetKeySettingsCommand.h"
#include "Nfc/Desfire/Commands/GetKeyVersionCommand.h"
#include "Nfc/Desfire/Commands/GetCardUidCommand.h"
#include "Nfc/Desfire/Commands/ChangeKeySettingsCommand.h"
#include "Nfc/Desfire/Commands/SetConfigurationCommand.h"
#include "Nfc/Desfire/Commands/CreateApplicationCommand.h"
#include "Nfc/Desfire/Commands/DeleteApplicationCommand.h"
#include "Nfc/Desfire/Commands/CreateStdDataFileCommand.h"
#include "Nfc/Desfire/Commands/CreateBackupDataFileCommand.h"
#include "Nfc/Desfire/Commands/CreateLinearRecordFileCommand.h"
#include "Nfc/Desfire/Commands/CreateCyclicRecordFileCommand.h"
#include "Nfc/Desfire/Commands/CreateValueFileCommand.h"
#include "Nfc/Desfire/Commands/DeleteFileCommand.h"
#include "Nfc/Desfire/Commands/ChangeFileSettingsCommand.h"
#include "Nfc/Desfire/Commands/ClearRecordFileCommand.h"
#include "Nfc/Desfire/Commands/GetValueCommand.h"
#include "Nfc/Desfire/Commands/CreditCommand.h"
#include "Nfc/Desfire/Commands/DebitCommand.h"
#include "Nfc/Desfire/Commands/LimitedCreditCommand.h"
#include "Nfc/Desfire/Commands/CommitTransactionCommand.h"
#include "Nfc/Desfire/Commands/ReadDataCommand.h"
#include "Nfc/Desfire/Commands/WriteDataCommand.h"
#include "Nfc/Desfire/Commands/ReadRecordsCommand.h"
#include "Nfc/Desfire/Commands/WriteRecordCommand.h"
#include "Nfc/Wire/IWire.h"
#include "Nfc/Wire/IsoWire.h"
#include "Error/DesfireError.h"

using namespace nfc;

DesfireCard::DesfireCard(IApduTransceiver& transceiver, IWire& wireRef)
    : transceiver(transceiver)
    , context()
    , wire(&wireRef)
    , plainPipe(nullptr)
    , macPipe(nullptr)
    , encPipe(nullptr)
{
    // TODO: Initialize pipes
    // Wire is now managed by CardManager and passed in
}

const DesfireContext& DesfireCard::getContext() const
{
    return context;
}

etl::expected<void, error::Error> DesfireCard::executeCommand(IDesfireCommand& command)
{
    // Reset command state
    command.reset();
    
    // Multi-stage loop for commands that require multiple frames
    while (!command.isComplete())
    {
        // 1. Build request (PDU format: [CMD][Data...])
        auto requestResult = command.buildRequest(context);
        if (!requestResult)
        {
            return etl::unexpected(requestResult.error());
        }
        
        DesfireRequest& request = requestResult.value();
        
        // 2. Build PDU (Protocol Data Unit)
        etl::vector<uint8_t, 256> pdu;
        pdu.push_back(request.commandCode);
        for (size_t i = 0; i < request.data.size(); ++i)
        {
            pdu.push_back(request.data[i]);
        }
        
        // 3. Wrap PDU into APDU using wire strategy
        etl::vector<uint8_t, 261> apdu = wire->wrap(pdu);
        
        // 4. Transceive APDU (adapter unwraps using configured wire)
        // Returns normalized PDU: [Status][Data...]
        auto pduResult = transceiver.transceive(apdu);
        if (!pduResult)
        {
            return etl::unexpected(pduResult.error());
        }
        
        etl::vector<uint8_t, 256>& unwrappedPdu = pduResult.value();
        
        // 7. Parse response (updates command state)
        auto parseResult = command.parseResponse(unwrappedPdu, context);
        if (!parseResult)
        {
            return etl::unexpected(parseResult.error());
        }
        
        DesfireResult& result = parseResult.value();
        
        // 8. Check for errors (unless it's an additional frame request)
        if (!result.isSuccess() && result.statusCode != 0xAF)
        {
            auto desfireStatus = static_cast<error::DesfireError>(result.statusCode);
            return etl::unexpected(error::Error::fromDesfire(desfireStatus));
        }
    }
    
    return {};
}

etl::expected<void, error::Error> DesfireCard::selectApplication(const etl::array<uint8_t, 3>& aid)
{
    SelectApplicationCommand command(aid);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::authenticate(
    uint8_t keyNo,
    const etl::vector<uint8_t, 24>& key,
    DesfireAuthMode mode)
{
    AuthenticateCommandOptions options;
    options.keyNo = keyNo;
    options.key = key;
    options.mode = mode;
    
    AuthenticateCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::createApplication(
    const etl::array<uint8_t, 3>& aid,
    uint8_t keySettings1,
    uint8_t keyCount,
    DesfireKeyType keyType)
{
    CreateApplicationCommandOptions options;
    options.aid = aid;
    options.keySettings1 = keySettings1;
    options.keyCount = keyCount;
    options.keyType = keyType;

    CreateApplicationCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::deleteApplication(const etl::array<uint8_t, 3>& aid)
{
    DeleteApplicationCommand command(aid);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::createStdDataFile(
    uint8_t fileNo,
    uint8_t communicationSettings,
    uint8_t readAccess,
    uint8_t writeAccess,
    uint8_t readWriteAccess,
    uint8_t changeAccess,
    uint32_t fileSize)
{
    CreateStdDataFileCommandOptions options;
    options.fileNo = fileNo;
    options.communicationSettings = communicationSettings;
    options.readAccess = readAccess;
    options.writeAccess = writeAccess;
    options.readWriteAccess = readWriteAccess;
    options.changeAccess = changeAccess;
    options.fileSize = fileSize;

    CreateStdDataFileCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::createBackupDataFile(
    uint8_t fileNo,
    uint8_t communicationSettings,
    uint8_t readAccess,
    uint8_t writeAccess,
    uint8_t readWriteAccess,
    uint8_t changeAccess,
    uint32_t fileSize)
{
    CreateBackupDataFileCommandOptions options;
    options.fileNo = fileNo;
    options.communicationSettings = communicationSettings;
    options.readAccess = readAccess;
    options.writeAccess = writeAccess;
    options.readWriteAccess = readWriteAccess;
    options.changeAccess = changeAccess;
    options.fileSize = fileSize;

    CreateBackupDataFileCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::createValueFile(
    uint8_t fileNo,
    uint8_t communicationSettings,
    uint8_t readAccess,
    uint8_t writeAccess,
    uint8_t readWriteAccess,
    uint8_t changeAccess,
    int32_t lowerLimit,
    int32_t upperLimit,
    int32_t limitedCreditValue,
    bool limitedCreditEnabled,
    bool freeGetValueEnabled)
{
    CreateValueFileCommandOptions options;
    options.fileNo = fileNo;
    options.communicationSettings = communicationSettings;
    options.readAccess = readAccess;
    options.writeAccess = writeAccess;
    options.readWriteAccess = readWriteAccess;
    options.changeAccess = changeAccess;
    options.lowerLimit = lowerLimit;
    options.upperLimit = upperLimit;
    options.limitedCreditValue = limitedCreditValue;
    options.valueOptions = static_cast<uint8_t>(
        (limitedCreditEnabled ? 0x01U : 0x00U) |
        (freeGetValueEnabled ? 0x02U : 0x00U));

    CreateValueFileCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::deleteFile(uint8_t fileNo)
{
    DeleteFileCommand command(fileNo);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::changeFileSettings(
    uint8_t fileNo,
    uint8_t communicationSettings,
    uint8_t readAccess,
    uint8_t writeAccess,
    uint8_t readWriteAccess,
    uint8_t changeAccess,
    uint8_t commandCommunicationSettings,
    DesfireAuthMode authMode,
    DesfireKeyType sessionKeyType)
{
    ChangeFileSettingsCommandOptions options;
    options.fileNo = fileNo;
    options.communicationSettings = communicationSettings;
    options.readAccess = readAccess;
    options.writeAccess = writeAccess;
    options.readWriteAccess = readWriteAccess;
    options.changeAccess = changeAccess;
    options.commandCommunicationSettings = commandCommunicationSettings;
    options.authMode = authMode;
    options.sessionKeyType = sessionKeyType;

    ChangeFileSettingsCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::clearRecordFile(uint8_t fileNo)
{
    ClearRecordFileCommand command(fileNo);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::createLinearRecordFile(
    uint8_t fileNo,
    uint8_t communicationSettings,
    uint8_t readAccess,
    uint8_t writeAccess,
    uint8_t readWriteAccess,
    uint8_t changeAccess,
    uint32_t recordSize,
    uint32_t maxRecords)
{
    CreateLinearRecordFileCommandOptions options;
    options.fileNo = fileNo;
    options.communicationSettings = communicationSettings;
    options.readAccess = readAccess;
    options.writeAccess = writeAccess;
    options.readWriteAccess = readWriteAccess;
    options.changeAccess = changeAccess;
    options.recordSize = recordSize;
    options.maxRecords = maxRecords;

    CreateLinearRecordFileCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::createCyclicRecordFile(
    uint8_t fileNo,
    uint8_t communicationSettings,
    uint8_t readAccess,
    uint8_t writeAccess,
    uint8_t readWriteAccess,
    uint8_t changeAccess,
    uint32_t recordSize,
    uint32_t maxRecords)
{
    CreateCyclicRecordFileCommandOptions options;
    options.fileNo = fileNo;
    options.communicationSettings = communicationSettings;
    options.readAccess = readAccess;
    options.writeAccess = writeAccess;
    options.readWriteAccess = readWriteAccess;
    options.changeAccess = changeAccess;
    options.recordSize = recordSize;
    options.maxRecords = maxRecords;

    CreateCyclicRecordFileCommand command(options);
    return executeCommand(command);
}

etl::expected<int32_t, error::Error> DesfireCard::getValue(uint8_t fileNo)
{
    GetValueCommand command(fileNo);
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    return command.getValue();
}

etl::expected<void, error::Error> DesfireCard::credit(uint8_t fileNo, int32_t value)
{
    CreditCommandOptions options;
    options.fileNo = fileNo;
    options.value = value;
    options.communicationSettings = context.authenticated ? 0x03U : 0x00U;

    CreditCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::debit(uint8_t fileNo, int32_t value)
{
    DebitCommandOptions options;
    options.fileNo = fileNo;
    options.value = value;
    options.communicationSettings = context.authenticated ? 0x03U : 0x00U;

    DebitCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::limitedCredit(uint8_t fileNo, int32_t value)
{
    LimitedCreditCommandOptions options;
    options.fileNo = fileNo;
    options.value = value;
    options.communicationSettings = context.authenticated ? 0x03U : 0x00U;

    LimitedCreditCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::commitTransaction()
{
    CommitTransactionCommand command;
    return executeCommand(command);
}

etl::expected<etl::vector<uint8_t, DesfireCard::MAX_DATA_IO_SIZE>, error::Error> DesfireCard::readData(
    uint8_t fileNo,
    uint32_t offset,
    uint32_t length,
    uint16_t chunkSize)
{
    uint8_t communicationSettings = context.authenticated ? 0x03U : 0x00U;

    auto settingsResult = getFileSettings(fileNo);
    if (settingsResult)
    {
        const DesfireFileSettingsInfo& settings = settingsResult.value();
        communicationSettings = settings.communicationSettings;

        if (settings.hasFileSize)
        {
            const uint64_t endExclusive = static_cast<uint64_t>(offset) + static_cast<uint64_t>(length);
            if (endExclusive > static_cast<uint64_t>(settings.fileSize))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::BoundaryError));
            }
        }
    }

    ReadDataCommandOptions options;
    options.fileNo = fileNo;
    options.offset = offset;
    options.length = length;
    options.chunkSize = chunkSize;
    options.communicationSettings = communicationSettings;

    ReadDataCommand command(options);
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    etl::vector<uint8_t, MAX_DATA_IO_SIZE> out;
    const auto& commandData = command.getData();
    for (size_t i = 0; i < commandData.size(); ++i)
    {
        out.push_back(commandData[i]);
    }

    return out;
}

etl::expected<void, error::Error> DesfireCard::writeData(
    uint8_t fileNo,
    uint32_t offset,
    const etl::ivector<uint8_t>& data,
    uint16_t chunkSize)
{
    if (data.empty())
    {
        return {};
    }

    uint8_t communicationSettings = context.authenticated ? 0x03U : 0x00U;

    auto settingsResult = getFileSettings(fileNo);
    if (settingsResult)
    {
        const DesfireFileSettingsInfo& settings = settingsResult.value();
        communicationSettings = settings.communicationSettings;

        if (settings.hasFileSize)
        {
            const uint64_t endExclusive = static_cast<uint64_t>(offset) + static_cast<uint64_t>(data.size());
            if (endExclusive > static_cast<uint64_t>(settings.fileSize))
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::BoundaryError));
            }
        }
    }

    WriteDataCommandOptions options;
    options.fileNo = fileNo;
    options.offset = offset;
    options.data = &data;
    options.chunkSize = chunkSize;
    options.communicationSettings = communicationSettings;

    WriteDataCommand command(options);
    return executeCommand(command);
}

etl::expected<etl::vector<uint8_t, DesfireCard::MAX_DATA_IO_SIZE>, error::Error> DesfireCard::readRecords(
    uint8_t fileNo,
    uint32_t recordOffset,
    uint32_t recordCount,
    uint16_t chunkSize)
{
    (void)chunkSize;

    auto settingsResult = getFileSettings(fileNo);
    if (!settingsResult)
    {
        return etl::unexpected(settingsResult.error());
    }

    const DesfireFileSettingsInfo& settings = settingsResult.value();
    if (!settings.hasRecordSettings || settings.recordSize == 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    const uint32_t currentRecords = settings.currentRecords;
    if (currentRecords == 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::BoundaryError));
    }

    if (recordOffset >= currentRecords)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::BoundaryError));
    }

    const uint32_t availableRecords = currentRecords - recordOffset;
    const uint32_t effectiveRecordCount = (recordCount == 0U) ? availableRecords : recordCount;
    if (effectiveRecordCount == 0U || effectiveRecordCount > availableRecords)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::BoundaryError));
    }

    const uint64_t expectedByteLength64 =
        static_cast<uint64_t>(effectiveRecordCount) * static_cast<uint64_t>(settings.recordSize);
    if (expectedByteLength64 == 0U || expectedByteLength64 > MAX_DATA_IO_SIZE)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
    }

    ReadRecordsCommandOptions options;
    options.fileNo = fileNo;
    options.recordOffset = recordOffset;
    options.recordCount = effectiveRecordCount;
    options.recordSize = settings.recordSize;
    options.expectedDataLength = static_cast<uint32_t>(expectedByteLength64);
    options.communicationSettings = settings.communicationSettings;

    ReadRecordsCommand command(options);
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    etl::vector<uint8_t, MAX_DATA_IO_SIZE> out;
    const auto& commandData = command.getData();
    for (size_t i = 0; i < commandData.size(); ++i)
    {
        out.push_back(commandData[i]);
    }

    return out;
}

etl::expected<void, error::Error> DesfireCard::writeRecord(
    uint8_t fileNo,
    uint32_t offset,
    const etl::ivector<uint8_t>& data,
    uint16_t chunkSize)
{
    if (data.empty())
    {
        return {};
    }

    auto settingsResult = getFileSettings(fileNo);
    if (!settingsResult)
    {
        return etl::unexpected(settingsResult.error());
    }

    const DesfireFileSettingsInfo& settings = settingsResult.value();
    if (!settings.hasRecordSettings || settings.recordSize == 0U)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::ParameterError));
    }

    if (offset >= settings.recordSize)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::BoundaryError));
    }

    const uint64_t endWithinRecord =
        static_cast<uint64_t>(offset) + static_cast<uint64_t>(data.size());
    if (endWithinRecord > static_cast<uint64_t>(settings.recordSize))
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::BoundaryError));
    }

    WriteRecordCommandOptions options;
    options.fileNo = fileNo;
    options.offset = offset;
    options.recordSize = settings.recordSize;
    options.data = &data;
    options.chunkSize = chunkSize;
    options.communicationSettings = settings.communicationSettings;

    WriteRecordCommand command(options);
    return executeCommand(command);
}

etl::expected<etl::vector<uint8_t, 96>, error::Error> DesfireCard::getVersion()
{
    GetVersionCommand command;
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    etl::vector<uint8_t, 96> payload;
    const etl::vector<uint8_t, 96>& commandPayload = command.getVersionData();
    for (size_t i = 0; i < commandPayload.size(); ++i)
    {
        payload.push_back(commandPayload[i]);
    }
    return payload;
}

etl::expected<void, error::Error> DesfireCard::formatPicc()
{
    FormatPiccCommand command;
    return executeCommand(command);
}

etl::expected<uint32_t, error::Error> DesfireCard::freeMemory()
{
    FreeMemoryCommand command;
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    return command.getFreeMemoryBytes();
}

etl::expected<etl::vector<etl::array<uint8_t, 3>, 84>, error::Error> DesfireCard::getApplicationIds()
{
    GetApplicationIdsCommand command;
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    etl::vector<etl::array<uint8_t, 3>, 84> aidList;
    const etl::vector<etl::array<uint8_t, 3>, 84>& commandAids = command.getApplicationIds();
    for (size_t i = 0; i < commandAids.size(); ++i)
    {
        aidList.push_back(commandAids[i]);
    }
    return aidList;
}

etl::expected<etl::vector<uint8_t, 32>, error::Error> DesfireCard::getFileIds()
{
    GetFileIdsCommand command;
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    etl::vector<uint8_t, 32> fileIds;
    const etl::vector<uint8_t, 32>& commandFileIds = command.getFileIds();
    for (size_t i = 0; i < commandFileIds.size(); ++i)
    {
        fileIds.push_back(commandFileIds[i]);
    }
    return fileIds;
}

etl::expected<DesfireFileSettingsInfo, error::Error> DesfireCard::getFileSettings(uint8_t fileNo)
{
    GetFileSettingsCommand command(fileNo);
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    DesfireFileSettingsInfo info;
    info.fileNo = command.getFileNo();
    info.fileType = command.getFileTypeRaw();
    info.communicationSettings = command.getCommunicationSettings();
    info.readAccess = command.getReadAccess();
    info.writeAccess = command.getWriteAccess();
    info.readWriteAccess = command.getReadWriteAccess();
    info.changeAccess = command.getChangeAccess();

    info.hasFileSize = command.hasFileSize();
    info.fileSize = command.getFileSize();

    info.hasValueSettings = command.hasValueSettings();
    info.lowerLimit = command.getLowerLimit();
    info.upperLimit = command.getUpperLimit();
    info.limitedCreditValue = command.getLimitedCreditValue();
    info.limitedCreditEnabled = command.isLimitedCreditEnabled();
    info.freeGetValueEnabled = command.isFreeGetValueEnabled();

    info.hasRecordSettings = command.hasRecordSettings();
    info.recordSize = command.getRecordSize();
    info.maxRecords = command.getMaxRecords();
    info.currentRecords = command.getCurrentRecords();

    return info;
}

etl::expected<etl::array<uint8_t, 2>, error::Error> DesfireCard::getKeySettings()
{
    GetKeySettingsCommand command;
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    etl::array<uint8_t, 2> keySettings = {
        command.getKeySettings1(),
        command.getKeySettings2()
    };
    return keySettings;
}

etl::expected<uint8_t, error::Error> DesfireCard::getKeyVersion(uint8_t keyNo)
{
    GetKeyVersionCommand command(keyNo);
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    return command.getKeyVersion();
}

etl::expected<void, error::Error> DesfireCard::changeKeySettings(
    uint8_t keySettings,
    DesfireAuthMode authMode,
    DesfireKeyType sessionKeyType)
{
    ChangeKeySettingsCommandOptions options;
    options.keySettings = keySettings;
    options.authMode = authMode;
    options.sessionKeyType = sessionKeyType;

    ChangeKeySettingsCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::setConfigurationPicc(
    uint8_t piccConfiguration,
    DesfireAuthMode authMode,
    DesfireKeyType sessionKeyType)
{
    SetConfigurationCommandOptions options;
    options.subcommand = SetConfigurationSubcommand::PiccConfiguration;
    options.piccConfiguration = piccConfiguration;
    options.authMode = authMode;
    options.sessionKeyType = sessionKeyType;

    SetConfigurationCommand command(options);
    return executeCommand(command);
}

etl::expected<void, error::Error> DesfireCard::setConfigurationAts(
    const etl::ivector<uint8_t>& ats,
    DesfireAuthMode authMode,
    DesfireKeyType sessionKeyType)
{
    SetConfigurationCommandOptions options;
    options.subcommand = SetConfigurationSubcommand::AtsConfiguration;
    for (size_t i = 0; i < ats.size(); ++i)
    {
        if (options.ats.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }
        options.ats.push_back(ats[i]);
    }
    options.authMode = authMode;
    options.sessionKeyType = sessionKeyType;

    SetConfigurationCommand command(options);
    return executeCommand(command);
}

etl::expected<etl::vector<uint8_t, 10>, error::Error> DesfireCard::getRealCardUid()
{
    GetCardUidCommand command;
    auto result = executeCommand(command);
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    etl::vector<uint8_t, 10> uid;
    const etl::vector<uint8_t, 10>& commandUid = command.getUid();
    for (size_t i = 0; i < commandUid.size(); ++i)
    {
        uid.push_back(commandUid[i]);
    }
    return uid;
}

etl::expected<etl::vector<uint8_t, 261>, error::Error> DesfireCard::wrapRequest(const DesfireRequest& request)
{
    // TODO: Implement request wrapping based on current communication mode
    return etl::unexpected(error::Error::fromHardware(error::HardwareError::NotSupported));
}

etl::expected<DesfireResult, error::Error> DesfireCard::unwrapResponse(const etl::ivector<uint8_t>& response)
{
    // TODO: Implement response unwrapping based on current communication mode
    return etl::unexpected(error::Error::fromHardware(error::HardwareError::NotSupported));
}
