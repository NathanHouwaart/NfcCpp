#pragma once

#include <cstdint>
#include <etl/array.h>
#include <etl/vector.h>
#include <etl/expected.h>

#include "IPn532Command.h"

#include "Error/Error.h"
#include "Comms/IHardwareBus.hpp"


enum class Pn532Baudrate {
    Baud9600,
    Baud19200,
    Baud38400,
    Baud57600,
    Baud115200,
    Baud230400
};

struct FirmwareInfo {
    uint8_t ic;
    uint8_t ver;
    uint8_t rev;
    uint8_t support;
};

struct GeneralStatus {
    uint8_t error;
    uint8_t field;
    uint8_t nrTargets;
    etl::array<uint8_t, 4> tags;
};

struct CommandResult {
    uint8_t status;
    etl::vector<uint8_t, 256> data;
};

struct Pn532Response {
    uint8_t command;
    etl::vector<uint8_t, 256> data;
};

using namespace error;

class Pn532Driver {
public:
    // Constructor
    explicit Pn532Driver(comms::IHardwareBus& bus);

    // Initialization
    void init();

    // Command execution
    etl::expected<CommandResult, Error> executeCommand(IPn532Command& command);

    // Firmware and status
    etl::expected<FirmwareInfo, Error> getFirmwareVersion();
    etl::expected<void, Error> performSelftest();
    etl::expected<GeneralStatus, Error> getGeneralStatus();

    // Configuration
    etl::expected<void, Error> setSamConfiguration(uint8_t mode);
    etl::expected<void, Error> setRfField(const bool state);
    etl::expected<void, Error> setMaxRetries(const uint8_t maxRetries);
    etl::expected<void, Error> setSerialBaudrate(Pn532Baudrate baudrate);

    // Register operations
    etl::expected<void, Error> writeRegister(const uint16_t reg, const uint8_t val);
    etl::expected<uint8_t, Error> readRegister(const uint16_t reg);

    // GPIO operations
    etl::expected<void, Error> writeGPIO(uint8_t newGpioState);
    etl::expected<uint8_t, Error> readGPIO();

    // Static utility
    static uint32_t calculateChecksum(const etl::ivector<uint8_t>& data);

private:
    // Member variables
    comms::IHardwareBus& bus;
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;
    static constexpr etl::array<uint8_t, 6> ACK_FRAME = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};

    // Private methods
    etl::expected<void, Error> sendCommand(uint8_t cmd, const etl::ivector<uint8_t>& data);
    etl::expected<Pn532Response, Error> getResponse(uint8_t onCommand, uint32_t timeoutMs);
    etl::expected<void, Error> sendAndAcknowledgeCommand(uint8_t command);
    
    void wakeUp();
    bool waitForChip(const int timeout);
    
    static bool checkAck(const etl::ivector<uint8_t>& buffer);
    static bool isValidFrame(const etl::ivector<uint8_t>& frame);
};
