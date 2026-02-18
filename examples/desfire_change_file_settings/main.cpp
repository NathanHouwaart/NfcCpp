/**
 * @file main.cpp
 * @brief DESFire ChangeFileSettings example
 *
 * Flow:
 *   1) Select application
 *   2) Optional authenticate
 *   3) Optional show current file settings
 *   4) ChangeFileSettings(fileNo, comm, access rights)
 *   5) Optional show updated file settings
 */

#include <algorithm>
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
#include <etl/array.h>
#include <etl/string.h>
#include <etl/vector.h>
#include "Comms/Serial/SerialBusWin.hpp"
#include "Pn532/Pn532Driver.h"
#include "Pn532/Pn532ApduAdapter.h"
#include "Nfc/Card/CardManager.h"
#include "Nfc/Card/ReaderCapabilities.h"
#include "Nfc/Desfire/DesfireAuthMode.h"
#include "Nfc/Desfire/DesfireCard.h"
#include "Nfc/Desfire/DesfireKeyType.h"

using namespace comms::serial;
using namespace pn532;
using namespace nfc;

namespace
{
    struct Args
    {
        std::string comPort;
        int baudRate = 115200;
        std::vector<uint8_t> aid = {0x00, 0x00, 0x00};

        uint8_t fileNo = 0x00;
        uint8_t newCommunicationSettings = 0x00;
        uint8_t readAccess = 0x00;
        uint8_t writeAccess = 0x00;
        uint8_t readWriteAccess = 0x00;
        uint8_t changeAccess = 0x00;
        uint8_t commandCommunicationSettings = 0xFFU; // auto

        bool authenticate = false;
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        std::optional<DesfireKeyType> sessionKeyType;
        uint8_t authKeyNo = 0x00;
        std::vector<uint8_t> authKey;

        bool showBefore = false;
        bool showAfter = false;
        bool confirmChange = false;
    };

    std::string toLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return value;
    }

    uint8_t parseByte(const std::string& value)
    {
        const int parsed = std::stoi(value, nullptr, 0);
        if (parsed < 0 || parsed > 255)
        {
            throw std::runtime_error("Value out of byte range: " + value);
        }
        return static_cast<uint8_t>(parsed);
    }

    std::vector<uint8_t> parseHex(const std::string& text)
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

        for (size_t i = 0U; i < filtered.size(); i += 2U)
        {
            const std::string byteText = filtered.substr(i, 2U);
            out.push_back(static_cast<uint8_t>(std::stoul(byteText, nullptr, 16)));
        }

        return out;
    }

    std::string toHex(const std::vector<uint8_t>& data)
    {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0');
        for (size_t i = 0U; i < data.size(); ++i)
        {
            if (i > 0U)
            {
                oss << ' ';
            }
            oss << std::setw(2) << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    DesfireAuthMode parseAuthMode(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "legacy" || lower == "des")
        {
            return DesfireAuthMode::LEGACY;
        }
        if (lower == "iso" || lower == "2k3des" || lower == "3k3des")
        {
            return DesfireAuthMode::ISO;
        }
        if (lower == "aes")
        {
            return DesfireAuthMode::AES;
        }
        throw std::runtime_error("Invalid auth mode: " + text);
    }

    DesfireKeyType parseKeyType(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "des")
        {
            return DesfireKeyType::DES;
        }
        if (lower == "2k3des")
        {
            return DesfireKeyType::DES3_2K;
        }
        if (lower == "3k3des")
        {
            return DesfireKeyType::DES3_3K;
        }
        if (lower == "aes")
        {
            return DesfireKeyType::AES;
        }

        throw std::runtime_error("Invalid key type: " + text);
    }

    bool isAuthKeyLengthValid(DesfireAuthMode mode, size_t keyLen)
    {
        if (mode == DesfireAuthMode::AES)
        {
            return keyLen == 16U;
        }
        if (mode == DesfireAuthMode::ISO)
        {
            return keyLen == 8U || keyLen == 16U || keyLen == 24U;
        }
        return keyLen == 8U || keyLen == 16U;
    }

    uint8_t parseCommunicationSettings(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "plain")
        {
            return 0x00U;
        }
        if (lower == "mac")
        {
            return 0x01U;
        }
        if (lower == "enc" || lower == "enciphered")
        {
            return 0x03U;
        }

        const uint8_t value = parseByte(text);
        if (value != 0x00U && value != 0x01U && value != 0x03U)
        {
            throw std::runtime_error("comm mode must be plain|mac|enc or 0x00|0x01|0x03");
        }
        return value;
    }

    uint8_t parseCommandCommunicationSettings(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "auto")
        {
            return 0xFFU;
        }
        return parseCommunicationSettings(text);
    }

    uint8_t parseAccessRight(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "free")
        {
            return 0x0EU;
        }
        if (lower == "never")
        {
            return 0x0FU;
        }
        if (lower.rfind("key", 0U) == 0U)
        {
            const std::string suffix = lower.substr(3U);
            if (suffix.empty())
            {
                throw std::runtime_error("Invalid access-right token: " + text);
            }

            const int keyNo = std::stoi(suffix, nullptr, 10);
            if (keyNo < 0 || keyNo > 13)
            {
                throw std::runtime_error("keyN access-right must be in range key0..key13");
            }
            return static_cast<uint8_t>(keyNo);
        }

        const uint8_t value = parseByte(text);
        if (value > 0x0FU)
        {
            throw std::runtime_error("Access-right nibble out of range (0..15): " + text);
        }
        return value;
    }

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>                                Default: 115200\n";
        std::cout << "  --aid <hex6>                              Default: 000000\n";
        std::cout << "  --file-no <n>                             Required (0..31)\n";
        std::cout << "  --new-comm-mode <plain|mac|enc|0x00|0x01|0x03> Required\n";
        std::cout << "  --read-access <n|keyN|free|never>         Required\n";
        std::cout << "  --write-access <n|keyN|free|never>        Required\n";
        std::cout << "  --read-write-access <n|keyN|free|never>   Required\n";
        std::cout << "  --change-access <n|keyN|free|never>       Required\n";
        std::cout << "  --command-comm-mode <auto|plain|mac|enc|0x00|0x01|0x03> Default: auto\n";
        std::cout << "  --authenticate                            Authenticate before ChangeFileSettings\n";
        std::cout << "  --auth-mode <legacy|iso|aes|des|2k3des|3k3des> Default: iso\n";
        std::cout << "  --session-key-type <des|2k3des|3k3des|aes> Optional\n";
        std::cout << "  --auth-key-no <n>                         Default: 0\n";
        std::cout << "  --auth-key-hex <hex>                      Required when --authenticate is set\n";
        std::cout << "  --show-before                             Read file settings before command\n";
        std::cout << "  --show-after                              Read file settings after command\n";
        std::cout << "  --confirm-change                          Execute command (otherwise dry-run)\n";
    }

    void printFileSettingsSummary(const DesfireFileSettingsInfo& settings, const std::string& label)
    {
        std::cout << label << "\n";
        std::cout << "  file type: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(settings.fileType) << std::dec << "\n";
        std::cout << "  comm mode: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(settings.communicationSettings) << std::dec << "\n";
        std::cout << "  access (R/W/RW/CAR): "
                  << static_cast<int>(settings.readAccess) << " / "
                  << static_cast<int>(settings.writeAccess) << " / "
                  << static_cast<int>(settings.readWriteAccess) << " / "
                  << static_cast<int>(settings.changeAccess) << "\n";
    }

    Args parseArgs(int argc, char* argv[])
    {
        if (argc < 2)
        {
            throw std::runtime_error("Missing COM port");
        }

        Args args;
        args.comPort = argv[1];

        bool fileNoProvided = false;
        bool commProvided = false;
        bool readAccessProvided = false;
        bool writeAccessProvided = false;
        bool readWriteAccessProvided = false;
        bool changeAccessProvided = false;

        for (int i = 2; i < argc; ++i)
        {
            const std::string opt = argv[i];
            auto requireValue = [&](const char* optionName) -> std::string
            {
                if ((i + 1) >= argc)
                {
                    throw std::runtime_error(std::string("Missing value for ") + optionName);
                }
                return argv[++i];
            };

            if (opt == "--baud")
            {
                args.baudRate = std::stoi(requireValue("--baud"));
            }
            else if (opt == "--aid")
            {
                args.aid = parseHex(requireValue("--aid"));
            }
            else if (opt == "--file-no")
            {
                args.fileNo = parseByte(requireValue("--file-no"));
                fileNoProvided = true;
            }
            else if (opt == "--new-comm-mode")
            {
                args.newCommunicationSettings = parseCommunicationSettings(requireValue("--new-comm-mode"));
                commProvided = true;
            }
            else if (opt == "--read-access")
            {
                args.readAccess = parseAccessRight(requireValue("--read-access"));
                readAccessProvided = true;
            }
            else if (opt == "--write-access")
            {
                args.writeAccess = parseAccessRight(requireValue("--write-access"));
                writeAccessProvided = true;
            }
            else if (opt == "--read-write-access")
            {
                args.readWriteAccess = parseAccessRight(requireValue("--read-write-access"));
                readWriteAccessProvided = true;
            }
            else if (opt == "--change-access")
            {
                args.changeAccess = parseAccessRight(requireValue("--change-access"));
                changeAccessProvided = true;
            }
            else if (opt == "--command-comm-mode")
            {
                args.commandCommunicationSettings =
                    parseCommandCommunicationSettings(requireValue("--command-comm-mode"));
            }
            else if (opt == "--authenticate")
            {
                args.authenticate = true;
            }
            else if (opt == "--auth-mode")
            {
                args.authMode = parseAuthMode(requireValue("--auth-mode"));
            }
            else if (opt == "--session-key-type")
            {
                args.sessionKeyType = parseKeyType(requireValue("--session-key-type"));
            }
            else if (opt == "--auth-key-no")
            {
                args.authKeyNo = parseByte(requireValue("--auth-key-no"));
            }
            else if (opt == "--auth-key-hex")
            {
                args.authKey = parseHex(requireValue("--auth-key-hex"));
            }
            else if (opt == "--show-before")
            {
                args.showBefore = true;
            }
            else if (opt == "--show-after")
            {
                args.showAfter = true;
            }
            else if (opt == "--confirm-change")
            {
                args.confirmChange = true;
            }
            else
            {
                throw std::runtime_error("Unknown argument: " + opt);
            }
        }

        if (args.aid.size() != 3U)
        {
            throw std::runtime_error("--aid must be exactly 3 bytes");
        }
        if (!fileNoProvided)
        {
            throw std::runtime_error("--file-no is required");
        }
        if (args.fileNo > 0x1FU)
        {
            throw std::runtime_error("--file-no must be in range 0..31");
        }
        if (!commProvided)
        {
            throw std::runtime_error("--new-comm-mode is required");
        }
        if (!readAccessProvided || !writeAccessProvided || !readWriteAccessProvided || !changeAccessProvided)
        {
            throw std::runtime_error(
                "--read-access, --write-access, --read-write-access, and --change-access are required");
        }

        if (args.authenticate)
        {
            if (args.authKey.empty())
            {
                throw std::runtime_error("--auth-key-hex is required when --authenticate is set");
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
            throw std::runtime_error("Input exceeds ETL capacity");
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

        std::cout << "DESFire ChangeFileSettings Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";
        std::cout << "AID: " << toHex(args.aid) << "\n";
        std::cout << "File no: " << static_cast<int>(args.fileNo) << "\n";
        std::cout << "New comm mode: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(args.newCommunicationSettings) << std::dec << "\n";
        std::cout << "New access (R/W/RW/CAR): "
                  << static_cast<int>(args.readAccess) << " / "
                  << static_cast<int>(args.writeAccess) << " / "
                  << static_cast<int>(args.readWriteAccess) << " / "
                  << static_cast<int>(args.changeAccess) << "\n";
        std::cout << "Command protection mode: ";
        if (args.commandCommunicationSettings == 0xFFU)
        {
            std::cout << "auto\n";
        }
        else
        {
            std::cout << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                      << static_cast<int>(args.commandCommunicationSettings) << std::dec << "\n";
        }

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

        const etl::array<uint8_t, 3> aid = {args.aid[0], args.aid[1], args.aid[2]};
        auto selectResult = desfire->selectApplication(aid);
        if (!selectResult)
        {
            std::cerr << "SelectApplication failed: " << selectResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "SelectApplication OK\n";

        if (args.authenticate)
        {
            const auto authKey = toEtl<24U>(args.authKey);
            auto authResult = desfire->authenticate(args.authKeyNo, authKey, args.authMode);
            if (!authResult)
            {
                std::cerr << "Authenticate failed: " << authResult.error().toString().c_str() << "\n";
                return 1;
            }
            std::cout << "Authenticate OK\n";
        }

        if (args.showBefore)
        {
            auto settingsResult = desfire->getFileSettings(args.fileNo);
            if (!settingsResult)
            {
                std::cerr << "GetFileSettings (before) failed: " << settingsResult.error().toString().c_str() << "\n";
                return 1;
            }
            printFileSettingsSummary(settingsResult.value(), "File settings (before):");
        }

        if (!args.confirmChange)
        {
            std::cout << "Dry run complete. Add --confirm-change to execute ChangeFileSettings.\n";
            return 0;
        }

        const DesfireKeyType sessionKeyType = args.sessionKeyType.value_or(DesfireKeyType::UNKNOWN);
        auto changeResult = desfire->changeFileSettings(
            args.fileNo,
            args.newCommunicationSettings,
            args.readAccess,
            args.writeAccess,
            args.readWriteAccess,
            args.changeAccess,
            args.commandCommunicationSettings,
            args.authMode,
            sessionKeyType);
        if (!changeResult)
        {
            std::cerr << "ChangeFileSettings failed: " << changeResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "ChangeFileSettings OK\n";

        if (args.showAfter)
        {
            auto settingsResult = desfire->getFileSettings(args.fileNo);
            if (!settingsResult)
            {
                std::cerr << "GetFileSettings (after) failed: " << settingsResult.error().toString().c_str() << "\n";
                return 1;
            }
            printFileSettingsSummary(settingsResult.value(), "File settings (after):");
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

