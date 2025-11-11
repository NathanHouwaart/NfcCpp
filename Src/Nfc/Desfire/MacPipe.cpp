/**
 * @file MacPipe.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief MAC pipe implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Desfire/MacPipe.h"

using namespace nfc;

etl::vector<uint8_t, 261> MacPipe::wrap(const etl::ivector<uint8_t>& pdu)
{
    // TODO: Implement MAC wrapping
    // 1. Calculate CMAC over PDU
    // 2. Append MAC to PDU
    etl::vector<uint8_t, 261> result;
    result.assign(pdu.begin(), pdu.end());
    return result;
}

etl::expected<etl::vector<uint8_t, 256>, error::Error> MacPipe::unwrap(const etl::ivector<uint8_t>& apdu)
{
    // TODO: Implement MAC unwrapping
    // 1. Extract MAC from end of APDU
    // 2. Verify MAC
    // 3. Return data without MAC
    etl::vector<uint8_t, 256> result;
    result.assign(apdu.begin(), apdu.end());
    return result;
}
