/**
 * @file main.cpp
 * @brief DESFire GetVersion example
 *
 * Flow:
 *   1) Connect PN532 and detect DESFire card
 *   2) Run GetVersion (0x60 + 0xAF chaining)
 *   3) Print raw payload and decoded EV1-style fields
 */

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <etl/string.h>
#include "Comms/Serial/SerialBusWin.hpp"
#include "Pn532/Pn532Driver.h"
#include "Pn532/Pn532ApduAdapter.h"
#include "Nfc/Card/CardManager.h"
#include "Nfc/Card/ReaderCapabilities.h"

using namespace comms::serial;
using namespace pn532;
using namespace nfc;

namespace
{
    struct Args
    {
        std::string comPort;
        int baudRate = 115200;
    };

    std::string toHex(const etl::ivector<uint8_t>& data)
    {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0');
        for (size_t i = 0; i < data.size(); ++i)
        {
            if (i > 0)
            {
                oss << ' ';
            }
            oss << std::setw(2) << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>                        Default: 115200\n";
    }

    Args parseArgs(int argc, char* argv[])
    {
        if (argc < 2)
        {
            throw std::runtime_error("Missing COM port");
        }

        Args args;
        args.comPort = argv[1];

        for (int i = 2; i < argc; ++i)
        {
            const std::string opt = argv[i];
            if (opt == "--baud")
            {
                if (i + 1 >= argc)
                {
                    throw std::runtime_error("Missing value for --baud");
                }
                args.baudRate = std::stoi(argv[++i]);
            }
            else
            {
                throw std::runtime_error("Unknown argument: " + opt);
            }
        }

        return args;
    }

    void printDecodedVersion(const etl::ivector<uint8_t>& version)
    {
        std::cout << "Version payload length: " << version.size() << " byte(s)\n";
        std::cout << "Raw payload: " << toHex(version) << "\n";

        if (version.size() < 14U)
        {
            std::cout << "Payload shorter than expected EV1 two header blocks (14 bytes)\n";
            return;
        }

        std::cout << "\nHardware block (bytes 0..6):\n";
        std::cout << "  Vendor ID:       0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(version[0]) << std::dec << "\n";
        std::cout << "  Type:            0x" << std::hex << std::uppercase << std::setw(2)
                  << static_cast<int>(version[1]) << std::dec << "\n";
        std::cout << "  Subtype:         0x" << std::hex << std::uppercase << std::setw(2)
                  << static_cast<int>(version[2]) << std::dec << "\n";
        std::cout << "  Version:         " << static_cast<int>(version[3]) << "." << static_cast<int>(version[4]) << "\n";
        std::cout << "  Storage size id: 0x" << std::hex << std::uppercase << std::setw(2)
                  << static_cast<int>(version[5]) << std::dec << "\n";
        std::cout << "  Protocol:        0x" << std::hex << std::uppercase << std::setw(2)
                  << static_cast<int>(version[6]) << std::dec << "\n";

        std::cout << "\nSoftware block (bytes 7..13):\n";
        std::cout << "  Vendor ID:       0x" << std::hex << std::uppercase << std::setw(2)
                  << static_cast<int>(version[7]) << std::dec << "\n";
        std::cout << "  Type:            0x" << std::hex << std::uppercase << std::setw(2)
                  << static_cast<int>(version[8]) << std::dec << "\n";
        std::cout << "  Subtype:         0x" << std::hex << std::uppercase << std::setw(2)
                  << static_cast<int>(version[9]) << std::dec << "\n";
        std::cout << "  Version:         " << static_cast<int>(version[10]) << "." << static_cast<int>(version[11]) << "\n";
        std::cout << "  Storage size id: 0x" << std::hex << std::uppercase << std::setw(2)
                  << static_cast<int>(version[12]) << std::dec << "\n";
        std::cout << "  Protocol:        0x" << std::hex << std::uppercase << std::setw(2)
                  << static_cast<int>(version[13]) << std::dec << "\n";

        if (version.size() < 28U)
        {
            std::cout << "\nPayload shorter than full EV1 footer block (14 bytes)\n";
            return;
        }

        std::cout << "\nUID (bytes 14..20): ";
        for (size_t i = 14; i < 21; ++i)
        {
            if (i > 14)
            {
                std::cout << ' ';
            }
            std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                      << static_cast<int>(version[i]);
        }
        std::cout << std::dec << "\n";

        std::cout << "Batch number (bytes 21..25): ";
        for (size_t i = 21; i < 26; ++i)
        {
            if (i > 21)
            {
                std::cout << ' ';
            }
            std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                      << static_cast<int>(version[i]);
        }
        std::cout << std::dec << "\n";

        std::cout << "Production week:   " << static_cast<int>(version[26]) << "\n";
        std::cout << "Production year:   " << static_cast<int>(version[27]) << "\n";
    }
}

int main(int argc, char* argv[])
{
    try
    {
        const Args args = parseArgs(argc, argv);

        std::cout << "DESFire GetVersion Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";

        etl::string<256> portName(args.comPort.c_str());
        SerialBusWin serial(portName, args.baudRate);
        auto serialInitResult = serial.init();
        if (!serialInitResult)
        {
            std::cerr << "Serial init failed: " << serialInitResult.error().toString().c_str() << "\n";
            return 1;
        }

        Pn532Driver pn532(serial);
        pn532.init();

        auto samResult = pn532.setSamConfiguration(0x01);
        if (!samResult)
        {
            std::cerr << "SAM config failed: " << samResult.error().toString().c_str() << "\n";
            return 1;
        }

        auto retryResult = pn532.setMaxRetries(1);
        if (!retryResult)
        {
            std::cerr << "Set retries failed: " << retryResult.error().toString().c_str() << "\n";
            return 1;
        }

        Pn532ApduAdapter adapter(pn532);
        CardManager cardManager(adapter, adapter, ReaderCapabilities::pn532());
        cardManager.setWire(WireKind::Iso);

        auto detectResult = cardManager.detectCard();
        if (!detectResult)
        {
            std::cerr << "Card detect failed: " << detectResult.error().toString().c_str() << "\n";
            return 1;
        }

        auto sessionResult = cardManager.createSession();
        if (!sessionResult)
        {
            std::cerr << "Session create failed: " << sessionResult.error().toString().c_str() << "\n";
            return 1;
        }

        CardSession* session = sessionResult.value();
        DesfireCard* desfire = session->getCardAs<DesfireCard>();
        if (!desfire)
        {
            std::cerr << "Detected card is not DESFire\n";
            return 1;
        }

        auto versionResult = desfire->getVersion();
        if (!versionResult)
        {
            std::cerr << "GetVersion failed: " << versionResult.error().toString().c_str() << "\n";
            return 1;
        }

        const auto& version = versionResult.value();
        printDecodedVersion(version);

        return 0;
    }
    catch (const std::exception& ex)
    {
        printUsage(argv[0]);
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}

