/**
 * @file GetKeySettingsCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get key settings command
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
     * @brief GetKeySettings command
     *
     * Executes DESFire GetKeySettings (INS 0x45) and exposes parsed bytes:
     * - KeySettings1
     * - KeySettings2 (key count + key type flags)
     */
    class GetKeySettingsCommand : public IDesfireCommand
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
         * @brief Construct GetKeySettings command
         */
        GetKeySettingsCommand();

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
         * @brief Get KeySettings1 byte
         *
         * @return uint8_t KeySettings1
         */
        uint8_t getKeySettings1() const;

        /**
         * @brief Get KeySettings2 byte
         *
         * @return uint8_t KeySettings2
         */
        uint8_t getKeySettings2() const;

        /**
         * @brief Get key count (lower nibble of KeySettings2)
         *
         * @return uint8_t Number of keys in application/PICC
         */
        uint8_t getKeyCount() const;

        /**
         * @brief Get key type flag field (upper bits of KeySettings2)
         *
         * Values:
         * - 0x00: DES/2K3DES family
         * - 0x40: 3K3DES
         * - 0x80: AES
         *
         * @return uint8_t Key type flag field
         */
        uint8_t getKeyTypeFlags() const;

        /**
         * @brief Get raw response payload bytes (without status)
         *
         * @return const etl::vector<uint8_t, 32>& Raw payload bytes
         */
        const etl::vector<uint8_t, 32>& getRawPayload() const;

    private:
        Stage stage;
        uint8_t keySettings1;
        uint8_t keySettings2;
        uint8_t keyCount;
        uint8_t keyTypeFlags;
        etl::vector<uint8_t, 32> rawPayload;
    };

} // namespace nfc

