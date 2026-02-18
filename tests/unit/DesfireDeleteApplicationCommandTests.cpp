#include <gtest/gtest.h>
#include "Nfc/Desfire/Commands/DeleteApplicationCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

TEST(DesfireDeleteApplicationCommandTests, BuildRequestEncodesAidAndCommandCode)
{
    const etl::array<uint8_t, 3> aid = {0xCC, 0xBB, 0xAA};
    DeleteApplicationCommand command(aid);
    DesfireContext context;

    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    const auto& request = requestResult.value();
    EXPECT_EQ(request.commandCode, 0xDA);
    EXPECT_EQ(request.expectedResponseLength, 0U);
    ASSERT_EQ(request.data.size(), 3U);
    EXPECT_EQ(request.data[0], 0xCC);
    EXPECT_EQ(request.data[1], 0xBB);
    EXPECT_EQ(request.data[2], 0xAA);
}

TEST(DesfireDeleteApplicationCommandTests, BuildRequestRejectsAfterCompletionWithoutReset)
{
    const etl::array<uint8_t, 3> aid = {0x01, 0x02, 0x03};
    DeleteApplicationCommand command(aid);
    DesfireContext context;

    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    etl::vector<uint8_t, 1> response;
    response.push_back(0x00);
    auto parseResult = command.parseResponse(response, context);
    ASSERT_TRUE(parseResult.has_value());
    EXPECT_TRUE(command.isComplete());

    auto secondBuild = command.buildRequest(context);
    ASSERT_FALSE(secondBuild.has_value());
    ASSERT_TRUE(secondBuild.error().is<error::DesfireError>());
    EXPECT_EQ(secondBuild.error().get<error::DesfireError>(), error::DesfireError::InvalidState);
}

TEST(DesfireDeleteApplicationCommandTests, ParseResponseSuccessCompletesCommand)
{
    const etl::array<uint8_t, 3> aid = {0x01, 0x02, 0x03};
    DeleteApplicationCommand command(aid);
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

TEST(DesfireDeleteApplicationCommandTests, ParseResponseRejectsEmptyResponse)
{
    const etl::array<uint8_t, 3> aid = {0x01, 0x02, 0x03};
    DeleteApplicationCommand command(aid);
    DesfireContext context;

    etl::vector<uint8_t, 1> response;
    auto parseResult = command.parseResponse(response, context);

    ASSERT_FALSE(parseResult.has_value());
    ASSERT_TRUE(parseResult.error().is<error::DesfireError>());
    EXPECT_EQ(parseResult.error().get<error::DesfireError>(), error::DesfireError::InvalidResponse);
}

