/**
 * test_cipher.cpp -- C++ unit tests for enigma_core cipher primitives
 * and the high-level encrypt/decrypt byte-buffer API.
 */

#include "enigma/cipher.h"
#include "enigma/hash.h"
#include "enigma/primitives.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

/* ── Minimal test framework ───────────────────────────────────────────────── */

static int passed = 0, failed = 0;

static void test(const char *name, bool condition) {
    const char *status = condition ? "\033[32mPASS\033[0m" : "\033[31mFAIL\033[0m";
    std::printf("  [%s] %s\n", status, name);
    if (condition) ++passed; else ++failed;
}

static void section(const char *name) {
    std::printf("\n── %s ──\n", name);
}

/* ── Helpers ──────────────────────────────────────────────────────────────── */

static std::vector<uint8_t> make_bytes(const char *s) {
    return std::vector<uint8_t>(reinterpret_cast<const uint8_t *>(s),
                                reinterpret_cast<const uint8_t *>(s) + std::strlen(s));
}

/* ── rotate_left / rotate_right ──────────────────────────────────────────── */

static void test_rotate() {
    section("rotate_left / rotate_right");

    // Rotate by 0 is identity
    uint8_t data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t orig[4];
    std::memcpy(orig, data, 4);
    rotate_left(data, 4, 0);
    test("rotate_left by 0 is identity", std::memcmp(data, orig, 4) == 0);

    // Left then right by same amount → identity
    std::memcpy(data, orig, 4);
    rotate_left(data, 4, 3);
    rotate_right(data, 4, 3);
    test("rotate_left then rotate_right is identity", std::memcmp(data, orig, 4) == 0);

    // Full cycle (rotate by total bits) → identity
    std::memcpy(data, orig, 4);
    rotate_left(data, 4, 32); // 32 = 4*8
    test("rotate_left by full width is identity", std::memcmp(data, orig, 4) == 0);

    // Right then left → identity
    std::memcpy(data, orig, 4);
    rotate_right(data, 4, 7);
    rotate_left(data, 4, 7);
    test("rotate_right then rotate_left is identity", std::memcmp(data, orig, 4) == 0);

    // Rotate 1 byte
    uint8_t single[1] = {0b10110001};
    uint8_t s_orig = single[0];
    rotate_left(single, 1, 1);
    rotate_right(single, 1, 1);
    test("Single-byte rotate round-trip", single[0] == s_orig);
}

/* ── xor_bytes ─────────────────────────────────────────────────────────────── */

static void test_xor() {
    section("xor_bytes");

    uint8_t data[8]  = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t key[3]   = {0xAA, 0xBB, 0xCC};
    uint8_t orig[8];
    std::memcpy(orig, data, 8);

    xor_bytes(data, 8, key, 3);
    test("XOR changes data",       std::memcmp(data, orig, 8) != 0);

    xor_bytes(data, 8, key, 3);
    test("Double XOR is identity", std::memcmp(data, orig, 8) == 0);

    // XOR with zeros is identity
    uint8_t zeros[4] = {};
    std::memcpy(data, orig, 4);
    xor_bytes(data, 4, zeros, 4);
    test("XOR with zero key is identity", std::memcmp(data, orig, 4) == 0);
}

/* ── byte_manipulations round-trip ────────────────────────────────────────── */

static void test_byte_manipulations() {
    section("byte_manipulations / byte_manipulations_reverse");

    const char *key_str = "mysecretkey_1234";
    const uint8_t *key = reinterpret_cast<const uint8_t *>(key_str);
    size_t klen = std::strlen(key_str);

    auto round_trip = [&](const char *msg, int iterat) {
        std::vector<uint8_t> data = make_bytes(msg);
        std::vector<uint8_t> orig = data;
        byte_manipulations(data.data(), data.size(), key, klen, iterat);
        bool changed = (data != orig);
        byte_manipulations_reverse(data.data(), data.size(), key, klen, iterat);
        bool restored = (data == orig);
        return changed && restored;
    };

    test("Short data round-trip (iterat=0)",  round_trip("Hello, World!", 0));
    test("Short data round-trip (iterat=1)",  round_trip("Hello, World!", 1));
    test("Short data round-trip (iterat=5)",  round_trip("Hello, World!", 5));
    test("Longer data round-trip",            round_trip("The quick brown fox jumps over the lazy dog", 0));
    test("Binary data round-trip", [&]{
        std::vector<uint8_t> data(256);
        for (int i = 0; i < 256; i++) data[i] = static_cast<uint8_t>(i);
        auto orig = data;
        byte_manipulations(data.data(), data.size(), key, klen, 0);
        byte_manipulations_reverse(data.data(), data.size(), key, klen, 0);
        return data == orig;
    }());
    test("Empty data (no-op)", [&]{
        uint8_t buf[1] = {};
        byte_manipulations(buf, 0, key, klen, 0);
        byte_manipulations_reverse(buf, 0, key, klen, 0);
        return true; // just shouldn't crash
    }());
    test("Short key (<6) is ignored", [&]{
        uint8_t data[4] = {1,2,3,4}, orig[4] = {1,2,3,4};
        uint8_t short_key[3] = {0xAA, 0xBB, 0xCC};
        byte_manipulations(data, 4, short_key, 3, 0);
        return std::memcmp(data, orig, 4) == 0; // Should be unchanged
    }());
}

/* ── encrypt_bytes / decrypt_bytes ────────────────────────────────────────── */

static void test_encrypt_decrypt_bytes() {
    section("encrypt_bytes / decrypt_bytes");

    const char *password = "testPassword123";
    const char *plaintext = "Hello, enigma world! This is a test message.";
    const uint8_t *in = reinterpret_cast<const uint8_t *>(plaintext);
    size_t in_len = std::strlen(plaintext);

    // Basic round-trip
    uint8_t *enc = nullptr;
    size_t   enc_len = 0;
    int rc = encrypt_bytes(in, in_len, password, 8, &enc, &enc_len);
    test("encrypt_bytes returns 0",    rc == 0);
    test("Output is non-null",         enc != nullptr);
    test("Output longer than input",   enc_len > in_len);
    test("Output starts with ENIGMA01", enc && std::memcmp(enc, "ENIGMA01", 8) == 0);

    uint8_t *dec = nullptr;
    size_t   dec_len = 0;
    rc = decrypt_bytes(enc, enc_len, password, &dec, &dec_len);
    test("decrypt_bytes returns 0",    rc == 0);
    test("Decrypted length matches",   dec_len == in_len);
    test("Decrypted content matches",  dec && std::memcmp(dec, in, in_len) == 0);
    std::free(enc); std::free(dec);

    // Wrong password
    enc = nullptr; enc_len = 0;
    encrypt_bytes(in, in_len, password, 8, &enc, &enc_len);
    dec = nullptr; dec_len = 0;
    rc = decrypt_bytes(enc, enc_len, "wrongPassword", &dec, &dec_len);
    test("Wrong password returns -2",  rc == -2);
    std::free(enc); std::free(dec);

    // Empty plaintext
    enc = nullptr; enc_len = 0;
    rc = encrypt_bytes(nullptr, 0, password, 8, &enc, &enc_len);
    test("Empty plaintext encrypts ok", rc == 0 && enc != nullptr);
    dec = nullptr; dec_len = 0;
    rc = decrypt_bytes(enc, enc_len, password, &dec, &dec_len);
    test("Empty plaintext decrypts ok", rc == 0 && dec_len == 0);
    std::free(enc); std::free(dec);

    // Larger data (> SUB_CHUNK 1024 bytes)
    std::vector<uint8_t> large(4096);
    for (size_t i = 0; i < large.size(); i++) large[i] = static_cast<uint8_t>(i & 0xFF);
    enc = nullptr; enc_len = 0;
    rc = encrypt_bytes(large.data(), large.size(), password, 8, &enc, &enc_len);
    test("Large data encrypts ok", rc == 0);
    dec = nullptr; dec_len = 0;
    rc = decrypt_bytes(enc, enc_len, password, &dec, &dec_len);
    test("Large data decrypts ok",      rc == 0);
    test("Large data round-trip exact",
         dec && dec_len == large.size() &&
         std::memcmp(dec, large.data(), large.size()) == 0);
    std::free(enc); std::free(dec);

    // Bad magic bytes
    uint8_t bad[16] = {'B','A','D','M','A','G','I','C','1','2','3','4','5','6','7','\0'};
    uint8_t *bad_dec = nullptr;
    size_t   bad_dec_len = 0;
    test("decrypt_bytes bad magic returns -1",
         decrypt_bytes(bad, 16, password, &bad_dec, &bad_dec_len) == -1);
    // bad_dec is nullptr on bad-magic path — don't free
}

/* ── encrypt_file / decrypt_file ─────────────────────────────────────────── */

static void test_encrypt_decrypt_file() {
    section("encrypt_file / decrypt_file");

    const char *in_path  = "test_input_tmp.txt";
    const char *enc_path = "test_enc_tmp.enc";
    const char *dec_path = "test_dec_tmp.txt";
    const char *password = "fileTestPass";
    const char *content  = "File encryption test content 1234567890!";

    // Write input file
    {
        std::ofstream f(in_path);
        f << content;
    }

    int rc = encrypt_file(in_path, enc_path, password, 8);
    test("encrypt_file returns 0", rc == 0);

    rc = decrypt_file(enc_path, dec_path, password);
    test("decrypt_file returns 0", rc == 0);

    // Read back and compare
    std::ifstream f(dec_path);
    std::string result((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    test("File content round-trips correctly", result == content);

    // Wrong password
    rc = decrypt_file(enc_path, dec_path, "wrongPass");
    test("decrypt_file wrong password returns -2", rc == -2);

    // Missing file
    rc = encrypt_file("nonexistent_file.xyz", enc_path, password, 8);
    test("encrypt_file missing input returns error", rc != 0);

    // Cleanup
    std::remove(in_path);
    std::remove(enc_path);
    std::remove(dec_path);
}

/* ── Encryption produces different output each time (due to random salt) ── */

static void test_randomness() {
    section("Ciphertext Randomness");

    const char *password = "testPass";
    const char *msg = "same plaintext";
    const uint8_t *in = reinterpret_cast<const uint8_t *>(msg);
    size_t in_len = std::strlen(msg);

    uint8_t *enc1 = nullptr, *enc2 = nullptr;
    size_t   len1 = 0, len2 = 0;
    encrypt_bytes(in, in_len, password, 8, &enc1, &len1);
    encrypt_bytes(in, in_len, password, 8, &enc2, &len2);

    // The two ciphertexts may differ in length (variable-length hash output),
    // but their embedded salts must differ since we use a fresh CSPRNG salt each time.
    auto read_salt = [](const uint8_t *enc, size_t len) -> std::string {
        if (!enc || len < 12) return "";
        uint16_t sl = static_cast<uint16_t>(enc[8]) | (static_cast<uint16_t>(enc[9]) << 8);
        if (10 + sl > len) return "";
        return std::string(reinterpret_cast<const char *>(enc + 10), sl);
    };
    std::string s1 = read_salt(enc1, len1);
    std::string s2 = read_salt(enc2, len2);
    test("Same plaintext → different salts (random salt)", !s1.empty() && s1 != s2);

    // Both should still decrypt correctly
    uint8_t *dec = nullptr;
    size_t   dec_len = 0;
    int rc1 = decrypt_bytes(enc1, len1, password, &dec, &dec_len);
    bool ok1 = (rc1 == 0 && dec_len == in_len && std::memcmp(dec, in, in_len) == 0);
    std::free(dec); dec = nullptr;
    int rc2 = decrypt_bytes(enc2, len2, password, &dec, &dec_len);
    bool ok2 = (rc2 == 0 && dec_len == in_len && std::memcmp(dec, in, in_len) == 0);
    std::free(dec);

    test("Both random ciphertexts decrypt correctly", ok1 && ok2);
    std::free(enc1); std::free(enc2);
}

/* ── Entry point ──────────────────────────────────────────────────────────── */

int main() {
    std::printf("=== enigma cipher tests ===\n");

    test_rotate();
    test_xor();
    test_byte_manipulations();
    test_encrypt_decrypt_bytes();
    test_encrypt_decrypt_file();
    test_randomness();

    std::printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
