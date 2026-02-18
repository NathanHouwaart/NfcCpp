// Standalone ChangeKey command calculator with explicit state machine.
// Build (MinGW on Windows):
//   g++ -std=c++17 tools/changekey_command.cpp -o tools/changekey_command.exe -lbcrypt
// Run:
//   tools/changekey_command.exe
//   tools/changekey_command.exe --verbose

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cwchar>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifndef BCRYPT_SUCCESS
#define BCRYPT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

namespace desfire {

enum class AuthScheme { LegacyCrc16, IsoCrc32 };
enum class KeyType { DES, K2_3DES, K3_3DES, AES };
enum class SessionCipher { DES, K2_3DES, K3_3DES, AES };
enum class LegacyDesCryptMode { CbcEncrypt, SendModeDecrypt };

enum class ChangeKeyState {
    Idle,
    ValidatingInput,
    NormalizingNewKey,
    DeterminingKeyRelationship,
    ApplyingDifferentKeyXor,
    BuildingKeyStream,
    ComputingCryptoCrc,
    ComputingNewKeyCrc,
    AssemblingCryptogram,
    PaddingCryptogram,
    EncryptingCryptogram,
    Completed,
    Failed,
};

static std::string state_to_string(ChangeKeyState s) {
    switch (s) {
        case ChangeKeyState::Idle: return "Idle";
        case ChangeKeyState::ValidatingInput: return "ValidatingInput";
        case ChangeKeyState::NormalizingNewKey: return "NormalizingNewKey";
        case ChangeKeyState::DeterminingKeyRelationship: return "DeterminingKeyRelationship";
        case ChangeKeyState::ApplyingDifferentKeyXor: return "ApplyingDifferentKeyXor";
        case ChangeKeyState::BuildingKeyStream: return "BuildingKeyStream";
        case ChangeKeyState::ComputingCryptoCrc: return "ComputingCryptoCrc";
        case ChangeKeyState::ComputingNewKeyCrc: return "ComputingNewKeyCrc";
        case ChangeKeyState::AssemblingCryptogram: return "AssemblingCryptogram";
        case ChangeKeyState::PaddingCryptogram: return "PaddingCryptogram";
        case ChangeKeyState::EncryptingCryptogram: return "EncryptingCryptogram";
        case ChangeKeyState::Completed: return "Completed";
        case ChangeKeyState::Failed: return "Failed";
    }
    return "Unknown";
}

struct ChangeKeyInput {
    AuthScheme auth_scheme;
    SessionCipher session_cipher;
    std::vector<uint8_t> session_key;
    uint8_t key_no;
    uint8_t authenticated_key_no;
    KeyType new_key_type;
    std::vector<uint8_t> new_key;
    std::optional<std::vector<uint8_t>> current_key;
    uint8_t aes_key_version = 0x00;
    uint8_t command_code = 0xC4;
    std::optional<std::vector<uint8_t>> iv;
    LegacyDesCryptMode legacy_mode = LegacyDesCryptMode::CbcEncrypt;
};

struct ChangeKeyResult {
    std::vector<uint8_t> key_data_for_crypto;
    std::vector<uint8_t> key_stream_with_aes_version;
    uint32_t crc_crypto = 0;
    std::optional<uint32_t> crc_new_key;
    std::vector<uint8_t> cryptogram_unpadded;
    std::vector<uint8_t> cryptogram_padded;
    std::vector<uint8_t> cryptogram_encrypted;
    std::vector<std::string> logs;
};

static std::string nt_hex(NTSTATUS s) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << static_cast<uint32_t>(s);
    return oss.str();
}

static std::vector<uint8_t> hex_to_bytes(std::string_view text) {
    std::string filtered;
    filtered.reserve(text.size());
    for (char c : text) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            filtered.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
    }
    if (filtered.size() % 2 != 0) throw std::runtime_error("Invalid hex length");

    std::vector<uint8_t> out;
    out.reserve(filtered.size() / 2);
    for (size_t i = 0; i < filtered.size(); i += 2) {
        out.push_back(static_cast<uint8_t>(std::stoul(filtered.substr(i, 2), nullptr, 16)));
    }
    return out;
}

static std::string bytes_to_hex(const std::vector<uint8_t>& bytes, bool spaces = true) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (spaces && i > 0) oss << ' ';
        oss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return oss.str();
}

static std::vector<uint8_t> u16le(uint16_t v) {
    return { static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>((v >> 8) & 0xFF) };
}

static std::vector<uint8_t> u32le(uint32_t v) {
    return {
        static_cast<uint8_t>(v & 0xFF),
        static_cast<uint8_t>((v >> 8) & 0xFF),
        static_cast<uint8_t>((v >> 16) & 0xFF),
        static_cast<uint8_t>((v >> 24) & 0xFF),
    };
}

static uint16_t crc16_desfire(const std::vector<uint8_t>& data) {
    uint16_t crc = 0x6363;
    for (uint8_t byte : data) {
        uint8_t ch = static_cast<uint8_t>(byte ^ static_cast<uint8_t>(crc & 0x00FF));
        ch = static_cast<uint8_t>(ch ^ static_cast<uint8_t>(ch << 4));
        crc = static_cast<uint16_t>(
            (crc >> 8) ^
            (static_cast<uint16_t>(ch) << 8) ^
            (static_cast<uint16_t>(ch) << 3) ^
            (static_cast<uint16_t>(ch) >> 4));
    }
    return crc;
}

static uint32_t crc32_standard(const std::vector<uint8_t>& data) {
    uint32_t crc = 0xFFFFFFFFU;
    for (uint8_t b : data) {
        crc ^= b;
        for (int i = 0; i < 8; ++i) {
            bool bit = (crc & 1U) != 0U;
            crc >>= 1;
            if (bit) crc ^= 0xEDB88320U;
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

static uint32_t crc32_desfire(const std::vector<uint8_t>& data) {
    return ~crc32_standard(data);
}

static std::vector<uint8_t> zero_pad(const std::vector<uint8_t>& in, size_t block_size) {
    std::vector<uint8_t> out = in;
    size_t pad = (block_size - (out.size() % block_size)) % block_size;
    out.insert(out.end(), pad, 0x00);
    return out;
}

static std::vector<uint8_t> xor_bytes(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    if (a.size() != b.size()) throw std::runtime_error("XOR size mismatch");
    std::vector<uint8_t> out(a.size());
    for (size_t i = 0; i < a.size(); ++i) out[i] = static_cast<uint8_t>(a[i] ^ b[i]);
    return out;
}

static std::vector<uint8_t> normalize_key_material(const std::vector<uint8_t>& key, KeyType t) {
    switch (t) {
        case KeyType::DES:
            if (key.size() == 8) {
                std::vector<uint8_t> out = key;
                out.insert(out.end(), key.begin(), key.end());
                return out;
            }
            if (key.size() == 16) return key;
            throw std::runtime_error("DES key must be 8 or 16 bytes");
        case KeyType::K2_3DES:
            if (key.size() != 16) throw std::runtime_error("2K3DES key must be 16 bytes");
            return key;
        case KeyType::K3_3DES:
            if (key.size() != 24) throw std::runtime_error("3K3DES key must be 24 bytes");
            return key;
        case KeyType::AES:
            if (key.size() != 16) throw std::runtime_error("AES key must be 16 bytes");
            return key;
    }
    throw std::runtime_error("Unsupported key type");
}

static std::vector<uint8_t> apply_des_key_version_bits(const std::vector<uint8_t>& key, uint8_t version) {
    if (key.size() != 8 && key.size() != 16 && key.size() != 24) {
        throw std::runtime_error("DES/3DES key must be 8,16,24 bytes");
    }
    std::vector<uint8_t> out = key;
    uint8_t mask = 0x80;
    for (size_t i = 0; i < 8; ++i) {
        uint8_t parity = (version & mask) ? 0x01 : 0x00;
        out[i] = static_cast<uint8_t>((out[i] & 0xFE) | parity);
        mask >>= 1;
    }
    for (size_t i = 8; i < out.size(); ++i) out[i] = static_cast<uint8_t>(out[i] & 0xFE);
    return out;
}

static std::vector<uint8_t> expand_2k3des_to_24(const std::vector<uint8_t>& key16) {
    if (key16.size() != 16) throw std::runtime_error("2K3DES expansion expects 16-byte key");
    std::vector<uint8_t> key24 = key16;
    key24.insert(key24.end(), key16.begin(), key16.begin() + 8);
    return key24;
}

static std::vector<uint8_t> bcrypt_crypt(
    const wchar_t* algorithm,
    const wchar_t* chaining_mode,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& input,
    bool encrypt
) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_KEY_HANDLE key_handle = nullptr;
    std::vector<uint8_t> key_object;
    std::vector<uint8_t> out;
    std::vector<uint8_t> iv_copy = iv;

    auto cleanup = [&]() {
        if (key_handle) BCryptDestroyKey(key_handle);
        if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    };

    NTSTATUS status = BCryptOpenAlgorithmProvider(&alg, algorithm, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) {
        cleanup();
        throw std::runtime_error("BCryptOpenAlgorithmProvider failed: " + nt_hex(status));
    }

    status = BCryptSetProperty(
        alg,
        BCRYPT_CHAINING_MODE,
        reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(chaining_mode)),
        static_cast<ULONG>((wcslen(chaining_mode) + 1) * sizeof(wchar_t)),
        0);
    if (!BCRYPT_SUCCESS(status)) {
        cleanup();
        throw std::runtime_error("BCryptSetProperty failed: " + nt_hex(status));
    }

    ULONG object_len = 0;
    ULONG out_len = 0;
    status = BCryptGetProperty(
        alg,
        BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&object_len),
        sizeof(object_len),
        &out_len,
        0);
    if (!BCRYPT_SUCCESS(status)) {
        cleanup();
        throw std::runtime_error("BCryptGetProperty failed: " + nt_hex(status));
    }

    key_object.assign(object_len, 0x00);
    status = BCryptGenerateSymmetricKey(
        alg,
        &key_handle,
        key_object.data(),
        static_cast<ULONG>(key_object.size()),
        const_cast<PUCHAR>(key.data()),
        static_cast<ULONG>(key.size()),
        0);
    if (!BCRYPT_SUCCESS(status)) {
        cleanup();
        throw std::runtime_error("BCryptGenerateSymmetricKey failed: " + nt_hex(status));
    }

    PUCHAR iv_ptr = iv_copy.empty() ? nullptr : iv_copy.data();
    ULONG iv_len = static_cast<ULONG>(iv_copy.size());
    status = encrypt
        ? BCryptEncrypt(
            key_handle,
            const_cast<PUCHAR>(input.data()),
            static_cast<ULONG>(input.size()),
            nullptr,
            iv_ptr,
            iv_len,
            nullptr,
            0,
            &out_len,
            0)
        : BCryptDecrypt(
            key_handle,
            const_cast<PUCHAR>(input.data()),
            static_cast<ULONG>(input.size()),
            nullptr,
            iv_ptr,
            iv_len,
            nullptr,
            0,
            &out_len,
            0);
    if (!BCRYPT_SUCCESS(status)) {
        cleanup();
        throw std::runtime_error("BCrypt size query failed: " + nt_hex(status));
    }

    out.assign(out_len, 0x00);
    iv_ptr = iv_copy.empty() ? nullptr : iv_copy.data();
    status = encrypt
        ? BCryptEncrypt(
            key_handle,
            const_cast<PUCHAR>(input.data()),
            static_cast<ULONG>(input.size()),
            nullptr,
            iv_ptr,
            iv_len,
            out.data(),
            static_cast<ULONG>(out.size()),
            &out_len,
            0)
        : BCryptDecrypt(
            key_handle,
            const_cast<PUCHAR>(input.data()),
            static_cast<ULONG>(input.size()),
            nullptr,
            iv_ptr,
            iv_len,
            out.data(),
            static_cast<ULONG>(out.size()),
            &out_len,
            0);
    if (!BCRYPT_SUCCESS(status)) {
        cleanup();
        throw std::runtime_error("BCrypt encrypt/decrypt failed: " + nt_hex(status));
    }

    out.resize(out_len);
    cleanup();
    return out;
}

static std::vector<uint8_t> encrypt_cbc(
    SessionCipher cipher,
    const std::vector<uint8_t>& session_key,
    const std::vector<uint8_t>& plaintext_padded,
    const std::optional<std::vector<uint8_t>>& iv_opt
) {
    if (cipher == SessionCipher::AES) {
        if (session_key.size() != 16) throw std::runtime_error("AES session key must be 16 bytes");
        std::vector<uint8_t> iv = iv_opt.value_or(std::vector<uint8_t>(16, 0x00));
        if (iv.size() != 16) throw std::runtime_error("AES IV must be 16 bytes");
        return bcrypt_crypt(BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC, session_key, iv, plaintext_padded, true);
    }

    std::vector<uint8_t> iv = iv_opt.value_or(std::vector<uint8_t>(8, 0x00));
    if (iv.size() != 8) throw std::runtime_error("DES/3DES IV must be 8 bytes");

    if (cipher == SessionCipher::DES) {
        if (session_key.size() != 8 && session_key.size() != 16) throw std::runtime_error("DES session key must be 8 or 16 bytes");
        std::vector<uint8_t> key8(session_key.begin(), session_key.begin() + 8);
        return bcrypt_crypt(BCRYPT_DES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC, key8, iv, plaintext_padded, true);
    }
    if (cipher == SessionCipher::K2_3DES) {
        if (session_key.size() != 16) throw std::runtime_error("2K3DES session key must be 16 bytes");
        std::vector<uint8_t> key24 = expand_2k3des_to_24(session_key);
        return bcrypt_crypt(BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC, key24, iv, plaintext_padded, true);
    }
    if (cipher == SessionCipher::K3_3DES) {
        if (session_key.size() != 24) throw std::runtime_error("3K3DES session key must be 24 bytes");
        return bcrypt_crypt(BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC, session_key, iv, plaintext_padded, true);
    }
    throw std::runtime_error("Unsupported session cipher");
}

static std::vector<uint8_t> decrypt_ecb_des(const std::vector<uint8_t>& key8, const std::vector<uint8_t>& block8) {
    return bcrypt_crypt(BCRYPT_DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB, key8, {}, block8, false);
}

static std::vector<uint8_t> decrypt_ecb_3des(const std::vector<uint8_t>& key24, const std::vector<uint8_t>& block8) {
    return bcrypt_crypt(BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB, key24, {}, block8, false);
}

static std::vector<uint8_t> encrypt_legacy_send_mode_des_tdes(
    SessionCipher cipher,
    const std::vector<uint8_t>& session_key,
    const std::vector<uint8_t>& plaintext_padded,
    const std::optional<std::vector<uint8_t>>& iv_opt
) {
    if (cipher != SessionCipher::DES && cipher != SessionCipher::K2_3DES) {
        throw std::runtime_error("SEND_MODE only valid for DES/2K3DES sessions");
    }
    if (plaintext_padded.size() % 8 != 0) throw std::runtime_error("SEND_MODE data must be multiple of 8");
    if (iv_opt.has_value()) {
        const auto& iv = *iv_opt;
        if (iv.size() != 8) throw std::runtime_error("SEND_MODE IV must be 8 bytes");
        if (std::any_of(iv.begin(), iv.end(), [](uint8_t b) { return b != 0; })) {
            throw std::runtime_error("SEND_MODE expects all-zero IV");
        }
    }

    std::vector<uint8_t> prev(8, 0x00);
    std::vector<uint8_t> out;
    out.reserve(plaintext_padded.size());
    for (size_t i = 0; i < plaintext_padded.size(); i += 8) {
        std::vector<uint8_t> blk(plaintext_padded.begin() + static_cast<long long>(i),
                                 plaintext_padded.begin() + static_cast<long long>(i + 8));
        std::vector<uint8_t> x = xor_bytes(blk, prev);
        std::vector<uint8_t> d;
        if (cipher == SessionCipher::DES) {
            std::vector<uint8_t> key8(session_key.begin(), session_key.begin() + 8);
            d = decrypt_ecb_des(key8, x);
        } else {
            std::vector<uint8_t> key24 = expand_2k3des_to_24(session_key);
            d = decrypt_ecb_3des(key24, x);
        }
        out.insert(out.end(), d.begin(), d.end());
        prev = d;
    }
    return out;
}

class ChangeKeyCommand {
public:
    explicit ChangeKeyCommand(bool echo_logs = false) : echo_logs_(echo_logs) {}

    ChangeKeyResult run(const ChangeKeyInput& in) {
        logs_.clear();
        state_ = ChangeKeyState::Idle;

        ChangeKeyResult r;
        try {
            transition(ChangeKeyState::ValidatingInput, "Start ChangeKey flow");
            transition(ChangeKeyState::NormalizingNewKey, "Normalize new key material");
            std::vector<uint8_t> new_key_mat = normalize_key_material(in.new_key, in.new_key_type);
            log_hex("new_key_material", new_key_mat);

            transition(ChangeKeyState::DeterminingKeyRelationship, "Determine same-key vs different-key");
            bool same = ((in.key_no & 0x0F) == (in.authenticated_key_no & 0x0F));
            log(std::string("changing_same_key = ") + (same ? "true" : "false"));

            std::vector<uint8_t> key_data_for_crypto;
            if (same) {
                key_data_for_crypto = new_key_mat;
            } else {
                transition(ChangeKeyState::ApplyingDifferentKeyXor, "XOR with current key material");
                if (!in.current_key.has_value()) throw std::runtime_error("current_key required for different-key update");
                std::vector<uint8_t> cur = normalize_key_material(*in.current_key, in.new_key_type);
                log_hex("current_key_material", cur);
                key_data_for_crypto = xor_bytes(new_key_mat, cur);
            }
            r.key_data_for_crypto = key_data_for_crypto;
            log_hex("key_data_for_crypto", key_data_for_crypto);

            transition(ChangeKeyState::BuildingKeyStream, "Append AES key version when needed");
            std::vector<uint8_t> key_stream = key_data_for_crypto;
            if (in.new_key_type == KeyType::AES) key_stream.push_back(in.aes_key_version);
            r.key_stream_with_aes_version = key_stream;
            log_hex("key_stream_with_aes_version", key_stream);

            transition(ChangeKeyState::ComputingCryptoCrc, "Compute CRC Crypto");
            std::vector<uint8_t> crc_crypto_bytes;
            if (in.auth_scheme == AuthScheme::LegacyCrc16) {
                uint16_t c = crc16_desfire(key_stream);
                r.crc_crypto = c;
                crc_crypto_bytes = u16le(c);
            } else {
                std::vector<uint8_t> crc_in = {in.command_code, in.key_no};
                crc_in.insert(crc_in.end(), key_stream.begin(), key_stream.end());
                uint32_t c = crc32_desfire(crc_in);
                r.crc_crypto = c;
                crc_crypto_bytes = u32le(c);
                log_hex("crc_crypto_input", crc_in);
            }
            log_hex("crc_crypto_bytes", crc_crypto_bytes);

            std::vector<uint8_t> crc_new_key_bytes;
            if (!same) {
                transition(ChangeKeyState::ComputingNewKeyCrc, "Compute CRC New Key");
                if (in.auth_scheme == AuthScheme::LegacyCrc16) {
                    uint16_t c = crc16_desfire(new_key_mat);
                    r.crc_new_key = c;
                    crc_new_key_bytes = u16le(c);
                } else {
                    uint32_t c = crc32_desfire(new_key_mat);
                    r.crc_new_key = c;
                    crc_new_key_bytes = u32le(c);
                }
                log_hex("crc_new_key_bytes", crc_new_key_bytes);
            }

            transition(ChangeKeyState::AssemblingCryptogram, "Build plaintext cryptogram");
            std::vector<uint8_t> plain = key_stream;
            plain.insert(plain.end(), crc_crypto_bytes.begin(), crc_crypto_bytes.end());
            plain.insert(plain.end(), crc_new_key_bytes.begin(), crc_new_key_bytes.end());
            r.cryptogram_unpadded = plain;
            log_hex("cryptogram_unpadded", plain);

            transition(ChangeKeyState::PaddingCryptogram, "Pad to block boundary");
            size_t bs = in.session_cipher == SessionCipher::AES ? 16 : 8;
            r.cryptogram_padded = zero_pad(plain, bs);
            log_hex("cryptogram_padded", r.cryptogram_padded);

            transition(ChangeKeyState::EncryptingCryptogram, "Encrypt cryptogram");
            if (in.legacy_mode == LegacyDesCryptMode::SendModeDecrypt) {
                r.cryptogram_encrypted = encrypt_legacy_send_mode_des_tdes(
                    in.session_cipher, in.session_key, r.cryptogram_padded, in.iv);
            } else {
                r.cryptogram_encrypted = encrypt_cbc(
                    in.session_cipher, in.session_key, r.cryptogram_padded, in.iv);
            }
            log_hex("cryptogram_encrypted", r.cryptogram_encrypted);

            transition(ChangeKeyState::Completed, "Done");
            r.logs = logs_;
            return r;
        } catch (const std::exception& ex) {
            transition(ChangeKeyState::Failed, ex.what());
            r.logs = logs_;
            throw;
        }
    }

private:
    void transition(ChangeKeyState next, const std::string& reason) {
        std::string line = "[State] " + state_to_string(state_) + " -> " + state_to_string(next) + " | " + reason;
        logs_.push_back(line);
        if (echo_logs_) std::cout << line << "\n";
        state_ = next;
    }
    void log(const std::string& msg) {
        std::string line = "  [Log] " + msg;
        logs_.push_back(line);
        if (echo_logs_) std::cout << line << "\n";
    }
    void log_hex(const std::string& label, const std::vector<uint8_t>& bytes) { log(label + " = " + bytes_to_hex(bytes)); }

    ChangeKeyState state_ = ChangeKeyState::Idle;
    std::vector<std::string> logs_;
    bool echo_logs_ = false;
};

struct ExampleCase {
    std::string name;
    ChangeKeyInput in;
    std::optional<uint32_t> expect_crc_crypto;
    std::optional<uint32_t> expect_crc_new;
    std::string expect_plain_hex;
    std::string expect_enc_hex;
};

static void expect_hex_eq(const std::string& label, const std::vector<uint8_t>& actual, const std::string& expected_hex) {
    std::string actual_hex = bytes_to_hex(actual);
    std::string expected = bytes_to_hex(hex_to_bytes(expected_hex));
    if (actual_hex != expected) {
        throw std::runtime_error(label + " mismatch\nExpected: " + expected + "\nActual:   " + actual_hex);
    }
}

static std::vector<ExampleCase> build_examples() {
    using AS = AuthScheme;
    using SS = SessionCipher;
    using KT = KeyType;
    using LM = LegacyDesCryptMode;

    std::vector<ExampleCase> v;

    v.push_back({"Vector 1: 2K3DES same-key (web)",
        {AS::IsoCrc32, SS::DES, hex_to_bytes("C8 6C E2 5E 4C 64 7E 56 C8 6C E2 5E 4C 64 7E 56"), 0x00, 0x00, KT::K2_3DES, hex_to_bytes("00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80"), std::nullopt, 0x00, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00"), LM::CbcEncrypt},
        0x5001FFC5, std::nullopt,
        "00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80 C5 FF 01 50 00 00 00 00",
        "BE DE 0F C6 ED 34 7D CF 0D 51 C7 17 DF 75 D9 7D 2C 5A 2B A6 CA C7 47 9D"});

    v.push_back({"Vector 2: 2K3DES different-key (web)",
        {AS::IsoCrc32, SS::DES, hex_to_bytes("CA A6 74 E8 CA E8 52 5E CA A6 74 E8 CA E8 52 5E"), 0x01, 0x00, KT::K2_3DES, hex_to_bytes("00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80"), hex_to_bytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"), 0x00, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00"), LM::CbcEncrypt},
        0xD7A73486, 0xC4EF3A3A,
        "00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80 86 34 A7 D7 3A 3A EF C4",
        "4E B6 69 E4 8D CA 58 47 49 54 2E 1B E8 9C B4 C7 84 5A 38 C5 7D 19 DE 59"});

    v.push_back({"Vector 3: AES same-key with non-zero IV (web)",
        {AS::IsoCrc32, SS::AES, hex_to_bytes("90 F7 A2 01 91 03 68 45 EC 63 DE CD 54 4B 99 31"), 0x00, 0x00, KT::AES, hex_to_bytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"), std::nullopt, 0x00, 0xC4, hex_to_bytes("8A 8F A3 6F 55 CD 21 0D D8 05 46 58 AC 70 D9 9A"), LM::CbcEncrypt},
        0x1B860F0A, std::nullopt,
        "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 0A 0F 86 1B 00 00 00 00 00 00 00 00 00 00 00",
        "63 53 75 E4 91 9F 8A F2 E9 E8 6B 1C 1B A5 5B 0C 08 07 EA F4 84 D7 A7 EF 6E 0C 30 84 16 0F 5A 61"});

    v.push_back({"Vector 4: AES different-key (web)",
        {AS::IsoCrc32, SS::AES, hex_to_bytes("C2 A1 E4 7B D8 10 00 44 FE 6D 00 A7 4D 7A B1 7C"), 0x01, 0x00, KT::AES, hex_to_bytes("00 10 20 30 40 50 60 70 80 90 A0 B0 B0 A0 90 80"), hex_to_bytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"), 0x10, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"), LM::CbcEncrypt},
        0x84B47033, 0x1979E3BF,
        "00 10 20 30 40 50 60 70 80 90 A0 B0 B0 A0 90 80 10 33 70 B4 84 BF E3 79 19 00 00 00 00 00 00 00",
        "E7 EC CB 6B D1 CA 64 BC 16 1A 12 B1 C0 24 F7 14 30 33 74 08 C8 A8 7E AC AB 7A 1F F1 89 51 FC A3"});

    v.push_back({"Vector 6: Android DES->DES (legacy SEND_MODE)",
        {AS::LegacyCrc16, SS::DES, hex_to_bytes("92 f1 35 8c ea e9 6a 10"), 0x00, 0x00, KT::DES, hex_to_bytes("00 00 00 00 00 00 00 00"), std::nullopt, 0x00, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00"), LM::SendModeDecrypt},
        std::nullopt, std::nullopt,
        "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 37 49 00 00 00 00 00 00",
        "EA 70 40 19 C3 EF 41 9F D6 3A E2 94 B4 01 4C 03 C6 F3 2A EC DD 56 19 D6"});

    v.push_back({"Vector 7: Android DES->2K3DES (legacy SEND_MODE)",
        {AS::LegacyCrc16, SS::DES, hex_to_bytes("41 2e 7a 0c fb a2 18 a4"), 0x00, 0x00, KT::K2_3DES, hex_to_bytes("00 00 00 00 00 00 00 00 02 02 02 02 02 02 02 02"), std::nullopt, 0x00, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00"), LM::SendModeDecrypt},
        std::nullopt, std::nullopt,
        "00 00 00 00 00 00 00 00 02 02 02 02 02 02 02 02 51 F7 00 00 00 00 00 00",
        "ED E9 D7 31 50 07 18 20 B2 DD DA 92 64 67 B8 B9 D8 A8 B9 78 7F 3F F5 BA"});

    v.push_back({"Vector 8: Android 2K3DES->DES (legacy SEND_MODE)",
        {AS::LegacyCrc16, SS::K2_3DES, hex_to_bytes("af 76 04 ee 62 d6 8a 14 20 83 f9 8d 46 dd 4a 86"), 0x00, 0x00, KT::DES, hex_to_bytes("00 00 00 00 00 00 00 00"), std::nullopt, 0x00, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00"), LM::SendModeDecrypt},
        std::nullopt, std::nullopt,
        "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 37 49 00 00 00 00 00 00",
        "79 5C 09 70 E7 F8 F6 28 83 51 02 45 4C 8B BA AA 30 25 32 0F 72 14 E4 38"});

    v.push_back({"Vector 9: Android DES->AES (legacy SEND_MODE, keyNo=0x80)",
        {AS::LegacyCrc16, SS::DES, hex_to_bytes("2b 12 bd 7c 1d 3f e9 f7"), 0x80, 0x00, KT::AES, hex_to_bytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"), std::nullopt, 0x00, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00"), LM::SendModeDecrypt},
        std::nullopt, std::nullopt,
        "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 75 45 00 00 00 00 00",
        "64 63 EA 36 5B 3D 33 4B DD 11 AF 0D 1A CC D6 98 A5 56 39 6E 58 EC B8 AE"});

    v.push_back({"Vector 10: Android AES->DES (ISO CRC32)",
        {AS::IsoCrc32, SS::AES, hex_to_bytes("f6 2a 18 d5 03 56 1e 42 c0 7c 13 13 c8 91 50 f1"), 0x00, 0x00, KT::DES, hex_to_bytes("00 00 00 00 00 00 00 00"), std::nullopt, 0x00, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"), LM::CbcEncrypt},
        std::nullopt, std::nullopt,
        "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 55 71 AA 87 00 00 00 00 00 00 00 00 00 00 00 00",
        "03 30 DC 9B A1 A3 07 56 C0 BA B7 2C B0 C3 58 2A 14 E9 EC 87 A9 D0 5C 50 A4 A5 8B C1 BB 33 77 F2"});

    std::vector<uint8_t> ff_v0 = apply_des_key_version_bits(hex_to_bytes("FF FF FF FF FF FF FF FF"), 0x00);
    std::vector<uint8_t> eleven_v0 = apply_des_key_version_bits(hex_to_bytes("11 11 11 11 11 11 11 11"), 0x00);

    v.push_back({"Vector 11: Custom DES 00->FF with keyVersion=0x00",
        {AS::LegacyCrc16, SS::DES, hex_to_bytes("92 f1 35 8c ea e9 6a 10"), 0x00, 0x00, KT::DES, ff_v0, std::nullopt, 0x00, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00"), LM::SendModeDecrypt},
        0x9867, std::nullopt,
        "FE FE FE FE FE FE FE FE FE FE FE FE FE FE FE FE 67 98 00 00 00 00 00 00",
        "D8 BA 7D 1C 90 65 4D D1 80 A6 1E 2B 56 AE B0 5C BE 37 DA AB 95 82 49 4B"});

    v.push_back({"Vector 12: Custom DES FF->11 with keyVersion=0x00",
        {AS::LegacyCrc16, SS::DES, hex_to_bytes("92 f1 35 8c ea e9 6a 10"), 0x00, 0x00, KT::DES, eleven_v0, std::nullopt, 0x00, 0xC4, hex_to_bytes("00 00 00 00 00 00 00 00"), LM::SendModeDecrypt},
        0x5462, std::nullopt,
        "10 10 10 10 10 10 10 10 10 10 10 10 10 10 10 10 62 54 00 00 00 00 00 00",
        "A3 CB B1 6B C6 2A CE 56 DB 0D 83 81 CE 31 A5 C7 31 8D D7 9E 00 1A CB 99"});

    return v;
}

static int run_suite(bool verbose) {
    ChangeKeyCommand cmd(verbose);
    auto tests = build_examples();
    int passed = 0;

    // Matches Python Vector 5 (temp.html legacy CRC16-only example).
    {
        std::vector<uint8_t> key_ff16 = hex_to_bytes("FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF");
        uint16_t c = crc16_desfire(key_ff16);
        if (c != 0xCB37) {
            throw std::runtime_error("Vector 5 CRC16 mismatch");
        }
        std::vector<uint8_t> plain = key_ff16;
        std::vector<uint8_t> cbytes = u16le(c);
        plain.insert(plain.end(), cbytes.begin(), cbytes.end());
        plain = zero_pad(plain, 8);
        expect_hex_eq("Vector 5 plain", plain, "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 37 CB 00 00 00 00 00 00");
        std::cout << "[PASS] Vector 5: temp.html legacy CRC16 example\n";
        ++passed;
    }

    for (const auto& t : tests) {
        try {
            ChangeKeyResult out = cmd.run(t.in);
            if (t.expect_crc_crypto.has_value() && out.crc_crypto != *t.expect_crc_crypto) {
                throw std::runtime_error("crc_crypto mismatch");
            }
            if (t.expect_crc_new.has_value()) {
                if (!out.crc_new_key.has_value() || *out.crc_new_key != *t.expect_crc_new) {
                    throw std::runtime_error("crc_new_key mismatch");
                }
            }
            expect_hex_eq(t.name + " plain", out.cryptogram_padded, t.expect_plain_hex);
            expect_hex_eq(t.name + " enc", out.cryptogram_encrypted, t.expect_enc_hex);
            std::cout << "[PASS] " << t.name << "\n";
            ++passed;
        } catch (const std::exception& ex) {
            std::cerr << "[FAIL] " << t.name << "\n" << ex.what() << "\n";
            return 1;
        }
    }
    std::cout << "\nAll vectors matched in C++ (" << passed << "/" << (tests.size() + 1) << ").\n";
    return 0;
}

} // namespace desfire

int main(int argc, char** argv) {
    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
        if (a == "--verbose" || a == "-v") verbose = true;
    }
    try {
        return desfire::run_suite(verbose);
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }
}
