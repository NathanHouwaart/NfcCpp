/**
 * @file GetFileSettingsCommand.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get file settings command implementation
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Nfc/Desfire/Commands/GetFileSettingsCommand.h"
#include "Nfc/Desfire/Commands/ValueOperationCryptoUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    constexpr uint8_t GET_FILE_SETTINGS_COMMAND_CODE = 0xF5;
}

GetFileSettingsCommand::GetFileSettingsCommand(uint8_t fileNo)
    : fileNo(fileNo)
    , stage(Stage::Initial)
    , fileType(FileType::Unknown)
    , fileTypeRaw(0xFFU)
    , communicationSettings(0x00U)
    , readAccess(0x0FU)
    , writeAccess(0x0FU)
    , readWriteAccess(0x0FU)
    , changeAccess(0x0FU)
    , fileSizeValid(false)
    , fileSize(0U)
    , valueSettingsValid(false)
    , lowerLimit(0U)
    , upperLimit(0U)
    , limitedCreditValue(0U)
    , limitedCreditEnabled(false)
    , freeGetValueEnabled(false)
    , recordSettingsValid(false)
    , recordSize(0U)
    , maxRecords(0U)
    , currentRecords(0U)
    , rawPayload()
    , requestIv()
    , hasRequestIv(false)
{
}

etl::string_view GetFileSettingsCommand::name() const
{
    return "GetFileSettings";
}

etl::expected<DesfireRequest, error::Error> GetFileSettingsCommand::buildRequest(const DesfireContext& context)
{
    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    requestIv.clear();
    hasRequestIv = false;
    if (context.authenticated)
    {
        etl::vector<uint8_t, 2> cmacMessage;
        cmacMessage.push_back(GET_FILE_SETTINGS_COMMAND_CODE);
        cmacMessage.push_back(fileNo);

        const valueop_detail::SessionCipher sessionCipher = valueop_detail::resolveSessionCipher(context);
        if (sessionCipher == valueop_detail::SessionCipher::AES)
        {
            hasRequestIv = valueop_detail::deriveAesPlainRequestIv(context, cmacMessage, requestIv);
        }
        else if (
            sessionCipher == valueop_detail::SessionCipher::DES3_3K ||
            (sessionCipher == valueop_detail::SessionCipher::DES3_2K &&
             context.authScheme == SessionAuthScheme::Iso))
        {
            hasRequestIv = valueop_detail::deriveTktdesPlainRequestIv(context, cmacMessage, requestIv);
        }
    }

    DesfireRequest request;
    request.commandCode = GET_FILE_SETTINGS_COMMAND_CODE;
    request.data.clear();
    request.data.push_back(fileNo);
    request.expectedResponseLength = 0;
    return request;
}

etl::expected<DesfireResult, error::Error> GetFileSettingsCommand::parseResponse(
    const etl::ivector<uint8_t>& response,
    DesfireContext& context)
{
    if (stage != Stage::Initial)
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
    }

    if (response.empty())
    {
        return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
    }

    DesfireResult result;
    result.statusCode = response[0];
    result.data.clear();

    if (!result.isSuccess())
    {
        return etl::unexpected(error::Error::fromDesfire(static_cast<error::DesfireError>(result.statusCode)));
    }

    etl::vector<uint8_t, 64> candidate;
    for (size_t i = 1; i < response.size(); ++i)
    {
        if (candidate.full())
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::LengthError));
        }
        candidate.push_back(response[i]);
    }

    const bool hasLegacyResponseMac =
        context.authenticated &&
        candidate.size() > 8U;
    etl::vector<uint8_t, 8> legacyResponseMac;
    if (hasLegacyResponseMac)
    {
        for (size_t i = candidate.size() - 8U; i < candidate.size(); ++i)
        {
            legacyResponseMac.push_back(candidate[i]);
        }
    }

    etl::vector<uint8_t, 64> authenticatedPayload;
    const bool authenticatedDecoded =
        context.authenticated &&
        tryDecodeAuthenticatedPayload(candidate, result.statusCode, context, authenticatedPayload);

    auto tryApplyCandidate = [&](const etl::ivector<uint8_t>& source, size_t length) -> bool
    {
        if (length > source.size())
        {
            return false;
        }

        etl::vector<uint8_t, 64> payload;
        for (size_t i = 0; i < length; ++i)
        {
            if (payload.full())
            {
                return false;
            }
            payload.push_back(source[i]);
        }

        if (!parsePayload(payload))
        {
            return false;
        }

        rawPayload.clear();
        result.data.clear();
        for (size_t i = 0; i < payload.size(); ++i)
        {
            if (rawPayload.full() || result.data.full())
            {
                return false;
            }
            rawPayload.push_back(payload[i]);
            result.data.push_back(payload[i]);
        }
        return true;
    };

    bool parsed = false;
    if (authenticatedDecoded)
    {
        parsed = tryApplyCandidate(authenticatedPayload, authenticatedPayload.size());
        if (!parsed)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
        }
    }
    else
    {
        // Compatibility fallback: some authenticated sessions append an 8-byte
        // trailer. Strip it before payload parsing and keep command-chain IV in sync.
        if (hasLegacyResponseMac)
        {
            parsed = tryApplyCandidate(candidate, candidate.size() - 8U);
            if (parsed)
            {
                valueop_detail::setContextIv(context, legacyResponseMac);
            }
        }

        if (!parsed)
        {
            parsed = tryApplyCandidate(candidate, candidate.size());
        }
        if (!parsed && context.authenticated)
        {
            if (candidate.size() >= 8U)
            {
                parsed = tryApplyCandidate(candidate, candidate.size() - 8U);
            }
            if (!parsed && candidate.size() >= 4U)
            {
                parsed = tryApplyCandidate(candidate, candidate.size() - 4U);
            }
        }
        if (!parsed)
        {
            return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
        }
    }

    stage = Stage::Complete;
    return result;
}

bool GetFileSettingsCommand::isComplete() const
{
    return stage == Stage::Complete;
}

void GetFileSettingsCommand::reset()
{
    stage = Stage::Initial;
    fileType = FileType::Unknown;
    fileTypeRaw = 0xFFU;
    communicationSettings = 0x00U;
    readAccess = 0x0FU;
    writeAccess = 0x0FU;
    readWriteAccess = 0x0FU;
    changeAccess = 0x0FU;
    fileSizeValid = false;
    fileSize = 0U;
    valueSettingsValid = false;
    lowerLimit = 0U;
    upperLimit = 0U;
    limitedCreditValue = 0U;
    limitedCreditEnabled = false;
    freeGetValueEnabled = false;
    recordSettingsValid = false;
    recordSize = 0U;
    maxRecords = 0U;
    currentRecords = 0U;
    rawPayload.clear();
    requestIv.clear();
    hasRequestIv = false;
}

uint8_t GetFileSettingsCommand::getFileNo() const
{
    return fileNo;
}

GetFileSettingsCommand::FileType GetFileSettingsCommand::getFileType() const
{
    return fileType;
}

uint8_t GetFileSettingsCommand::getFileTypeRaw() const
{
    return fileTypeRaw;
}

uint8_t GetFileSettingsCommand::getCommunicationSettings() const
{
    return communicationSettings;
}

uint8_t GetFileSettingsCommand::getReadAccess() const
{
    return readAccess;
}

uint8_t GetFileSettingsCommand::getWriteAccess() const
{
    return writeAccess;
}

uint8_t GetFileSettingsCommand::getReadWriteAccess() const
{
    return readWriteAccess;
}

uint8_t GetFileSettingsCommand::getChangeAccess() const
{
    return changeAccess;
}

bool GetFileSettingsCommand::hasFileSize() const
{
    return fileSizeValid;
}

uint32_t GetFileSettingsCommand::getFileSize() const
{
    return fileSize;
}

bool GetFileSettingsCommand::hasValueSettings() const
{
    return valueSettingsValid;
}

uint32_t GetFileSettingsCommand::getLowerLimit() const
{
    return lowerLimit;
}

uint32_t GetFileSettingsCommand::getUpperLimit() const
{
    return upperLimit;
}

uint32_t GetFileSettingsCommand::getLimitedCreditValue() const
{
    return limitedCreditValue;
}

bool GetFileSettingsCommand::isLimitedCreditEnabled() const
{
    return limitedCreditEnabled;
}

bool GetFileSettingsCommand::isFreeGetValueEnabled() const
{
    return freeGetValueEnabled;
}

bool GetFileSettingsCommand::hasRecordSettings() const
{
    return recordSettingsValid;
}

uint32_t GetFileSettingsCommand::getRecordSize() const
{
    return recordSize;
}

uint32_t GetFileSettingsCommand::getMaxRecords() const
{
    return maxRecords;
}

uint32_t GetFileSettingsCommand::getCurrentRecords() const
{
    return currentRecords;
}

const etl::vector<uint8_t, 64>& GetFileSettingsCommand::getRawPayload() const
{
    return rawPayload;
}

uint32_t GetFileSettingsCommand::readLe24(const etl::ivector<uint8_t>& payload, size_t offset)
{
    return static_cast<uint32_t>(payload[offset]) |
           (static_cast<uint32_t>(payload[offset + 1U]) << 8U) |
           (static_cast<uint32_t>(payload[offset + 2U]) << 16U);
}

uint32_t GetFileSettingsCommand::readLe32(const etl::ivector<uint8_t>& payload, size_t offset)
{
    return static_cast<uint32_t>(payload[offset]) |
           (static_cast<uint32_t>(payload[offset + 1U]) << 8U) |
           (static_cast<uint32_t>(payload[offset + 2U]) << 16U) |
           (static_cast<uint32_t>(payload[offset + 3U]) << 24U);
}

GetFileSettingsCommand::FileType GetFileSettingsCommand::toFileType(uint8_t rawType)
{
    switch (rawType)
    {
        case 0x00:
            return FileType::StandardData;
        case 0x01:
            return FileType::BackupData;
        case 0x02:
            return FileType::Value;
        case 0x03:
            return FileType::LinearRecord;
        case 0x04:
            return FileType::CyclicRecord;
        default:
            return FileType::Unknown;
    }
}

bool GetFileSettingsCommand::parsePayload(const etl::ivector<uint8_t>& payload)
{
    if (payload.size() < 4U)
    {
        return false;
    }

    fileTypeRaw = payload[0];
    fileType = toFileType(fileTypeRaw);
    communicationSettings = payload[1];

    const uint8_t access1 = payload[2];
    const uint8_t access2 = payload[3];
    readWriteAccess = static_cast<uint8_t>((access1 >> 4U) & 0x0FU);
    changeAccess = static_cast<uint8_t>(access1 & 0x0FU);
    readAccess = static_cast<uint8_t>((access2 >> 4U) & 0x0FU);
    writeAccess = static_cast<uint8_t>(access2 & 0x0FU);

    fileSizeValid = false;
    fileSize = 0U;
    valueSettingsValid = false;
    lowerLimit = 0U;
    upperLimit = 0U;
    limitedCreditValue = 0U;
    limitedCreditEnabled = false;
    freeGetValueEnabled = false;
    recordSettingsValid = false;
    recordSize = 0U;
    maxRecords = 0U;
    currentRecords = 0U;

    size_t offset = 4U;
    switch (fileType)
    {
        case FileType::StandardData:
        case FileType::BackupData:
            if (payload.size() < (offset + 3U))
            {
                return false;
            }
            fileSize = readLe24(payload, offset);
            fileSizeValid = true;
            return true;

        case FileType::Value:
            if (payload.size() < (offset + 13U))
            {
                return false;
            }
            lowerLimit = readLe32(payload, offset);
            upperLimit = readLe32(payload, offset + 4U);
            limitedCreditValue = readLe32(payload, offset + 8U);
            {
                const uint8_t flags = payload[offset + 12U];
                limitedCreditEnabled = (flags & 0x01U) != 0U;
                freeGetValueEnabled = (flags & 0x02U) != 0U;
            }
            valueSettingsValid = true;
            return true;

        case FileType::LinearRecord:
        case FileType::CyclicRecord:
            if (payload.size() < (offset + 9U))
            {
                return false;
            }
            recordSize = readLe24(payload, offset);
            maxRecords = readLe24(payload, offset + 3U);
            currentRecords = readLe24(payload, offset + 6U);
            recordSettingsValid = true;
            return true;

        case FileType::Unknown:
        default:
            // Unknown file type: keep common header fields only.
            return true;
    }
}

bool GetFileSettingsCommand::tryDecodeAuthenticatedPayload(
    const etl::ivector<uint8_t>& candidate,
    uint8_t statusCode,
    DesfireContext& context,
    etl::vector<uint8_t, 64>& outPayload)
{
    outPayload.clear();

    if (!hasRequestIv || candidate.size() < 8U)
    {
        return false;
    }

    const size_t payloadLength = candidate.size() - 8U;
    etl::vector<uint8_t, 16> nextIv;

    if (requestIv.size() == 16U && context.sessionKeyEnc.size() >= 16U)
    {
        if (!valueop_detail::verifyAesAuthenticatedPlainPayload(
                candidate,
                statusCode,
                context,
                requestIv,
                payloadLength,
                8U,
                nextIv))
        {
            return false;
        }
    }
    else if (requestIv.size() == 8U &&
             (context.sessionKeyEnc.size() == 16U || context.sessionKeyEnc.size() == 24U))
    {
        if (!valueop_detail::verifyTktdesAuthenticatedPlainPayload(
                candidate,
                statusCode,
                context,
                requestIv,
                payloadLength,
                8U,
                nextIv))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    for (size_t i = 0; i < payloadLength; ++i)
    {
        outPayload.push_back(candidate[i]);
    }

    valueop_detail::setContextIv(context, nextIv);

    return true;
}
