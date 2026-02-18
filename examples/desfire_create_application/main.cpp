/**
 * @file main.cpp
 * @brief DESFire create application example
 *
 * Flow:
 *   1) Select PICC (000000)
 *   2) Authenticate PICC master key
 *   3) Create application
 *   4) Select created application
 *   5) Authenticate application key
 */

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
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
#include "Nfc/Desfire/DesfireKeyType.h"
#include "Error/DesfireError.h"

using namespace comms::serial;
using namespace pn532;
using namespace nfc;

namespace
{
    enum class AuthModeChoice : uint8_t
    {
        Auto,
        Legacy,
        Iso,
        Aes
    };

    struct Args
    {
        std::string comPort;
        int baudRate = 115200;

        DesfireAuthMode piccAuthMode = DesfireAuthMode::ISO;
        uint8_t piccAuthKeyNo = 0x00;
        std::vector<uint8_t> piccAuthKey;

        std::vector<uint8_t> appAid;
        uint8_t appKeySettings = 0x0F;
        uint8_t appKeyCount = 1;
        DesfireKeyType appKeyType = DesfireKeyType::AES;
        uint8_t appAuthKeyNo = 0x00;
        AuthModeChoice appAuthModeChoice = AuthModeChoice::Auto;
        std::optional<std::vector<uint8_t>> appAuthKey;

        bool allowExisting = false;
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

    DesfireKeyType parseKeyType(const std::string& text)
    {
        if (text == "des")
        {
            return DesfireKeyType::DES;
        }
        if (text == "2k3des")
        {
            return DesfireKeyType::DES3_2K;
        }
        if (text == "3k3des")
        {
            return DesfireKeyType::DES3_3K;
        }
        if (text == "aes")
        {
            return DesfireKeyType::AES;
        }
        throw std::runtime_error("Invalid key type: " + text);
    }

    AuthModeChoice parseAuthModeChoice(const std::string& text)
    {
        if (text == "auto")
        {
            return AuthModeChoice::Auto;
        }
        if (text == "legacy")
        {
            return AuthModeChoice::Legacy;
        }
        if (text == "iso")
        {
            return AuthModeChoice::Iso;
        }
        if (text == "aes")
        {
            return AuthModeChoice::Aes;
        }
        throw std::runtime_error("Invalid auth mode choice: " + text);
    }

    size_t expectedKeySize(DesfireKeyType keyType)
    {
        switch (keyType)
        {
            case DesfireKeyType::DES:
                return 8;
            case DesfireKeyType::DES3_2K:
                return 16;
            case DesfireKeyType::DES3_3K:
                return 24;
            case DesfireKeyType::AES:
                return 16;
            default:
                return 0;
        }
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

    DesfireAuthMode defaultAppAuthMode(DesfireKeyType keyType)
    {
        switch (keyType)
        {
            case DesfireKeyType::AES:
                return DesfireAuthMode::AES;
            case DesfireKeyType::DES3_3K:
            case DesfireKeyType::DES3_2K:
                return DesfireAuthMode::ISO;
            case DesfireKeyType::DES:
            default:
                return DesfireAuthMode::LEGACY;
        }
    }

    DesfireAuthMode resolveAppAuthMode(AuthModeChoice choice, DesfireKeyType keyType)
    {
        switch (choice)
        {
            case AuthModeChoice::Legacy:
                return DesfireAuthMode::LEGACY;
            case AuthModeChoice::Iso:
                return DesfireAuthMode::ISO;
            case AuthModeChoice::Aes:
                return DesfireAuthMode::AES;
            case AuthModeChoice::Auto:
            default:
                return defaultAppAuthMode(keyType);
        }
    }

    std::vector<uint8_t> defaultAppAuthKey(DesfireKeyType keyType)
    {
        const size_t len = expectedKeySize(keyType);
        return std::vector<uint8_t>(len, 0x00);
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
        std::cout << "  --app-key-settings <n>            Default: 0x0F\n";
        std::cout << "  --app-key-count <n>               Default: 1 (1..14)\n";
        std::cout << "  --app-key-type <des|2k3des|3k3des|aes> Default: aes\n";
        std::cout << "  --app-auth-key-no <n>             Default: 0\n";
        std::cout << "  --app-auth-mode <auto|legacy|iso|aes> Default: auto\n";
        std::cout << "  --app-auth-key-hex <hex>          Default: all-zero key for app type\n";
        std::cout << "  --allow-existing                  Continue when app already exists\n";
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
            else if (opt == "--app-key-settings")
            {
                args.appKeySettings = parseByte(requireValue("--app-key-settings"));
            }
            else if (opt == "--app-key-count")
            {
                args.appKeyCount = parseByte(requireValue("--app-key-count"));
            }
            else if (opt == "--app-key-type")
            {
                args.appKeyType = parseKeyType(requireValue("--app-key-type"));
            }
            else if (opt == "--app-auth-key-no")
            {
                args.appAuthKeyNo = parseByte(requireValue("--app-auth-key-no"));
            }
            else if (opt == "--app-auth-mode")
            {
                args.appAuthModeChoice = parseAuthModeChoice(requireValue("--app-auth-mode"));
            }
            else if (opt == "--app-auth-key-hex")
            {
                args.appAuthKey = parseHex(requireValue("--app-auth-key-hex"));
            }
            else if (opt == "--allow-existing")
            {
                args.allowExisting = true;
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
        if (args.appKeyCount == 0 || args.appKeyCount > 14)
        {
            throw std::runtime_error("--app-key-count must be in range 1..14");
        }

        const DesfireAuthMode appAuthMode = resolveAppAuthMode(args.appAuthModeChoice, args.appKeyType);
        if (args.appAuthKey.has_value())
        {
            if (!isAuthKeyLengthValid(appAuthMode, args.appAuthKey->size()))
            {
                throw std::runtime_error("Invalid --app-auth-key-hex length for selected app auth mode");
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
        const DesfireAuthMode appAuthMode = resolveAppAuthMode(args.appAuthModeChoice, args.appKeyType);
        const std::vector<uint8_t> appAuthKeyStd = args.appAuthKey.value_or(defaultAppAuthKey(args.appKeyType));

        std::cout << "DESFire CreateApplication Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";
        std::cout << "Target AID: " << toHex(args.appAid) << "\n";
        std::cout << "App key count: " << static_cast<int>(args.appKeyCount) << "\n";
        std::cout << "App key settings: 0x" << std::hex << std::uppercase
                  << static_cast<int>(args.appKeySettings) << std::dec << "\n";

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
        auto createResult = desfire->createApplication(
            targetAid,
            args.appKeySettings,
            args.appKeyCount,
            args.appKeyType);
        if (!createResult)
        {
            if (args.allowExisting &&
                createResult.error().is<error::DesfireError>() &&
                createResult.error().get<error::DesfireError>() == error::DesfireError::DuplicateError)
            {
                std::cout << "CreateApplication returned DuplicateError; continuing (--allow-existing)\n";
            }
            else
            {
                std::cerr << "CreateApplication failed: " << createResult.error().toString().c_str() << "\n";
                return 1;
            }
        }
        else
        {
            std::cout << "CreateApplication OK\n";
        }

        auto selectAppResult = desfire->selectApplication(targetAid);
        if (!selectAppResult)
        {
            std::cerr << "Select created app failed: " << selectAppResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "Select created app OK\n";

        const auto appAuthKey = toEtl<24>(appAuthKeyStd);
        auto appAuthResult = desfire->authenticate(args.appAuthKeyNo, appAuthKey, appAuthMode);
        if (!appAuthResult)
        {
            std::cerr << "Application authenticate failed: " << appAuthResult.error().toString().c_str() << "\n";
            return 1;
        }

        std::cout << "Application authenticate OK\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        printUsage(argv[0]);
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}
