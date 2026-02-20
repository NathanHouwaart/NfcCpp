/**
 * @file ChangeKeySettingsCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire change key settings command
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "../IDesfireCommand.h"
#include "../DesfireAuthMode.h"
#include "../DesfireKeyType.h"
#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include <etl/vector.h>

namespace nfc
{
    /**
     * @brief ChangeKeySettings command options
     */
    struct ChangeKeySettingsCommandOptions
    {
        uint8_t keySettings;
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        DesfireKeyType sessionKeyType = DesfireKeyType::UNKNOWN;
    };

    /**
     * @brief ChangeKeySettings command
     *
     * Changes KeySettings1 (command 0x54) of the currently selected PICC/application.
     */
    class ChangeKeySettingsCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Command stage
         */
        enum class Stage : uint8_t
        {
            Initial,
            Complete
        };

        /**
         * @brief Construct change key settings command
         *
         * @param options Command options
         */
        explicit ChangeKeySettingsCommand(const ChangeKeySettingsCommandOptions& options);

        /**
         * @brief Get command name
         *
         * @return etl::string_view Command name
         */
        etl::string_view name() const override;

        /**
         * @brief Build request
         *
         * @param context DESFire context
         * @return etl::expected<DesfireRequest, error::Error> Request or error
         */
        etl::expected<DesfireRequest, error::Error> buildRequest(const DesfireContext& context) override;

        /**
         * @brief Parse response
         *
         * @param response Response data
         * @param context DESFire context
         * @return etl::expected<DesfireResult, error::Error> Result or error
         */
        etl::expected<DesfireResult, error::Error> parseResponse(
            const etl::ivector<uint8_t>& response,
            DesfireContext& context) override;

        /**
         * @brief Check if command is complete
         *
         * @return true Command complete
         * @return false Command not complete
         */
        bool isComplete() const override;

        /**
         * @brief Reset command state
         */
        void reset() override;

    private:
        etl::expected<etl::vector<uint8_t, 32>, error::Error> buildEncryptedPayload(const DesfireContext& context);

        SecureMessagingPolicy::SessionCipher resolveSessionCipher(const DesfireContext& context) const;
        bool useLegacySendMode(SecureMessagingPolicy::SessionCipher cipher) const;
        uint16_t calculateCrc16(const etl::ivector<uint8_t>& data) const;
        uint32_t calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const;

        ChangeKeySettingsCommandOptions options;
        Stage stage;
        etl::vector<uint8_t, 16> pendingIv;
        bool updateContextIv;
    };

} // namespace nfc
