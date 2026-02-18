/**
 * @file DebitCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire debit command
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <cstdint>
#include <etl/vector.h>
#include "../IDesfireCommand.h"
#include "ValueOperationCryptoUtils.h"

namespace nfc
{
    /**
     * @brief Debit command options
     */
    struct DebitCommandOptions
    {
        uint8_t fileNo;
        int32_t value;
        uint8_t communicationSettings = 0xFFU; // 0x00 plain, 0x01 mac, 0x03 enc, 0xFF auto
    };

    /**
     * @brief Debit command
     *
     * Executes DESFire Debit (INS 0xDC).
     */
    class DebitCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Construct debit command
         *
         * @param options Command options
         */
        explicit DebitCommand(const DebitCommandOptions& options);

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
         * @return true Command completed
         * @return false More frames needed
         */
        bool isComplete() const override;

        /**
         * @brief Reset command state
         */
        void reset() override;

    private:
        DebitCommandOptions options;
        bool complete;
        valueop_detail::SessionCipher sessionCipher;
        bool updateContextIv;
        etl::vector<uint8_t, 16> pendingIv;
    };

} // namespace nfc
