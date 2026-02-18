/**
 * @file main.cpp
 * @brief DESFire SetConfiguration example
 *
 * Flow:
 *   1) Select PICC application (000000)
 *   2) Authenticate with PICC master key
 *   3) SetConfiguration (PICC flags or ATS)
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
    enum class OperationMode : uint8_t
    {
        PiccConfig,
        Ats
    };

    struct Args
    {
        std::string comPort;
        int baudRate = 115200;
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        std::optional<DesfireKeyType> sessionKeyType;
        uint8_t authKeyNo = 0x00;
        std::vector<uint8_t> authKey;

        std::optional<OperationMode> mode;

        bool configByteProvided = false;
        uint8_t piccConfigByte = 0x00;
        std::optional<bool> disableFormat;
        std::optional<bool> enableRandomUid;

        std::vector<uint8_t> ats;

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
        oss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
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

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>                                 Default: 115200\n";
        std::cout << "  --auth-mode <legacy|iso|aes|des|2k3des|3k3des> Default: iso\n";
        std::cout << "  --session-key-type <des|2k3des|3k3des|aes> Optional\n";
        std::cout << "  --auth-key-no <n>                          Default: 0\n";
        std::cout << "  --auth-key-hex <hex>                       Required\n";
        std::cout << "  --mode <picc|ats>                          Required\n";
        std::cout << "  --config-byte <n>                          Base PICC config byte (for mode=picc)\n";
        std::cout << "  --disable-format <0|1>                     Bit0 override (for mode=picc)\n";
        std::cout << "  --enable-random-uid <0|1>                  Bit1 override (for mode=picc)\n";
        std::cout << "  --ats-hex <hex>                            ATS bytes (for mode=ats)\n";
        std::cout << "  --confirm-change                           Actually executes SetConfiguration\n\n";
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
            else if (opt == "--mode")
            {
                const std::string modeText = toLower(requireValue("--mode"));
                if (modeText == "picc")
                {
                    args.mode = OperationMode::PiccConfig;
                }
                else if (modeText == "ats")
                {
                    args.mode = OperationMode::Ats;
                }
                else
                {
                    throw std::runtime_error("Invalid --mode value: " + modeText);
                }
            }
            else if (opt == "--config-byte")
            {
                args.piccConfigByte = parseByte(requireValue("--config-byte"));
                args.configByteProvided = true;
            }
            else if (opt == "--disable-format")
            {
                args.disableFormat = parseBoolStrict(requireValue("--disable-format"));
            }
            else if (opt == "--enable-random-uid")
            {
                args.enableRandomUid = parseBoolStrict(requireValue("--enable-random-uid"));
            }
            else if (opt == "--ats-hex")
            {
                args.ats = parseHex(requireValue("--ats-hex"));
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

        if (args.authKey.empty())
        {
            throw std::runtime_error("--auth-key-hex is required");
        }
        if (!isAuthKeyLengthValid(args.authMode, args.authKey.size()))
        {
            throw std::runtime_error("Invalid --auth-key-hex length for selected --auth-mode");
        }
        if (!args.mode.has_value())
        {
            throw std::runtime_error("--mode is required");
        }

        if (args.mode.value() == OperationMode::PiccConfig)
        {
            const bool hasHelpers =
                args.disableFormat.has_value() ||
                args.enableRandomUid.has_value();
            if (!args.configByteProvided && !hasHelpers)
            {
                throw std::runtime_error(
                    "mode=picc requires --config-byte and/or helper overrides");
            }

            auto applyBit = [&](const std::optional<bool>& value, uint8_t mask)
            {
                if (!value.has_value())
                {
                    return;
                }
                if (value.value())
                {
                    args.piccConfigByte = static_cast<uint8_t>(args.piccConfigByte | mask);
                }
                else
                {
                    args.piccConfigByte = static_cast<uint8_t>(args.piccConfigByte & static_cast<uint8_t>(~mask));
                }
            };

            applyBit(args.disableFormat, 0x01U);
            applyBit(args.enableRandomUid, 0x02U);
        }
        else
        {
            if (args.ats.empty())
            {
                throw std::runtime_error("mode=ats requires --ats-hex");
            }
        }

        return args;
    }

    std::vector<uint8_t> normalizeAtsWithTl(const std::vector<uint8_t>& input)
    {
        if (input.empty())
        {
            throw std::runtime_error("ATS payload is empty");
        }

        std::vector<uint8_t> out;

        if (input[0] == static_cast<uint8_t>(input.size()))
        {
            out = input;
        }
        else
        {
            if (input.size() >= 20U)
            {
                throw std::runtime_error(
                    "ATS payload too long without TL. Max 19 bytes when TL is auto-added.");
            }
            out.reserve(input.size() + 1U);
            out.push_back(static_cast<uint8_t>(input.size() + 1U));
            for (size_t i = 0; i < input.size(); ++i)
            {
                out.push_back(input[i]);
            }
        }

        if (out.size() > 20U)
        {
            throw std::runtime_error("ATS including TL must be <= 20 bytes");
        }

        if (out[0] != out.size())
        {
            throw std::runtime_error("ATS TL byte does not match ATS length");
        }

        return out;
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
        Args args = parseArgs(argc, argv);

        const etl::array<uint8_t, 3> piccAid = {0x00, 0x00, 0x00};

        std::vector<uint8_t> normalizedAts;
        if (args.mode.value() == OperationMode::Ats)
        {
            normalizedAts = normalizeAtsWithTl(args.ats);
        }

        std::cout << "DESFire SetConfiguration Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";
        std::cout << "Scope: PICC (AID 00 00 00)\n";
        if (args.mode.value() == OperationMode::PiccConfig)
        {
            std::cout << "Mode: PICC config\n";
            std::cout << "Config byte: " << hexByte(args.piccConfigByte) << "\n";
            std::cout << "  disable_format: " << (((args.piccConfigByte & 0x01U) != 0U) ? "true" : "false") << "\n";
            std::cout << "  enable_random_uid: " << (((args.piccConfigByte & 0x02U) != 0U) ? "true" : "false") << "\n";
        }
        else
        {
            std::cout << "Mode: ATS\n";
            std::cout << "ATS bytes: " << toHex(normalizedAts) << "\n";
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

        auto selectResult = desfire->selectApplication(piccAid);
        if (!selectResult)
        {
            std::cerr << "SelectApplication(PICC) failed: "
                      << selectResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "SelectApplication(PICC) OK\n";

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
            std::cout << "Dry run complete. Add --confirm-change to execute SetConfiguration.\n";
            return 0;
        }

        const DesfireKeyType sessionKeyType = args.sessionKeyType.value_or(DesfireKeyType::UNKNOWN);
        if (args.mode.value() == OperationMode::PiccConfig)
        {
            auto setResult = desfire->setConfigurationPicc(
                args.piccConfigByte,
                args.authMode,
                sessionKeyType);
            if (!setResult)
            {
                std::cerr << "SetConfiguration(PICC) failed: "
                          << setResult.error().toString().c_str() << "\n";
                return 1;
            }
            std::cout << "SetConfiguration(PICC) OK\n";
        }
        else
        {
            const etl::vector<uint8_t, 32> ats = toEtl<32>(normalizedAts);
            auto setResult = desfire->setConfigurationAts(
                ats,
                args.authMode,
                sessionKeyType);
            if (!setResult)
            {
                std::cerr << "SetConfiguration(ATS) failed: "
                          << setResult.error().toString().c_str() << "\n";
                return 1;
            }
            std::cout << "SetConfiguration(ATS) OK\n";
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

