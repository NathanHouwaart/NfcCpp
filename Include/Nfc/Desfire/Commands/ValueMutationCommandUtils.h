/**
 * @file ValueMutationCommandUtils.h
 * @brief Shared helper utilities for Credit/Debit/LimitedCredit commands
 */

#pragma once

#include "Nfc/Desfire/Commands/ValueOperationCryptoUtils.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Nfc/Desfire/DesfireRequest.h"
#include "Nfc/Desfire/DesfireResult.h"
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
            valueop_detail::appendLe32(request.data, static_cast<uint32_t>(value));
        }

        template <typename TOptions>
        inline etl::expected<DesfireRequest, error::Error> buildRequest(
            uint8_t commandCode,
            const TOptions& options,
            const DesfireContext& context,
            valueop_detail::SessionCipher& sessionCipher,
            bool& updateContextIv,
            etl::vector<uint8_t, 16>& pendingIv)
        {
            DesfireRequest request;
            request.commandCode = commandCode;
            request.data.clear();

            const uint8_t communicationSettings = resolveCommunicationSettings(options, context);
            if (communicationSettings == 0x03U)
            {
                if (!context.authenticated || context.sessionKeyEnc.empty())
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::AuthenticationError));
                }

                sessionCipher = valueop_detail::resolveSessionCipher(context);
                if (sessionCipher == valueop_detail::SessionCipher::UNKNOWN)
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
                }

                auto encryptedPayloadResult = valueop_detail::buildEncryptedValuePayload(
                    commandCode,
                    options.fileNo,
                    options.value,
                    context,
                    sessionCipher,
                    pendingIv);
                if (!encryptedPayloadResult)
                {
                    return etl::unexpected(encryptedPayloadResult.error());
                }

                request.data.push_back(options.fileNo);
                const auto& payload = encryptedPayloadResult.value();
                for (size_t i = 0; i < payload.size(); ++i)
                {
                    request.data.push_back(payload[i]);
                }
                updateContextIv = true;
            }
            else
            {
                if (communicationSettings == 0x01U)
                {
                    return etl::unexpected(error::Error::fromDesfire(error::DesfireError::InvalidState));
                }

                sessionCipher = valueop_detail::SessionCipher::UNKNOWN;
                updateContextIv = false;
                pendingIv.clear();

                request.data.push_back(options.fileNo);
                appendLe32(request, options.value);
            }

            request.expectedResponseLength = 0U;
            return request;
        }

        inline etl::expected<DesfireResult, error::Error> parseResponse(
            const etl::ivector<uint8_t>& response,
            DesfireContext& context,
            valueop_detail::SessionCipher sessionCipher,
            bool updateContextIv,
            const etl::ivector<uint8_t>& pendingIv)
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
                if (sessionCipher == valueop_detail::SessionCipher::AES)
                {
                    auto nextIvResult = valueop_detail::deriveAesResponseIvForValueOperation(response, context, pendingIv);
                    if (!nextIvResult)
                    {
                        return etl::unexpected(nextIvResult.error());
                    }

                    valueop_detail::setContextIv(context, nextIvResult.value());
                }
                else if (sessionCipher == valueop_detail::SessionCipher::DES3_3K)
                {
                    auto nextIvResult = valueop_detail::deriveTktdesResponseIvForValueOperation(response, context, pendingIv);
                    if (!nextIvResult)
                    {
                        return etl::unexpected(nextIvResult.error());
                    }

                    valueop_detail::setContextIv(context, nextIvResult.value());
                }
                else if (
                    sessionCipher == valueop_detail::SessionCipher::DES3_2K &&
                    !valueop_detail::useLegacyDesCryptoMode(context, sessionCipher))
                {
                    auto nextIvResult = valueop_detail::deriveTktdesResponseIvForValueOperation(response, context, pendingIv);
                    if (!nextIvResult)
                    {
                        return etl::unexpected(nextIvResult.error());
                    }

                    valueop_detail::setContextIv(context, nextIvResult.value());
                }
                else if (
                    sessionCipher == valueop_detail::SessionCipher::DES ||
                    (sessionCipher == valueop_detail::SessionCipher::DES3_2K &&
                     valueop_detail::useLegacyDesCryptoMode(context, sessionCipher)))
                {
                    // Legacy DES/2K3DES secure messaging uses command-local chaining.
                    // Keep IV reset between commands.
                    etl::vector<uint8_t, 8> zeroIv;
                    zeroIv.resize(8U, 0x00U);
                    valueop_detail::setContextIv(context, zeroIv);
                }
                else
                {
                    valueop_detail::setContextIv(context, pendingIv);
                }
            }

            return result;
        }
    } // namespace value_mutation_detail
} // namespace nfc
