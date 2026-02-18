/**
 * @file DeleteFileCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire delete file command
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
     * @brief Delete file command
     *
     * Deletes a file in the currently selected application (INS 0xDF)
     * using payload FileNo(1).
     */
    class DeleteFileCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Construct delete file command
         *
         * @param fileNo File number (0..31)
         */
        explicit DeleteFileCommand(uint8_t fileNo);

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
         * @brief Check if complete
         *
         * @return true Command complete
         * @return false Command not complete
         */
        bool isComplete() const override;

        /**
         * @brief Reset state
         */
        void reset() override;

    private:
        uint8_t fileNo;
        bool complete;
        etl::vector<uint8_t, 16> requestIv;
        bool hasRequestIv;
    };

} // namespace nfc
