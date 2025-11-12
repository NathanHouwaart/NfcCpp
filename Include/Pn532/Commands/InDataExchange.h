/**
 * @file InDataExchange.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief InDataExchange command for APDU communication
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "Pn532/IPn532Command.h"
#include <etl/vector.h>
#include <cstdint>

namespace pn532
{
    /**
     * @brief InDataExchange status codes
     * 
     */
    enum class InDataExchangeStatus : uint8_t
    {
        Success = 0x00,
        Timeout = 0x01,
        CrcError = 0x02,
        ParityError = 0x03,
        ErroneousBitCount = 0x04,
        MifareFramingError = 0x05,
        BitCollisionError = 0x06,
        BufferSizeInsufficient = 0x07,
        RfBufferOverflow = 0x09,
        RfFieldNotSwitched = 0x0A,
        RfProtocolError = 0x0B,
        TemperatureError = 0x0D,
        InternalBufferOverflow = 0x0E,
        InvalidParameter = 0x10,
        DepCommandNotSupported = 0x12,
        DataFormatMismatch = 0x13,
        AuthenticationError = 0x14,
        UidCheckByteWrong = 0x23,
        InvalidDeviceState = 0x25,
        OperationNotAllowed = 0x26,
        CommandNotAcceptable = 0x27,
        TargetReleased = 0x29,
        CardIdMismatch = 0x2A,
        CardDisappeared = 0x2B,
        Nfcid3Mismatch = 0x2C,
        OverCurrent = 0x2D,
        NadMissing = 0x2E
    };

    /**
     * @brief Convert InDataExchangeStatus to string
     * 
     * @param status Status code
     * @return const char* Status description
     */
    const char* inDataExchangeStatusToString(InDataExchangeStatus status);

    /**
     * @brief InDataExchange command options
     * 
     */
    struct InDataExchangeOptions
    {
        uint8_t targetNumber;
        etl::vector<uint8_t, 255> payload;
        uint32_t responseTimeoutMs;

        InDataExchangeOptions() 
            : targetNumber(0x01), responseTimeoutMs(1000) {}
    };

    /**
     * @brief InDataExchange command
     * 
     */
    class InDataExchange : public IPn532Command
    {
    public:
        explicit InDataExchange(const InDataExchangeOptions& opts);

        etl::string_view name() const override;
        CommandRequest buildRequest() override;
        etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) override;
        bool expectsDataFrame() const override;

        /**
         * @brief Get the status byte from the response
         * 
         * @return uint8_t Status byte
         */
        uint8_t getStatusByte() const;

        /**
         * @brief Get the status as enum
         * 
         * @return InDataExchangeStatus Status enum
         */
        InDataExchangeStatus getStatus() const;

        /**
         * @brief Get status as string
         * 
         * @return const char* Status description
         */
        const char* getStatusString() const;

        /**
         * @brief Check if exchange was successful
         * 
         * @return bool True if success (status == 0x00)
         */
        bool isSuccess() const;

        /**
         * @brief Get response data from card
         * 
         * @return const etl::ivector<uint8_t>& Response data
         */
        const etl::ivector<uint8_t>& getResponseData() const;

    private:
        InDataExchangeOptions options;
        uint8_t cachedStatusByte;
        etl::vector<uint8_t, 255> cachedResponse;
    };

} // namespace pn532
