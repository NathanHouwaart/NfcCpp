/**
 * @file PlainPipe.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Plain pipe implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Nfc/Desfire/PlainPipe.h"

using namespace nfc;

etl::vector<uint8_t, 261> PlainPipe::wrap(const etl::ivector<uint8_t>& pdu)
{
    // Pass through - no security
    etl::vector<uint8_t, 261> result;
    result.assign(pdu.begin(), pdu.end());
    return result;
}

etl::expected<etl::vector<uint8_t, 256>, error::Error> PlainPipe::unwrap(const etl::ivector<uint8_t>& apdu)
{
    // Pass through - no security
    etl::vector<uint8_t, 256> result;
    result.assign(apdu.begin(), apdu.end());
    return result;
}
