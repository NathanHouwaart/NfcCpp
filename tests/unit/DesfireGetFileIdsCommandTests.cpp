#include <gtest/gtest.h>
#include "Nfc/Desfire/Commands/GetFileIdsCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

TEST(DesfireGetFileIdsCommandTests, ParsesPlainUnauthenticatedResponse)
{
    GetFileIdsCommand command;
    DesfireContext context;
    context.authenticated = false;

    etl::vector<uint8_t, 16> response;
    response.push_back(0x00);
    response.push_back(0x01);
    response.push_back(0x02);
    response.push_back(0x1F);

    auto result = command.parseResponse(response, context);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(command.getFileIds().size(), 3U);
    EXPECT_EQ(command.getFileIds()[0], 0x01);
    EXPECT_EQ(command.getFileIds()[1], 0x02);
    EXPECT_EQ(command.getFileIds()[2], 0x1F);
}

TEST(DesfireGetFileIdsCommandTests, StripsAuthenticatedTrailingCmac8)
{
    GetFileIdsCommand command;
    DesfireContext context;
    context.authenticated = true;

    // One real file ID (0x01) + 8-byte CMAC-like tail (>0x1F values).
    etl::vector<uint8_t, 16> response;
    response.push_back(0x00);
    response.push_back(0x01);
    response.push_back(0x73);
    response.push_back(0xA8);
    response.push_back(0x05);
    response.push_back(0x18);
    response.push_back(0x54);
    response.push_back(0x7B);
    response.push_back(0x5C);
    response.push_back(0x48);

    auto result = command.parseResponse(response, context);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(command.getFileIds().size(), 1U);
    EXPECT_EQ(command.getFileIds()[0], 0x01);
}

TEST(DesfireGetFileIdsCommandTests, RejectsOutOfRangeFileIdWhenUnauthenticated)
{
    GetFileIdsCommand command;
    DesfireContext context;
    context.authenticated = false;

    etl::vector<uint8_t, 8> response;
    response.push_back(0x00);
    response.push_back(0x01);
    response.push_back(0x42); // invalid file id (> 0x1F)

    auto result = command.parseResponse(response, context);
    ASSERT_FALSE(result.has_value());
    ASSERT_TRUE(result.error().is<error::DesfireError>());
    EXPECT_EQ(result.error().get<error::DesfireError>(), error::DesfireError::InvalidResponse);
}

