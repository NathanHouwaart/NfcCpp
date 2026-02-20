/**
 * @file CreateFileCommandUtils.h
 * @brief Shared helpers for Create*File command implementations
 */

#pragma once

#include "Nfc/Desfire/DesfireRequest.h"
#include "Nfc/Desfire/DesfireResult.h"
#include "Error/Error.h"
#include "Error/DesfireError.h"
#include <etl/expected.h>

namespace nfc
{
    namespace create_file_detail
    {
        inline void appendLe24(etl::ivector<uint8_t>& target, uint32_t value)
        {
            target.push_back(static_cast<uint8_t>(value & 0xFFU));
            target.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
            target.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
        }

        template <typename TOptions>
        inline bool validateCommonOptions(const TOptions& options)
        {
            if (options.fileNo > 0x1FU)
            {
                return false;
            }

            if (options.communicationSettings != 0x00U &&
                options.communicationSettings != 0x01U &&
                options.communicationSettings != 0x03U)
            {
                return false;
            }

            return !(options.readAccess > 0x0FU ||
                     options.writeAccess > 0x0FU ||
                     options.readWriteAccess > 0x0FU ||
                     options.changeAccess > 0x0FU);
        }

        template <typename TOptions>
        inline bool validateDataFileOptions(const TOptions& options)
        {
            if (!validateCommonOptions(options))
            {
                return false;
            }

            return options.fileSize != 0U && options.fileSize <= 0x00FFFFFFU;
        }

        template <typename TOptions>
        inline bool validateRecordFileOptions(const TOptions& options)
        {
            if (!validateCommonOptions(options))
            {
                return false;
            }

            if (options.recordSize == 0U || options.recordSize > 0x00FFFFFFU)
            {
                return false;
            }

            return options.maxRecords != 0U && options.maxRecords <= 0x00FFFFFFU;
        }

        template <typename TOptions>
        inline bool buildAccessRightsBytes(const TOptions& options, uint8_t& lowByte, uint8_t& highByte)
        {
            if (!validateCommonOptions(options))
            {
                return false;
            }

            lowByte = static_cast<uint8_t>(((options.readWriteAccess & 0x0FU) << 4U) | (options.changeAccess & 0x0FU));
            highByte = static_cast<uint8_t>(((options.readAccess & 0x0FU) << 4U) | (options.writeAccess & 0x0FU));
            return true;
        }

        template <typename TOptions>
        inline DesfireRequest buildCreateDataFileRequest(
            uint8_t commandCode,
            const TOptions& options,
            uint8_t accessLow,
            uint8_t accessHigh)
        {
            DesfireRequest request;
            request.commandCode = commandCode;
            request.data.clear();
            request.data.push_back(options.fileNo);
            request.data.push_back(options.communicationSettings);
            request.data.push_back(accessLow);
            request.data.push_back(accessHigh);
            appendLe24(request.data, options.fileSize);
            request.expectedResponseLength = 0U;
            return request;
        }

        template <typename TOptions>
        inline DesfireRequest buildCreateRecordFileRequest(
            uint8_t commandCode,
            const TOptions& options,
            uint8_t accessLow,
            uint8_t accessHigh)
        {
            DesfireRequest request;
            request.commandCode = commandCode;
            request.data.clear();
            request.data.push_back(options.fileNo);
            request.data.push_back(options.communicationSettings);
            request.data.push_back(accessLow);
            request.data.push_back(accessHigh);
            appendLe24(request.data, options.recordSize);
            appendLe24(request.data, options.maxRecords);
            request.expectedResponseLength = 0U;
            return request;
        }

        inline etl::expected<DesfireResult, error::Error> parseSimpleResponse(const etl::ivector<uint8_t>& response)
        {
            if (response.empty())
            {
                return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidResponse));
            }

            DesfireResult result;
            result.statusCode = response[0];
            result.data.clear();
            for (size_t i = 1U; i < response.size(); ++i)
            {
                result.data.push_back(response[i]);
            }
            return result;
        }
    } // namespace create_file_detail
} // namespace nfc
