/**
 * @file ChangeKeyCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire change key command
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "../IDesfireCommand.h"
#include "../DesfireKeyType.h"
#include "../DesfireAuthMode.h"
#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include <etl/vector.h>
#include <etl/optional.h>

namespace nfc
{
    enum class ChangeKeyLegacyIvMode : uint8_t
    {
        Zero,               // Classic legacy SEND_MODE with zero seed IV
        SessionEncryptedRndB // Seed SEND_MODE with first 8 bytes of encrypted RndB from auth
    };

    /**
     * @brief Change key command options
     */
    struct ChangeKeyCommandOptions
    {
        uint8_t keyNo;
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        DesfireKeyType sessionKeyType = DesfireKeyType::UNKNOWN;
        DesfireKeyType newKeyType;
        DesfireKeyType oldKeyType = DesfireKeyType::UNKNOWN;
        etl::vector<uint8_t, 24> newKey;
        uint8_t newKeyVersion;
        etl::optional<etl::vector<uint8_t, 24>> oldKey;
        ChangeKeyLegacyIvMode legacyIvMode = ChangeKeyLegacyIvMode::Zero;
    };

    /**
     * @brief Change key command
     * 
     * Changes a key in the current application or PICC
     */
    class ChangeKeyCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Change key stage
         */
        enum class Stage : uint8_t
        {
            Initial,
            Complete
        };

        /**
         * @brief Construct change key command
         * 
         * @param options Change key options
         */
        explicit ChangeKeyCommand(const ChangeKeyCommandOptions& options);

        /**
         * @brief Get command name
         * 
         * @return etl::string_view Command name
         */
        etl::string_view name() const override;

        /**
         * @brief Build the request
         * 
         * @param context DESFire context
         * @return etl::expected<DesfireRequest, error::Error> Request or error
         */
        etl::expected<DesfireRequest, error::Error> buildRequest(const DesfireContext& context) override;

        /**
         * @brief Parse the response
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
         * @return true Command completed
         * @return false More frames needed
         */
        bool isComplete() const override;

        /**
         * @brief Reset command state
         */
        void reset() override;

    private:
        /**
         * @brief Build key cryptogram
         * 
         * @param context DESFire context
         * @return etl::expected<etl::vector<uint8_t, 48>, error::Error> Key cryptogram or error
         */
        etl::expected<etl::vector<uint8_t, 48>, error::Error> buildKeyCryptogram(const DesfireContext& context);

        /**
         * @brief Get key size for key type
         * 
         * @return size_t Key size in bytes
         */
        size_t getKeySize(DesfireKeyType keyType) const;

        /**
         * @brief Normalize key material based on key type
         * 
         * @param keyData Key bytes
         * @return etl::expected<etl::vector<uint8_t, 24>, error::Error> Normalized key material or error
         */
        etl::expected<etl::vector<uint8_t, 24>, error::Error> normalizeKeyMaterial(
            const etl::ivector<uint8_t>& keyData,
            DesfireKeyType keyType) const;

        /**
         * @brief Determine session cipher used to encrypt the cryptogram
         * 
         * @param context DESFire context
         * @return SessionCipher Session cipher
         */
        SecureMessagingPolicy::SessionCipher resolveSessionCipher(const DesfireContext& context) const;

        /**
         * @brief Compute DESFire CRC16
         * 
         * @param data Input data
         * @return uint16_t CRC16 value
         */
        uint16_t calculateCrc16(const etl::ivector<uint8_t>& data) const;

        /**
         * @brief Compute DESFire CRC32 (inverted standard CRC32)
         * 
         * @param data Input data
         * @return uint32_t CRC32 value
         */
        uint32_t calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const;

        /**
         * @brief Check if legacy SEND_MODE cryptogram encryption should be used
         * 
         * @param cipher Session cipher
         * @return true Use legacy SEND_MODE decrypt pipeline
         * @return false Use normal CBC encryption
         */
        bool useLegacySendMode(SecureMessagingPolicy::SessionCipher cipher) const;

        ChangeKeyCommandOptions options;
        Stage stage;
        etl::vector<uint8_t, 16> pendingIv;
        bool updateContextIv;
        bool sameKeyChange;
        uint8_t effectiveKeyNo;
    };

} // namespace nfc
