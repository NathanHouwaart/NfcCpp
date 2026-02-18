/**
 * @file main.cpp
 * @brief DESFire value operations example (GetValue/Credit/Debit/LimitedCredit)
 *
 * Flow:
 *   1) Select application
 *   2) Optional authenticate
 *   3) Execute one value operation
 */

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
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
#include "Error/DesfireError.h"

using namespace comms::serial;
using namespace pn532;
using namespace nfc;

namespace
{
    enum class Operation : uint8_t
    {
        Get,
        Credit,
        Debit,
        LimitedCredit
    };

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
        bool operationSet = false;
        Operation operation = Operation::Get;
        bool valueSet = false;
        int32_t value = 0;

        bool showBefore = false;
        bool showAfter = false;
        bool commit = false;
        bool allowNoChangesOnCommit = false;
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

    int32_t parseInt32(const std::string& value)
    {
        const long long parsed = std::stoll(value, nullptr, 0);
        if (parsed < static_cast<long long>((std::numeric_limits<int32_t>::min)()) ||
            parsed > static_cast<long long>((std::numeric_limits<int32_t>::max)()))
        {
            throw std::runtime_error("Value out of int32 range: " + value);
        }
        return static_cast<int32_t>(parsed);
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

    Operation parseOperation(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "get" || lower == "getvalue")
        {
            return Operation::Get;
        }
        if (lower == "credit")
        {
            return Operation::Credit;
        }
        if (lower == "debit")
        {
            return Operation::Debit;
        }
        if (lower == "limitedcredit" || lower == "limited-credit")
        {
            return Operation::LimitedCredit;
        }
        throw std::runtime_error("Invalid --op: " + text);
    }

    bool operationNeedsValue(Operation op)
    {
        return op == Operation::Credit || op == Operation::Debit || op == Operation::LimitedCredit;
    }

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>                                Default: 115200\n";
        std::cout << "  --aid <hex6>                              Required (3-byte app AID)\n";
        std::cout << "  --file-no <n>                             Default: 0 (0..31)\n";
        std::cout << "  --op <get|credit|debit|limited-credit>    Required\n";
        std::cout << "  --value <n>                               Required for credit/debit/limited-credit\n";
        std::cout << "  --show-before                             Read value before operation\n";
        std::cout << "  --show-after                              Read value after operation\n";
        std::cout << "  --commit                                  Commit transaction after credit/debit/limited-credit\n";
        std::cout << "  --allow-no-changes-on-commit              Treat NoChanges (0x0C) as success on commit\n";
        std::cout << "  --authenticate                            Authenticate before operation\n";
        std::cout << "  --auth-mode <legacy|iso|aes|des|2k3des|3k3des> Default: iso\n";
        std::cout << "  --auth-key-no <n>                         Default: 0\n";
        std::cout << "  --auth-key-hex <hex>                      Required when --authenticate is set\n";
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
                aidProvided = true;
            }
            else if (opt == "--file-no")
            {
                args.fileNo = parseByte(requireValue("--file-no"));
            }
            else if (opt == "--op")
            {
                args.operation = parseOperation(requireValue("--op"));
                args.operationSet = true;
            }
            else if (opt == "--value")
            {
                args.value = parseInt32(requireValue("--value"));
                args.valueSet = true;
            }
            else if (opt == "--show-before")
            {
                args.showBefore = true;
            }
            else if (opt == "--show-after")
            {
                args.showAfter = true;
            }
            else if (opt == "--commit")
            {
                args.commit = true;
            }
            else if (opt == "--allow-no-changes-on-commit")
            {
                args.allowNoChangesOnCommit = true;
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

        if (!aidProvided)
        {
            throw std::runtime_error("--aid is required");
        }
        if (args.aid.size() != 3U)
        {
            throw std::runtime_error("--aid must be exactly 3 bytes");
        }
        if (args.fileNo > 0x1FU)
        {
            throw std::runtime_error("--file-no must be in range 0..31");
        }
        if (!args.operationSet)
        {
            throw std::runtime_error("--op is required");
        }
        if (operationNeedsValue(args.operation) && !args.valueSet)
        {
            throw std::runtime_error("--value is required for the selected --op");
        }
        if (operationNeedsValue(args.operation) && args.value < 0)
        {
            throw std::runtime_error("--value must be >= 0 for credit/debit/limited-credit");
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

        if (operationNeedsValue(args.operation) && !args.showBefore && !args.showAfter)
        {
            args.showAfter = true;
        }
        if (!operationNeedsValue(args.operation) && args.commit)
        {
            throw std::runtime_error("--commit is only valid for credit/debit/limited-credit");
        }
        if (!args.commit && args.allowNoChangesOnCommit)
        {
            throw std::runtime_error("--allow-no-changes-on-commit requires --commit");
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

        std::cout << "DESFire Value Operations Example\n";
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

        if (args.showBefore && args.operation != Operation::Get)
        {
            auto beforeResult = desfire->getValue(args.fileNo);
            if (!beforeResult)
            {
                std::cerr << "GetValue (before) failed: " << beforeResult.error().toString().c_str() << "\n";
                return 1;
            }
            std::cout << "Value before: " << beforeResult.value() << "\n";
        }

        if (args.operation == Operation::Get)
        {
            auto valueResult = desfire->getValue(args.fileNo);
            if (!valueResult)
            {
                std::cerr << "GetValue failed: " << valueResult.error().toString().c_str() << "\n";
                return 1;
            }
            std::cout << "GetValue OK: " << valueResult.value() << "\n";
            return 0;
        }

        etl::expected<void, error::Error> opResult = {};
        if (args.operation == Operation::Credit)
        {
            opResult = desfire->credit(args.fileNo, args.value);
        }
        else if (args.operation == Operation::Debit)
        {
            opResult = desfire->debit(args.fileNo, args.value);
        }
        else
        {
            opResult = desfire->limitedCredit(args.fileNo, args.value);
        }

        if (!opResult)
        {
            std::cerr << "Value operation failed: " << opResult.error().toString().c_str() << "\n";
            return 1;
        }
        std::cout << "Value operation command accepted\n";

        if (args.commit)
        {
            auto commitResult = desfire->commitTransaction();
            if (!commitResult)
            {
                if (args.allowNoChangesOnCommit &&
                    commitResult.error().is<error::DesfireError>() &&
                    commitResult.error().get<error::DesfireError>() == error::DesfireError::NoChanges)
                {
                    std::cout << "CommitTransaction returned NoChanges; continuing "
                              << "(--allow-no-changes-on-commit)\n";
                }
                else
                {
                    std::cerr << "CommitTransaction failed: " << commitResult.error().toString().c_str() << "\n";
                    return 1;
                }
            }
            else
            {
                std::cout << "CommitTransaction OK\n";
            }
        }

        if (args.showAfter)
        {
            auto afterResult = desfire->getValue(args.fileNo);
            if (!afterResult)
            {
                std::cerr << "GetValue (after) failed: " << afterResult.error().toString().c_str() << "\n";
                return 1;
            }
            std::cout << "Value after: " << afterResult.value() << "\n";
        }

        if (!args.commit)
        {
            std::cout << "Note: Credit/Debit/LimitedCredit on DESFire value files may require --commit "
                         "to persist changes.\n";
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
