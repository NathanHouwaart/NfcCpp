/**
 * @file ChangeFileSettingsCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire change file settings command
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "../IDesfireCommand.h"
#include "../DesfireAuthMode.h"
#include "../DesfireKeyType.h"
#include <etl/vector.h>

namespace nfc
{
    /**
     * @brief ChangeFileSettings command options
     */
    struct ChangeFileSettingsCommandOptions
    {
        uint8_t fileNo = 0x00U;
        uint8_t communicationSettings = 0x00U; // New file comm mode: 0x00/0x01/0x03
        uint8_t readAccess = 0x00U;            // 0x0..0xF
        uint8_t writeAccess = 0x00U;           // 0x0..0xF
        uint8_t readWriteAccess = 0x00U;       // 0x0..0xF
        uint8_t changeAccess = 0x00U;          // 0x0..0xF

        // How the command itself is protected:
        // 0x00 plain, 0x01 MAC (not implemented), 0x03 enc, 0xFF auto.
        uint8_t commandCommunicationSettings = 0xFFU;

        // Optional overrides for ambiguous session-key interpretation.
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        DesfireKeyType sessionKeyType = DesfireKeyType::UNKNOWN;
    };

    /**
     * @brief ChangeFileSettings command
     *
     * Executes DESFire ChangeFileSettings (INS 0x5F).
     *
     * Payload structure (logical):
     * - fileNo (1 byte)
     * - communication settings (1 byte)
     * - access rights byte 1 (RW/CAR)
     * - access rights byte 2 (R/W)
     */
    class ChangeFileSettingsCommand : public IDesfireCommand
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
         * @brief Construct ChangeFileSettings command
         *
         * @param options Command options
         */
        explicit ChangeFileSettingsCommand(const ChangeFileSettingsCommandOptions& options);

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
        enum class SessionCipher : uint8_t
        {
            DES,
            DES3_2K,
            DES3_3K,
            AES,
            UNKNOWN
        };

        bool validateOptions() const;
        bool buildAccessRightsBytes(uint8_t& access1, uint8_t& access2) const;
        uint8_t resolveCommandCommunicationSettings(const DesfireContext& context) const;
        SessionCipher resolveSessionCipher(const DesfireContext& context) const;
        bool useLegacySendMode(SessionCipher cipher) const;
        bool deriveAesPlainRequestIv(
            const DesfireContext& context,
            uint8_t access1,
            uint8_t access2,
            etl::vector<uint8_t, 16>& outIv) const;

        etl::expected<etl::vector<uint8_t, 32>, error::Error> buildEncryptedPayload(
            const DesfireContext& context,
            uint8_t access1,
            uint8_t access2);

        ChangeFileSettingsCommandOptions options;
        Stage stage;
        uint8_t activeCommandCommunicationSettings;
        SessionCipher sessionCipher;
        etl::vector<uint8_t, 16> pendingIv;
        bool updateContextIv;
        etl::vector<uint8_t, 16> requestIv;
        bool hasRequestIv;
    };

} // namespace nfc
