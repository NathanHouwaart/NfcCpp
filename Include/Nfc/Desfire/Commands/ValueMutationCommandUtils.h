/**
 * @file ValueMutationCommandUtils.h
 * @brief Shared helper utilities for Credit/Debit/LimitedCredit commands
 */

#pragma once

#include "Nfc/Desfire/DesfireContext.h"
#include "Nfc/Desfire/DesfireRequest.h"
#include "Nfc/Desfire/DesfireResult.h"
#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include "Error/Error.h"
#include "Error/DesfireError.h"
#include <etl/expected.h>
#include <etl/vector.h>

namespace nfc
{
    namespace value_mutation_detail
    {
        template <typename TOptions>
        inline bool validateOptions(const TOptions& options)
        {
            if (options.fileNo > 0x1FU)
            {
                return false;
            }

            if (options.value < 0)
            {
                return false;
            }

            return options.communicationSettings == 0x00U ||
                   options.communicationSettings == 0x01U ||
                   options.communicationSettings == 0x03U ||
                   options.communicationSettings == 0xFFU;
        }

        template <typename TOptions>
        inline uint8_t resolveCommunicationSettings(const TOptions& options, const DesfireContext& context)
        {
            if (options.communicationSettings != 0xFFU)
            {
                return options.communicationSettings;
            }

            return context.authenticated ? 0x03U : 0x00U;
        }

        inline void appendLe32(DesfireRequest& request, int32_t value)
        {
            const uint32_t rawValue = static_cast<uint32_t>(value);
            request.data.push_back(static_cast<uint8_t>(rawValue & 0xFFU));
            request.data.push_back(static_cast<uint8_t>((rawValue >> 8U) & 0xFFU));
            request.data.push_back(static_cast<uint8_t>((rawValue >> 16U) & 0xFFU));
            request.data.push_back(static_cast<uint8_t>((rawValue >> 24U) & 0xFFU));
        }

        template <typename TOptions>
        inline etl::expected<DesfireRequest, error::Error> buildRequest(
            uint8_t commandCode,
            const TOptions& options,
            const DesfireContext& context,
            bool& updateContextIv,
            etl::vector<uint8_t, 16>& requestState)
        {
            DesfireRequest request;
            request.commandCode = commandCode;
            request.data.clear();

            const uint8_t communicationSettings = resolveCommunicationSettings(options, context);
            if (communicationSettings == 0x03U)
            {
                auto protectionResult = SecureMessagingPolicy::protectValueOperationRequest(
                    context,
                    commandCode,
                    options.fileNo,
                    options.value);
                if (!protectionResult)
                {
                    return etl::unexpected(protectionResult.error());
                }

                request.data.push_back(options.fileNo);
                const auto& protection = protectionResult.value();
                const auto& payload = protection.encryptedPayload;
                for (size_t i = 0; i < payload.size(); ++i)
                {
                    request.data.push_back(payload[i]);
                }
                requestState.clear();
                for (size_t i = 0; i < protection.requestState.size(); ++i)
                {
                    requestState.push_back(protection.requestState[i]);
                }
                updateContextIv = true;
            }
            else
            {
                if (communicationSettings == 0x01U)
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
                }

                updateContextIv = false;
                requestState.clear();

                request.data.push_back(options.fileNo);
                appendLe32(request, options.value);
            }

            request.expectedResponseLength = 0U;
            return request;
        }

        inline etl::expected<DesfireResult, error::Error> parseResponse(
            const etl::ivector<uint8_t>& response,
            DesfireContext& context,
            bool updateContextIv,
            const etl::ivector<uint8_t>& requestState)
        {
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

            if (result.isSuccess() && updateContextIv)
            {
                auto ivUpdateResult = SecureMessagingPolicy::updateContextIvForValueOperationResponse(
                    context,
                    response,
                    requestState);
                if (!ivUpdateResult)
                {
                    return etl::unexpected(ivUpdateResult.error());
                }
            }

            return result;
        }
    } // namespace value_mutation_detail
} // namespace nfc
