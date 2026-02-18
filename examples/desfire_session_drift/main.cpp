/**
 * @file main.cpp
 * @brief DESFire long-session drift test example
 *
 * Goal:
 *   - Build multiple applications (AES, 3K3DES, DES, 2K3DES)
 *   - Create all major file types in each app
 *   - Run mixed secure operations with either:
 *       - drift mode: authenticate once per app workload
 *       - baseline mode: re-authenticate before every operation
 *
 * This helps catch IV/CMAC progression drift that only appears in long,
 * mixed command sessions.
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
#include "Nfc/Desfire/DesfireContext.h"
#include "Nfc/Desfire/Commands/ChangeKeyCommand.h"
#include "Error/DesfireError.h"

using namespace comms::serial;
using namespace pn532;
using namespace nfc;

namespace
{
    constexpr uint8_t FILE_STD = 1U;
    constexpr uint8_t FILE_BKP = 2U;
    constexpr uint8_t FILE_VAL = 3U;
    constexpr uint8_t FILE_LIN = 4U;
    constexpr uint8_t FILE_CYC = 5U;

    constexpr uint8_t COMM_ENC = 0x03U;
    constexpr uint8_t ACCESS_KEY0 = 0x00U;

    constexpr uint32_t DATA_FILE_SIZE = 32U;
    constexpr uint32_t RECORD_SIZE = 16U;
    constexpr uint32_t MAX_RECORDS = 6U;

    enum class Mode : uint8_t
    {
        Drift,
        Baseline
    };

    struct Args
    {
        std::string comPort;
        int baudRate = 115200;
        Mode mode = Mode::Drift;
        uint32_t repeatCount = 3U;
        bool traceIv = false;
        bool recreateApps = false;
        bool allowExisting = false;
        uint16_t chunkSize = 0U;

        DesfireAuthMode piccAuthMode = DesfireAuthMode::LEGACY;
        uint8_t piccAuthKeyNo = 0x00U;
        std::vector<uint8_t> piccAuthKey;

        uint8_t appKeySettings = 0x0FU;
        uint8_t appKeyCount = 2U;

        std::optional<etl::array<uint8_t, 3>> aidAes;
        std::optional<etl::array<uint8_t, 3>> aid3k3des;
        std::optional<etl::array<uint8_t, 3>> aidDes;
        std::optional<etl::array<uint8_t, 3>> aid2k3des;
    };

    struct AppProfile
    {
        std::string name;
        etl::array<uint8_t, 3> aid;
        DesfireKeyType keyType = DesfireKeyType::UNKNOWN;
        DesfireAuthMode authMode = DesfireAuthMode::ISO;
        std::vector<uint8_t> key0;
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

    etl::array<uint8_t, 3> parseAid3(const std::string& text)
    {
        const auto aid = parseHex(text);
        if (aid.size() != 3U)
        {
            throw std::runtime_error("AID must be exactly 3 bytes");
        }
        return etl::array<uint8_t, 3>{aid[0], aid[1], aid[2]};
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

    std::string toHexAid(const etl::array<uint8_t, 3>& aid)
    {
        std::vector<uint8_t> bytes = {aid[0], aid[1], aid[2]};
        return toHex(bytes);
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

    std::string authModeName(DesfireAuthMode mode)
    {
        switch (mode)
        {
            case DesfireAuthMode::LEGACY:
                return "legacy";
            case DesfireAuthMode::ISO:
                return "iso";
            case DesfireAuthMode::AES:
                return "aes";
            default:
                return "unknown";
        }
    }

    std::string keyTypeName(DesfireKeyType type)
    {
        switch (type)
        {
            case DesfireKeyType::DES:
                return "des";
            case DesfireKeyType::DES3_2K:
                return "2k3des";
            case DesfireKeyType::DES3_3K:
                return "3k3des";
            case DesfireKeyType::AES:
                return "aes";
            default:
                return "unknown";
        }
    }

    std::string authSchemeName(SessionAuthScheme scheme)
    {
        switch (scheme)
        {
            case SessionAuthScheme::None:
                return "none";
            case SessionAuthScheme::Legacy:
                return "legacy";
            case SessionAuthScheme::Iso:
                return "iso";
            case SessionAuthScheme::Aes:
                return "aes";
            default:
                return "unknown";
        }
    }

    Mode parseMode(const std::string& text)
    {
        const std::string lower = toLower(text);
        if (lower == "drift")
        {
            return Mode::Drift;
        }
        if (lower == "baseline")
        {
            return Mode::Baseline;
        }
        throw std::runtime_error("Invalid --mode (use drift|baseline)");
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

    std::vector<uint8_t> defaultPiccAuthKeyForMode(DesfireAuthMode mode)
    {
        if (mode == DesfireAuthMode::AES)
        {
            return std::vector<uint8_t>(16U, 0x00U);
        }
        if (mode == DesfireAuthMode::ISO)
        {
            return std::vector<uint8_t>(16U, 0x00U);
        }
        return std::vector<uint8_t>(8U, 0x00U);
    }

    bool isDesfireError(const error::Error& err, error::DesfireError code)
    {
        return err.is<error::DesfireError>() && err.get<error::DesfireError>() == code;
    }

    template <typename TResult>
    bool checkOk(const etl::expected<TResult, error::Error>& result, const std::string& step)
    {
        if (result)
        {
            return true;
        }
        std::cerr << step << " failed: " << result.error().toString().c_str() << "\n";
        return false;
    }

    bool checkCreateLikeResult(
        const etl::expected<void, error::Error>& result,
        const std::string& step,
        bool allowExisting)
    {
        if (result)
        {
            return true;
        }

        if (allowExisting && isDesfireError(result.error(), error::DesfireError::DuplicateError))
        {
            std::cout << step << " returned DuplicateError; continuing (--allow-existing)\n";
            return true;
        }

        std::cerr << step << " failed: " << result.error().toString().c_str() << "\n";
        return false;
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

    size_t expectedKeySize(DesfireKeyType keyType)
    {
        switch (keyType)
        {
            case DesfireKeyType::DES:
                return 8U;
            case DesfireKeyType::DES3_2K:
                return 16U;
            case DesfireKeyType::DES3_3K:
                return 24U;
            case DesfireKeyType::AES:
                return 16U;
            default:
                return 0U;
        }
    }

    std::vector<uint8_t> zeroKeyForType(DesfireKeyType keyType)
    {
        const size_t keySize = expectedKeySize(keyType);
        if (keySize == 0U)
        {
            throw std::runtime_error("Unsupported key type for zero-key initialization");
        }
        return std::vector<uint8_t>(keySize, 0x00U);
    }

    bool isAllZero(const std::vector<uint8_t>& data)
    {
        for (uint8_t b : data)
        {
            if (b != 0x00U)
            {
                return false;
            }
        }
        return true;
    }

    bool isIntegrityError(const error::Error& err)
    {
        return isDesfireError(err, error::DesfireError::IntegrityError);
    }

    bool authenticateWithKey(
        DesfireCard& desfire,
        uint8_t keyNo,
        const std::vector<uint8_t>& key,
        DesfireAuthMode mode,
        const std::string& label)
    {
        const auto etlKey = toEtl<24U>(key);
        auto authResult = desfire.authenticate(keyNo, etlKey, mode);
        return checkOk(authResult, label);
    }

    bool initializeAppMasterKey(DesfireCard& desfire, const AppProfile& profile)
    {
        const std::vector<uint8_t> defaultKey = zeroKeyForType(profile.keyType);
        if (profile.key0.size() != defaultKey.size())
        {
            std::cerr << "Profile key length mismatch for " << profile.name << "\n";
            return false;
        }

        if (isAllZero(profile.key0))
        {
            return true;
        }

        if (!authenticateWithKey(
                desfire,
                0U,
                defaultKey,
                profile.authMode,
                "Authenticate (" + profile.name + " default key0)"))
        {
            return false;
        }

        auto runChangeKey = [&](DesfireAuthMode mode, ChangeKeyLegacyIvMode legacyIvMode)
            -> etl::expected<void, error::Error>
        {
            ChangeKeyCommandOptions options;
            options.keyNo = 0U;
            options.authMode = mode;
            options.sessionKeyType = profile.keyType;
            options.newKeyType = profile.keyType;
            options.oldKeyType = profile.keyType;
            options.newKey = toEtl<24U>(profile.key0);
            options.newKeyVersion = 0U;
            options.legacyIvMode = legacyIvMode;

            ChangeKeyCommand command(options);
            return desfire.executeCommand(command);
        };

        auto changeResult = runChangeKey(profile.authMode, ChangeKeyLegacyIvMode::Zero);
        if (!changeResult &&
            profile.authMode == DesfireAuthMode::ISO &&
            profile.key0.size() != 24U &&
            isIntegrityError(changeResult.error()))
        {
            if (!authenticateWithKey(
                    desfire,
                    0U,
                    defaultKey,
                    DesfireAuthMode::LEGACY,
                    "Authenticate (" + profile.name + " default key0, legacy fallback)"))
            {
                return false;
            }

            changeResult = runChangeKey(DesfireAuthMode::LEGACY, ChangeKeyLegacyIvMode::Zero);
            if (!changeResult && isIntegrityError(changeResult.error()))
            {
                if (!authenticateWithKey(
                        desfire,
                        0U,
                        defaultKey,
                        DesfireAuthMode::LEGACY,
                        "Authenticate (" + profile.name + " default key0, seeded fallback)"))
                {
                    return false;
                }
                changeResult = runChangeKey(
                    DesfireAuthMode::LEGACY,
                    ChangeKeyLegacyIvMode::SessionEncryptedRndB);
            }
        }

        if (!changeResult)
        {
            if (isDesfireError(changeResult.error(), error::DesfireError::NoChanges))
            {
                return true;
            }
            std::cerr << "Initialize key0 failed (" << profile.name << "): "
                      << changeResult.error().toString().c_str() << "\n";
            return false;
        }

        std::cout << "Initialized app key0 for " << profile.name << "\n";
        return true;
    }

    void traceContext(const DesfireCard& desfire, const std::string& tag)
    {
        const DesfireContext& ctx = desfire.getContext();
        std::cout << "[TRACE] " << tag
                  << " | auth=" << (ctx.authenticated ? "yes" : "no")
                  << " scheme=" << authSchemeName(ctx.authScheme)
                  << " keyNo=" << static_cast<int>(ctx.keyNo)
                  << " iv=" << toHex(ctx.iv)
                  << "\n";
    }

    bool containsSubsequence(const etl::ivector<uint8_t>& haystack, const std::vector<uint8_t>& needle)
    {
        if (needle.empty())
        {
            return true;
        }
        if (haystack.size() < needle.size())
        {
            return false;
        }

        for (size_t start = 0U; start <= (haystack.size() - needle.size()); ++start)
        {
            bool allMatch = true;
            for (size_t i = 0U; i < needle.size(); ++i)
            {
                if (haystack[start + i] != needle[i])
                {
                    allMatch = false;
                    break;
                }
            }
            if (allMatch)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<uint8_t> makePayload(uint8_t marker, uint8_t appIndex, uint32_t iteration)
    {
        std::vector<uint8_t> out(16U, 0x00U);
        out[0] = marker;
        out[1] = appIndex;
        out[2] = static_cast<uint8_t>(iteration & 0xFFU);
        out[3] = static_cast<uint8_t>((iteration >> 8U) & 0xFFU);
        out[4] = static_cast<uint8_t>(0xA0U + appIndex);
        out[5] = static_cast<uint8_t>(0xB0U + appIndex);
        out[6] = static_cast<uint8_t>(0xC0U + appIndex);
        out[7] = static_cast<uint8_t>(0xD0U + appIndex);
        out[8] = static_cast<uint8_t>(0x10U + iteration);
        out[9] = static_cast<uint8_t>(0x20U + iteration);
        out[10] = static_cast<uint8_t>(0x30U + iteration);
        out[11] = static_cast<uint8_t>(0x40U + iteration);
        out[12] = static_cast<uint8_t>(0x50U + appIndex);
        out[13] = static_cast<uint8_t>(0x60U + appIndex);
        out[14] = static_cast<uint8_t>(0x70U + appIndex);
        out[15] = static_cast<uint8_t>(0x80U + appIndex);
        return out;
    }

    void printUsage(const char* exeName)
    {
        std::cout << "Usage:\n";
        std::cout << "  " << exeName << " <COM_PORT> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --baud <n>                         Default: 115200\n";
        std::cout << "  --mode <drift|baseline>            Default: drift\n";
        std::cout << "  --repeat <n>                       Default: 3\n";
        std::cout << "  --trace-iv                         Print auth/session/IV state around steps\n";
        std::cout << "  --chunk-size <n>                   Default: 0 (command default)\n";
        std::cout << "  --recreate-apps                    Delete test apps before create\n";
        std::cout << "  --allow-existing                   Continue on DuplicateError during create\n";
        std::cout << "  --picc-auth-mode <legacy|iso|aes>  Default: legacy\n";
        std::cout << "  --picc-auth-key-no <n>             Default: 0\n";
        std::cout << "  --picc-auth-key-hex <hex>          Default: zero key for selected mode\n";
        std::cout << "  --app-key-settings <n>             Default: 0x0F\n";
        std::cout << "  --app-key-count <n>                Default: 2 (1..14)\n";
        std::cout << "  --aid-aes <hex6>                   Default: A1A551\n";
        std::cout << "  --aid-3k3des <hex6>                Default: A1A552\n";
        std::cout << "  --aid-des <hex6>                   Default: A1A553\n";
        std::cout << "  --aid-2k3des <hex6>                Default: A1A554\n";
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
            else if (opt == "--mode")
            {
                args.mode = parseMode(requireValue("--mode"));
            }
            else if (opt == "--repeat")
            {
                args.repeatCount = parseUInt32(requireValue("--repeat"));
            }
            else if (opt == "--trace-iv")
            {
                args.traceIv = true;
            }
            else if (opt == "--chunk-size")
            {
                args.chunkSize = parseUInt16(requireValue("--chunk-size"));
            }
            else if (opt == "--recreate-apps")
            {
                args.recreateApps = true;
            }
            else if (opt == "--allow-existing")
            {
                args.allowExisting = true;
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
            else if (opt == "--app-key-settings")
            {
                args.appKeySettings = parseByte(requireValue("--app-key-settings"));
            }
            else if (opt == "--app-key-count")
            {
                args.appKeyCount = parseByte(requireValue("--app-key-count"));
            }
            else if (opt == "--aid-aes")
            {
                args.aidAes = parseAid3(requireValue("--aid-aes"));
            }
            else if (opt == "--aid-3k3des")
            {
                args.aid3k3des = parseAid3(requireValue("--aid-3k3des"));
            }
            else if (opt == "--aid-des")
            {
                args.aidDes = parseAid3(requireValue("--aid-des"));
            }
            else if (opt == "--aid-2k3des")
            {
                args.aid2k3des = parseAid3(requireValue("--aid-2k3des"));
            }
            else
            {
                throw std::runtime_error("Unknown argument: " + opt);
            }
        }

        if (args.repeatCount == 0U)
        {
            throw std::runtime_error("--repeat must be > 0");
        }
        if (args.chunkSize > 240U)
        {
            throw std::runtime_error("--chunk-size must be in range 0..240");
        }
        if (args.appKeyCount == 0U || args.appKeyCount > 14U)
        {
            throw std::runtime_error("--app-key-count must be in range 1..14");
        }

        if (args.piccAuthKey.empty())
        {
            args.piccAuthKey = defaultPiccAuthKeyForMode(args.piccAuthMode);
        }
        if (!isAuthKeyLengthValid(args.piccAuthMode, args.piccAuthKey.size()))
        {
            throw std::runtime_error("Invalid PICC key length for --picc-auth-mode");
        }

        return args;
    }

    std::vector<AppProfile> buildProfiles(const Args& args)
    {
        std::vector<AppProfile> profiles;

        AppProfile aes;
        aes.name = "AES";
        aes.aid = args.aidAes.value_or(etl::array<uint8_t, 3>{0xA1U, 0xA5U, 0x51U});
        aes.keyType = DesfireKeyType::AES;
        aes.authMode = DesfireAuthMode::AES;
        aes.key0 = parseHex("11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00");
        profiles.push_back(aes);

        AppProfile threeK;
        threeK.name = "3K3DES";
        threeK.aid = args.aid3k3des.value_or(etl::array<uint8_t, 3>{0xA1U, 0xA5U, 0x52U});
        threeK.keyType = DesfireKeyType::DES3_3K;
        threeK.authMode = DesfireAuthMode::ISO;
        threeK.key0 = parseHex("10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27");
        profiles.push_back(threeK);

        AppProfile des;
        des.name = "DES";
        des.aid = args.aidDes.value_or(etl::array<uint8_t, 3>{0xA1U, 0xA5U, 0x53U});
        des.keyType = DesfireKeyType::DES;
        des.authMode = DesfireAuthMode::LEGACY;
        des.key0 = parseHex("D1 D2 D3 D4 D5 D6 D7 D8");
        profiles.push_back(des);

        AppProfile twoK;
        twoK.name = "2K3DES";
        twoK.aid = args.aid2k3des.value_or(etl::array<uint8_t, 3>{0xA1U, 0xA5U, 0x54U});
        twoK.keyType = DesfireKeyType::DES3_2K;
        twoK.authMode = DesfireAuthMode::ISO;
        twoK.key0 = parseHex("21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30");
        profiles.push_back(twoK);

        return profiles;
    }

    bool authenticateWithProfile(DesfireCard& desfire, const AppProfile& profile)
    {
        const auto key = toEtl<24U>(profile.key0);
        auto authResult = desfire.authenticate(0U, key, profile.authMode);
        if (!checkOk(authResult, "Authenticate (" + profile.name + ")"))
        {
            return false;
        }
        return true;
    }

    template <typename Func>
    bool runStep(
        DesfireCard& desfire,
        const Args& args,
        const AppProfile& profile,
        const std::string& stepName,
        Func&& fn)
    {
        if (args.mode == Mode::Baseline)
        {
            if (!authenticateWithProfile(desfire, profile))
            {
                return false;
            }
        }

        if (args.traceIv)
        {
            traceContext(desfire, profile.name + " | " + stepName + " | before");
        }

        const bool ok = fn();
        if (!ok)
        {
            return false;
        }

        if (args.traceIv)
        {
            traceContext(desfire, profile.name + " | " + stepName + " | after");
        }

        return true;
    }

    bool selectAid(DesfireCard& desfire, const etl::array<uint8_t, 3>& aid, const std::string& label)
    {
        auto selectResult = desfire.selectApplication(aid);
        if (!checkOk(selectResult, "SelectApplication " + label))
        {
            return false;
        }
        return true;
    }

    bool createFilesForProfile(DesfireCard& desfire, const Args& args, const AppProfile& profile)
    {
        if (!runStep(desfire, args, profile, "CreateStdDataFile", [&]()
            {
                auto result = desfire.createStdDataFile(
                    FILE_STD,
                    COMM_ENC,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    DATA_FILE_SIZE);
                return checkCreateLikeResult(result, "CreateStdDataFile", args.allowExisting);
            }))
        {
            return false;
        }

        if (!runStep(desfire, args, profile, "CreateBackupDataFile", [&]()
            {
                auto result = desfire.createBackupDataFile(
                    FILE_BKP,
                    COMM_ENC,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    DATA_FILE_SIZE);
                return checkCreateLikeResult(result, "CreateBackupDataFile", args.allowExisting);
            }))
        {
            return false;
        }

        if (!runStep(desfire, args, profile, "CreateValueFile", [&]()
            {
                auto result = desfire.createValueFile(
                    FILE_VAL,
                    COMM_ENC,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    -100000,
                    100000,
                    1000,
                    true,
                    false);
                return checkCreateLikeResult(result, "CreateValueFile", args.allowExisting);
            }))
        {
            return false;
        }

        if (!runStep(desfire, args, profile, "CreateLinearRecordFile", [&]()
            {
                auto result = desfire.createLinearRecordFile(
                    FILE_LIN,
                    COMM_ENC,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    RECORD_SIZE,
                    MAX_RECORDS);
                return checkCreateLikeResult(result, "CreateLinearRecordFile", args.allowExisting);
            }))
        {
            return false;
        }

        if (!runStep(desfire, args, profile, "CreateCyclicRecordFile", [&]()
            {
                auto result = desfire.createCyclicRecordFile(
                    FILE_CYC,
                    COMM_ENC,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    ACCESS_KEY0,
                    RECORD_SIZE,
                    MAX_RECORDS);
                return checkCreateLikeResult(result, "CreateCyclicRecordFile", args.allowExisting);
            }))
        {
            return false;
        }

        return true;
    }

    bool runWorkloadForProfile(DesfireCard& desfire, const Args& args, const AppProfile& profile, uint8_t appIndex)
    {
        if (args.mode == Mode::Drift)
        {
            if (!authenticateWithProfile(desfire, profile))
            {
                return false;
            }
        }

        for (uint32_t iteration = 0U; iteration < args.repeatCount; ++iteration)
        {
            std::cout << "\n[" << profile.name << "] Iteration " << (iteration + 1U)
                      << " / " << args.repeatCount << "\n";

            const std::vector<uint8_t> stdPayload = makePayload(0xA1U, appIndex, iteration);
            const std::vector<uint8_t> bkpPayload = makePayload(0xB2U, appIndex, iteration);
            const std::vector<uint8_t> linPayload = makePayload(0xC3U, appIndex, iteration);
            const std::vector<uint8_t> cycPayload = makePayload(0xD4U, appIndex, iteration);
            const int32_t creditDelta = static_cast<int32_t>(100 + iteration);

            if (!runStep(desfire, args, profile, "WriteData(std)", [&]()
                {
                    const auto payload = toEtl<DesfireCard::MAX_DATA_IO_SIZE>(stdPayload);
                    auto result = desfire.writeData(FILE_STD, 0U, payload, args.chunkSize);
                    return checkOk(result, "WriteData(std)");
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "ReadData(std)", [&]()
                {
                    auto result = desfire.readData(
                        FILE_STD,
                        0U,
                        static_cast<uint32_t>(stdPayload.size()),
                        args.chunkSize);
                    if (!checkOk(result, "ReadData(std)"))
                    {
                        return false;
                    }

                    const auto& data = result.value();
                    if (data.size() != stdPayload.size())
                    {
                        std::cerr << "ReadData(std) length mismatch\n";
                        return false;
                    }
                    for (size_t i = 0U; i < stdPayload.size(); ++i)
                    {
                        if (data[i] != stdPayload[i])
                        {
                            std::cerr << "ReadData(std) content mismatch\n";
                            return false;
                        }
                    }
                    return true;
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "WriteData(backup)", [&]()
                {
                    const auto payload = toEtl<DesfireCard::MAX_DATA_IO_SIZE>(bkpPayload);
                    auto result = desfire.writeData(FILE_BKP, 0U, payload, args.chunkSize);
                    return checkOk(result, "WriteData(backup)");
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "CommitTransaction(backup)", [&]()
                {
                    auto result = desfire.commitTransaction();
                    return checkOk(result, "CommitTransaction(backup)");
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "ReadData(backup)", [&]()
                {
                    auto result = desfire.readData(
                        FILE_BKP,
                        0U,
                        static_cast<uint32_t>(bkpPayload.size()),
                        args.chunkSize);
                    if (!checkOk(result, "ReadData(backup)"))
                    {
                        return false;
                    }

                    const auto& data = result.value();
                    if (data.size() != bkpPayload.size())
                    {
                        std::cerr << "ReadData(backup) length mismatch\n";
                        return false;
                    }
                    for (size_t i = 0U; i < bkpPayload.size(); ++i)
                    {
                        if (data[i] != bkpPayload[i])
                        {
                            std::cerr << "ReadData(backup) content mismatch\n";
                            return false;
                        }
                    }
                    return true;
                }))
            {
                return false;
            }

            int32_t valueBefore = 0;
            if (!runStep(desfire, args, profile, "GetValue(before)", [&]()
                {
                    auto result = desfire.getValue(FILE_VAL);
                    if (!checkOk(result, "GetValue(before)"))
                    {
                        return false;
                    }
                    valueBefore = result.value();
                    std::cout << "  Value before: " << valueBefore << "\n";
                    return true;
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "Credit", [&]()
                {
                    auto result = desfire.credit(FILE_VAL, creditDelta);
                    return checkOk(result, "Credit");
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "CommitTransaction(value)", [&]()
                {
                    auto result = desfire.commitTransaction();
                    return checkOk(result, "CommitTransaction(value)");
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "GetValue(after)", [&]()
                {
                    auto result = desfire.getValue(FILE_VAL);
                    if (!checkOk(result, "GetValue(after)"))
                    {
                        return false;
                    }
                    const int32_t valueAfter = result.value();
                    const int32_t expected = static_cast<int32_t>(valueBefore + creditDelta);
                    std::cout << "  Value after: " << valueAfter << " (expected " << expected << ")\n";
                    if (valueAfter != expected)
                    {
                        std::cerr << "Value mismatch after credit\n";
                        return false;
                    }
                    return true;
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "WriteRecord(linear)", [&]()
                {
                    const auto payload = toEtl<DesfireCard::MAX_DATA_IO_SIZE>(linPayload);
                    auto result = desfire.writeRecord(FILE_LIN, 0U, payload, args.chunkSize);
                    return checkOk(result, "WriteRecord(linear)");
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "CommitTransaction(linear)", [&]()
                {
                    auto result = desfire.commitTransaction();
                    return checkOk(result, "CommitTransaction(linear)");
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "ReadRecords(linear)", [&]()
                {
                    auto result = desfire.readRecords(FILE_LIN, 0U, 0U, args.chunkSize);
                    if (!checkOk(result, "ReadRecords(linear)"))
                    {
                        return false;
                    }

                    const auto& data = result.value();
                    if (!containsSubsequence(data, linPayload))
                    {
                        std::cerr << "Linear record payload not found in read data\n";
                        return false;
                    }
                    return true;
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "WriteRecord(cyclic)", [&]()
                {
                    const auto payload = toEtl<DesfireCard::MAX_DATA_IO_SIZE>(cycPayload);
                    auto result = desfire.writeRecord(FILE_CYC, 0U, payload, args.chunkSize);
                    return checkOk(result, "WriteRecord(cyclic)");
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "CommitTransaction(cyclic)", [&]()
                {
                    auto result = desfire.commitTransaction();
                    return checkOk(result, "CommitTransaction(cyclic)");
                }))
            {
                return false;
            }

            if (!runStep(desfire, args, profile, "ReadRecords(cyclic)", [&]()
                {
                    auto result = desfire.readRecords(FILE_CYC, 0U, 0U, args.chunkSize);
                    if (!checkOk(result, "ReadRecords(cyclic)"))
                    {
                        return false;
                    }

                    const auto& data = result.value();
                    if (!containsSubsequence(data, cycPayload))
                    {
                        std::cerr << "Cyclic record payload not found in read data\n";
                        return false;
                    }
                    return true;
                }))
            {
                return false;
            }
        }

        return true;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        const Args args = parseArgs(argc, argv);
        const auto profiles = buildProfiles(args);

        std::cout << "DESFire Session Drift Example\n";
        std::cout << "COM: " << args.comPort << " @ " << args.baudRate << "\n";
        std::cout << "Mode: " << ((args.mode == Mode::Drift) ? "drift" : "baseline") << "\n";
        std::cout << "Repeat per app: " << args.repeatCount << "\n";
        std::cout << "Recreate apps: " << (args.recreateApps ? "yes" : "no") << "\n";
        std::cout << "Allow existing: " << (args.allowExisting ? "yes" : "no") << "\n";
        std::cout << "PICC auth mode: " << authModeName(args.piccAuthMode) << "\n";

        etl::string<256> portName(args.comPort.c_str());
        SerialBusWin serial(portName, args.baudRate);
        auto serialInitResult = serial.init();
        if (!checkOk(serialInitResult, "Serial init"))
        {
            return 1;
        }

        Pn532Driver pn532(serial);
        pn532.init();

        auto samResult = pn532.setSamConfiguration(0x01);
        if (!checkOk(samResult, "SAM configuration"))
        {
            return 1;
        }

        auto retryResult = pn532.setMaxRetries(1);
        if (!checkOk(retryResult, "Set max retries"))
        {
            return 1;
        }

        Pn532ApduAdapter adapter(pn532);
        CardManager cardManager(adapter, adapter, ReaderCapabilities::pn532());
        cardManager.setWire(WireKind::Iso);

        auto detectResult = cardManager.detectCard();
        if (!checkOk(detectResult, "Card detect"))
        {
            return 1;
        }

        auto sessionResult = cardManager.createSession();
        if (!checkOk(sessionResult, "Create session"))
        {
            return 1;
        }

        CardSession* session = sessionResult.value();
        DesfireCard* desfire = session->getCardAs<DesfireCard>();
        if (!desfire)
        {
            std::cerr << "Detected card is not DESFire\n";
            return 1;
        }

        const etl::array<uint8_t, 3> piccAid = {0x00U, 0x00U, 0x00U};

        for (size_t i = 0U; i < profiles.size(); ++i)
        {
            const AppProfile& profile = profiles[i];
            std::cout << "\n=== App " << (i + 1U) << "/" << profiles.size()
                      << " [" << profile.name << "] AID " << toHexAid(profile.aid)
                      << " keyType=" << keyTypeName(profile.keyType)
                      << " ===\n";

            if (!selectAid(*desfire, piccAid, "(PICC)"))
            {
                return 1;
            }

            {
                const auto piccKey = toEtl<24U>(args.piccAuthKey);
                auto piccAuthResult = desfire->authenticate(args.piccAuthKeyNo, piccKey, args.piccAuthMode);
                if (!checkOk(piccAuthResult, "Authenticate PICC"))
                {
                    return 1;
                }
            }

            if (args.recreateApps)
            {
                auto deleteResult = desfire->deleteApplication(profile.aid);
                if (!deleteResult)
                {
                    if (isDesfireError(deleteResult.error(), error::DesfireError::ApplicationNotFound))
                    {
                        std::cout << "DeleteApplication: app not found, continuing\n";
                    }
                    else
                    {
                        std::cerr << "DeleteApplication failed: "
                                  << deleteResult.error().toString().c_str() << "\n";
                        return 1;
                    }
                }
                else
                {
                    std::cout << "DeleteApplication OK\n";
                }
            }

            auto createAppResult = desfire->createApplication(
                profile.aid,
                args.appKeySettings,
                args.appKeyCount,
                profile.keyType);
            const bool appAlreadyExisted =
                (!createAppResult &&
                 args.allowExisting &&
                 isDesfireError(createAppResult.error(), error::DesfireError::DuplicateError));
            if (!checkCreateLikeResult(createAppResult, "CreateApplication", args.allowExisting))
            {
                return 1;
            }

            if (!selectAid(*desfire, profile.aid, "(" + profile.name + ")"))
            {
                return 1;
            }

            if (!appAlreadyExisted)
            {
                if (!initializeAppMasterKey(*desfire, profile))
                {
                    return 1;
                }
            }

            if (!authenticateWithProfile(*desfire, profile))
            {
                return 1;
            }

            if (!createFilesForProfile(*desfire, args, profile))
            {
                return 1;
            }

            if (!runWorkloadForProfile(*desfire, args, profile, static_cast<uint8_t>(i)))
            {
                return 1;
            }

            std::cout << "App workload completed: " << profile.name << "\n";
        }

        std::cout << "\nSession drift scenario completed successfully.\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        printUsage(argv[0]);
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}
