/**
 * @file NativeWire.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Native wire protocol implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Wire/NativeWire.h"

namespace nfc
{
    etl::vector<uint8_t, 261> NativeWire::wrap(const etl::ivector<uint8_t>& pdu)
    {
        // TODO: Implement native wrapping
        // Native mode typically just passes through the PDU
        etl::vector<uint8_t, 261> result(pdu.begin(), pdu.end());
        return result;
    }

    etl::expected<etl::vector<uint8_t, 256>, error::Error> NativeWire::unwrap(const etl::ivector<uint8_t>& apdu)
    {
        // TODO: Implement native unwrapping
        // Native mode typically just passes through the response
        etl::vector<uint8_t, 256> result(apdu.begin(), apdu.end());
        return result;
    }

} // namespace nfc
