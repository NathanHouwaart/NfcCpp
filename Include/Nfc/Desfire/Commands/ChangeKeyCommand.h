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
#include <etl/vector.h>
#include <etl/optional.h>
#include <etl/utility.h>

namespace nfc
{
    /**
     * @brief Change key command options
     */
    struct ChangeKeyCommandOptions
    {
        uint8_t keyNo;
        DesfireKeyType newKeyType;
        etl::vector<uint8_t, 24> newKey;
        uint8_t newKeyVersion;
        etl::optional<etl::vector<uint8_t, 24>> oldKey;
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
         * @return etl::vector<uint8_t, 48> Key cryptogram
         */
        etl::vector<uint8_t, 48> buildKeyCryptogram(const DesfireContext& context);

        /**
         * @brief Get key size for key type
         * 
         * @return size_t Key size in bytes
         */
        size_t getKeySize() const;

        /**
         * @brief Derive session keys from master key
         * 
         * @param masterKey Master key
         * @param keyType Key type
         * @return etl::pair<etl::vector<uint8_t, 24>, etl::vector<uint8_t, 24>> Encryption and MAC keys
         */
        etl::pair<etl::vector<uint8_t, 24>, etl::vector<uint8_t, 24>> deriveSessionKeys(
            const etl::vector<uint8_t, 24>& masterKey,
            DesfireKeyType keyType);

        ChangeKeyCommandOptions options;
        Stage stage;
    };

} // namespace nfc
