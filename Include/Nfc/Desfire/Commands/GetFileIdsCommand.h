/**
 * @file GetFileIdsCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get file IDs command
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "../IDesfireCommand.h"
#include <etl/vector.h>

namespace nfc
{
    /**
     * @brief GetFileIDs command
     *
     * Executes DESFire GetFileIDs (INS 0x6F) and returns all active file IDs
     * in the selected application.
     */
    class GetFileIdsCommand : public IDesfireCommand
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
         * @brief Construct GetFileIDs command
         */
        GetFileIdsCommand();

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

        /**
         * @brief Get parsed file IDs
         *
         * @return const etl::vector<uint8_t, 32>& File IDs
         */
        const etl::vector<uint8_t, 32>& getFileIds() const;

        /**
         * @brief Get raw response payload bytes (without status)
         *
         * @return const etl::vector<uint8_t, 40>& Raw payload bytes
         */
        const etl::vector<uint8_t, 40>& getRawPayload() const;

    private:
        Stage stage;
        etl::vector<uint8_t, 40> rawPayload;
        etl::vector<uint8_t, 32> fileIds;
        etl::vector<uint8_t, 16> requestIv;
        bool hasRequestIv = false;
    };

} // namespace nfc
