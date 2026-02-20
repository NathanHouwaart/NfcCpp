/**
 * @file SetConfigurationCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire SetConfiguration command
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
     * @brief SetConfiguration subcommand selector
     */
    enum class SetConfigurationSubcommand : uint8_t
    {
        PiccConfiguration = 0x00, // Configure PICC behavior flags
        AtsConfiguration = 0x01   // Configure ATS bytes
    };

    /**
     * @brief SetConfiguration command options
     */
    struct SetConfigurationCommandOptions
    {
        SetConfigurationSubcommand subcommand = SetConfigurationSubcommand::PiccConfiguration;
        uint8_t piccConfiguration = 0x00;
        etl::vector<uint8_t, 32> ats;
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        DesfireKeyType sessionKeyType = DesfireKeyType::UNKNOWN;
    };

    /**
     * @brief SetConfiguration command
     *
     * Executes DESFire SetConfiguration (INS 0x5C).
     * The first request byte is the subcommand selector, followed by encrypted payload.
     */
    class SetConfigurationCommand : public IDesfireCommand
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
         * @brief Construct SetConfiguration command
         *
         * @param options Command options
         */
        explicit SetConfigurationCommand(const SetConfigurationCommandOptions& options);

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
        etl::expected<etl::vector<uint8_t, 64>, error::Error> buildEncryptedPayload(const DesfireContext& context);

        SecureMessagingPolicy::SessionCipher resolveSessionCipher(const DesfireContext& context) const;
        bool useLegacySendMode(SecureMessagingPolicy::SessionCipher cipher) const;
        uint32_t calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const;

        SetConfigurationCommandOptions options;
        Stage stage;
        etl::vector<uint8_t, 16> pendingIv;
        bool updateContextIv;
    };

} // namespace nfc
