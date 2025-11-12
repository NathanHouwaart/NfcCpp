#include "Pn532/Pn532Driver.h"
#include "Pn532/Commands/GetFirmwareVersion.h"
#include "Pn532/Pn532ResponseFrame.h"
#include "Nfc/BufferSizes.h"
#include "Utils/Logging.h"
#include "Utils/Timing.h"

#include "Pn532/CommandRequest.h"
#include "Pn532/Pn532RequestFrame.h"
#include "Pn532/Pn532Constants.h"

using namespace error;
using namespace pn532;

// Constructor
Pn532Driver::Pn532Driver(comms::IHardwareBus &bus)
    : bus(bus)
{
}

// Initialization
void Pn532Driver::init()
{
}

// Command execution
etl::expected<CommandResponse, Error> Pn532Driver::executeCommand(IPn532Command &command)
{
    LOG_INFO("Executing command: %.*s", static_cast<int>(command.name().size()), command.name().data());

    // 1. Build the command request
    const CommandRequest request = command.buildRequest();

    // 2. Transceive the frame (handles protocol-level communication)
    auto responseFrameResult = this->transceive(request);
    if (!responseFrameResult)
    {
        LOG_ERROR("Failed to transceive frame");
        return etl::unexpected(responseFrameResult.error());
    }

    // 3. Parse the response frame into a command response (command-specific parsing)
    auto commandResponseResult = command.parseResponse(responseFrameResult.value());
    if (!commandResponseResult)
    {
        LOG_ERROR("Failed to parse command response");
        return etl::unexpected(commandResponseResult.error());
    }

    LOG_INFO("Command '%.*s' executed successfully", static_cast<int>(command.name().size()), command.name().data());
    return commandResponseResult.value();
}

etl::expected<Pn532ResponseFrame, Error> Pn532Driver::transceive(const CommandRequest &request)
{
    // 0. Build PN532 frame from request
    auto frameResult = Pn532RequestFrame::build(request);
    if (!frameResult)
    {
        LOG_ERROR("Failed to build PN532 frame");
        return etl::unexpected(frameResult.error());
    }

    const auto &frame = frameResult.value();
    LOG_HEX("INFO", "Sending frame", frame.data(), frame.size());

    // 1. Send the command
    auto sendResult = this->sendCommand(frame);
    if (!sendResult)
    {
        LOG_ERROR("Failed to send command");
        return etl::unexpected(sendResult.error());
    }

    // 2. Wait for response
    if (!waitForChip(DEFAULT_TIMEOUT_MS))
    {
        LOG_ERROR("Timeout waiting for PN532 response");
        return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
    }

    // 3. Read ACK frame
    etl::vector<uint8_t, nfc::buffer::PN532_FRAME_MAX> responseBuffer;
    auto result = bus.read(responseBuffer, ACK_FRAME.size());
    if (!result)
    {
        LOG_ERROR("Failed to read ACK frame");
        return etl::unexpected(result.error());
    }

    LOG_HEX("INFO", "Received ACK frame", responseBuffer.data(), responseBuffer.size());

    // 4. Validate ACK frame
    if (!checkAck(responseBuffer))
    {
        LOG_ERROR("Invalid ACK frame received");
        return etl::unexpected(Error::fromPn532(Pn532Error::InvalidAckFrame));
    }

    // 5. Wait for the rest of the response frame
    if (!waitForChip(DEFAULT_TIMEOUT_MS))
    {
        LOG_ERROR("Timeout waiting for PN532 response frame");
        return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
    }

    // 6. Read first part of response to determine length
    etl::vector<uint8_t, nfc::buffer::PN532_FRAME_MAX> fullResponseBuffer;
    result = bus.read(responseBuffer, 4);
    if (!result || result.value() != 4)
    {
        LOG_ERROR("Expected 4 bytes, got %zu", result.value_or(0));
        return etl::unexpected(result.error());
    }
    
    // Copy to fullResponseBuffer
    fullResponseBuffer.insert(fullResponseBuffer.end(), responseBuffer.begin(), responseBuffer.begin() + 4);

    // 7. Determine full frame length
    size_t frameLength = responseBuffer[3] + 3; // Length + TFI + CMD + checksum
    result = bus.read(responseBuffer, frameLength);
    if (!result)
    {
        LOG_ERROR("Failed to read full response frame");
        return etl::unexpected(result.error());
    }
    // Copy to fullResponseBuffer
    fullResponseBuffer.insert(fullResponseBuffer.end(), responseBuffer.begin(), responseBuffer.begin() + frameLength);

    LOG_HEX("INFO", "Received full response frame", fullResponseBuffer.data(), fullResponseBuffer.size());

    // 8. Parse the response frame
    auto parseResult = Pn532Driver::parseResponseFrame(fullResponseBuffer, request.getCommandCode()); // Command code is at index 1
    if (!parseResult)
    {
        LOG_ERROR("Failed to parse response frame");
        return etl::unexpected(parseResult.error());
    }

    return parseResult.value();
}

// Firmware and status
etl::expected<FirmwareInfo, Error> Pn532Driver::getFirmwareVersion()
{
    GetFirmwareVersion cmd;
    auto result = executeCommand(cmd);

    if (!result.has_value())
    {
        return etl::unexpected(result.error());
    }

    return cmd.getFirmwareInfo();
}

etl::expected<void, Error> Pn532Driver::performSelftest()
{
    // TODO: Implement self test
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

etl::expected<GeneralStatus, Error> Pn532Driver::getGeneralStatus()
{
    // TODO: Implement get general status
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

// Configuration
etl::expected<void, Error> Pn532Driver::setSamConfiguration(uint8_t mode)
{
    // TODO: Implement set SAM configuration
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

etl::expected<void, Error> Pn532Driver::setRfField(const bool state)
{
    // TODO: Implement set RF field
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

etl::expected<void, Error> Pn532Driver::setMaxRetries(const uint8_t maxRetries)
{
    // TODO: Implement set max retries
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

etl::expected<void, Error> Pn532Driver::setSerialBaudrate(Pn532Baudrate baudrate)
{
    // TODO: Implement set serial baudrate
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

// Register operations
etl::expected<void, Error> Pn532Driver::writeRegister(const uint16_t reg, const uint8_t val)
{
    // TODO: Implement write register
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

etl::expected<uint8_t, Error> Pn532Driver::readRegister(const uint16_t reg)
{
    // TODO: Implement read register
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

// GPIO operations
etl::expected<void, Error> Pn532Driver::writeGPIO(uint8_t newGpioState)
{
    // TODO: Implement write GPIO
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

etl::expected<uint8_t, Error> Pn532Driver::readGPIO()
{
    // TODO: Implement read GPIO
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

// Static utility
uint32_t Pn532Driver::calculateChecksum(const etl::ivector<uint8_t> &data)
{
    // TODO: Implement checksum calculation
    return 0;
}

// ==============================================================================
// Private methods
// ==============================================================================

etl::expected<void, Error> Pn532Driver::sendCommand(const etl::ivector<uint8_t> &data)
{
    auto result = this->wakeUp(); // Wake up the PN532
    if (!result)
    {
        return etl::unexpected(result.error());
    }

    return bus.write(data); // Send data to the bus
}

etl::expected<Pn532Response, Error> Pn532Driver::getResponse(uint8_t onCommand, uint32_t timeoutMs)
{
    // TODO: Implement get response

    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

etl::expected<void, Error> Pn532Driver::sendAndAcknowledgeCommand(uint8_t command)
{
    // TODO: Implement send and acknowledge command
    return etl::unexpected(Error::fromPn532(Pn532Error::Timeout));
}

etl::expected<void, Error> Pn532Driver::wakeUp()
{
    // Send a few dummy bytes to wake up the PN532
    etl::vector<uint8_t, 10> wakeupBytes = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return bus.write(wakeupBytes);
}

bool Pn532Driver::waitForChip(const int timeout)
{
    // Get the start time for timeout calculation
    const uint32_t start = utils::get_tick_ms();
    const uint32_t timeoutMs = static_cast<uint32_t>(timeout);
    const uint32_t pollIntervalMs = 2; // Poll every 2ms

    // Wait until timeout or data is available
    while (!utils::has_timeout(start, timeoutMs))
    {
        if (bus.available() > 0)
        {
            return true; // PN532 has pushed something into the RX queue
        }

        // Small delay between polls to avoid busy-waiting
        utils::delay_ms(pollIntervalMs);
    }

    // Timeout occurred
    LOG_ERROR("Timeout waiting for PN532 chip after %d ms", timeout);
    return false;
}

bool Pn532Driver::checkAck(const etl::ivector<uint8_t> &buffer)
{
    // ACK frame: 0x00 0x00 0xFF 0x00 0xFF 0x00
    if (buffer.size() < ACK_FRAME.size())
    {
        return false;
    }

    // Compare against expected ACK frame
    for (size_t i = 0; i < ACK_FRAME.size(); i++)
    {
        if (buffer[i] != ACK_FRAME[i])
        {
            return false;
        }
    }

    return true;
}

etl::expected<Pn532ResponseFrame, Error> Pn532Driver::parseResponseFrame(
    const etl::ivector<uint8_t> &frame,
    uint8_t sentCommandCode)
{
    // 1. Search for the 0x00 0x00 0xFF start sequence
    bool foundStartSequence = false;
    size_t index = 0;

    for (size_t i = 0; i + 2 < frame.size(); i++)
    {
        if (frame[i] == 0x00 && frame[i + 1] == 0x00 && frame[i + 2] == 0xFF)
        {
            index = i;
            foundStartSequence = true;
            break;
        }
    }

    if (!foundStartSequence)
    {
        LOG_ERROR("Start sequence 0x00 0x00 0xFF not found in buffer");
        return etl::unexpected(Error::fromPn532(Pn532Error::FrameCheckFailed));
    }

    // Skip past preamble and start code
    index += 3;

    // 2. Check we have enough bytes for length and length checksum
    if (index + 2 > frame.size())
    {
        LOG_ERROR("Frame too short for length bytes");
        return etl::unexpected(Error::fromPn532(Pn532Error::FrameCheckFailed));
    }

    // 3. Extract and validate packet length
    uint8_t packetLength = frame[index++];
    if (packetLength == 0 || packetLength > nfc::buffer::PN532_DATA_MAX)
    {
        LOG_ERROR("Invalid packet length: %u", packetLength);
        return etl::unexpected(Error::fromPn532(Pn532Error::FrameCheckFailed));
    }

    // 4. Validate length checksum (LEN + LCS should equal 0x00)
    uint8_t lengthChecksum = frame[index++];
    if (static_cast<uint8_t>(packetLength + lengthChecksum) != 0x00)
    {
        LOG_ERROR("Invalid length checksum");
        return etl::unexpected(Error::fromPn532(Pn532Error::FrameCheckFailed));
    }

    // 5. Check we have enough bytes for the packet data + checksum
    if (index + packetLength + 1 > frame.size()) // +1 for DCS (Data Checksum)
    {
        LOG_ERROR("Frame too short for packet data");
        return etl::unexpected(Error::fromPn532(Pn532Error::FrameCheckFailed));
    }

    // Store start of data section for checksum calculation
    size_t dataStartIndex = index;

    // 6. Validate direction byte (TFI - should be 0xD5 for device to host)
    uint8_t directionByte = frame[index++];
    if (directionByte != protocol::TFI_DEVICE_TO_HOST)
    {
        LOG_ERROR("Invalid direction byte: 0x%02X, expected 0xD5", directionByte);
        return etl::unexpected(Error::fromPn532(Pn532Error::FrameCheckFailed));
    }

    // 7. Extract and validate command code
    uint8_t commandCode = frame[index++];
    if (commandCode != static_cast<uint8_t>(sentCommandCode + protocol::RESPONSE_CODE_OFFSET))
    {
        LOG_ERROR("Command code mismatch: received 0x%02X, expected 0x%02X",
                  commandCode, static_cast<uint8_t>(sentCommandCode + protocol::RESPONSE_CODE_OFFSET));
        return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
    }

    // 8. Extract payload data
    // Payload length = packetLength - TFI (1 byte) - Command Code (1 byte)
    size_t payloadLength = packetLength - 2;
    etl::vector<uint8_t, Pn532ResponseFrame::MaxPayloadSize> payload;

    for (size_t i = 0; i < payloadLength; i++)
    {
        payload.push_back(frame[index++]);
    }

    // 9. Validate data checksum (DCS)
    uint8_t receivedChecksum = frame[index++];

    // Calculate checksum: TFI + PD0...PDn should sum to 0x00
    uint8_t calculatedChecksum = 0;
    for (size_t i = dataStartIndex; i < dataStartIndex + packetLength; i++)
    {
        calculatedChecksum += frame[i];
    }
    calculatedChecksum = (~calculatedChecksum) + 1; // Two's complement

    if (receivedChecksum != calculatedChecksum)
    {
        LOG_ERROR("Data checksum mismatch: received 0x%02X, calculated 0x%02X",
                  receivedChecksum, calculatedChecksum);
        return etl::unexpected(Error::fromPn532(Pn532Error::FrameCheckFailed));
    }

    // 10. Check postamble (0x00) if present
    if (index < frame.size() && frame[index] != 0x00)
    {
        LOG_WARN("Expected postamble 0x00, got 0x%02X", frame[index]);
    }

    LOG_INFO("Successfully parsed response frame for command 0x%02X with %zu bytes payload",
             commandCode, payloadLength);

    // 11. Construct and return the response frame
    return Pn532ResponseFrame(commandCode, payload);
}
