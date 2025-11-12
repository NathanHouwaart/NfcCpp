/**
 * @file SerialBusWin.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Windows implementation of serial communication bus
 * @version 0.1
 * @date 2025-11-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdint.h>
#include <Windows.h>

#include "Comms/Serial/SerialBusWin.hpp"
#include "Utils/Logging.h"

namespace comms
{

    namespace serial
    {
        using namespace error;

        // ==============================================================================
        // Initialization and Teardown
        // ==============================================================================

        SerialBusWin::SerialBusWin(const etl::string<256> &portname, uint32_t baudrate)
            : portName(portname), baudRate(baudrate), serialHandler(INVALID_HANDLE_VALUE)
        {
        }

        SerialBusWin::~SerialBusWin()
        {
            this->close();
        }

        etl::expected<void, Error> SerialBusWin::init()
        {
            // Open the port
            auto result = this->open();
            if (!result)
            {
                return etl::unexpected(result.error());
            }

            // Set timeouts
            result = this->setTimeout(1000); // Default timeout 1000 ms
            if (!result)
            {
                return etl::unexpected(result.error());
            }

            // Set Baud Rate
            result = this->setBaudRate(this->baudRate);
            if (!result)
            {
                return etl::unexpected(result.error());
            }

            LOG_INFO("Serial port %s initialized with baud rate %u", portName.c_str(), baudRate);
            return {};
        }

        // ==============================================================================
        // Open and Close 
        // ==============================================================================

        etl::expected<void, Error> SerialBusWin::open()
        {
            // Implementation to open the serial port on Windows
            etl::string<260> fullPortName("\\\\.\\" );
            fullPortName += portName;

            this->serialHandler = CreateFileA(
                fullPortName.c_str(),           // port name
                GENERIC_READ | GENERIC_WRITE,   // read/write access
                0,                              // no sharing
                NULL,                           // default security attributes   
                OPEN_EXISTING,                  // opens existing port only
                FILE_ATTRIBUTE_NORMAL,          // normal file attributes
                NULL                            // no template file
            );
            if (this->serialHandler == INVALID_HANDLE_VALUE)
            {
                LOG_ERROR("Error opening serial port: %s", portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::DeviceNotFound));
            }
            
            LOG_INFO("Serial port %s opened", portName.c_str());
            this->setIsOpen(true);

            return {};
        }

        void SerialBusWin::close()
        {
            if (this->isOpen())
            {
                auto res = CloseHandle(this->serialHandler);
                this->serialHandler = INVALID_HANDLE_VALUE;
                if (!res)
                {
                    LOG_ERROR("Error closing serial port: %s", portName.c_str());
                    return;
                }
                this->setIsOpen(false);
                LOG_INFO("Serial port %s closed successfully", portName.c_str());
            }
        }

        // ==============================================================================
        // Read and Write
        // ==============================================================================

        etl::expected<void, Error> SerialBusWin::write(const etl::ivector<uint8_t> &data)
        {
            // Implementation to write data to the serial port on Windows
            DWORD bytesWritten;
            BOOL result = WriteFile(
                this->serialHandler,                    // handle to the serial port
                data.data(),                            // data to be written to the port
                static_cast<DWORD>(data.size()),        // number of bytes to write
                &bytesWritten,                          // number of bytes written
                NULL                                    // no overlapped structure
            );

            if (!result || bytesWritten != data.size())
            {
                LOG_ERROR("Error writing to serial port: %s", portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::WriteFailed));
            }
            LOG_INFO("Wrote %u bytes to serial port: %s", bytesWritten, portName.c_str());
            return {};
        }

        etl::expected<size_t, Error> SerialBusWin::read(etl::ivector<uint8_t> &buffer, size_t length)
        {
            // Implementation to read data from the serial port on Windows
            if(buffer.capacity() < length){
                LOG_ERROR("Read buffer too small for requested length on serial port: %s", portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::BufferOverflow));
            }

            DWORD bytesRead;
            BOOL result = ReadFile(
                this->serialHandler,                // handle to the serial port
                buffer.data(),                      // buffer to store read data
                static_cast<DWORD>(length),         // number of bytes to read
                &bytesRead,                         // number of bytes read
                NULL                                // no overlapped structure
            );

            if (!result)
            {
                LOG_ERROR("Error reading from serial port: %s", portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::ReadFailed));
            }
            LOG_INFO("Read %u bytes from serial port: %s", bytesRead, portName.c_str());
            buffer.uninitialized_resize(bytesRead);  // Resize buffer to actual bytes read, keep data intact
            return static_cast<size_t>(bytesRead);
        }

        etl::expected<void, Error> SerialBusWin::flush()
        {
            // Implementation to flush the serial port buffers on Windows
            BOOL result = FlushFileBuffers(this->serialHandler);
            if (!result)
            {
                LOG_ERROR("Error flushing serial port buffers: %s", portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::BusError));
            }
            LOG_INFO("Serial port buffers flushed successfully: %s", portName.c_str());
            return {};
        }

        size_t SerialBusWin::available() const
        {
            // Implementation to check available bytes to read on the serial port on Windows
            COMSTAT comStat;
            DWORD errors;

            if (!ClearCommError(this->serialHandler, &errors, &comStat))
            {
                LOG_ERROR("Error checking available bytes on serial port: %s", portName.c_str());
                return 0;
            }
            // LOG_INFO("Available bytes to read on serial port %s: %lu", portName.c_str(), comStat.cbInQue);
            return static_cast<size_t>(comStat.cbInQue);
        }

        // ==============================================================================
        // Bus-specific Properties
        // ==============================================================================

        etl::expected<void, Error> SerialBusWin::setBaudRate(uint32_t baudrate)
        {
            // Implementation to set baud rate on Windows
            this->dcbHandler.DCBlength = sizeof(dcbHandler);

            if (!GetCommState(this->serialHandler, &this->dcbHandler))
            {
                LOG_ERROR("Error getting current serial port state: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::Unknown));
            }

            this->dcbHandler.BaudRate = baudrate;

            if (!SetCommState(this->serialHandler, &this->dcbHandler))
            {
                LOG_ERROR("Error setting baud rate on serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::Unknown));
            }

            this->baudRate = baudrate;
            
            LOG_INFO("Baud rate set to %u on serial port: %s", baudrate, this->portName.c_str());
            return {};
        }

        etl::expected<void, Error> SerialBusWin::setParity(Parity parity)
        {
            // Implementation to set parity on Windows
            this->dcbHandler.DCBlength = sizeof(dcbHandler);

            if (!GetCommState(this->serialHandler, &this->dcbHandler))
            {
                LOG_ERROR("Error getting current serial port state: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::Unknown));
            }

            switch (parity)
            {
            case Parity::None:
                this->dcbHandler.Parity = NOPARITY;
                break;
            case Parity::Even:
                this->dcbHandler.Parity = EVENPARITY;
                break;
            case Parity::Odd:
                this->dcbHandler.Parity = ODDPARITY;
                break;
            default:
                LOG_ERROR("Invalid parity setting for serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::InvalidConfiguration));
            }

            if (!SetCommState(this->serialHandler, &this->dcbHandler))
            {
                LOG_ERROR("Error setting parity on serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::Unknown));
            }

            LOG_INFO("Parity set on serial port: %s", this->portName.c_str());
            return {};
        }

        etl::expected<void, Error> SerialBusWin::setStopBits(StopBits stopBits)
        {
            // Implementation to set stop bits on Windows
            this->dcbHandler.DCBlength = sizeof(dcbHandler);

            if (!GetCommState(this->serialHandler, &this->dcbHandler))
            {
                LOG_ERROR("Error getting current serial port state: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::Unknown));
            }

            switch (stopBits)
            {
            case StopBits::One:
                this->dcbHandler.StopBits = ONESTOPBIT;
                break;
            case StopBits::Two:
                this->dcbHandler.StopBits = TWOSTOPBITS;
                break;
            default:
                LOG_ERROR("Invalid stop bits setting for serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::InvalidConfiguration));
            }

            if (!SetCommState(this->serialHandler, &this->dcbHandler))
            {
                LOG_ERROR("Error setting stop bits on serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::Unknown));
            }

            LOG_INFO("Stop bits set on serial port: %s", this->portName.c_str());
            return {};
        }

        etl::expected<void, Error> SerialBusWin::setFlowControl(FlowControl flowControl)
        {
            // Implementation to set flow control on Windows
            this->dcbHandler.DCBlength = sizeof(this->dcbHandler);

            if (!GetCommState(this->serialHandler, &this->dcbHandler))
            {
                LOG_ERROR("Error getting current serial port state: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::Unknown));
            }

            switch (flowControl)
            {
            case FlowControl::None:
                this->dcbHandler.fOutxCtsFlow = FALSE;
                this->dcbHandler.fRtsControl = RTS_CONTROL_DISABLE;
                this->dcbHandler.fInX = FALSE;
                this->dcbHandler.fOutX = FALSE;
                break;
            case FlowControl::Hardware:
                this->dcbHandler.fOutxCtsFlow = TRUE;
                this->dcbHandler.fRtsControl = RTS_CONTROL_HANDSHAKE;
                break;
            case FlowControl::Software:
                this->dcbHandler.fInX = TRUE;
                this->dcbHandler.fOutX = TRUE;
                break;
            default:
                LOG_ERROR("Invalid flow control setting for serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::InvalidConfiguration));
            }

            if (!SetCommState(this->serialHandler, &this->dcbHandler))
            {
                LOG_ERROR("Error setting flow control on serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::Unknown));
            }

            LOG_INFO("Flow control set on serial port: %s", this->portName.c_str());
            return {};
        }

        etl::expected<void, Error> SerialBusWin::setTimeout(uint32_t timeoutMs)
        {
            // Implementation to set timeouts on Windows
            this->serialTimeouts.ReadIntervalTimeout = timeoutMs;
            this->serialTimeouts.ReadTotalTimeoutConstant = timeoutMs;
            this->serialTimeouts.ReadTotalTimeoutMultiplier = 1;
            this->serialTimeouts.WriteTotalTimeoutConstant = timeoutMs;
            this->serialTimeouts.WriteTotalTimeoutMultiplier = 10;

            if (!SetCommTimeouts(this->serialHandler, &this->serialTimeouts))
            {
                LOG_ERROR("Error setting timeouts on serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::Unknown));
            }

            LOG_INFO("Timeout set to %u ms on serial port: %s", timeoutMs, this->portName.c_str());
            return {};
        }

        // ==============================================================================
        // Bus Properties
        // ==============================================================================
        
        etl::expected<void, Error> SerialBusWin::setProperty(BusProperty property, uint32_t value)
        {
            switch (property)
            {
            case BusProperty::BaudRate:
                return this->setBaudRate(value);
            case BusProperty::Timeout:
                return this->setTimeout(value);
            case BusProperty::Parity:
                return this->setParity(static_cast<Parity>(value));
            case BusProperty::StopBits:
                return this->setStopBits(static_cast<StopBits>(value));
            case BusProperty::FlowControl:
                return this->setFlowControl(static_cast<FlowControl>(value));
            default:
                LOG_ERROR("Property not supported on serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::NotSupported));
            }
        }

        etl::expected<uint32_t, Error> SerialBusWin::getProperty(BusProperty property) const
        {
            switch (property)
            {
            case BusProperty::BaudRate:
                return this->baudRate;
            // Add other properties as needed
            default:
                LOG_ERROR("Property not supported on serial port: %s", this->portName.c_str());
                return etl::unexpected(Error::fromHardware(HardwareError::NotSupported));
            }
        }

    } // namespace serial

} // namespace comms