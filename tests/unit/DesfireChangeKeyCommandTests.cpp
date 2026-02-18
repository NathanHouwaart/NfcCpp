#include <gtest/gtest.h>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>
#include "Nfc/Desfire/Commands/ChangeKeyCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

namespace
{
    std::vector<uint8_t> hexToBytes(std::string_view text)
    {
        auto hexValue = [](char c) -> uint8_t
        {
            if (c >= '0' && c <= '9')
            {
                return static_cast<uint8_t>(c - '0');
            }
            if (c >= 'A' && c <= 'F')
            {
                return static_cast<uint8_t>(10 + (c - 'A'));
            }
            return static_cast<uint8_t>(10 + (c - 'a'));
        };

        std::vector<uint8_t> bytes;
        bytes.reserve(text.size() / 2);

        bool hasHighNibble = false;
        uint8_t highNibble = 0;

        for (char c : text)
        {
            if (!std::isxdigit(static_cast<unsigned char>(c)))
            {
                continue;
            }

            if (!hasHighNibble)
            {
                highNibble = hexValue(c);
                hasHighNibble = true;
            }
            else
            {
                const uint8_t value = static_cast<uint8_t>((highNibble << 4) | hexValue(c));
                bytes.push_back(value);
                hasHighNibble = false;
            }
        }

        if (hasHighNibble)
        {
            throw std::runtime_error("Odd number of hex digits");
        }

        return bytes;
    }

    template <size_t Capacity>
    etl::vector<uint8_t, Capacity> toEtl(const std::vector<uint8_t>& bytes)
    {
        if (bytes.size() > Capacity)
        {
            throw std::runtime_error("Input exceeds ETL vector capacity");
        }

        etl::vector<uint8_t, Capacity> out;
        for (uint8_t b : bytes)
        {
            out.push_back(b);
        }
        return out;
    }

    DesfireContext buildContext(
        const std::vector<uint8_t>& sessionKey,
        const std::vector<uint8_t>& iv,
        uint8_t authenticatedKeyNo,
        const std::vector<uint8_t>& selectedAid = {})
    {
        DesfireContext context;
        context.authenticated = true;
        context.commMode = CommMode::Enciphered;
        context.keyNo = authenticatedKeyNo;

        for (uint8_t b : sessionKey)
        {
            context.sessionKeyEnc.push_back(b);
        }
        for (uint8_t b : iv)
        {
            context.iv.push_back(b);
        }
        if (!selectedAid.empty())
        {
            for (uint8_t b : selectedAid)
            {
                context.selectedAid.push_back(b);
            }
        }

        return context;
    }

    std::vector<uint8_t> toStdVector(const etl::ivector<uint8_t>& data)
    {
        return std::vector<uint8_t>(data.begin(), data.end());
    }
}

TEST(DesfireChangeKeyCommandTests, BuildRequestIsoDesSameKeyMatchesVector1)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::ISO;
    options.sessionKeyType = DesfireKeyType::DES;
    options.newKeyType = DesfireKeyType::DES3_2K;
    options.newKey = toEtl<24>(hexToBytes("00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("C8 6C E2 5E 4C 64 7E 56 C8 6C E2 5E 4C 64 7E 56"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00);

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    const auto& request = requestResult.value();
    EXPECT_EQ(request.commandCode, 0xC4);
    EXPECT_EQ(request.expectedResponseLength, 0U);

    std::vector<uint8_t> expectedData = hexToBytes(
        "00 BE DE 0F C6 ED 34 7D CF 0D 51 C7 17 DF 75 D9 7D 2C 5A 2B A6 CA C7 47 9D");
    EXPECT_EQ(toStdVector(request.data), expectedData);

    etl::vector<uint8_t, 2> response;
    response.push_back(0x00);

    auto parseResult = command.parseResponse(response, context);
    ASSERT_TRUE(parseResult.has_value());
    EXPECT_TRUE(command.isComplete());
    EXPECT_FALSE(context.authenticated);
    EXPECT_TRUE(context.sessionKeyEnc.empty());
    EXPECT_TRUE(context.sessionKeyMac.empty());
    EXPECT_EQ(toStdVector(context.iv), hexToBytes("00 00 00 00 00 00 00 00"));
}

TEST(DesfireChangeKeyCommandTests, BuildRequestIsoDesDifferentKeyMatchesVector2)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x01;
    options.authMode = DesfireAuthMode::ISO;
    options.sessionKeyType = DesfireKeyType::DES;
    options.newKeyType = DesfireKeyType::DES3_2K;
    options.newKey = toEtl<24>(hexToBytes("00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80"));
    options.newKeyVersion = 0x00;
    options.oldKey = toEtl<24>(hexToBytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"));

    DesfireContext context = buildContext(
        hexToBytes("CA A6 74 E8 CA E8 52 5E CA A6 74 E8 CA E8 52 5E"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00);

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    std::vector<uint8_t> expectedData = hexToBytes(
        "01 4E B6 69 E4 8D CA 58 47 49 54 2E 1B E8 9C B4 C7 84 5A 38 C5 7D 19 DE 59");
    EXPECT_EQ(toStdVector(requestResult.value().data), expectedData);
}

TEST(DesfireChangeKeyCommandTests, BuildRequestAesSameKeyMatchesVector3)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::AES;
    options.sessionKeyType = DesfireKeyType::AES;
    options.newKeyType = DesfireKeyType::AES;
    options.newKey = toEtl<24>(hexToBytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("90 F7 A2 01 91 03 68 45 EC 63 DE CD 54 4B 99 31"),
        hexToBytes("8A 8F A3 6F 55 CD 21 0D D8 05 46 58 AC 70 D9 9A"),
        0x00);

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    std::vector<uint8_t> expectedData = hexToBytes(
        "00 63 53 75 E4 91 9F 8A F2 E9 E8 6B 1C 1B A5 5B 0C 08 07 EA F4 84 D7 A7 EF 6E 0C 30 84 16 0F 5A 61");
    EXPECT_EQ(toStdVector(requestResult.value().data), expectedData);
}

TEST(DesfireChangeKeyCommandTests, BuildRequestLegacyDesSameKeyMatchesVector6)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::LEGACY;
    options.sessionKeyType = DesfireKeyType::DES;
    options.newKeyType = DesfireKeyType::DES;
    options.newKey = toEtl<24>(hexToBytes("00 00 00 00 00 00 00 00"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("92 F1 35 8C EA E9 6A 10"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00);

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    std::vector<uint8_t> expectedData = hexToBytes(
        "00 EA 70 40 19 C3 EF 41 9F D6 3A E2 94 B4 01 4C 03 C6 F3 2A EC DD 56 19 D6");
    EXPECT_EQ(toStdVector(requestResult.value().data), expectedData);

    const std::vector<uint8_t> initialIv = toStdVector(context.iv);

    etl::vector<uint8_t, 1> response;
    response.push_back(0x00);
    auto parseResult = command.parseResponse(response, context);
    ASSERT_TRUE(parseResult.has_value());

    EXPECT_EQ(toStdVector(context.iv), initialIv);
}

TEST(DesfireChangeKeyCommandTests, LegacyDesParityBitsAreNormalizedInChangeKeyPayload)
{
    ChangeKeyCommandOptions oddParityOptions;
    oddParityOptions.keyNo = 0x00;
    oddParityOptions.authMode = DesfireAuthMode::LEGACY;
    oddParityOptions.sessionKeyType = DesfireKeyType::DES;
    oddParityOptions.newKeyType = DesfireKeyType::DES;
    oddParityOptions.newKey = toEtl<24>(hexToBytes("D1 D2 D3 D4 D5 D6 D7 D8"));
    oddParityOptions.newKeyVersion = 0x00;

    ChangeKeyCommandOptions evenParityOptions = oddParityOptions;
    evenParityOptions.newKey = toEtl<24>(hexToBytes("D0 D2 D2 D4 D4 D6 D6 D8"));

    DesfireContext contextOdd = buildContext(
        hexToBytes("92 F1 35 8C EA E9 6A 10"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00);

    DesfireContext contextEven = buildContext(
        hexToBytes("92 F1 35 8C EA E9 6A 10"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00);

    ChangeKeyCommand oddCommand(oddParityOptions);
    ChangeKeyCommand evenCommand(evenParityOptions);

    auto oddRequestResult = oddCommand.buildRequest(contextOdd);
    auto evenRequestResult = evenCommand.buildRequest(contextEven);

    ASSERT_TRUE(oddRequestResult.has_value());
    ASSERT_TRUE(evenRequestResult.has_value());
    EXPECT_EQ(toStdVector(oddRequestResult.value().data), toStdVector(evenRequestResult.value().data));
}

TEST(DesfireChangeKeyCommandTests, BuildRequestLegacyDesSeededWithEncryptedRndBMatchesExpected)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::LEGACY;
    options.sessionKeyType = DesfireKeyType::DES;
    options.newKeyType = DesfireKeyType::DES;
    options.newKey = toEtl<24>(hexToBytes("00 00 00 00 00 00 00 00"));
    options.newKeyVersion = 0x00;
    options.legacyIvMode = ChangeKeyLegacyIvMode::SessionEncryptedRndB;

    DesfireContext context = buildContext(
        hexToBytes("92 F1 35 8C EA E9 6A 10"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00);
    for (uint8_t b : hexToBytes("01 02 03 04 05 06 07 08"))
    {
        context.sessionEncRndB.push_back(b);
    }

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    std::vector<uint8_t> expectedData = hexToBytes(
        "00 10 02 16 96 1E DB C7 5E C7 C1 86 1E F6 41 ED 54 02 A3 C1 76 1F F4 45 95");
    EXPECT_EQ(toStdVector(requestResult.value().data), expectedData);
}

TEST(DesfireChangeKeyCommandTests, MissingOldKeyForDifferentSlotReturnsParameterError)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x01;
    options.authMode = DesfireAuthMode::ISO;
    options.sessionKeyType = DesfireKeyType::DES;
    options.newKeyType = DesfireKeyType::DES3_2K;
    options.newKey = toEtl<24>(hexToBytes("00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("CA A6 74 E8 CA E8 52 5E CA A6 74 E8 CA E8 52 5E"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00);

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_FALSE(requestResult.has_value());
    ASSERT_TRUE(requestResult.error().is<error::DesfireError>());
    EXPECT_EQ(requestResult.error().get<error::DesfireError>(), error::DesfireError::ParameterError);
}

TEST(DesfireChangeKeyCommandTests, ParseResponsePropagatesCardError)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::ISO;
    options.sessionKeyType = DesfireKeyType::DES;
    options.newKeyType = DesfireKeyType::DES3_2K;
    options.newKey = toEtl<24>(hexToBytes("00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("C8 6C E2 5E 4C 64 7E 56 C8 6C E2 5E 4C 64 7E 56"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00);

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    etl::vector<uint8_t, 1> response;
    response.push_back(static_cast<uint8_t>(error::DesfireError::IntegrityError));
    auto parseResult = command.parseResponse(response, context);

    ASSERT_FALSE(parseResult.has_value());
    ASSERT_TRUE(parseResult.error().is<error::DesfireError>());
    EXPECT_EQ(parseResult.error().get<error::DesfireError>(), error::DesfireError::IntegrityError);
    EXPECT_FALSE(command.isComplete());
}

TEST(DesfireChangeKeyCommandTests, PiccMasterChangeToAesSetsKeyNumberFlag80)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::LEGACY;
    options.sessionKeyType = DesfireKeyType::DES;
    options.newKeyType = DesfireKeyType::AES;
    options.newKey = toEtl<24>(hexToBytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("2B 12 BD 7C 1D 3F E9 F7"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00,
        hexToBytes("00 00 00")); // PICC selected

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    std::vector<uint8_t> expectedData = hexToBytes(
        "80 64 63 EA 36 5B 3D 33 4B DD 11 AF 0D 1A CC D6 98 A5 56 39 6E 58 EC B8 AE");
    EXPECT_EQ(toStdVector(requestResult.value().data), expectedData);
}

TEST(DesfireChangeKeyCommandTests, PiccMasterRejectsNonZeroKeyNumber)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x01;
    options.authMode = DesfireAuthMode::LEGACY;
    options.sessionKeyType = DesfireKeyType::DES;
    options.newKeyType = DesfireKeyType::AES;
    options.newKey = toEtl<24>(hexToBytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("2B 12 BD 7C 1D 3F E9 F7"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00,
        hexToBytes("00 00 00")); // PICC selected

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_FALSE(requestResult.has_value());
    ASSERT_TRUE(requestResult.error().is<error::DesfireError>());
    EXPECT_EQ(requestResult.error().get<error::DesfireError>(), error::DesfireError::ParameterError);
}

TEST(DesfireChangeKeyCommandTests, PiccMasterIsoSessionStillUsesKeyNumberFlag80)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::ISO;
    options.sessionKeyType = DesfireKeyType::DES3_2K;
    options.newKeyType = DesfireKeyType::AES;
    options.newKey = toEtl<24>(hexToBytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("C8 6C E2 5E 4C 64 7E 56 C8 6C E2 5E 4C 64 7E 56"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00,
        hexToBytes("00 00 00")); // PICC selected

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());
    ASSERT_FALSE(requestResult.value().data.empty());
    EXPECT_EQ(requestResult.value().data[0], 0x80);
}

TEST(DesfireChangeKeyCommandTests, NonPiccRejectsAesTo2K3DesFamilyChange)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::AES;
    options.sessionKeyType = DesfireKeyType::AES;
    options.newKeyType = DesfireKeyType::DES3_2K;
    options.newKey = toEtl<24>(hexToBytes("11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("90 F7 A2 01 91 03 68 45 EC 63 DE CD 54 4B 99 31"),
        hexToBytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"),
        0x00,
        hexToBytes("BA DA 55"));

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_FALSE(requestResult.has_value());
    ASSERT_TRUE(requestResult.error().is<error::DesfireError>());
    EXPECT_EQ(requestResult.error().get<error::DesfireError>(), error::DesfireError::ParameterError);
}

TEST(DesfireChangeKeyCommandTests, NonPiccRejectsAesTo3K3DesFamilyChange)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::AES;
    options.sessionKeyType = DesfireKeyType::AES;
    options.newKeyType = DesfireKeyType::DES3_3K;
    options.newKey = toEtl<24>(hexToBytes(
        "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("90 F7 A2 01 91 03 68 45 EC 63 DE CD 54 4B 99 31"),
        hexToBytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"),
        0x00,
        hexToBytes("BA DA 55"));

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_FALSE(requestResult.has_value());
    ASSERT_TRUE(requestResult.error().is<error::DesfireError>());
    EXPECT_EQ(requestResult.error().get<error::DesfireError>(), error::DesfireError::ParameterError);
}

TEST(DesfireChangeKeyCommandTests, NonPiccAllowsDesAnd2K3DesWithinSameFamily)
{
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::ISO;
    options.sessionKeyType = DesfireKeyType::DES3_2K;
    options.newKeyType = DesfireKeyType::DES;
    options.newKey = toEtl<24>(hexToBytes("00 00 00 00 00 00 00 00"));
    options.newKeyVersion = 0x00;

    DesfireContext context = buildContext(
        hexToBytes("C8 6C E2 5E 4C 64 7E 56 C8 6C E2 5E 4C 64 7E 56"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00,
        hexToBytes("BA DA 55"));

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());
}

TEST(DesfireChangeKeyCommandTests, FallsBackToContextCipherWhenSessionTypeOverrideConflicts)
{
    // Degenerate ISO 2K3DES keys can yield DES-sized session material.
    // Ensure ChangeKey uses the live session cipher instead of failing InvalidState.
    ChangeKeyCommandOptions options;
    options.keyNo = 0x00;
    options.authMode = DesfireAuthMode::ISO;
    options.sessionKeyType = DesfireKeyType::DES3_2K; // caller hint
    options.newKeyType = DesfireKeyType::DES3_2K;
    options.newKey = toEtl<24>(hexToBytes("21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30"));
    options.newKeyVersion = 0x00;

    // Live session context indicates DES-sized session key.
    DesfireContext context = buildContext(
        hexToBytes("01 02 03 04 05 06 07 08"),
        hexToBytes("00 00 00 00 00 00 00 00"),
        0x00,
        hexToBytes("A1 A5 54"));

    ChangeKeyCommand command(options);
    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());
    EXPECT_EQ(requestResult.value().commandCode, 0xC4);
    ASSERT_FALSE(requestResult.value().data.empty());
    EXPECT_EQ(requestResult.value().data[0], 0x00);

    etl::vector<uint8_t, 1> response;
    response.push_back(0x00);
    auto parseResult = command.parseResponse(response, context);
    ASSERT_TRUE(parseResult.has_value());
    EXPECT_TRUE(command.isComplete());
}

TEST(DesfireChangeKeyCommandTests, MatchesStandaloneVectorSuite)
{
    struct VectorCase
    {
        const char* name;
        uint8_t keyNo;
        uint8_t authenticatedKeyNo;
        DesfireAuthMode authMode;
        DesfireKeyType sessionKeyType;
        const char* sessionKeyHex;
        const char* ivHex;
        DesfireKeyType newKeyType;
        const char* newKeyHex;
        std::optional<const char*> oldKeyHex;
        uint8_t newKeyVersion;
        const char* expectedEncryptedHex;
    };

    const VectorCase cases[] =
    {
        {
            "Vector 1",
            0x00, 0x00,
            DesfireAuthMode::ISO, DesfireKeyType::DES,
            "C8 6C E2 5E 4C 64 7E 56 C8 6C E2 5E 4C 64 7E 56",
            "00 00 00 00 00 00 00 00",
            DesfireKeyType::DES3_2K,
            "00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80",
            std::nullopt,
            0x00,
            "BE DE 0F C6 ED 34 7D CF 0D 51 C7 17 DF 75 D9 7D 2C 5A 2B A6 CA C7 47 9D"
        },
        {
            "Vector 2",
            0x01, 0x00,
            DesfireAuthMode::ISO, DesfireKeyType::DES,
            "CA A6 74 E8 CA E8 52 5E CA A6 74 E8 CA E8 52 5E",
            "00 00 00 00 00 00 00 00",
            DesfireKeyType::DES3_2K,
            "00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80",
            "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00",
            0x00,
            "4E B6 69 E4 8D CA 58 47 49 54 2E 1B E8 9C B4 C7 84 5A 38 C5 7D 19 DE 59"
        },
        {
            "Vector 3",
            0x00, 0x00,
            DesfireAuthMode::ISO, DesfireKeyType::AES,
            "90 F7 A2 01 91 03 68 45 EC 63 DE CD 54 4B 99 31",
            "8A 8F A3 6F 55 CD 21 0D D8 05 46 58 AC 70 D9 9A",
            DesfireKeyType::AES,
            "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00",
            std::nullopt,
            0x00,
            "63 53 75 E4 91 9F 8A F2 E9 E8 6B 1C 1B A5 5B 0C 08 07 EA F4 84 D7 A7 EF 6E 0C 30 84 16 0F 5A 61"
        },
        {
            "Vector 4",
            0x01, 0x00,
            DesfireAuthMode::ISO, DesfireKeyType::AES,
            "C2 A1 E4 7B D8 10 00 44 FE 6D 00 A7 4D 7A B1 7C",
            "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00",
            DesfireKeyType::AES,
            "00 10 20 30 40 50 60 70 80 90 A0 B0 B0 A0 90 80",
            "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00",
            0x10,
            "E7 EC CB 6B D1 CA 64 BC 16 1A 12 B1 C0 24 F7 14 30 33 74 08 C8 A8 7E AC AB 7A 1F F1 89 51 FC A3"
        },
        {
            "Vector 6",
            0x00, 0x00,
            DesfireAuthMode::LEGACY, DesfireKeyType::DES,
            "92 F1 35 8C EA E9 6A 10",
            "00 00 00 00 00 00 00 00",
            DesfireKeyType::DES,
            "00 00 00 00 00 00 00 00",
            std::nullopt,
            0x00,
            "EA 70 40 19 C3 EF 41 9F D6 3A E2 94 B4 01 4C 03 C6 F3 2A EC DD 56 19 D6"
        },
        {
            "Vector 7",
            0x00, 0x00,
            DesfireAuthMode::LEGACY, DesfireKeyType::DES,
            "41 2E 7A 0C FB A2 18 A4",
            "00 00 00 00 00 00 00 00",
            DesfireKeyType::DES3_2K,
            "00 00 00 00 00 00 00 00 02 02 02 02 02 02 02 02",
            std::nullopt,
            0x00,
            "ED E9 D7 31 50 07 18 20 B2 DD DA 92 64 67 B8 B9 D8 A8 B9 78 7F 3F F5 BA"
        },
        {
            "Vector 8",
            0x00, 0x00,
            DesfireAuthMode::LEGACY, DesfireKeyType::DES3_2K,
            "AF 76 04 EE 62 D6 8A 14 20 83 F9 8D 46 DD 4A 86",
            "00 00 00 00 00 00 00 00",
            DesfireKeyType::DES,
            "00 00 00 00 00 00 00 00",
            std::nullopt,
            0x00,
            "79 5C 09 70 E7 F8 F6 28 83 51 02 45 4C 8B BA AA 30 25 32 0F 72 14 E4 38"
        },
        {
            "Vector 9",
            0x80, 0x00,
            DesfireAuthMode::LEGACY, DesfireKeyType::DES,
            "2B 12 BD 7C 1D 3F E9 F7",
            "00 00 00 00 00 00 00 00",
            DesfireKeyType::AES,
            "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00",
            std::nullopt,
            0x00,
            "64 63 EA 36 5B 3D 33 4B DD 11 AF 0D 1A CC D6 98 A5 56 39 6E 58 EC B8 AE"
        },
        {
            "Vector 10",
            0x00, 0x00,
            DesfireAuthMode::ISO, DesfireKeyType::AES,
            "F6 2A 18 D5 03 56 1E 42 C0 7C 13 13 C8 91 50 F1",
            "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00",
            DesfireKeyType::DES,
            "00 00 00 00 00 00 00 00",
            std::nullopt,
            0x00,
            "03 30 DC 9B A1 A3 07 56 C0 BA B7 2C B0 C3 58 2A 14 E9 EC 87 A9 D0 5C 50 A4 A5 8B C1 BB 33 77 F2"
        },
        {
            "Vector 11",
            0x00, 0x00,
            DesfireAuthMode::LEGACY, DesfireKeyType::DES,
            "92 F1 35 8C EA E9 6A 10",
            "00 00 00 00 00 00 00 00",
            DesfireKeyType::DES,
            "FE FE FE FE FE FE FE FE",
            std::nullopt,
            0x00,
            "D8 BA 7D 1C 90 65 4D D1 80 A6 1E 2B 56 AE B0 5C BE 37 DA AB 95 82 49 4B"
        },
        {
            "Vector 12",
            0x00, 0x00,
            DesfireAuthMode::LEGACY, DesfireKeyType::DES,
            "92 F1 35 8C EA E9 6A 10",
            "00 00 00 00 00 00 00 00",
            DesfireKeyType::DES,
            "10 10 10 10 10 10 10 10",
            std::nullopt,
            0x00,
            "A3 CB B1 6B C6 2A CE 56 DB 0D 83 81 CE 31 A5 C7 31 8D D7 9E 00 1A CB 99"
        },
    };

    for (const auto& vectorCase : cases)
    {
        SCOPED_TRACE(vectorCase.name);

        ChangeKeyCommandOptions options;
        options.keyNo = vectorCase.keyNo;
        options.authMode = vectorCase.authMode;
        options.sessionKeyType = vectorCase.sessionKeyType;
        options.newKeyType = vectorCase.newKeyType;
        options.newKey = toEtl<24>(hexToBytes(vectorCase.newKeyHex));
        options.newKeyVersion = vectorCase.newKeyVersion;
        if (vectorCase.oldKeyHex.has_value())
        {
            options.oldKey = toEtl<24>(hexToBytes(vectorCase.oldKeyHex.value()));
        }

        DesfireContext context = buildContext(
            hexToBytes(vectorCase.sessionKeyHex),
            hexToBytes(vectorCase.ivHex),
            vectorCase.authenticatedKeyNo);

        ChangeKeyCommand command(options);
        auto requestResult = command.buildRequest(context);
        ASSERT_TRUE(requestResult.has_value());

        std::vector<uint8_t> expectedData;
        expectedData.push_back(vectorCase.keyNo);
        const auto encrypted = hexToBytes(vectorCase.expectedEncryptedHex);
        expectedData.insert(expectedData.end(), encrypted.begin(), encrypted.end());

        EXPECT_EQ(toStdVector(requestResult.value().data), expectedData);
    }
}
