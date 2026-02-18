/**
 * @file CommitTransactionCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire commit transaction command
 * @version 0.1
 * @date 2026-02-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "../IDesfireCommand.h"

namespace nfc
{
    /**
     * @brief Commit transaction command
     *
     * Executes DESFire CommitTransaction (INS 0xC7).
     * Finalizes pending transactional updates (for example backup/value file updates).
     */
    class CommitTransactionCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Construct commit transaction command
         */
        CommitTransactionCommand();

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
        bool complete;
    };

} // namespace nfc

