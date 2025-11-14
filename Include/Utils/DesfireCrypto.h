/**
 * @file DesfireCrypto.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire cryptographic operations
 * @version 0.1
 * @date 2025-11-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/vector.h>
#include <cstdint>
#include <cstddef>

namespace crypto
{
    /**
     * @brief Rotate buffer left by n bytes
     * 
     * @param data Buffer to rotate
     * @param n Number of bytes to rotate
     */
    void rotateLeft(etl::ivector<uint8_t>& data, size_t n = 1);

    /**
     * @brief Rotate buffer right by n bytes
     * 
     * @param data Buffer to rotate
     * @param n Number of bytes to rotate
     */
    void rotateRight(etl::ivector<uint8_t>& data, size_t n = 1);

    /**
     * @brief Generate random bytes
     * 
     * @param output Output buffer
     * @param length Number of bytes to generate
     */
    void generateRandom(etl::ivector<uint8_t>& output, size_t length);

    /**
     * @brief Convert byte array to uint64_t (big-endian)
     * 
     * @param data Input data (8 bytes)
     * @return uint64_t Converted value
     */
    uint64_t bytesToUint64(const uint8_t* data);

    /**
     * @brief Convert uint64_t to byte array (big-endian)
     * 
     * @param value Input value
     * @param output Output buffer (8 bytes)
     */
    void uint64ToBytes(uint64_t value, uint8_t* output);

    /**
     * @brief DES/3DES encryption/decryption utilities
     */
    class DesFireCrypto
    {
    public:
        /**
         * @brief Encrypt single 8-byte block with DES
         * 
         * @param plaintext Input plaintext (8 bytes)
         * @param key DES key (8 bytes)
         * @param ciphertext Output ciphertext (8 bytes)
         */
        static void desEncrypt(
            const uint8_t* plaintext,
            const uint8_t* key,
            uint8_t* ciphertext);

        /**
         * @brief Decrypt single 8-byte block with DES
         * 
         * @param ciphertext Input ciphertext (8 bytes)
         * @param key DES key (8 bytes)
         * @param plaintext Output plaintext (8 bytes)
         */
        static void desDecrypt(
            const uint8_t* ciphertext,
            const uint8_t* key,
            uint8_t* plaintext);

        /**
         * @brief Encrypt single 8-byte block with 3DES (2-key)
         * 
         * @param plaintext Input plaintext (8 bytes)
         * @param key 3DES key (16 bytes: K1||K2)
         * @param ciphertext Output ciphertext (8 bytes)
         */
        static void des3Encrypt(
            const uint8_t* plaintext,
            const uint8_t* key,
            uint8_t* ciphertext);

        /**
         * @brief Decrypt single 8-byte block with 3DES (2-key)
         * 
         * @param ciphertext Input ciphertext (8 bytes)
         * @param key 3DES key (16 bytes: K1||K2)
         * @param plaintext Output plaintext (8 bytes)
         */
        static void des3Decrypt(
            const uint8_t* ciphertext,
            const uint8_t* key,
            uint8_t* plaintext);

        /**
         * @brief Encrypt data with 3DES in CBC mode
         * 
         * @param plaintext Input plaintext (multiple of 8 bytes)
         * @param plaintextLen Length of plaintext
         * @param key 3DES key (16 bytes: K1||K2)
         * @param iv Initialization vector (8 bytes)
         * @param ciphertext Output ciphertext (same length as plaintext)
         */
        static void des3CbcEncrypt(
            const uint8_t* plaintext,
            size_t plaintextLen,
            const uint8_t* key,
            const uint8_t* iv,
            uint8_t* ciphertext);

        /**
         * @brief Decrypt data with 3DES in CBC mode
         * 
         * @param ciphertext Input ciphertext (multiple of 8 bytes)
         * @param ciphertextLen Length of ciphertext
         * @param key 3DES key (16 bytes: K1||K2)
         * @param iv Initialization vector (8 bytes)
         * @param plaintext Output plaintext (same length as ciphertext)
         */
        static void des3CbcDecrypt(
            const uint8_t* ciphertext,
            size_t ciphertextLen,
            const uint8_t* key,
            const uint8_t* iv,
            uint8_t* plaintext);

        /**
         * @brief Generate DESFire session key from authentication data
         * 
         * For Legacy/ISO auth: generates 16-byte session key from RndA and RndB
         * Key format: RndA[0..3]||RndB[0..3]||RndA[4..7]||RndB[4..7]
         * Then clears LSB of each byte (parity bits)
         * 
         * @param rndA Reader's random (8 bytes)
         * @param rndB Card's random (8 bytes)
         * @param sessionKey Output session key (16 bytes)
         */
        static void generateSessionKey(
            const uint8_t* rndA,
            const uint8_t* rndB,
            uint8_t* sessionKey);

        /**
         * @brief Clear LSB (parity bits) from DES/3DES key
         * 
         * @param key Key buffer to modify
         * @param length Key length
         */
        static void clearParityBits(uint8_t* key, size_t length);
    };

} // namespace crypto

