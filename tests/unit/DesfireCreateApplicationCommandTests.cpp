#include <gtest/gtest.h>
#include "Nfc/Desfire/Commands/CreateApplicationCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

TEST(DesfireCreateApplicationCommandTests, BuildRequestEncodesBasicPayloadFor2K3Des)
{
    CreateApplicationCommandOptions options;
    options.aid = {0x34, 0x12, 0x00};
    options.keySettings1 = 0x0F;
    options.keyCount = 0x02;
    options.keyType = DesfireKeyType::DES3_2K;

    CreateApplicationCommand command(options);
    DesfireContext context;

    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    const auto& request = requestResult.value();
    EXPECT_EQ(request.commandCode, 0xCA);
    EXPECT_EQ(request.expectedResponseLength, 0U);
    ASSERT_EQ(request.data.size(), 5U);
    EXPECT_EQ(request.data[0], 0x34);
    EXPECT_EQ(request.data[1], 0x12);
    EXPECT_EQ(request.data[2], 0x00);
    EXPECT_EQ(request.data[3], 0x0F);
    EXPECT_EQ(request.data[4], 0x02);
}

TEST(DesfireCreateApplicationCommandTests, BuildRequestEncodesAesKeyTypeBits)
{
    CreateApplicationCommandOptions options;
    options.aid = {0xAA, 0xBB, 0xCC};
    options.keySettings1 = 0x0F;
    options.keyCount = 0x05;
    options.keyType = DesfireKeyType::AES;

    CreateApplicationCommand command(options);
    DesfireContext context;

    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());
    ASSERT_EQ(requestResult.value().data.size(), 5U);
    EXPECT_EQ(requestResult.value().data[4], 0x85);
}

TEST(DesfireCreateApplicationCommandTests, BuildRequestEncodes3K3DesKeyTypeBits)
{
    CreateApplicationCommandOptions options;
    options.aid = {0xAA, 0xBB, 0xCC};
    options.keySettings1 = 0x0F;
    options.keyCount = 0x03;
    options.keyType = DesfireKeyType::DES3_3K;

    CreateApplicationCommand command(options);
    DesfireContext context;

    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());
    ASSERT_EQ(requestResult.value().data.size(), 5U);
    EXPECT_EQ(requestResult.value().data[4], 0x43);
}

TEST(DesfireCreateApplicationCommandTests, BuildRequestRejectsInvalidKeyCount)
{
    CreateApplicationCommandOptions options;
    options.aid = {0x01, 0x02, 0x03};
    options.keySettings1 = 0x0F;
    options.keyCount = 0x00;
    options.keyType = DesfireKeyType::AES;

    CreateApplicationCommand command(options);
    DesfireContext context;
    auto requestResult = command.buildRequest(context);

    ASSERT_FALSE(requestResult.has_value());
    ASSERT_TRUE(requestResult.error().is<error::DesfireError>());
    EXPECT_EQ(requestResult.error().get<error::DesfireError>(), error::DesfireError::ParameterError);
}

TEST(DesfireCreateApplicationCommandTests, BuildRequestRejectsUnknownKeyType)
{
    CreateApplicationCommandOptions options;
    options.aid = {0x01, 0x02, 0x03};
    options.keySettings1 = 0x0F;
    options.keyCount = 0x01;
    options.keyType = DesfireKeyType::UNKNOWN;

    CreateApplicationCommand command(options);
    DesfireContext context;
    auto requestResult = command.buildRequest(context);

    ASSERT_FALSE(requestResult.has_value());
    ASSERT_TRUE(requestResult.error().is<error::DesfireError>());
    EXPECT_EQ(requestResult.error().get<error::DesfireError>(), error::DesfireError::ParameterError);
}

TEST(DesfireCreateApplicationCommandTests, ParseResponseSuccessCompletesCommand)
{
    CreateApplicationCommandOptions options;
    options.aid = {0x01, 0x02, 0x03};
    options.keySettings1 = 0x0F;
    options.keyCount = 0x01;
    options.keyType = DesfireKeyType::AES;

    CreateApplicationCommand command(options);
    DesfireContext context;

    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());
    EXPECT_FALSE(command.isComplete());

    etl::vector<uint8_t, 2> response;
    response.push_back(0x00);
    auto parseResult = command.parseResponse(response, context);
    ASSERT_TRUE(parseResult.has_value());
    EXPECT_EQ(parseResult.value().statusCode, 0x00);
    EXPECT_TRUE(command.isComplete());
}

TEST(DesfireCreateApplicationCommandTests, ParseResponseRejectsEmptyResponse)
{
    CreateApplicationCommandOptions options;
    options.aid = {0x01, 0x02, 0x03};
    options.keySettings1 = 0x0F;
    options.keyCount = 0x01;
    options.keyType = DesfireKeyType::AES;

    CreateApplicationCommand command(options);
    DesfireContext context;

    etl::vector<uint8_t, 2> response;
    auto parseResult = command.parseResponse(response, context);

    ASSERT_FALSE(parseResult.has_value());
    ASSERT_TRUE(parseResult.error().is<error::DesfireError>());
    EXPECT_EQ(parseResult.error().get<error::DesfireError>(), error::DesfireError::InvalidResponse);
}

