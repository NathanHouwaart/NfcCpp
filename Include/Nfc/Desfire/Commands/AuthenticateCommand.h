/**
 * @file AuthenticateCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire authenticate command
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
#include <etl/vector.h>

namespace nfc
{
    /**
     * @brief Authenticate command options
     */
    struct AuthenticateCommandOptions
    {
        DesfireAuthMode mode;
        uint8_t keyNo;
        etl::vector<uint8_t, 24> key;
    };

    /**
     * @brief Authenticate command
     * 
     * Performs DESFire authentication (Legacy/ISO/AES)
     */
    class AuthenticateCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Authentication stage
         */
        enum class Stage : uint8_t
        {
            Initial,
            ChallengeSent,
            ResponseSent,
            Complete
        };

        /**
         * @brief Construct authenticate command
         * 
         * @param options Authentication options
         */
        explicit AuthenticateCommand(const AuthenticateCommandOptions& options);

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
         * @brief Generate authentication response
         */
        void generateAuthResponse();

        /**
         * @brief Verify authentication confirmation
         * 
         * @param response Response data
         * @return true Verification successful
         * @return false Verification failed
         */
        bool verifyAuthConfirmation(const etl::ivector<uint8_t>& response);

        /**
         * @brief Decrypt challenge from card
         */
        void decryptChallenge();

        /**
         * @brief Encrypt data
         * 
         * @param plaintext Input plaintext
         * @param ciphertext Output ciphertext
         */
        void encrypt(const etl::ivector<uint8_t>& plaintext, etl::vector<uint8_t, 32>& ciphertext);

        /**
         * @brief Decrypt data
         * 
         * @param ciphertext Input ciphertext
         * @param plaintext Output plaintext
         */
        void decrypt(const etl::ivector<uint8_t>& ciphertext, etl::vector<uint8_t, 32>& plaintext);

        AuthenticateCommandOptions options;
        Stage stage;
        etl::vector<uint8_t, 16> encryptedChallenge;
        etl::vector<uint8_t, 32> encryptedResponse;
        etl::vector<uint8_t, 16> rndA;
        etl::vector<uint8_t, 16> rndB;
        etl::vector<uint8_t, 8> currentIv;  // Current IV for CBC operations
    };

} // namespace nfc
