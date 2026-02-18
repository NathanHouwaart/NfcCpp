/**
 * @file GetKeyVersionCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get key version command
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
     * @brief GetKeyVersion command
     *
     * Executes DESFire GetKeyVersion (INS 0x64) for one key number.
     */
    class GetKeyVersionCommand : public IDesfireCommand
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
         * @brief Construct GetKeyVersion command
         *
         * @param keyNo Target key number
         */
        explicit GetKeyVersionCommand(uint8_t keyNo);

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
         * @brief Get parsed key version
         *
         * @return uint8_t Key version byte
         */
        uint8_t getKeyVersion() const;

        /**
         * @brief Get raw response payload bytes (without status)
         *
         * @return const etl::vector<uint8_t, 16>& Raw payload bytes
         */
        const etl::vector<uint8_t, 16>& getRawPayload() const;

    private:
        uint8_t keyNo;
        Stage stage;
        uint8_t keyVersion;
        etl::vector<uint8_t, 16> rawPayload;
    };

} // namespace nfc

