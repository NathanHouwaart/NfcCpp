/**
 * @file EncPipe.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Encryption pipe implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Desfire/EncPipe.h"

using namespace nfc;

etl::vector<uint8_t, 261> EncPipe::wrap(const etl::ivector<uint8_t>& pdu)
{
    // TODO: Implement encryption wrapping
    // 1. Apply padding
    // 2. Encrypt with session key
    // 3. Append MAC
    etl::vector<uint8_t, 261> result;
    result.assign(pdu.begin(), pdu.end());
    return result;
}

etl::expected<etl::vector<uint8_t, 256>, error::Error> EncPipe::unwrap(const etl::ivector<uint8_t>& apdu)
{
    // TODO: Implement encryption unwrapping
    // 1. Verify MAC
    // 2. Decrypt with session key
    // 3. Remove padding
    etl::vector<uint8_t, 256> result;
    result.assign(apdu.begin(), apdu.end());
    return result;
}
