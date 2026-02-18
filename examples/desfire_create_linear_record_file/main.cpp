/**
 * @file main.cpp
 * @brief DESFire CreateLinearRecordFile example
 *
 * Flow:
 *   1) Select target application
 *   2) Authenticate with an application key
 *   3) Create linear record file
 */

#include <algorithm>
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
        std::vector<uint8_t> aid = {0x00, 0x00, 0x00};
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        uint8_t authKeyNo = 0x00;
        std::vector<uint8_t> authKey;

        uint8_t fileNo = 0x00;
        uint8_t communicationSettings = 0x00; // plain
        uint8_t readAccess = 0x00;
        uint8_t writeAccess = 0x00;
        uint8_t readWriteAccess = 0x00;
        uint8_t changeAccess = 0x00;
        uint32_t recordSize = 0U;
        uint32_t maxRecords = 0U;

        bool allowExisting = false;
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

    uint32_t parseUInt32(const std::string& value)
    {
        const unsigned long parsed = std::stoul(value, nullptr, 0);
        if (parsed > 0xFFFFFFFFUL)
        {
            throw std::runtime_error("Value out of uint32 range: " + value);
        }
        return static_cast<uint32_t>(parsed);
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

    uint8_t parseCommunicationSettings(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "plain")
        {
            return 0x00;
        }
        if (lower == "mac")
        {
            return 0x01;
        }
        if (lower == "enc" || lower == "enciphered")
        {
            return 0x03;
        }

        const uint8_t value = parseByte(text);
        if (value != 0x00 && value != 0x01 && value != 0x03)
        {
            throw std::runtime_error("--comm-mode must be plain|mac|enc or 0x00|0x01|0x03");
        }
        return value;
    }

    uint8_t parseAccessRight(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "free")
        {
            return 0x0E;
        }
        if (lower == "never")
        {
            return 0x0F;
        }
        if (lower.rfind("key", 0U) == 0U)
        {
            const std::string suffix = lower.substr(3);
            if (suffix.empty())
            {
                throw std::runtime_error("Invalid access right token: " + text);
            }

            const int keyNo = std::stoi(suffix, nullptr, 10);
            if (keyNo < 0 || keyNo > 13)
            {
                throw std::runtime_error("keyN access right must be in range key0..key13");
            }
            return static_cast<uint8_t>(keyNo);
        }

        const uint8_t value = parseByte(text);
        if (value > 0x0F)
        {
            throw std::runtime_error("Access right nibble out of range (0..15): " + text);
        }
        return value;
    }

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>                               Default: 115200\n";
        std::cout << "  --aid <hex6>                             Required (3-byte application AID)\n";
        std::cout << "  --auth-mode <legacy|iso|aes|des|2k3des|3k3des> Default: iso\n";
        std::cout << "  --auth-key-no <n>                        Default: 0\n";
        std::cout << "  --auth-key-hex <hex>                     Required\n";
        std::cout << "  --file-no <n>                            Default: 0 (0..31)\n";
        std::cout << "  --comm-mode <plain|mac|enc|0x00|0x01|0x03> Default: plain\n";
        std::cout << "  --read-access <n|keyN|free|never>        Default: 0\n";
        std::cout << "  --write-access <n|keyN|free|never>       Default: 0\n";
        std::cout << "  --read-write-access <n|keyN|free|never>  Default: 0\n";
        std::cout << "  --change-access <n|keyN|free|never>      Default: 0\n";
        std::cout << "  --record-size <n>                        Required (1..16777215)\n";
        std::cout << "  --max-records <n>                        Required (1..16777215)\n";
        std::cout << "  --allow-existing                         Continue on DuplicateError\n";
    }

    Args parseArgs(int argc, char* argv[])
    {
        if (argc < 2)
        {
            throw std::runtime_error("Missing COM port");
        }

        Args args;
        args.comPort = argv[1];

        bool aidProvided = false;
        bool authKeyProvided = false;
        bool recordSizeProvided = false;
        bool maxRecordsProvided = false;

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
                aidProvided = true;
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
                authKeyProvided = true;
            }
            else if (opt == "--file-no")
            {
                args.fileNo = parseByte(requireValue("--file-no"));
            }
            else if (opt == "--comm-mode")
            {
                args.communicationSettings = parseCommunicationSettings(requireValue("--comm-mode"));
            }
            else if (opt == "--read-access")
            {
                args.readAccess = parseAccessRight(requireValue("--read-access"));
            }
            else if (opt == "--write-access")
            {
                args.writeAccess = parseAccessRight(requireValue("--write-access"));
            }
            else if (opt == "--read-write-access")
            {
                args.readWriteAccess = parseAccessRight(requireValue("--read-write-access"));
            }
            else if (opt == "--change-access")
            {
                args.changeAccess = parseAccessRight(requireValue("--change-access"));
            }
            else if (opt == "--record-size")
            {
                args.recordSize = parseUInt32(requireValue("--record-size"));
                recordSizeProvided = true;
            }
            else if (opt == "--max-records")
            {
                args.maxRecords = parseUInt32(requireValue("--max-records"));
                maxRecordsProvided = true;
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

        if (!aidProvided)
        {
            throw std::runtime_error("--aid is required");
        }
        if (args.aid.size() != 3)
        {
            throw std::runtime_error("--aid must be exactly 3 bytes");
        }
        if (!authKeyProvided)
        {
            throw std::runtime_error("--auth-key-hex is required");
        }
        if (!isAuthKeyLengthValid(args.authMode, args.authKey.size()))
        {
            throw std::runtime_error("Invalid --auth-key-hex length for selected --auth-mode");
        }
        if (args.fileNo > 0x1F)
        {
            throw std::runtime_error("--file-no must be in range 0..31");
        }
        if (!recordSizeProvided)
        {
            throw std::runtime_error("--record-size is required");
        }
        if (args.recordSize == 0U || args.recordSize > 0x00FFFFFFU)
        {
            throw std::runtime_error("--record-size must be in range 1..16777215");
        }
        if (!maxRecordsProvided)
        {
            throw std::runtime_error("--max-records is required");
        }
        if (args.maxRecords == 0U || args.maxRecords > 0x00FFFFFFU)
        {
            throw std::runtime_error("--max-records must be in range 1..16777215");
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

        std::cout << "DESFire CreateLinearRecordFile Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";
        std::cout << "AID: " << toHex(args.aid) << "\n";
        std::cout << "File no: " << static_cast<int>(args.fileNo) << "\n";
        std::cout << "Record size: " << args.recordSize << "\n";
        std::cout << "Max records: " << args.maxRecords << "\n";

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

        auto createResult = desfire->createLinearRecordFile(
            args.fileNo,
            args.communicationSettings,
            args.readAccess,
            args.writeAccess,
            args.readWriteAccess,
            args.changeAccess,
            args.recordSize,
            args.maxRecords);
        if (!createResult)
        {
            if (args.allowExisting &&
                createResult.error().is<error::DesfireError>() &&
                createResult.error().get<error::DesfireError>() == error::DesfireError::DuplicateError)
            {
                std::cout << "CreateLinearRecordFile returned DuplicateError; continuing (--allow-existing)\n";
                return 0;
            }

            std::cerr << "CreateLinearRecordFile failed: " << createResult.error().toString().c_str() << "\n";
            return 1;
        }

        std::cout << "CreateLinearRecordFile OK\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        printUsage(argv[0]);
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}

