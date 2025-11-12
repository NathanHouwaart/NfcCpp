#pragma once

#include <cstdint>
#include <etl/array.h>
#include <etl/vector.h>
#include <etl/expected.h>

#include "IPn532Command.h"

#include "Error/Error.h"
#include "Comms/IHardwareBus.hpp"

// Command-specific structures
#include "Commands/GetFirmwareVersion.h"
#include "Commands/GetGeneralStatus.h"
#include "Commands/SetSerialBaudRate.h"

using namespace error;

namespace pn532
{
    // Legacy struct - deprecated, use CommandResponse instead
    struct Pn532Response
    {
        uint8_t command;
        etl::vector<uint8_t, 256> data;
    };

    class Pn532Driver
    {
    public:
        // Constructor
        explicit Pn532Driver(comms::IHardwareBus &bus);

        // Initialization
        void init();

        // Command execution
        etl::expected<CommandResponse, Error> executeCommand(IPn532Command &command);

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
        static uint32_t calculateChecksum(const etl::ivector<uint8_t> &data);

    private:
        // Member variables
        comms::IHardwareBus &bus;
        static constexpr uint32_t DEFAULT_TIMEOUT_MS = 500;
        static constexpr etl::array<uint8_t, 6> ACK_FRAME = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
        
        // Private methods
        etl::expected<Pn532ResponseFrame, Error> transceive(const CommandRequest & request);
        etl::expected<void, Error> sendCommand(const etl::ivector<uint8_t> &data);
        etl::expected<Pn532Response, Error> getResponse(uint8_t onCommand, uint32_t timeoutMs);
        etl::expected<void, Error> sendAndAcknowledgeCommand(uint8_t command);

        etl::expected<void, Error> wakeUp();
        bool waitForChip(const int timeout);

        static bool checkAck(const etl::ivector<uint8_t> &buffer);
        static etl::expected<Pn532ResponseFrame, Error> parseResponseFrame(
            const etl::ivector<uint8_t> &frame, 
            uint8_t sentCommandCode);
    };

} // namespace pn532