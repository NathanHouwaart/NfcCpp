/**
 * @file Pn532RequestFrame.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief PN532 request frame builder implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Pn532RequestFrame.h"
#include "Error/Pn532Error.h"

using namespace pn532;
using namespace error;
using namespace nfc::buffer;

etl::expected<etl::vector<uint8_t, PN532_FRAME_MAX>, Error> 
Pn532RequestFrame::build(const CommandRequest& request)
{
    etl::vector<uint8_t, PN532_FRAME_MAX> frame;

    // Calculate frame length: TFI (1) + Command Code (1) + Data (n)
    const size_t dataLength = request.size();
    const uint8_t frameLength = 2 + dataLength; // TFI + CMD + data

    // Check if frame fits in maximum size
    if (frameLength > PN532_DATA_MAX)
    {
        return etl::unexpected(Error::fromPn532(Pn532Error::InvalidParameter));
    }

    // 1. Preamble
    frame.push_back(PREAMBLE);

    // 2. Start codes
    frame.push_back(START_CODE_1);
    frame.push_back(START_CODE_2);

    // 3. Length
    frame.push_back(frameLength);

    // 4. Length checksum (LCS)
    frame.push_back(calculateLengthChecksum(frameLength));

    // 5. TFI (host to PN532)
    frame.push_back(TFI_HOST_TO_PN532);

    // 6. Command code
    frame.push_back(request.getCommandCode());

    // 7. Data payload
    const etl::ivector<uint8_t>& data = request.data();
    for (size_t i = 0; i < data.size(); ++i)
    {
        frame.push_back(data[i]);
    }

    // 8. Data checksum (DCS) - checksum of TFI + CMD + Data
    etl::vector<uint8_t, PN532_DATA_MAX> checksumData;
    checksumData.push_back(TFI_HOST_TO_PN532);
    checksumData.push_back(request.getCommandCode());
    for (size_t i = 0; i < data.size(); ++i)
    {
        checksumData.push_back(data[i]);
    }
    frame.push_back(calculateChecksum(checksumData));

    // 9. Postamble
    frame.push_back(POSTAMBLE);

    return frame;
}

etl::vector<uint8_t, 6> Pn532RequestFrame::buildAck()
{
    etl::vector<uint8_t, 6> ack;
    ack.push_back(0x00);
    ack.push_back(0x00);
    ack.push_back(0xFF);
    ack.push_back(0x00);
    ack.push_back(0xFF);
    ack.push_back(0x00);
    return ack;
}

etl::vector<uint8_t, 6> Pn532RequestFrame::buildNack()
{
    etl::vector<uint8_t, 6> nack;
    nack.push_back(0x00);
    nack.push_back(0x00);
    nack.push_back(0xFF);
    nack.push_back(0xFF);
    nack.push_back(0x00);
    nack.push_back(0x00);
    return nack;
}

uint8_t Pn532RequestFrame::calculateChecksum(const etl::ivector<uint8_t>& data)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < data.size(); ++i)
    {
        sum += data[i];
    }
    // DCS = ~sum + 1 (two's complement)
    return static_cast<uint8_t>(~sum + 1);
}

uint8_t Pn532RequestFrame::calculateLengthChecksum(uint8_t length)
{
    // LCS = ~LEN + 1 (two's complement)
    return static_cast<uint8_t>(~length + 1);
}
