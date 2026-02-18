/**
 * @file GetCardUidCommand.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire get card UID command
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
     * @brief GetCardUID command
     *
     * Executes DESFire GetCardUID (INS 0x51). The card returns the real UID
     * (7 bytes) when permitted. In authenticated sessions the response can be
     * encrypted; this command attempts to decode and validate CRC.
     */
    class GetCardUidCommand : public IDesfireCommand
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
         * @brief Construct GetCardUID command
         */
        GetCardUidCommand();

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
         * @brief Get parsed UID
         *
         * @return const etl::vector<uint8_t, 10>& UID bytes
         */
        const etl::vector<uint8_t, 10>& getUid() const;

    private:
        enum class SessionCipher : uint8_t
        {
            DES,
            DES3_2K,
            DES3_3K,
            AES,
            UNKNOWN
        };

        SessionCipher resolveSessionCipher(const DesfireContext& context) const;

        uint16_t calculateCrc16(const etl::ivector<uint8_t>& data) const;
        uint32_t calculateCrc32Desfire(const etl::ivector<uint8_t>& data) const;

        bool tryDecodeEncryptedUid(
            const etl::ivector<uint8_t>& payload,
            DesfireContext& context,
            etl::vector<uint8_t, 10>& outUid);

        bool decryptPayload(
            const etl::ivector<uint8_t>& ciphertext,
            const DesfireContext& context,
            SessionCipher cipher,
            etl::vector<uint8_t, 32>& plaintext) const;

        Stage stage;
        etl::vector<uint8_t, 10> uid;
    };

} // namespace nfc

