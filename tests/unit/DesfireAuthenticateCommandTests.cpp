#include <gtest/gtest.h>
#include <array>
#include <vector>
#include <cstring>
#include <aes.hpp>
#include <cppdes/des3cbc.h>
#include "Nfc/Desfire/Commands/AuthenticateCommand.h"
#include "Nfc/Desfire/DesfireContext.h"
#include "Utils/DesfireCrypto.h"

using namespace nfc;
using namespace crypto;

namespace
{
    etl::vector<uint8_t, 16> rotateLeft8(const std::array<uint8_t, 8>& in)
    {
        etl::vector<uint8_t, 16> out;
        for (size_t i = 1; i < 8; ++i)
        {
            out.push_back(in[i]);
        }
        out.push_back(in[0]);
        return out;
    }

    etl::vector<uint8_t, 16> toEtl16(const uint8_t* data, size_t len)
    {
        etl::vector<uint8_t, 16> out;
        for (size_t i = 0; i < len; ++i)
        {
            out.push_back(data[i]);
        }
        return out;
    }

    etl::vector<uint8_t, 24> toEtl24(const std::vector<uint8_t>& data)
    {
        etl::vector<uint8_t, 24> out;
        for (uint8_t b : data)
        {
            out.push_back(b);
        }
        return out;
    }

    void desCbcDecrypt(
        const uint8_t* ciphertext,
        size_t length,
        const uint8_t* key8,
        const uint8_t* iv8,
        uint8_t* plaintext)
    {
        uint8_t previous[8];
        std::memcpy(previous, iv8, 8);

        for (size_t offset = 0; offset < length; offset += 8)
        {
            uint8_t block[8];
            DesFireCrypto::desDecrypt(ciphertext + offset, key8, block);
            for (size_t i = 0; i < 8; ++i)
            {
                plaintext[offset + i] = static_cast<uint8_t>(block[i] ^ previous[i]);
            }
            std::memcpy(previous, ciphertext + offset, 8);
        }
    }

    void desCbcEncrypt(
        const uint8_t* plaintext,
        size_t length,
        const uint8_t* key8,
        const uint8_t* iv8,
        uint8_t* ciphertext)
    {
        uint8_t previous[8];
        std::memcpy(previous, iv8, 8);

        for (size_t offset = 0; offset < length; offset += 8)
        {
            uint8_t block[8];
            for (size_t i = 0; i < 8; ++i)
            {
                block[i] = static_cast<uint8_t>(plaintext[offset + i] ^ previous[i]);
            }
            DesFireCrypto::desEncrypt(block, key8, ciphertext + offset);
            std::memcpy(previous, ciphertext + offset, 8);
        }
    }

    void legacySendModeDecodeToPlaintext(
        const uint8_t* ciphertext,
        size_t length,
        const uint8_t* key8,
        uint8_t* plaintext)
    {
        uint8_t previousCipher[8] = {0};

        for (size_t offset = 0; offset < length; offset += 8)
        {
            uint8_t decrypted[8];
            // PCD legacy send mode uses D_K(X_i XOR Y_{i-1}).
            // Card recovers X_i with E_K(Y_i) XOR Y_{i-1}.
            DesFireCrypto::desEncrypt(ciphertext + offset, key8, decrypted);

            for (size_t i = 0; i < 8; ++i)
            {
                plaintext[offset + i] = static_cast<uint8_t>(decrypted[i] ^ previousCipher[i]);
            }

            std::memcpy(previousCipher, ciphertext + offset, 8);
        }
    }

    void des3kCbcEncrypt(
        const uint8_t* plaintext,
        size_t length,
        const uint8_t* key24,
        const uint8_t* iv8,
        uint8_t* ciphertext)
    {
        DES3CBC des3Cbc(
            bytesToUint64(key24),
            bytesToUint64(key24 + 8),
            bytesToUint64(key24 + 16),
            bytesToUint64(iv8));

        for (size_t offset = 0; offset < length; offset += 8)
        {
            const uint64_t block = bytesToUint64(plaintext + offset);
            const uint64_t cipherBlock = des3Cbc.encrypt(block);
            uint64ToBytes(cipherBlock, ciphertext + offset);
        }
    }

    void des3kCbcDecrypt(
        const uint8_t* ciphertext,
        size_t length,
        const uint8_t* key24,
        const uint8_t* iv8,
        uint8_t* plaintext)
    {
        DES3CBC des3Cbc(
            bytesToUint64(key24),
            bytesToUint64(key24 + 8),
            bytesToUint64(key24 + 16),
            bytesToUint64(iv8));

        for (size_t offset = 0; offset < length; offset += 8)
        {
            const uint64_t plainBlock = des3Cbc.decrypt(bytesToUint64(ciphertext + offset));
            uint64ToBytes(plainBlock, plaintext + offset);
        }
    }

    std::array<uint8_t, 16> rotateLeft16(const std::array<uint8_t, 16>& in)
    {
        std::array<uint8_t, 16> out = {};
        for (size_t i = 0; i < 15; ++i)
        {
            out[i] = in[i + 1];
        }
        out[15] = in[0];
        return out;
    }
}

TEST(DesfireAuthenticateCommandTests, IsoDegenerate16ByteKeyDerivesDesSessionKey)
{
    AuthenticateCommandOptions options;
    options.mode = DesfireAuthMode::ISO;
    options.keyNo = 0x00;
    options.key = toEtl24(std::vector<uint8_t>(16, 0x00)); // K1 == K2 -> DES-style session key

    AuthenticateCommand command(options);
    DesfireContext context;

    auto request1 = command.buildRequest(context);
    ASSERT_TRUE(request1.has_value());
    EXPECT_EQ(request1.value().commandCode, static_cast<uint8_t>(DesfireAuthMode::ISO));

    const std::array<uint8_t, 8> rndB = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uint8_t encRndB[8] = {0};
    DesFireCrypto::desEncrypt(rndB.data(), options.key.data(), encRndB);

    etl::vector<uint8_t, 9> response1;
    response1.push_back(0xAF);
    for (uint8_t b : encRndB)
    {
        response1.push_back(b);
    }

    auto parse1 = command.parseResponse(response1, context);
    ASSERT_TRUE(parse1.has_value());
    EXPECT_FALSE(command.isComplete());

    auto request2 = command.buildRequest(context);
    ASSERT_TRUE(request2.has_value());
    ASSERT_EQ(request2.value().commandCode, 0xAF);
    ASSERT_EQ(request2.value().data.size(), 16U);

    uint8_t plainAB[16] = {0};
    desCbcDecrypt(request2.value().data.data(), 16, options.key.data(), encRndB, plainAB);

    std::array<uint8_t, 8> rndA = {};
    std::memcpy(rndA.data(), plainAB, 8);

    const etl::vector<uint8_t, 16> rndBrot = rotateLeft8(rndB);
    for (size_t i = 0; i < 8; ++i)
    {
        EXPECT_EQ(plainAB[8 + i], rndBrot[i]);
    }

    etl::vector<uint8_t, 16> rndArot = rotateLeft8(rndA);
    uint8_t encRndArot[8] = {0};
    desCbcEncrypt(
        rndArot.data(),
        8,
        options.key.data(),
        request2.value().data.data() + 8,
        encRndArot);

    etl::vector<uint8_t, 9> response2;
    response2.push_back(0x00);
    for (uint8_t b : encRndArot)
    {
        response2.push_back(b);
    }

    auto parse2 = command.parseResponse(response2, context);
    ASSERT_TRUE(parse2.has_value());
    EXPECT_TRUE(command.isComplete());
    EXPECT_TRUE(context.authenticated);

    ASSERT_EQ(context.sessionKeyEnc.size(), 8U);

    std::array<uint8_t, 8> expectedSession = {
        rndA[0], rndA[1], rndA[2], rndA[3],
        rndB[0], rndB[1], rndB[2], rndB[3]
    };
    for (size_t i = 0; i < expectedSession.size(); ++i)
    {
        expectedSession[i] = static_cast<uint8_t>(expectedSession[i] & 0xFE);
        EXPECT_EQ(context.sessionKeyEnc[i], expectedSession[i]);
    }
}

TEST(DesfireAuthenticateCommandTests, LegacyDesAuthenticationWithNonZeroKeyUsesLegacySendMode)
{
    AuthenticateCommandOptions options;
    options.mode = DesfireAuthMode::LEGACY;
    options.keyNo = 0x00;
    options.key = toEtl24({
        0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8
    });

    AuthenticateCommand command(options);
    DesfireContext context;

    auto request1 = command.buildRequest(context);
    ASSERT_TRUE(request1.has_value());
    EXPECT_EQ(request1.value().commandCode, static_cast<uint8_t>(DesfireAuthMode::LEGACY));

    // AuthenticateCommand normalizes DES key parity/version bits internally.
    uint8_t normalizedKey[8];
    for (size_t i = 0; i < 8; ++i)
    {
        normalizedKey[i] = static_cast<uint8_t>(options.key[i] & 0xFE);
    }

    const std::array<uint8_t, 8> rndB = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t encRndB[8] = {0};
    // Single-block receive path: decrypt(cipher) on PCD side, so card sends E_K(RndB).
    DesFireCrypto::desEncrypt(rndB.data(), normalizedKey, encRndB);

    etl::vector<uint8_t, 9> response1;
    response1.push_back(0xAF);
    for (uint8_t b : encRndB)
    {
        response1.push_back(b);
    }

    auto parse1 = command.parseResponse(response1, context);
    ASSERT_TRUE(parse1.has_value());
    EXPECT_FALSE(command.isComplete());

    auto request2 = command.buildRequest(context);
    ASSERT_TRUE(request2.has_value());
    ASSERT_EQ(request2.value().commandCode, 0xAF);
    ASSERT_EQ(request2.value().data.size(), 16U);

    // Decode the host request as the PICC would for legacy send mode.
    uint8_t plainAB[16] = {0};
    legacySendModeDecodeToPlaintext(
        request2.value().data.data(),
        request2.value().data.size(),
        normalizedKey,
        plainAB);

    std::array<uint8_t, 8> rndA = {};
    std::memcpy(rndA.data(), plainAB, 8);

    const etl::vector<uint8_t, 16> rndBrot = rotateLeft8(rndB);
    for (size_t i = 0; i < 8; ++i)
    {
        EXPECT_EQ(plainAB[8 + i], rndBrot[i]);
    }

    etl::vector<uint8_t, 16> rndArot = rotateLeft8(rndA);
    uint8_t encRndArot[8] = {0};
    // Single-block response verification on PCD side is decrypt(cipher), so card sends E_K(RndA').
    DesFireCrypto::desEncrypt(rndArot.data(), normalizedKey, encRndArot);

    etl::vector<uint8_t, 9> response2;
    response2.push_back(0x00);
    for (uint8_t b : encRndArot)
    {
        response2.push_back(b);
    }

    auto parse2 = command.parseResponse(response2, context);
    ASSERT_TRUE(parse2.has_value());
    EXPECT_TRUE(command.isComplete());
    EXPECT_TRUE(context.authenticated);
    ASSERT_EQ(context.sessionKeyEnc.size(), 8U);
}

TEST(DesfireAuthenticateCommandTests, IsoTwoKey3DesDerives16ByteSessionKey)
{
    AuthenticateCommandOptions options;
    options.mode = DesfireAuthMode::ISO;
    options.keyNo = 0x00;
    options.key = toEtl24({
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    });

    AuthenticateCommand command(options);
    DesfireContext context;

    auto request1 = command.buildRequest(context);
    ASSERT_TRUE(request1.has_value());

    const std::array<uint8_t, 8> rndB = {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE};
    uint8_t encRndB[8] = {0};
    uint8_t iv0[8] = {0};
    DesFireCrypto::des3CbcEncrypt(rndB.data(), 8, options.key.data(), iv0, encRndB);

    etl::vector<uint8_t, 9> response1;
    response1.push_back(0xAF);
    for (uint8_t b : encRndB)
    {
        response1.push_back(b);
    }

    auto parse1 = command.parseResponse(response1, context);
    ASSERT_TRUE(parse1.has_value());

    auto request2 = command.buildRequest(context);
    ASSERT_TRUE(request2.has_value());
    ASSERT_EQ(request2.value().data.size(), 16U);

    uint8_t plainAB[16] = {0};
    DesFireCrypto::des3CbcDecrypt(
        request2.value().data.data(),
        16,
        options.key.data(),
        encRndB,
        plainAB);

    std::array<uint8_t, 8> rndA = {};
    std::memcpy(rndA.data(), plainAB, 8);

    const etl::vector<uint8_t, 16> rndBrot = rotateLeft8(rndB);
    for (size_t i = 0; i < 8; ++i)
    {
        EXPECT_EQ(plainAB[8 + i], rndBrot[i]);
    }

    etl::vector<uint8_t, 16> rndArot = rotateLeft8(rndA);
    uint8_t encRndArot[8] = {0};
    DesFireCrypto::des3CbcEncrypt(
        rndArot.data(),
        8,
        options.key.data(),
        request2.value().data.data() + 8,
        encRndArot);

    etl::vector<uint8_t, 9> response2;
    response2.push_back(0x00);
    for (uint8_t b : encRndArot)
    {
        response2.push_back(b);
    }

    auto parse2 = command.parseResponse(response2, context);
    ASSERT_TRUE(parse2.has_value());
    EXPECT_TRUE(command.isComplete());
    EXPECT_TRUE(context.authenticated);
    ASSERT_EQ(context.sessionKeyEnc.size(), 16U);

    std::array<uint8_t, 16> expectedSession = {
        rndA[0], rndA[1], rndA[2], rndA[3],
        rndB[0], rndB[1], rndB[2], rndB[3],
        rndA[4], rndA[5], rndA[6], rndA[7],
        rndB[4], rndB[5], rndB[6], rndB[7]
    };

    for (size_t i = 0; i < expectedSession.size(); ++i)
    {
        expectedSession[i] = static_cast<uint8_t>(expectedSession[i] & 0xFE);
        EXPECT_EQ(context.sessionKeyEnc[i], expectedSession[i]);
    }
}

TEST(DesfireAuthenticateCommandTests, IsoThreeKey3DesDerives24ByteSessionKey)
{
    AuthenticateCommandOptions options;
    options.mode = DesfireAuthMode::ISO;
    options.keyNo = 0x00;
    options.key = toEtl24({
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE
    });

    AuthenticateCommand command(options);
    DesfireContext context;

    auto request1 = command.buildRequest(context);
    ASSERT_TRUE(request1.has_value());
    EXPECT_EQ(request1.value().commandCode, static_cast<uint8_t>(DesfireAuthMode::ISO));
    EXPECT_EQ(request1.value().expectedResponseLength, 16U);

    const std::array<uint8_t, 16> rndB = {
        0x01, 0x23, 0x45, 0x67,
        0x89, 0xAB, 0xCD, 0xEF,
        0x10, 0x32, 0x54, 0x76,
        0x98, 0xBA, 0xDC, 0xFE
    };

    uint8_t encRndB[16] = {0};
    uint8_t iv0[8] = {0};
    des3kCbcEncrypt(rndB.data(), sizeof(encRndB), options.key.data(), iv0, encRndB);

    etl::vector<uint8_t, 17> response1;
    response1.push_back(0xAF);
    for (uint8_t b : encRndB)
    {
        response1.push_back(b);
    }

    auto parse1 = command.parseResponse(response1, context);
    ASSERT_TRUE(parse1.has_value());

    auto request2 = command.buildRequest(context);
    ASSERT_TRUE(request2.has_value());
    ASSERT_EQ(request2.value().commandCode, 0xAF);
    ASSERT_EQ(request2.value().data.size(), 32U);

    uint8_t plainAB[32] = {0};
    des3kCbcDecrypt(
        request2.value().data.data(),
        sizeof(plainAB),
        options.key.data(),
        encRndB + 8,
        plainAB);

    std::array<uint8_t, 16> rndA = {};
    std::memcpy(rndA.data(), plainAB, 16);

    const std::array<uint8_t, 16> rndBrot = rotateLeft16(rndB);
    for (size_t i = 0; i < 16; ++i)
    {
        EXPECT_EQ(plainAB[16 + i], rndBrot[i]);
    }

    const std::array<uint8_t, 16> rndArot = rotateLeft16(rndA);
    uint8_t encRndArot[16] = {0};
    des3kCbcEncrypt(
        rndArot.data(),
        sizeof(encRndArot),
        options.key.data(),
        request2.value().data.data() + 24,
        encRndArot);

    etl::vector<uint8_t, 17> response2;
    response2.push_back(0x00);
    for (uint8_t b : encRndArot)
    {
        response2.push_back(b);
    }

    auto parse2 = command.parseResponse(response2, context);
    ASSERT_TRUE(parse2.has_value());
    EXPECT_TRUE(command.isComplete());
    EXPECT_TRUE(context.authenticated);
    ASSERT_EQ(context.sessionKeyEnc.size(), 24U);

    std::array<uint8_t, 24> expectedSession = {
        rndA[0], rndA[1], rndA[2], rndA[3],
        rndB[0], rndB[1], rndB[2], rndB[3],
        rndA[6], rndA[7], rndA[8], rndA[9],
        rndB[6], rndB[7], rndB[8], rndB[9],
        rndA[12], rndA[13], rndA[14], rndA[15],
        rndB[12], rndB[13], rndB[14], rndB[15]
    };

    for (size_t i = 0; i < expectedSession.size(); ++i)
    {
        expectedSession[i] = static_cast<uint8_t>(expectedSession[i] & 0xFE);
        EXPECT_EQ(context.sessionKeyEnc[i], expectedSession[i]);
    }
}

TEST(DesfireAuthenticateCommandTests, AesAuthenticationDerivesAesSessionKey)
{
    AuthenticateCommandOptions options;
    options.mode = DesfireAuthMode::AES;
    options.keyNo = 0x00;
    options.key = toEtl24({
        0x00, 0x11, 0x22, 0x33,
        0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB,
        0xCC, 0xDD, 0xEE, 0xFF
    });

    AuthenticateCommand command(options);
    DesfireContext context;

    auto request1 = command.buildRequest(context);
    ASSERT_TRUE(request1.has_value());
    EXPECT_EQ(request1.value().commandCode, static_cast<uint8_t>(DesfireAuthMode::AES));

    const std::array<uint8_t, 16> rndB = {
        0x00, 0x01, 0x02, 0x03,
        0x10, 0x11, 0x12, 0x13,
        0x20, 0x21, 0x22, 0x23,
        0x30, 0x31, 0x32, 0x33
    };

    uint8_t encRndB[16];
    std::memcpy(encRndB, rndB.data(), sizeof(encRndB));
    uint8_t iv0[16] = {0};
    AES_ctx aesStart;
    AES_init_ctx_iv(&aesStart, options.key.data(), iv0);
    AES_CBC_encrypt_buffer(&aesStart, encRndB, sizeof(encRndB));

    etl::vector<uint8_t, 17> response1;
    response1.push_back(0xAF);
    for (uint8_t b : encRndB)
    {
        response1.push_back(b);
    }

    auto parse1 = command.parseResponse(response1, context);
    ASSERT_TRUE(parse1.has_value());
    EXPECT_FALSE(command.isComplete());

    auto request2 = command.buildRequest(context);
    ASSERT_TRUE(request2.has_value());
    ASSERT_EQ(request2.value().commandCode, 0xAF);
    ASSERT_EQ(request2.value().data.size(), 32U);

    uint8_t plainAB[32];
    std::memcpy(plainAB, request2.value().data.data(), sizeof(plainAB));
    AES_ctx aesReq;
    AES_init_ctx_iv(&aesReq, options.key.data(), encRndB);
    AES_CBC_decrypt_buffer(&aesReq, plainAB, sizeof(plainAB));

    std::array<uint8_t, 16> rndA = {};
    std::memcpy(rndA.data(), plainAB, 16);

    const std::array<uint8_t, 16> rndBrot = rotateLeft16(rndB);
    for (size_t i = 0; i < 16; ++i)
    {
        EXPECT_EQ(plainAB[16 + i], rndBrot[i]);
    }

    const std::array<uint8_t, 16> rndArot = rotateLeft16(rndA);
    uint8_t encRndArot[16];
    std::memcpy(encRndArot, rndArot.data(), sizeof(encRndArot));

    uint8_t ivCardResp[16];
    std::memcpy(ivCardResp, request2.value().data.data() + 16, sizeof(ivCardResp));

    AES_ctx aesResp;
    AES_init_ctx_iv(&aesResp, options.key.data(), ivCardResp);
    AES_CBC_encrypt_buffer(&aesResp, encRndArot, sizeof(encRndArot));

    etl::vector<uint8_t, 17> response2;
    response2.push_back(0x00);
    for (uint8_t b : encRndArot)
    {
        response2.push_back(b);
    }

    auto parse2 = command.parseResponse(response2, context);
    ASSERT_TRUE(parse2.has_value());
    EXPECT_TRUE(command.isComplete());
    EXPECT_TRUE(context.authenticated);

    ASSERT_EQ(context.sessionKeyEnc.size(), 16U);
    std::array<uint8_t, 16> expectedSession = {
        rndA[0], rndA[1], rndA[2], rndA[3],
        rndB[0], rndB[1], rndB[2], rndB[3],
        rndA[12], rndA[13], rndA[14], rndA[15],
        rndB[12], rndB[13], rndB[14], rndB[15]
    };

    for (size_t i = 0; i < expectedSession.size(); ++i)
    {
        EXPECT_EQ(context.sessionKeyEnc[i], expectedSession[i]);
    }
}
