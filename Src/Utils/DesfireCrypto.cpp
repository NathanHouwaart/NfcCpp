/**
 * @file DesfireCrypto.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire cryptographic operations implementation
 * @version 0.1
 * @date 2025-11-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Utils/DesfireCrypto.h"
#include <cppdes/des.h>
#include <cppdes/des3.h>
#include <cppdes/des3cbc.h>
#include <cstring>
#include <random>

namespace crypto
{
    void rotateLeft(etl::ivector<uint8_t>& data, size_t n)
    {
        if (data.empty() || n == 0)
            return;

        n = n % data.size(); // Handle n > size
        
        // Save first n bytes
        etl::vector<uint8_t, 32> temp;
        for (size_t i = 0; i < n; ++i)
        {
            temp.push_back(data[i]);
        }

        // Shift remaining bytes left
        for (size_t i = 0; i < data.size() - n; ++i)
        {
            data[i] = data[i + n];
        }

        // Put saved bytes at end
        for (size_t i = 0; i < n; ++i)
        {
            data[data.size() - n + i] = temp[i];
        }
    }

    void rotateRight(etl::ivector<uint8_t>& data, size_t n)
    {
        if (data.empty() || n == 0)
            return;

        n = n % data.size(); // Handle n > size
        
        // Save last n bytes
        etl::vector<uint8_t, 32> temp;
        for (size_t i = data.size() - n; i < data.size(); ++i)
        {
            temp.push_back(data[i]);
        }

        // Shift remaining bytes right
        for (size_t i = data.size() - 1; i >= n; --i)
        {
            data[i] = data[i - n];
        }

        // Put saved bytes at start
        for (size_t i = 0; i < n; ++i)
        {
            data[i] = temp[i];
        }
    }

    void generateRandom(etl::ivector<uint8_t>& output, size_t length)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint32_t> dis(0, 255);

        output.clear();
        for (size_t i = 0; i < length; ++i)
        {
            output.push_back(static_cast<uint8_t>(dis(gen)));
        }
    }

    uint64_t bytesToUint64(const uint8_t* data)
    {
        uint64_t result = 0;
        for (int i = 0; i < 8; ++i)
        {
            result = (result << 8) | data[i];
        }
        return result;
    }

    void uint64ToBytes(uint64_t value, uint8_t* output)
    {
        for (int i = 7; i >= 0; --i)
        {
            output[i] = value & 0xFF;
            value >>= 8;
        }
    }

    void DesFireCrypto::desEncrypt(
        const uint8_t* plaintext,
        const uint8_t* key,
        uint8_t* ciphertext)
    {
        uint64_t plainBlock = bytesToUint64(plaintext);
        uint64_t keyValue = bytesToUint64(key);
        
        DES des(keyValue);
        uint64_t cipherBlock = des.encrypt(plainBlock);
        
        uint64ToBytes(cipherBlock, ciphertext);
    }

    void DesFireCrypto::desDecrypt(
        const uint8_t* ciphertext,
        const uint8_t* key,
        uint8_t* plaintext)
    {
        uint64_t cipherBlock = bytesToUint64(ciphertext);
        uint64_t keyValue = bytesToUint64(key);
        
        DES des(keyValue);
        uint64_t plainBlock = des.decrypt(cipherBlock);
        
        uint64ToBytes(plainBlock, plaintext);
    }

    void DesFireCrypto::des3Encrypt(
        const uint8_t* plaintext,
        const uint8_t* key,
        uint8_t* ciphertext)
    {
        uint64_t plainBlock = bytesToUint64(plaintext);
        uint64_t key1 = bytesToUint64(key);
        uint64_t key2 = bytesToUint64(key + 8);
        
        // For 2-key 3DES: K3 = K1
        DES3 des3(key1, key2, key1);
        uint64_t cipherBlock = des3.encrypt(plainBlock);
        
        uint64ToBytes(cipherBlock, ciphertext);
    }

    void DesFireCrypto::des3Decrypt(
        const uint8_t* ciphertext,
        const uint8_t* key,
        uint8_t* plaintext)
    {
        uint64_t cipherBlock = bytesToUint64(ciphertext);
        uint64_t key1 = bytesToUint64(key);
        uint64_t key2 = bytesToUint64(key + 8);
        
        // For 2-key 3DES: K3 = K1
        DES3 des3(key1, key2, key1);
        uint64_t plainBlock = des3.decrypt(cipherBlock);
        
        uint64ToBytes(plainBlock, plaintext);
    }

    void DesFireCrypto::des3CbcEncrypt(
        const uint8_t* plaintext,
        size_t plaintextLen,
        const uint8_t* key,
        const uint8_t* iv,
        uint8_t* ciphertext)
    {
        uint64_t key1 = bytesToUint64(key);
        uint64_t key2 = bytesToUint64(key + 8);
        uint64_t ivValue = bytesToUint64(iv);
        
        // For 2-key 3DES: K3 = K1
        DES3CBC des3cbc(key1, key2, key1, ivValue);
        
        // Process each 8-byte block
        for (size_t i = 0; i < plaintextLen; i += 8)
        {
            uint64_t plainBlock = bytesToUint64(plaintext + i);
            uint64_t cipherBlock = des3cbc.encrypt(plainBlock);
            uint64ToBytes(cipherBlock, ciphertext + i);
        }
    }

    void DesFireCrypto::des3CbcDecrypt(
        const uint8_t* ciphertext,
        size_t ciphertextLen,
        const uint8_t* key,
        const uint8_t* iv,
        uint8_t* plaintext)
    {
        uint64_t key1 = bytesToUint64(key);
        uint64_t key2 = bytesToUint64(key + 8);
        uint64_t ivValue = bytesToUint64(iv);
        
        // For 2-key 3DES: K3 = K1
        DES3CBC des3cbc(key1, key2, key1, ivValue);
        
        // Process each 8-byte block
        for (size_t i = 0; i < ciphertextLen; i += 8)
        {
            uint64_t cipherBlock = bytesToUint64(ciphertext + i);
            uint64_t plainBlock = des3cbc.decrypt(cipherBlock);
            uint64ToBytes(plainBlock, plaintext + i);
        }
    }

    void DesFireCrypto::generateSessionKey(
        const uint8_t* rndA,
        const uint8_t* rndB,
        uint8_t* sessionKey)
    {
        // Session key format: RndA[0..3]||RndB[0..3]||RndA[4..7]||RndB[4..7]
        for (int i = 0; i < 4; ++i)
        {
            sessionKey[i] = rndA[i];
            sessionKey[i + 4] = rndB[i];
            sessionKey[i + 8] = rndA[i + 4];
            sessionKey[i + 12] = rndB[i + 4];
        }
        
        // Clear parity bits (LSB of each byte)
        clearParityBits(sessionKey, 16);
    }

    void DesFireCrypto::clearParityBits(uint8_t* key, size_t length)
    {
        for (size_t i = 0; i < length; ++i)
        {
            key[i] &= 0xFE; // Clear LSB
        }
    }

} // namespace crypto

