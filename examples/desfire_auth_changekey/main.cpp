/**
 * @file main.cpp
 * @brief DESFire authenticate and change key example
 *
 * Usage (example):
 *   desfire_auth_changekey_example COM5 --auth-key-hex 00000000000000000000000000000000 --new-key-hex 00112233445566778899AABBCCDDEEFF --confirm-change
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
#include "Comms/Serial/SerialBusWin.hpp"
#include "Pn532/Pn532Driver.h"
#include "Pn532/Pn532ApduAdapter.h"
#include "Nfc/Card/CardManager.h"
#include "Nfc/Card/ReaderCapabilities.h"
#include "Nfc/Desfire/DesfireAuthMode.h"
#include "Nfc/Desfire/DesfireKeyType.h"
#include "Nfc/Desfire/Commands/ChangeKeyCommand.h"
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
        uint8_t authKeyNo = 0x00;
        uint8_t changeKeyNo = 0x00;
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        std::optional<DesfireKeyType> currentKeyType;
        DesfireKeyType newKeyType = DesfireKeyType::AES;
        std::optional<DesfireKeyType> oldKeyType;
        uint8_t newKeyVersion = 0x00;
        std::vector<uint8_t> aid = {0x00, 0x00, 0x00}; // PICC application
        std::vector<uint8_t> authKey;
        std::vector<uint8_t> newKey;
        std::optional<std::vector<uint8_t>> oldKey;
        bool confirmChange = false;
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

    size_t expectedNewKeySize(DesfireKeyType keyType)
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

    enum class KeyFamily : uint8_t
    {
        DesOr2K,
        ThreeK,
        Aes,
        Unknown
    };

    KeyFamily keyFamilyFromType(DesfireKeyType keyType)
    {
        switch (keyType)
        {
            case DesfireKeyType::DES:
            case DesfireKeyType::DES3_2K:
                return KeyFamily::DesOr2K;
            case DesfireKeyType::DES3_3K:
                return KeyFamily::ThreeK;
            case DesfireKeyType::AES:
                return KeyFamily::Aes;
            default:
                return KeyFamily::Unknown;
        }
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

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>               Default: 115200\n";
        std::cout << "  --aid <hex6>             Default: 000000 (PICC)\n";
        std::cout << "  --auth-mode <legacy|iso|aes|des|2k3des|3k3des>   Default: iso\n";
        std::cout << "  --current-key-type <des|2k3des|3k3des|aes> Optional but recommended\n";
        std::cout << "  --auth-key-no <n>        Default: 0\n";
        std::cout << "  --auth-key-hex <hex>     Required (16 bytes for aes, 8/16/24 for legacy/iso)\n";
        std::cout << "  --change-key-no <n>      Default: 0\n";
        std::cout << "  --new-key-type <des|2k3des|3k3des|aes>   Default: aes\n";
        std::cout << "  --new-key-hex <hex>      Required\n";
        std::cout << "  --old-key-hex <hex>      Required when changing a different key slot\n";
        std::cout << "  --old-key-type <des|2k3des|3k3des|aes>   Optional (defaults to --current-key-type, else --new-key-type)\n";
        std::cout << "  --new-key-version <n>    Default: 0 (used for AES key change payload)\n";
        std::cout << "  --confirm-change         Actually executes ChangeKey\n\n";
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
            else if (opt == "--current-key-type")
            {
                args.currentKeyType = parseKeyType(requireValue("--current-key-type"));
            }
            else if (opt == "--auth-key-no")
            {
                args.authKeyNo = parseByte(requireValue("--auth-key-no"));
            }
            else if (opt == "--auth-key-hex")
            {
                args.authKey = parseHex(requireValue("--auth-key-hex"));
            }
            else if (opt == "--change-key-no")
            {
                args.changeKeyNo = parseByte(requireValue("--change-key-no"));
            }
            else if (opt == "--new-key-type")
            {
                args.newKeyType = parseKeyType(requireValue("--new-key-type"));
            }
            else if (opt == "--new-key-hex")
            {
                args.newKey = parseHex(requireValue("--new-key-hex"));
            }
            else if (opt == "--old-key-type")
            {
                args.oldKeyType = parseKeyType(requireValue("--old-key-type"));
            }
            else if (opt == "--old-key-hex")
            {
                args.oldKey = parseHex(requireValue("--old-key-hex"));
            }
            else if (opt == "--new-key-version")
            {
                args.newKeyVersion = parseByte(requireValue("--new-key-version"));
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
        if (args.newKey.empty())
        {
            throw std::runtime_error("--new-key-hex is required");
        }
        if (args.aid.size() != 3)
        {
            throw std::runtime_error("--aid must be exactly 3 bytes");
        }
        if (args.authMode == DesfireAuthMode::AES)
        {
            if (args.authKey.size() != 16)
            {
                throw std::runtime_error("For AES auth, --auth-key-hex must be 16 bytes");
            }
        }
        else if (args.authMode == DesfireAuthMode::LEGACY || args.authMode == DesfireAuthMode::ISO)
        {
            if (args.authMode == DesfireAuthMode::ISO)
            {
                if (args.authKey.size() != 8 && args.authKey.size() != 16 && args.authKey.size() != 24)
                {
                    throw std::runtime_error("For ISO auth, --auth-key-hex must be 8, 16, or 24 bytes");
                }
            }
            else if (args.authKey.size() != 8 && args.authKey.size() != 16)
            {
                throw std::runtime_error("For legacy auth, --auth-key-hex must be 8 or 16 bytes");
            }
        }
        else
        {
            throw std::runtime_error("Unsupported --auth-mode value");
        }

        if (args.currentKeyType.has_value())
        {
            if (args.currentKeyType.value() == DesfireKeyType::AES && args.authMode != DesfireAuthMode::AES)
            {
                throw std::runtime_error("AES current key type requires --auth-mode aes");
            }
            if (args.currentKeyType.value() == DesfireKeyType::DES3_3K &&
                (args.authMode != DesfireAuthMode::ISO || args.authKey.size() != 24))
            {
                throw std::runtime_error("3k3des current key type requires --auth-mode iso with 24-byte --auth-key-hex");
            }
            if ((args.currentKeyType.value() == DesfireKeyType::DES ||
                 args.currentKeyType.value() == DesfireKeyType::DES3_2K) &&
                args.authMode == DesfireAuthMode::AES)
            {
                throw std::runtime_error("des/2k3des current key type is incompatible with --auth-mode aes");
            }
        }

        const size_t requiredNewKeySize = expectedNewKeySize(args.newKeyType);
        if (requiredNewKeySize == 0 || args.newKey.size() != requiredNewKeySize)
        {
            throw std::runtime_error("New key length does not match --new-key-type");
        }

        if (args.oldKeyType.has_value() && !args.oldKey.has_value())
        {
            throw std::runtime_error("--old-key-type requires --old-key-hex");
        }

        if (args.oldKey.has_value())
        {
            const DesfireKeyType effectiveOldKeyType =
                args.oldKeyType.value_or(args.currentKeyType.value_or(args.newKeyType));
            const size_t requiredOldKeySize = expectedNewKeySize(effectiveOldKeyType);
            if (requiredOldKeySize == 0)
            {
                throw std::runtime_error("Invalid --old-key-type");
            }

            if (args.oldKey->size() != requiredOldKeySize &&
                !(effectiveOldKeyType == DesfireKeyType::DES && args.oldKey->size() == 16))
            {
                throw std::runtime_error("--old-key-hex length does not match --old-key-type");
            }
        }

        auto inferCurrentKeyType = [&]() -> DesfireKeyType
        {
            if (args.currentKeyType.has_value())
            {
                return args.currentKeyType.value();
            }

            if (args.authMode == DesfireAuthMode::AES)
            {
                return DesfireKeyType::AES;
            }

            if (args.authMode == DesfireAuthMode::ISO && args.authKey.size() == 24)
            {
                return DesfireKeyType::DES3_3K;
            }

            if (args.authKey.size() == 16)
            {
                return DesfireKeyType::DES3_2K;
            }

            return DesfireKeyType::DES;
        };

        const bool piccSelected = args.aid[0] == 0x00 && args.aid[1] == 0x00 && args.aid[2] == 0x00;
        if (!piccSelected)
        {
            const KeyFamily currentFamily = keyFamilyFromType(inferCurrentKeyType());
            const KeyFamily requestedFamily = keyFamilyFromType(args.newKeyType);
            if (currentFamily != KeyFamily::Unknown &&
                requestedFamily != KeyFamily::Unknown &&
                currentFamily != requestedFamily)
            {
                throw std::runtime_error(
                    "Application key family cannot be changed with ChangeKey. "
                    "Use DeleteApplication/CreateApplication to switch between DES/2K3DES, 3K3DES, and AES.");
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

    bool isIntegrityError(const error::Error& err)
    {
        return err.is<error::DesfireError>() &&
               err.get<error::DesfireError>() == error::DesfireError::IntegrityError;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        const Args args = parseArgs(argc, argv);

        std::cout << "DESFire Authenticate + ChangeKey Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";
        std::cout << "AID: " << toHex(args.aid) << "\n";
        std::cout << "Auth key no: " << static_cast<int>(args.authKeyNo) << "\n";
        std::cout << "Change key no: " << static_cast<int>(args.changeKeyNo) << "\n";
        if (args.currentKeyType.has_value())
        {
            std::cout << "Current key type override set\n";
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

        etl::array<uint8_t, 3> aid = {args.aid[0], args.aid[1], args.aid[2]};
        auto selectResult = desfire->selectApplication(aid);
        if (!selectResult)
        {
            std::cerr << "SelectApplication failed: " << selectResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "SelectApplication OK\n";

        etl::vector<uint8_t, 24> authKey = toEtl<24>(args.authKey);
        auto authResult = desfire->authenticate(args.authKeyNo, authKey, args.authMode);
        if (!authResult)
        {
            std::cerr << "Authenticate failed: " << authResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "Authenticate OK\n";

        if (!args.confirmChange)
        {
            std::cout << "Skipping ChangeKey (missing --confirm-change)\n";
            return 0;
        }

        if (((args.changeKeyNo & 0x0F) != (args.authKeyNo & 0x0F)) && !args.oldKey.has_value())
        {
            std::cerr << "Changing a different key slot requires --old-key-hex\n";
            return 1;
        }

        auto runChangeKey = [&](DesfireAuthMode changeCryptoMode, ChangeKeyLegacyIvMode legacyIvMode) -> etl::expected<void, error::Error>
        {
            ChangeKeyCommandOptions changeOptions;
            changeOptions.keyNo = args.changeKeyNo;
            changeOptions.authMode = changeCryptoMode;
            changeOptions.sessionKeyType = args.currentKeyType.value_or(DesfireKeyType::UNKNOWN);
            changeOptions.newKeyType = args.newKeyType;
            const DesfireKeyType effectiveOldKeyType =
                args.oldKeyType.value_or(args.currentKeyType.value_or(DesfireKeyType::UNKNOWN));
            changeOptions.oldKeyType = effectiveOldKeyType;
            changeOptions.newKey = toEtl<24>(args.newKey);
            changeOptions.newKeyVersion = args.newKeyVersion;
            changeOptions.legacyIvMode = legacyIvMode;
            if (args.oldKey.has_value())
            {
                changeOptions.oldKey = toEtl<24>(*args.oldKey);
            }

            ChangeKeyCommand changeCommand(changeOptions);
            return desfire->executeCommand(changeCommand);
        };

        auto changeResult = runChangeKey(args.authMode, ChangeKeyLegacyIvMode::Zero);
        if (!changeResult &&
            args.authMode == DesfireAuthMode::ISO &&
            args.authKey.size() != 24 &&
            isIntegrityError(changeResult.error()))
        {
            std::cout << "ChangeKey returned IntegrityError in ISO mode.\n";
            std::cout << "Retrying with legacy authenticate + legacy ChangeKey framing...\n";

            auto legacyAuthResult = desfire->authenticate(args.authKeyNo, authKey, DesfireAuthMode::LEGACY);
            if (!legacyAuthResult)
            {
                std::cerr << "Legacy re-authenticate failed: " << legacyAuthResult.error().toString().c_str() << "\n";
                std::cerr << "Original ChangeKey error: " << changeResult.error().toString().c_str() << "\n";
                return 1;
            }

            changeResult = runChangeKey(DesfireAuthMode::LEGACY, ChangeKeyLegacyIvMode::Zero);
            if (changeResult)
            {
                std::cout << "ChangeKey OK (legacy fallback)\n";
                return 0;
            }

            if (isIntegrityError(changeResult.error()))
            {
                std::cout << "Legacy zero-IV ChangeKey still failed with IntegrityError.\n";
                std::cout << "Retrying legacy DES-chain with encrypted-RndB seed...\n";

                legacyAuthResult = desfire->authenticate(args.authKeyNo, authKey, DesfireAuthMode::LEGACY);
                if (!legacyAuthResult)
                {
                    std::cerr << "Legacy re-authenticate (seeded retry) failed: "
                              << legacyAuthResult.error().toString().c_str() << "\n";
                    std::cerr << "Original ChangeKey error: " << changeResult.error().toString().c_str() << "\n";
                    return 1;
                }

                changeResult = runChangeKey(DesfireAuthMode::LEGACY, ChangeKeyLegacyIvMode::SessionEncryptedRndB);
                if (changeResult)
                {
                    std::cout << "ChangeKey OK (legacy encrypted-RndB seeded fallback)\n";
                    return 0;
                }
            }
        }

        if (!changeResult)
        {
            if (changeResult.error().is<error::DesfireError>() &&
                changeResult.error().get<error::DesfireError>() == error::DesfireError::NoChanges)
            {
                std::cout << "ChangeKey returned NoChanges (card reports key unchanged)\n";
                return 0;
            }

            std::cerr << "ChangeKey failed: " << changeResult.error().toString().c_str() << "\n";
            return 1;
        }

        std::cout << "ChangeKey OK\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        printUsage(argv[0]);
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}
