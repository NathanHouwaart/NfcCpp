/**
 * @file main.cpp
 * @brief DESFire GetApplicationIDs example
 *
 * Flow:
 *   1) Select PICC (000000)
 *   2) Optional PICC authenticate
 *   3) Run GetApplicationIDs (0x6A + 0xAF chaining)
 *   4) Print all AIDs
 */

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <etl/array.h>
#include <etl/string.h>
#include <etl/vector.h>
#include "Comms/Serial/SerialBusWin.hpp"
#include "Pn532/Pn532Driver.h"
#include "Pn532/Pn532ApduAdapter.h"
#include "Nfc/Card/CardManager.h"
#include "Nfc/Card/ReaderCapabilities.h"
#include "Nfc/Desfire/DesfireAuthMode.h"

using namespace comms::serial;
using namespace pn532;
using namespace nfc;

namespace
{
    struct Args
    {
        std::string comPort;
        int baudRate = 115200;
        bool authenticate = false;
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        uint8_t authKeyNo = 0x00;
        std::vector<uint8_t> authKey;
    };

    uint8_t parseByte(const std::string& value)
    {
        const int parsed = std::stoi(value, nullptr, 0);
        if (parsed < 0 || parsed > 255)
        {
            throw std::runtime_error("Value out of byte range: " + value);
        }
        return static_cast<uint8_t>(parsed);
    }

    std::vector<uint8_t> parseHex(std::string text)
    {
        std::vector<uint8_t> out;
        std::string filtered;
        filtered.reserve(text.size());

        for (char c : text)
        {
            if (std::isxdigit(static_cast<unsigned char>(c)))
            {
                filtered.push_back(c);
            }
        }

        if ((filtered.size() % 2U) != 0U)
        {
            throw std::runtime_error("Hex string has odd number of digits");
        }

        for (size_t i = 0; i < filtered.size(); i += 2U)
        {
            const std::string byteText = filtered.substr(i, 2);
            out.push_back(static_cast<uint8_t>(std::stoul(byteText, nullptr, 16)));
        }

        return out;
    }

    std::string toHex(const etl::array<uint8_t, 3>& aid)
    {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0')
            << std::setw(2) << static_cast<int>(aid[0])
            << std::setw(2) << static_cast<int>(aid[1])
            << std::setw(2) << static_cast<int>(aid[2]);
        return oss.str();
    }

    DesfireAuthMode parseAuthMode(const std::string& text)
    {
        if (text == "legacy")
        {
            return DesfireAuthMode::LEGACY;
        }
        if (text == "iso")
        {
            return DesfireAuthMode::ISO;
        }
        if (text == "aes")
        {
            return DesfireAuthMode::AES;
        }
        if (text == "des")
        {
            return DesfireAuthMode::LEGACY;
        }
        if (text == "2k3des" || text == "3k3des")
        {
            return DesfireAuthMode::ISO;
        }
        throw std::runtime_error("Invalid auth mode: " + text);
    }

    bool isAuthKeyLengthValid(DesfireAuthMode mode, size_t keyLen)
    {
        if (mode == DesfireAuthMode::AES)
        {
            return keyLen == 16;
        }
        if (mode == DesfireAuthMode::ISO)
        {
            return keyLen == 8 || keyLen == 16 || keyLen == 24;
        }
        return keyLen == 8 || keyLen == 16;
    }

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>                        Default: 115200\n";
        std::cout << "  --authenticate                    Authenticate at PICC before list\n";
        std::cout << "  --auth-mode <legacy|iso|aes|des|2k3des|3k3des> Default: iso\n";
        std::cout << "  --auth-key-no <n>                 Default: 0\n";
        std::cout << "  --auth-key-hex <hex>              Required when --authenticate is used\n";
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

            auto requireValue = [&](const char* optionName) -> std::string
            {
                if (i + 1 >= argc)
                {
                    throw std::runtime_error(std::string("Missing value for ") + optionName);
                }
                return argv[++i];
            };

            if (opt == "--baud")
            {
                args.baudRate = std::stoi(requireValue("--baud"));
            }
            else if (opt == "--authenticate")
            {
                args.authenticate = true;
            }
            else if (opt == "--auth-mode")
            {
                args.authMode = parseAuthMode(requireValue("--auth-mode"));
            }
            else if (opt == "--auth-key-no")
            {
                args.authKeyNo = parseByte(requireValue("--auth-key-no"));
            }
            else if (opt == "--auth-key-hex")
            {
                args.authKey = parseHex(requireValue("--auth-key-hex"));
            }
            else
            {
                throw std::runtime_error("Unknown argument: " + opt);
            }
        }

        if (args.authenticate)
        {
            if (args.authKey.empty())
            {
                throw std::runtime_error("--auth-key-hex is required when --authenticate is used");
            }
            if (!isAuthKeyLengthValid(args.authMode, args.authKey.size()))
            {
                throw std::runtime_error("Invalid --auth-key-hex length for selected --auth-mode");
            }
        }

        return args;
    }

    template <size_t Capacity>
    etl::vector<uint8_t, Capacity> toEtl(const std::vector<uint8_t>& in)
    {
        if (in.size() > Capacity)
        {
            throw std::runtime_error("Buffer exceeds ETL capacity");
        }
        etl::vector<uint8_t, Capacity> out;
        for (uint8_t b : in)
        {
            out.push_back(b);
        }
        return out;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        const Args args = parseArgs(argc, argv);

        std::cout << "DESFire GetApplicationIDs Example\n";
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

        const etl::array<uint8_t, 3> piccAid = {0x00, 0x00, 0x00};
        auto selectResult = desfire->selectApplication(piccAid);
        if (!selectResult)
        {
            std::cerr << "Select PICC failed: " << selectResult.error().toString().c_str() << "\n";
            return 1;
        }

        if (args.authenticate)
        {
            const etl::vector<uint8_t, 24> authKey = toEtl<24>(args.authKey);
            auto authResult = desfire->authenticate(args.authKeyNo, authKey, args.authMode);
            if (!authResult)
            {
                std::cerr << "Authenticate failed: " << authResult.error().toString().c_str() << "\n";
                return 1;
            }
            std::cout << "PICC authenticate OK\n";
        }

        auto appIdsResult = desfire->getApplicationIds();
        if (!appIdsResult)
        {
            std::cerr << "GetApplicationIDs failed: " << appIdsResult.error().toString().c_str() << "\n";
            return 1;
        }

        const auto& aids = appIdsResult.value();
        std::cout << "Application count: " << aids.size() << "\n";
        for (size_t i = 0; i < aids.size(); ++i)
        {
            std::cout << "  [" << i << "] " << toHex(aids[i]) << "\n";
        }

        return 0;
    }
    catch (const std::exception& ex)
    {
        printUsage(argv[0]);
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}

