/**
 * @file main.cpp
 * @brief DESFire ChangeKeySettings example
 *
 * Flow:
 *   1) Select application (default PICC 000000)
 *   2) Authenticate with selected key
 *   3) ChangeKeySettings (INS 0x54)
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
#include <etl/string.h>
#include <etl/vector.h>
#include "Comms/Serial/SerialBusWin.hpp"
#include "Pn532/Pn532Driver.h"
#include "Pn532/Pn532ApduAdapter.h"
#include "Nfc/Card/CardManager.h"
#include "Nfc/Card/ReaderCapabilities.h"
#include "Nfc/Desfire/DesfireAuthMode.h"
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
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        std::optional<DesfireKeyType> sessionKeyType;
        uint8_t authKeyNo = 0x00;
        std::vector<uint8_t> authKey;

        bool keySettingsProvided = false;
        uint8_t keySettings = 0x00;
        std::optional<bool> allowChangeMk;
        std::optional<bool> allowListingWithoutMk;
        std::optional<bool> allowCreateDeleteWithoutMk;
        std::optional<bool> configurationChangeable;
        std::optional<uint8_t> changeKeyAccess;

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

    bool parseBoolStrict(const std::string& value)
    {
        const std::string lower = toLower(value);
        if (lower == "1" || lower == "true" || lower == "yes" || lower == "on")
        {
            return true;
        }
        if (lower == "0" || lower == "false" || lower == "no" || lower == "off")
        {
            return false;
        }
        throw std::runtime_error("Invalid boolean value: " + value);
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

    std::string hexByte(uint8_t value)
    {
        std::ostringstream oss;
        oss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(value);
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
            return keyLen == 16;
        }
        if (mode == DesfireAuthMode::ISO)
        {
            return keyLen == 8 || keyLen == 16 || keyLen == 24;
        }
        return keyLen == 8 || keyLen == 16;
    }

    uint8_t parseChangeKeyAccess(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "mk" || lower == "master" || lower == "masterkey")
        {
            return 0x00;
        }
        if (lower == "same" || lower == "same-key" || lower == "target" || lower == "target-key")
        {
            return 0x0E;
        }
        if (lower == "frozen" || lower == "freeze")
        {
            return 0x0F;
        }
        if (lower.rfind("key", 0U) == 0U)
        {
            const std::string suffix = lower.substr(3);
            if (suffix.empty())
            {
                throw std::runtime_error("Invalid --change-key-access value: " + text);
            }
            const int keyNo = std::stoi(suffix, nullptr, 10);
            if (keyNo < 0 || keyNo > 13)
            {
                throw std::runtime_error("--change-key-access keyN supports key0..key13");
            }
            return static_cast<uint8_t>(keyNo);
        }

        const uint8_t value = parseByte(text);
        if (value > 0x0F)
        {
            throw std::runtime_error("--change-key-access numeric value must be in range 0..15");
        }
        return value;
    }

    const char* changeKeyAccessToText(uint8_t nibble)
    {
        if (nibble <= 0x0D)
        {
            return "specific key";
        }
        if (nibble == 0x0E)
        {
            return "same key being changed";
        }
        return "all key changes frozen";
    }

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>                                 Default: 115200\n";
        std::cout << "  --aid <hex6>                               Default: 000000 (PICC)\n";
        std::cout << "  --auth-mode <legacy|iso|aes|des|2k3des|3k3des> Default: iso\n";
        std::cout << "  --session-key-type <des|2k3des|3k3des|aes> Optional\n";
        std::cout << "  --auth-key-no <n>                          Default: 0\n";
        std::cout << "  --auth-key-hex <hex>                       Required\n";
        std::cout << "  --key-settings <n>                         Base KeySettings1 byte\n";
        std::cout << "  --allow-change-mk <0|1>                    Bit0 override\n";
        std::cout << "  --allow-listing-without-mk <0|1>           Bit1 override\n";
        std::cout << "  --allow-create-delete-without-mk <0|1>     Bit2 override\n";
        std::cout << "  --configuration-changeable <0|1>           Bit3 override\n";
        std::cout << "  --change-key-access <mk|keyN|same|frozen|0..15> High nibble override\n";
        std::cout << "  --confirm-change                           Actually executes ChangeKeySettings\n\n";
        std::cout << "Safety:\n";
        std::cout << "  Without --confirm-change the tool authenticates only and exits.\n";
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
            else if (opt == "--aid")
            {
                args.aid = parseHex(requireValue("--aid"));
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
            else if (opt == "--key-settings")
            {
                args.keySettings = parseByte(requireValue("--key-settings"));
                args.keySettingsProvided = true;
            }
            else if (opt == "--allow-change-mk")
            {
                args.allowChangeMk = parseBoolStrict(requireValue("--allow-change-mk"));
            }
            else if (opt == "--allow-listing-without-mk")
            {
                args.allowListingWithoutMk = parseBoolStrict(requireValue("--allow-listing-without-mk"));
            }
            else if (opt == "--allow-create-delete-without-mk")
            {
                args.allowCreateDeleteWithoutMk =
                    parseBoolStrict(requireValue("--allow-create-delete-without-mk"));
            }
            else if (opt == "--configuration-changeable")
            {
                args.configurationChangeable = parseBoolStrict(requireValue("--configuration-changeable"));
            }
            else if (opt == "--change-key-access")
            {
                args.changeKeyAccess = parseChangeKeyAccess(requireValue("--change-key-access"));
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

        if (args.aid.size() != 3)
        {
            throw std::runtime_error("--aid must be exactly 3 bytes");
        }
        if (args.authKey.empty())
        {
            throw std::runtime_error("--auth-key-hex is required");
        }
        if (!isAuthKeyLengthValid(args.authMode, args.authKey.size()))
        {
            throw std::runtime_error("Invalid --auth-key-hex length for selected --auth-mode");
        }

        const bool hasAnyHelper =
            args.allowChangeMk.has_value() ||
            args.allowListingWithoutMk.has_value() ||
            args.allowCreateDeleteWithoutMk.has_value() ||
            args.configurationChangeable.has_value() ||
            args.changeKeyAccess.has_value();

        if (!args.keySettingsProvided && !hasAnyHelper)
        {
            throw std::runtime_error(
                "Provide --key-settings or at least one helper override option");
        }

        auto applyBit = [&](const std::optional<bool>& value, uint8_t mask)
        {
            if (!value.has_value())
            {
                return;
            }
            if (value.value())
            {
                args.keySettings = static_cast<uint8_t>(args.keySettings | mask);
            }
            else
            {
                args.keySettings = static_cast<uint8_t>(args.keySettings & static_cast<uint8_t>(~mask));
            }
        };

        applyBit(args.allowChangeMk, 0x01U);
        applyBit(args.allowListingWithoutMk, 0x02U);
        applyBit(args.allowCreateDeleteWithoutMk, 0x04U);
        applyBit(args.configurationChangeable, 0x08U);

        if (args.changeKeyAccess.has_value())
        {
            const uint8_t lowNibble = static_cast<uint8_t>(args.keySettings & 0x0FU);
            args.keySettings = static_cast<uint8_t>(lowNibble | static_cast<uint8_t>(args.changeKeyAccess.value() << 4U));
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

        const uint8_t changeRule = static_cast<uint8_t>((args.keySettings >> 4U) & 0x0FU);
        std::cout << "DESFire ChangeKeySettings Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";
        std::cout << "AID: " << toHex(args.aid) << "\n";
        std::cout << "Auth key no: " << static_cast<int>(args.authKeyNo) << "\n";
        std::cout << "New KeySettings1: " << hexByte(args.keySettings) << "\n";
        std::cout << "  allow_change_mk: " << (((args.keySettings & 0x01U) != 0U) ? "true" : "false") << "\n";
        std::cout << "  listing_without_mk: " << (((args.keySettings & 0x02U) != 0U) ? "true" : "false") << "\n";
        std::cout << "  create_delete_without_mk: " << (((args.keySettings & 0x04U) != 0U) ? "true" : "false") << "\n";
        std::cout << "  configuration_changeable: " << (((args.keySettings & 0x08U) != 0U) ? "true" : "false") << "\n";
        if (changeRule <= 0x0DU)
        {
            std::cout << "  change_key_access: key" << static_cast<int>(changeRule)
                      << " (" << changeKeyAccessToText(changeRule) << ")\n";
        }
        else
        {
            std::cout << "  change_key_access: " << static_cast<int>(changeRule)
                      << " (" << changeKeyAccessToText(changeRule) << ")\n";
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

        const etl::vector<uint8_t, 24> authKey = toEtl<24>(args.authKey);
        auto authResult = desfire->authenticate(args.authKeyNo, authKey, args.authMode);
        if (!authResult)
        {
            std::cerr << "Authenticate failed: " << authResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "Authenticate OK\n";

        if (!args.confirmChange)
        {
            std::cout << "Dry run complete. Add --confirm-change to execute ChangeKeySettings.\n";
            return 0;
        }

        const DesfireKeyType sessionKeyType = args.sessionKeyType.value_or(DesfireKeyType::UNKNOWN);
        auto changeResult = desfire->changeKeySettings(args.keySettings, args.authMode, sessionKeyType);
        if (!changeResult)
        {
            std::cerr << "ChangeKeySettings failed: " << changeResult.error().toString().c_str() << "\n";
            return 1;
        }

        std::cout << "ChangeKeySettings OK\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        printUsage(argv[0]);
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}

