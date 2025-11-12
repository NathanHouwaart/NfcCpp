/**
 * @file GetGeneralStatus.h
 * @author your name (you@domain.com)
 * @brief GetGeneralStatus command
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "Pn532/IPn532Command.h"
#include <etl/string.h>
#include <cstdint>

namespace pn532
{

    /**
     * @brief General status structure
     * 
     */
    struct GeneralStatus
    {
        uint8_t err;            // Last error code detected
        uint8_t field;          // External field present status
        uint8_t nbTg;           // Number of tags connected

        etl::array<uint8_t, 4> tg1;  // Status of tag 1
        etl::array<uint8_t, 4> tg2;  // Status of tag 2

        uint8_t samStatus;      // SAM status
        
        /**
         * @brief Get string representation
         * 
         * @return etl::string<255> String representation
         */
        etl::string<255> toString() const;
    };

    /**
     * @brief Get general status command
     * 
     */
    class GetGeneralStatus : public IPn532Command
    {
    public:
        GetGeneralStatus() = default;

        etl::string_view name() const override;
        CommandRequest buildRequest() override;
        etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) override;
        bool expectsDataFrame() const override;

        /**
         * @brief Get the cached general status
         * 
         * @return const GeneralStatus& General status
         */
        const GeneralStatus& getGeneralStatus() const;

    private:
        GeneralStatus cachedStatus;
    };
} // namespace pn532