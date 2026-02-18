/**
 * @file main.cpp
 * @brief DESFire delete application example
 *
 * Flow:
 *   1) Select PICC (000000)
 *   2) Authenticate PICC master key
 *   3) Delete application
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
#include <etl/string.h>
#include <etl/vector.h>
#include "Comms/Serial/SerialBusWin.hpp"
#include "Pn532/Pn532Driver.h"
#include "Pn532/Pn532ApduAdapter.h"
#include "Nfc/Card/CardManager.h"
#include "Nfc/Card/ReaderCapabilities.h"
#include "Nfc/Desfire/DesfireAuthMode.h"
#include "Error/DesfireError.h"

using namespace comms::serial;
using namespace pn532;
using namespace nfc;

namespace
{
    struct Args
    {
        std::string comPort;
        int baudRate = 115200;
        DesfireAuthMode piccAuthMode = DesfireAuthMode::ISO;
        uint8_t piccAuthKeyNo = 0x00;
        std::vector<uint8_t> piccAuthKey;
        std::vector<uint8_t> appAid;
        bool allowMissing = false;
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

        if (filtered.size() % 2 != 0)
        {
            throw std::runtime_error("Hex string has odd number of digits");
        }

        for (size_t i = 0; i < filtered.size(); i += 2)
        {
            const std::string byteText = filtered.substr(i, 2);
            out.push_back(static_cast<uint8_t>(std::stoul(byteText, nullptr, 16)));
        }

        return out;
    }

    std::string toHex(const std::vector<uint8_t>& data)
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
        std::cout << "  --picc-auth-mode <legacy|iso|aes> Default: iso\n";
        std::cout << "  --picc-auth-key-no <n>            Default: 0\n";
        std::cout << "  --picc-auth-key-hex <hex>         Required\n";
        std::cout << "  --app-aid <hex6>                  Required (3-byte AID)\n";
        std::cout << "  --allow-missing                   Continue when app does not exist\n";
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
            else if (opt == "--picc-auth-mode")
            {
                args.piccAuthMode = parseAuthMode(requireValue("--picc-auth-mode"));
            }
            else if (opt == "--picc-auth-key-no")
            {
                args.piccAuthKeyNo = parseByte(requireValue("--picc-auth-key-no"));
            }
            else if (opt == "--picc-auth-key-hex")
            {
                args.piccAuthKey = parseHex(requireValue("--picc-auth-key-hex"));
            }
            else if (opt == "--app-aid")
            {
                args.appAid = parseHex(requireValue("--app-aid"));
            }
            else if (opt == "--allow-missing")
            {
                args.allowMissing = true;
            }
            else
            {
                throw std::runtime_error("Unknown argument: " + opt);
            }
        }

        if (args.piccAuthKey.empty())
        {
            throw std::runtime_error("--picc-auth-key-hex is required");
        }
        if (!isAuthKeyLengthValid(args.piccAuthMode, args.piccAuthKey.size()))
        {
            throw std::runtime_error("Invalid --picc-auth-key-hex length for --picc-auth-mode");
        }
        if (args.appAid.size() != 3)
        {
            throw std::runtime_error("--app-aid must be exactly 3 bytes");
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

        std::cout << "DESFire DeleteApplication Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";
        std::cout << "Target AID: " << toHex(args.appAid) << "\n";

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
        auto selectPiccResult = desfire->selectApplication(piccAid);
        if (!selectPiccResult)
        {
            std::cerr << "Select PICC failed: " << selectPiccResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "Select PICC OK\n";

        const auto piccAuthKey = toEtl<24>(args.piccAuthKey);
        auto piccAuthResult = desfire->authenticate(args.piccAuthKeyNo, piccAuthKey, args.piccAuthMode);
        if (!piccAuthResult)
        {
            std::cerr << "PICC authenticate failed: " << piccAuthResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "PICC authenticate OK\n";

        const etl::array<uint8_t, 3> targetAid = {args.appAid[0], args.appAid[1], args.appAid[2]};
        auto deleteResult = desfire->deleteApplication(targetAid);
        if (!deleteResult)
        {
            if (args.allowMissing &&
                deleteResult.error().is<error::DesfireError>() &&
                deleteResult.error().get<error::DesfireError>() == error::DesfireError::ApplicationNotFound)
            {
                std::cout << "DeleteApplication returned ApplicationNotFound; continuing (--allow-missing)\n";
                return 0;
            }

            std::cerr << "DeleteApplication failed: " << deleteResult.error().toString().c_str() << "\n";
            return 1;
        }

        std::cout << "DeleteApplication OK\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        printUsage(argv[0]);
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}

