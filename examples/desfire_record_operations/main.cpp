/**
 * @file main.cpp
 * @brief DESFire ReadRecords + WriteRecord example
 *
 * Flow:
 *   1) Select application
 *   2) Optional authenticate
 *   3) Optional WriteRecord
 *   4) Optional CommitTransaction
 *   5) Optional ReadRecords
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
#include "Nfc/Desfire/DesfireCard.h"

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

        bool authenticate = false;
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        uint8_t authKeyNo = 0x00;
        std::vector<uint8_t> authKey;

        uint8_t fileNo = 0x00;
        uint16_t chunkSize = 0U;

        bool writeRequested = false;
        uint32_t writeOffset = 0U;
        std::vector<uint8_t> writeData;
        bool commitAfterWrite = false;

        bool readRequested = false;
        uint32_t readOffset = 0U;
        uint32_t readCount = 0U;
        bool readCountProvided = false;
        bool autoReadAfterWrite = false;
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

    uint16_t parseUInt16(const std::string& value)
    {
        const unsigned long parsed = std::stoul(value, nullptr, 0);
        if (parsed > 0xFFFFUL)
        {
            throw std::runtime_error("Value out of uint16 range: " + value);
        }
        return static_cast<uint16_t>(parsed);
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

        for (size_t i = 0; i < filtered.size(); i += 2U)
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
        for (size_t i = 0; i < data.size(); ++i)
        {
            if (i > 0U)
            {
                oss << ' ';
            }
            oss << std::setw(2) << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    std::string toHex(const etl::ivector<uint8_t>& data)
    {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0');
        for (size_t i = 0; i < data.size(); ++i)
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

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>                                Default: 115200\n";
        std::cout << "  --aid <hex6>                              Default: 000000\n";
        std::cout << "  --file-no <n>                             Default: 0 (0..31)\n";
        std::cout << "  --chunk-size <n>                          Default: command default (max 240)\n";
        std::cout << "  --authenticate                            Authenticate before read/write\n";
        std::cout << "  --auth-mode <legacy|iso|aes|des|2k3des|3k3des> Default: iso\n";
        std::cout << "  --auth-key-no <n>                         Default: 0\n";
        std::cout << "  --auth-key-hex <hex>                      Required when --authenticate is set\n";
        std::cout << "  --write-offset <n>                        Default: 0 (byte offset within record)\n";
        std::cout << "  --write-hex <hex>                         Record payload bytes\n";
        std::cout << "  --commit                                  Commit after write\n";
        std::cout << "  --read-offset <n>                         Default: 0\n";
        std::cout << "  --read-count <n>                          Records to read (0 means all from offset)\n";
        std::cout << "  --read-length <n>                         Alias for --read-count\n";
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
            }
            else if (opt == "--chunk-size")
            {
                args.chunkSize = parseUInt16(requireValue("--chunk-size"));
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
            else if (opt == "--write-offset")
            {
                args.writeOffset = parseUInt32(requireValue("--write-offset"));
            }
            else if (opt == "--write-hex")
            {
                args.writeData = parseHex(requireValue("--write-hex"));
                args.writeRequested = true;
            }
            else if (opt == "--commit")
            {
                args.commitAfterWrite = true;
            }
            else if (opt == "--read-offset")
            {
                args.readOffset = parseUInt32(requireValue("--read-offset"));
            }
            else if (opt == "--read-length")
            {
                args.readCount = parseUInt32(requireValue("--read-length"));
                args.readRequested = true;
                args.readCountProvided = true;
            }
            else if (opt == "--read-count")
            {
                args.readCount = parseUInt32(requireValue("--read-count"));
                args.readRequested = true;
                args.readCountProvided = true;
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
        if (args.fileNo > 0x1FU)
        {
            throw std::runtime_error("--file-no must be in range 0..31");
        }
        if (args.chunkSize > 240U)
        {
            throw std::runtime_error("--chunk-size must be in range 0..240");
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
        if (args.writeRequested && args.writeData.empty())
        {
            throw std::runtime_error("--write-hex cannot be empty");
        }

        if (!args.readRequested && args.writeRequested)
        {
            args.readRequested = true;
            args.readOffset = 0U;
            args.autoReadAfterWrite = true;
        }

        if (!args.readRequested && !args.writeRequested)
        {
            throw std::runtime_error("Specify at least one operation: --write-hex and/or --read-count");
        }
        if (args.commitAfterWrite && !args.writeRequested)
        {
            throw std::runtime_error("--commit can only be used together with --write-hex");
        }

        if (args.readRequested && args.readCount > 0x00FFFFFFU)
        {
            throw std::runtime_error("--read-count must be in range 0..16777215");
        }
        if (args.writeRequested && args.writeData.size() > DesfireCard::MAX_DATA_IO_SIZE)
        {
            throw std::runtime_error("--write-hex exceeds supported max (4096 bytes)");
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
        Args args = parseArgs(argc, argv);

        std::cout << "DESFire ReadRecords + WriteRecord Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";
        std::cout << "AID: " << toHex(args.aid) << "\n";
        std::cout << "File no: " << static_cast<int>(args.fileNo) << "\n";

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

        auto settingsResult = desfire->getFileSettings(args.fileNo);
        if (!settingsResult)
        {
            std::cerr << "GetFileSettings failed: " << settingsResult.error().toString().c_str() << "\n";
            return 1;
        }
        auto settings = settingsResult.value();
        if (!settings.hasRecordSettings || settings.recordSize == 0U)
        {
            std::cerr << "Selected file is not a linear/cyclic record file\n";
            return 1;
        }

        std::cout << "Record size: " << settings.recordSize << " bytes\n";
        std::cout << "Current/Max records: " << settings.currentRecords << " / " << settings.maxRecords << "\n";

        if (args.writeRequested)
        {
            if (args.writeOffset >= settings.recordSize)
            {
                std::cerr << "write-offset out of range. offset=" << args.writeOffset
                          << ", recordSize=" << settings.recordSize << "\n";
                return 1;
            }

            const uint64_t writeEnd = static_cast<uint64_t>(args.writeOffset) + static_cast<uint64_t>(args.writeData.size());
            if (writeEnd > static_cast<uint64_t>(settings.recordSize))
            {
                std::cerr << "Write exceeds record boundary. offset=" << args.writeOffset
                          << ", dataLen=" << args.writeData.size()
                          << ", recordSize=" << settings.recordSize << "\n";
                return 1;
            }

            const auto writeData = toEtl<DesfireCard::MAX_DATA_IO_SIZE>(args.writeData);
            auto writeResult = desfire->writeRecord(args.fileNo, args.writeOffset, writeData, args.chunkSize);
            if (!writeResult)
            {
                std::cerr << "WriteRecord failed: " << writeResult.error().toString().c_str() << "\n";
                return 1;
            }

            std::cout << "WriteRecord OK (" << args.writeData.size()
                      << " bytes at offset " << args.writeOffset << ")\n";

            if (args.commitAfterWrite)
            {
                auto commitResult = desfire->commitTransaction();
                if (!commitResult)
                {
                    std::cerr << "CommitTransaction failed: " << commitResult.error().toString().c_str() << "\n";
                    return 1;
                }
                std::cout << "CommitTransaction OK\n";
            }

            // Refresh record counters after write/commit.
            settingsResult = desfire->getFileSettings(args.fileNo);
            if (!settingsResult)
            {
                std::cerr << "GetFileSettings (post-write) failed: " << settingsResult.error().toString().c_str() << "\n";
                return 1;
            }
            settings = settingsResult.value();
            std::cout << "Current/Max records (post-write): " << settings.currentRecords
                      << " / " << settings.maxRecords << "\n";

            if (args.autoReadAfterWrite && !args.readCountProvided)
            {
                args.readCount = 1U;
            }
        }

        if (args.readRequested)
        {
            if (settings.currentRecords == 0U)
            {
                std::cerr << "Record file is empty (currentRecords=0)\n";
                return 1;
            }

            if (args.readOffset >= settings.currentRecords)
            {
                std::cerr << "read-offset out of range. offset=" << args.readOffset
                          << ", currentRecords=" << settings.currentRecords << "\n";
                return 1;
            }

            const uint32_t availableRecords = settings.currentRecords - args.readOffset;
            const uint32_t effectiveReadCount = (args.readCount == 0U) ? availableRecords : args.readCount;
            if (effectiveReadCount == 0U || effectiveReadCount > availableRecords)
            {
                std::cerr << "Requested read-count exceeds available records. requested=" << effectiveReadCount
                          << ", available=" << availableRecords << "\n";
                return 1;
            }

            const uint64_t expectedBytes64 =
                static_cast<uint64_t>(effectiveReadCount) * static_cast<uint64_t>(settings.recordSize);
            if (expectedBytes64 > DesfireCard::MAX_DATA_IO_SIZE)
            {
                std::cerr << "Requested read exceeds max supported buffer (" << DesfireCard::MAX_DATA_IO_SIZE
                          << " bytes)\n";
                return 1;
            }

            auto readResult = desfire->readRecords(args.fileNo, args.readOffset, effectiveReadCount, args.chunkSize);
            if (!readResult)
            {
                std::cerr << "ReadRecords failed: " << readResult.error().toString().c_str() << "\n";
                return 1;
            }

            const auto& readData = readResult.value();
            std::cout << "ReadRecords OK (" << effectiveReadCount
                      << " records, " << readData.size() << " bytes)\n";
            std::cout << "Data: " << toHex(readData) << "\n";
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
