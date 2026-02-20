#include "Nfc/Desfire/SecureMessagingPolicy.h"
#include "Nfc/Desfire/DesfireContext.h"
#include <gtest/gtest.h>

using namespace nfc;

namespace
{
    void fillVector(etl::vector<uint8_t, 24>& target, size_t count, uint8_t seed)
    {
        target.clear();
        for (size_t i = 0U; i < count; ++i)
        {
            target.push_back(static_cast<uint8_t>(seed + static_cast<uint8_t>(i)));
        }
    }
}

TEST(DesfireSecureMessagingPolicyTests, DerivePlainRequestIvAesReturns16Bytes)
{
    DesfireContext context;
    context.authenticated = true;
    context.authScheme = SessionAuthScheme::Aes;
    fillVector(context.sessionKeyEnc, 16U, 0x10U);
    fillVector(context.sessionKeyMac, 16U, 0x20U);
    context.iv.resize(16U, 0x00U);

    etl::vector<uint8_t, 2> message;
    message.push_back(0x6EU);
    message.push_back(0x00U);

    auto result = SecureMessagingPolicy::derivePlainRequestIv(context, message, true);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 16U);
}

TEST(DesfireSecureMessagingPolicyTests, DerivePlainRequestIvIso3K3DesReturns8Bytes)
{
    DesfireContext context;
    context.authenticated = true;
    context.authScheme = SessionAuthScheme::Iso;
    fillVector(context.sessionKeyEnc, 24U, 0x30U);
    fillVector(context.sessionKeyMac, 24U, 0x40U);
    context.iv.resize(8U, 0x00U);

    etl::vector<uint8_t, 2> message;
    message.push_back(0xF5U);
    message.push_back(0x01U);

    auto result = SecureMessagingPolicy::derivePlainRequestIv(context, message, true);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 8U);
}

TEST(DesfireSecureMessagingPolicyTests, DerivePlainRequestIvIso2K3DesReturns8Bytes)
{
    DesfireContext context;
    context.authenticated = true;
    context.authScheme = SessionAuthScheme::Iso;
    fillVector(context.sessionKeyEnc, 16U, 0x50U);
    fillVector(context.sessionKeyMac, 16U, 0x60U);
    context.iv.resize(8U, 0x00U);

    etl::vector<uint8_t, 2> message;
    message.push_back(0xF5U);
    message.push_back(0x02U);

    auto result = SecureMessagingPolicy::derivePlainRequestIv(context, message, true);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 8U);
}

TEST(DesfireSecureMessagingPolicyTests, UpdateContextIvFromEncryptedCiphertextAesUsesLastBlock)
{
    DesfireContext context;
    context.authenticated = true;
    context.authScheme = SessionAuthScheme::Aes;
    fillVector(context.sessionKeyEnc, 16U, 0x11U);
    context.iv.resize(16U, 0x00U);

    etl::vector<uint8_t, 32> ciphertext;
    for (uint8_t i = 1U; i <= 32U; ++i)
    {
        ciphertext.push_back(i);
    }

    auto result = SecureMessagingPolicy::updateContextIvFromEncryptedCiphertext(context, ciphertext);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(context.iv.size(), 16U);
    for (size_t i = 0U; i < 16U; ++i)
    {
        EXPECT_EQ(context.iv[i], static_cast<uint8_t>(17U + i));
    }
}

TEST(DesfireSecureMessagingPolicyTests, UpdateContextIvFromEncryptedCiphertextIso2K3DesUsesLastBlock)
{
    DesfireContext context;
    context.authenticated = true;
    context.authScheme = SessionAuthScheme::Iso;
    fillVector(context.sessionKeyEnc, 16U, 0x21U);
    context.iv.resize(8U, 0x00U);

    etl::vector<uint8_t, 16> ciphertext;
    for (uint8_t i = 1U; i <= 16U; ++i)
    {
        ciphertext.push_back(i);
    }

    auto result = SecureMessagingPolicy::updateContextIvFromEncryptedCiphertext(context, ciphertext);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(context.iv.size(), 8U);
    for (size_t i = 0U; i < 8U; ++i)
    {
        EXPECT_EQ(context.iv[i], static_cast<uint8_t>(9U + i));
    }
}

TEST(DesfireSecureMessagingPolicyTests, ApplyLegacyCommandBoundaryIvPolicyResetsLegacyDesIv)
{
    DesfireContext context;
    context.authenticated = true;
    context.authScheme = SessionAuthScheme::Legacy;
    fillVector(context.sessionKeyEnc, 8U, 0x71U);
    context.iv.clear();
    for (uint8_t i = 0U; i < 8U; ++i)
    {
        context.iv.push_back(static_cast<uint8_t>(0xA0U + i));
    }

    SecureMessagingPolicy::applyLegacyCommandBoundaryIvPolicy(context);
    ASSERT_EQ(context.iv.size(), 8U);
    for (size_t i = 0U; i < context.iv.size(); ++i)
    {
        EXPECT_EQ(context.iv[i], 0x00U);
    }
}

TEST(DesfireSecureMessagingPolicyTests, ApplyLegacyCommandBoundaryIvPolicyResetsLegacy2K3DesIv)
{
    DesfireContext context;
    context.authenticated = true;
    context.authScheme = SessionAuthScheme::Legacy;
    fillVector(context.sessionKeyEnc, 16U, 0x81U);
    context.iv.clear();
    for (uint8_t i = 0U; i < 8U; ++i)
    {
        context.iv.push_back(static_cast<uint8_t>(0xB0U + i));
    }

    SecureMessagingPolicy::applyLegacyCommandBoundaryIvPolicy(context);
    ASSERT_EQ(context.iv.size(), 8U);
    for (size_t i = 0U; i < context.iv.size(); ++i)
    {
        EXPECT_EQ(context.iv[i], 0x00U);
    }
}
