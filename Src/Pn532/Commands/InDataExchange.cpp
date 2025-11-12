/**
 * @file InDataExchange.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief InDataExchange command implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Commands/InDataExchange.h"
#include "Error/Pn532Error.h"

using namespace error;

namespace pn532
{
    const char* inDataExchangeStatusToString(InDataExchangeStatus status)
    {
        switch (status)
        {
        case InDataExchangeStatus::Success: return "Success";
        case InDataExchangeStatus::Timeout: return "Timeout - target has not answered";
        case InDataExchangeStatus::CrcError: return "CRC error detected by CIU";
        case InDataExchangeStatus::ParityError: return "Parity error detected by CIU";
        case InDataExchangeStatus::ErroneousBitCount: return "Erroneous Bit Count during anti-collision/select";
        case InDataExchangeStatus::MifareFramingError: return "Framing error during Mifare operation";
        case InDataExchangeStatus::BitCollisionError: return "Abnormal bit-collision during anti-collision";
        case InDataExchangeStatus::BufferSizeInsufficient: return "Communication buffer size insufficient";
        case InDataExchangeStatus::RfBufferOverflow: return "RF Buffer overflow detected";
        case InDataExchangeStatus::RfFieldNotSwitched: return "RF field not switched on in time";
        case InDataExchangeStatus::RfProtocolError: return "RF Protocol error";
        case InDataExchangeStatus::TemperatureError: return "Temperature error";
        case InDataExchangeStatus::InternalBufferOverflow: return "Internal buffer overflow";
        case InDataExchangeStatus::InvalidParameter: return "Invalid parameter";
        case InDataExchangeStatus::DepCommandNotSupported: return "DEP command not supported in target mode";
        case InDataExchangeStatus::DataFormatMismatch: return "Data format does not match specification";
        case InDataExchangeStatus::AuthenticationError: return "Mifare authentication error";
        case InDataExchangeStatus::UidCheckByteWrong: return "UID Check byte is wrong";
        case InDataExchangeStatus::InvalidDeviceState: return "Invalid device state";
        case InDataExchangeStatus::OperationNotAllowed: return "Operation not allowed in this configuration";
        case InDataExchangeStatus::CommandNotAcceptable: return "Command not acceptable in current context";
        case InDataExchangeStatus::TargetReleased: return "Target has been released by initiator";
        case InDataExchangeStatus::CardIdMismatch: return "Card ID does not match";
        case InDataExchangeStatus::CardDisappeared: return "Previously activated card has disappeared";
        case InDataExchangeStatus::Nfcid3Mismatch: return "NFCID3 initiator/target mismatch";
        case InDataExchangeStatus::OverCurrent: return "Over-current event detected";
        case InDataExchangeStatus::NadMissing: return "NAD missing in DEP frame";
        default: return "Unknown status";
        }
    }

    InDataExchange::InDataExchange(const InDataExchangeOptions& opts)
        : options(opts), cachedStatusByte(0xFF)
    {
    }

    etl::string_view InDataExchange::name() const
    {
        return "InDataExchange";
    }

    CommandRequest InDataExchange::buildRequest()
    {
        // Build payload: [Tg] [DataOut...]
        etl::vector<uint8_t, 256> payload;
        payload.push_back(options.targetNumber);
        
        // Add command payload
        for (size_t i = 0; i < options.payload.size(); ++i)
        {
            payload.push_back(options.payload[i]);
        }
        
        return createCommandRequest(0x40, payload, options.responseTimeoutMs); // 0x40 = InDataExchange
    }

    etl::expected<CommandResponse, Error> InDataExchange::parseResponse(const Pn532ResponseFrame& frame)
    {
        const auto& data = frame.data();
        
        // InDataExchange response format: [Status] [DataIn...]
        if (data.empty())
        {
            return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
        }

        // Extract status byte
        cachedStatusByte = data[0];
        
        // Extract response data from card (everything after status byte)
        cachedResponse.clear();
        if (data.size() > 1)
        {
            cachedResponse.assign(data.begin() + 1, data.end());
        }
        
        // NOTE: We return OK even if card-level status indicates error.
        // This separates transport errors (PN532 communication) from card errors.
        // Callers should check isSuccess() or getStatus() for card-level errors.
        
        return createCommandResponse(frame.getCommandCode(), data);
    }

    bool InDataExchange::expectsDataFrame() const
    {
        return true;
    }

    uint8_t InDataExchange::getStatusByte() const
    {
        return cachedStatusByte;
    }

    InDataExchangeStatus InDataExchange::getStatus() const
    {
        return static_cast<InDataExchangeStatus>(cachedStatusByte);
    }

    const char* InDataExchange::getStatusString() const
    {
        return inDataExchangeStatusToString(getStatus());
    }

    bool InDataExchange::isSuccess() const
    {
        return cachedStatusByte == static_cast<uint8_t>(InDataExchangeStatus::Success);
    }

    const etl::ivector<uint8_t>& InDataExchange::getResponseData() const
    {
        return cachedResponse;
    }

} // namespace pn532
