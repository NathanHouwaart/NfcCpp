#include <gtest/gtest.h>
#include "Nfc/Desfire/Commands/ReadDataCommand.h"
#include "Nfc/Desfire/Commands/WriteDataCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Error/DesfireError.h"

using namespace nfc;

TEST(DesfireReadDataCommandTests, BuildRequestEncodesFileOffsetAndChunkLength)
{
    ReadDataCommandOptions options;
    options.fileNo = 0x01;
    options.offset = 0x001122U;
    options.length = 10U;
    options.chunkSize = 4U;

    ReadDataCommand command(options);
    DesfireContext context;

    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value()) << requestResult.error().toString().c_str();

    const auto& request = requestResult.value();
    EXPECT_EQ(request.commandCode, 0xBD);
    ASSERT_EQ(request.data.size(), 7U);
    EXPECT_EQ(request.data[0], 0x01);
    EXPECT_EQ(request.data[1], 0x22);
    EXPECT_EQ(request.data[2], 0x11);
    EXPECT_EQ(request.data[3], 0x00);
    EXPECT_EQ(request.data[4], 0x04);
    EXPECT_EQ(request.data[5], 0x00);
    EXPECT_EQ(request.data[6], 0x00);
}

TEST(DesfireReadDataCommandTests, HandlesAdditionalFrameWithinChunk)
{
    ReadDataCommandOptions options;
    options.fileNo = 0x00;
    options.offset = 0x000000U;
    options.length = 5U;
    options.chunkSize = 5U;

    ReadDataCommand command(options);
    DesfireContext context;

    auto firstRequest = command.buildRequest(context);
    ASSERT_TRUE(firstRequest.has_value());
    EXPECT_EQ(firstRequest.value().commandCode, 0xBD);

    etl::vector<uint8_t, 16> firstResponse;
    firstResponse.push_back(0xAF);
    firstResponse.push_back(0x10);
    firstResponse.push_back(0x11);
    auto firstParse = command.parseResponse(firstResponse, context);
    ASSERT_TRUE(firstParse.has_value());
    EXPECT_FALSE(command.isComplete());

    auto continuationRequest = command.buildRequest(context);
    ASSERT_TRUE(continuationRequest.has_value());
    EXPECT_EQ(continuationRequest.value().commandCode, 0xAF);

    etl::vector<uint8_t, 16> secondResponse;
    secondResponse.push_back(0x00);
    secondResponse.push_back(0x12);
    secondResponse.push_back(0x13);
    secondResponse.push_back(0x14);
    auto secondParse = command.parseResponse(secondResponse, context);
    ASSERT_TRUE(secondParse.has_value());
    EXPECT_TRUE(command.isComplete());

    const auto& data = command.getData();
    ASSERT_EQ(data.size(), 5U);
    EXPECT_EQ(data[0], 0x10);
    EXPECT_EQ(data[1], 0x11);
    EXPECT_EQ(data[2], 0x12);
    EXPECT_EQ(data[3], 0x13);
    EXPECT_EQ(data[4], 0x14);
}

TEST(DesfireReadDataCommandTests, TrimsTrailingAuthenticatedMacBytes)
{
    ReadDataCommandOptions options;
    options.fileNo = 0x00;
    options.offset = 0x000000U;
    options.length = 4U;
    options.chunkSize = 4U;
    options.communicationSettings = 0x00U;

    ReadDataCommand command(options);
    DesfireContext context;
    context.authenticated = true;

    auto requestResult = command.buildRequest(context);
    ASSERT_TRUE(requestResult.has_value());

    etl::vector<uint8_t, 16> response;
    response.push_back(0x00);
    response.push_back(0xDE);
    response.push_back(0xAD);
    response.push_back(0xBE);
    response.push_back(0xEF);
    response.push_back(0xAA);
    response.push_back(0xBB);
    response.push_back(0xCC);
    response.push_back(0xDD);

    auto parseResult = command.parseResponse(response, context);
    ASSERT_TRUE(parseResult.has_value());
    EXPECT_TRUE(command.isComplete());

    const auto& data = command.getData();
    ASSERT_EQ(data.size(), 4U);
    EXPECT_EQ(data[0], 0xDE);
    EXPECT_EQ(data[1], 0xAD);
    EXPECT_EQ(data[2], 0xBE);
    EXPECT_EQ(data[3], 0xEF);
}

TEST(DesfireWriteDataCommandTests, BuildAndParseChunkedWrites)
{
    etl::vector<uint8_t, 16> payload;
    payload.push_back(0x01);
    payload.push_back(0x02);
    payload.push_back(0x03);
    payload.push_back(0x04);
    payload.push_back(0x05);
    payload.push_back(0x06);

    WriteDataCommandOptions options;
    options.fileNo = 0x02;
    options.offset = 2U;
    options.data = &payload;
    options.chunkSize = 4U;

    WriteDataCommand command(options);
    DesfireContext context;

    auto request1 = command.buildRequest(context);
    ASSERT_TRUE(request1.has_value());
    EXPECT_EQ(request1.value().commandCode, 0x3D);
    ASSERT_EQ(request1.value().data.size(), 11U);
    EXPECT_EQ(request1.value().data[0], 0x02);
    EXPECT_EQ(request1.value().data[1], 0x02);
    EXPECT_EQ(request1.value().data[2], 0x00);
    EXPECT_EQ(request1.value().data[3], 0x00);
    EXPECT_EQ(request1.value().data[4], 0x04);
    EXPECT_EQ(request1.value().data[5], 0x00);
    EXPECT_EQ(request1.value().data[6], 0x00);
    EXPECT_EQ(request1.value().data[7], 0x01);
    EXPECT_EQ(request1.value().data[10], 0x04);

    etl::vector<uint8_t, 2> okResponse;
    okResponse.push_back(0x00);
    auto parse1 = command.parseResponse(okResponse, context);
    ASSERT_TRUE(parse1.has_value());
    EXPECT_FALSE(command.isComplete());

    auto request2 = command.buildRequest(context);
    ASSERT_TRUE(request2.has_value());
    EXPECT_EQ(request2.value().commandCode, 0x3D);
    ASSERT_EQ(request2.value().data.size(), 9U);
    EXPECT_EQ(request2.value().data[1], 0x06); // offset now 2 + 4
    EXPECT_EQ(request2.value().data[4], 0x02); // remaining length
    EXPECT_EQ(request2.value().data[7], 0x05);
    EXPECT_EQ(request2.value().data[8], 0x06);

    auto parse2 = command.parseResponse(okResponse, context);
    ASSERT_TRUE(parse2.has_value());
    EXPECT_TRUE(command.isComplete());
}

TEST(DesfireWriteDataCommandTests, RejectsEmptyPayload)
{
    etl::vector<uint8_t, 4> payload;

    WriteDataCommandOptions options;
    options.fileNo = 0x00;
    options.offset = 0U;
    options.data = &payload;
    options.chunkSize = 16U;

    WriteDataCommand command(options);
    DesfireContext context;

    auto requestResult = command.buildRequest(context);
    ASSERT_FALSE(requestResult.has_value());
    ASSERT_TRUE(requestResult.error().is<error::DesfireError>());
    EXPECT_EQ(requestResult.error().get<error::DesfireError>(), error::DesfireError::ParameterError);
}
