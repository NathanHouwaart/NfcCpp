/**
 * @file CreateApplicationCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire create application command
 * @version 0.1
 * @date 2026-02-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "../IDesfireCommand.h"
#include "../DesfireKeyType.h"
#include <etl/array.h>

namespace nfc
{
    /**
     * @brief Create application command options
     */
    struct CreateApplicationCommandOptions
    {
        etl::array<uint8_t, 3> aid;
        uint8_t keySettings1;
        uint8_t keyCount;
        DesfireKeyType keyType;
    };

    /**
     * @brief Create application command
     *
     * Creates a DESFire application (INS 0xCA) using the basic 5-byte payload:
     * AID(3) + KeySettings1 + KeySettings2.
     */
    class CreateApplicationCommand : public IDesfireCommand
    {
    public:
        /**
         * @brief Construct create application command
         *
         * @param options Command options
         */
        explicit CreateApplicationCommand(const CreateApplicationCommandOptions& options);

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
        /**
         * @brief Encode KeySettings2 nibble flags from key count + type.
         *
         * @param out Encoded byte output
         * @return true Encoding successful
         * @return false Invalid key type/count
         */
        bool encodeKeySettings2(uint8_t& out) const;

        CreateApplicationCommandOptions options;
        bool complete;
    };

} // namespace nfc

