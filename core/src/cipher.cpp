/**
 * cipher.cpp -- ENIGMA01 format: encrypt_bytes / decrypt_bytes.
 *
 * This file owns the binary file format and the key-derivation flow.
 * Low-level primitives live in primitives.cpp.
 * Salt generation lives in salt.cpp.
 * File I/O wrappers live in file_io.cpp.
 *
 * ENIGMA01 format (all integers little-endian):
 *   [8]          magic "ENIGMA01"
 *   [2 LE]       salt_len
 *   [salt_len]   salt string (printable ASCII, unique per encryption)
 *   [2 LE]       cost  (uint16, work factor passed to hash_password)
 *   [2 LE]       verify_len
 *   [verify_len] hash-of-hash  (password verification tag)
 *   [remaining]  encrypted data in 1 KB sub-chunks
 */

#include "enigma/cipher.h"
#include "enigma/hash.h"
#include "enigma/primitives.h"
#include "salt.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>

/* ── Format constants ────────────────────────────────────────────────────── */

static constexpr size_t SUB_CHUNK = 1024;  // encrypt in 1 KB blocks
static const char MAGIC[8] = {'E','N','I','G','M','A','0','1'};

/* ── Little-endian uint16 helpers ────────────────────────────────────────── */

static void write_le16(uint8_t *p, uint16_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
}

static uint16_t read_le16(const uint8_t *p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

/* ── encrypt_bytes ───────────────────────────────────────────────────────── */

int encrypt_bytes(const uint8_t *in, size_t in_len,
                  const char *password, int cost,
                  uint8_t **out, size_t *out_len)
{
    // 1. Fresh unique salt (CSPRNG + counter so back-to-back calls never match)
    std::string salt = generate_cipher_salt();

    // 2. Derive key: hash_password(password, cost, salt)
    char *hashed = hash_password(password, cost, salt.c_str());
    if (!hashed) return -1;

    // 3. Verification tag: hash_password(key, cost, salt)
    char *verify = hash_password(hashed, cost, salt.c_str());
    if (!verify) { std::free(hashed); return -1; }

    // 4. Encrypt data in 1 KB sub-chunks
    std::vector<uint8_t> enc_data;
    if (in && in_len > 0)
        enc_data.assign(in, in + in_len);

    for (size_t off = 0; off < enc_data.size(); off += SUB_CHUNK) {
        size_t chunk     = std::min(SUB_CHUNK, enc_data.size() - off);
        int    chunk_idx = static_cast<int>(off / SUB_CHUNK);
        byte_manipulations(enc_data.data() + off, chunk,
                           reinterpret_cast<const uint8_t *>(hashed),
                           std::strlen(hashed), chunk_idx);
    }

    // 5. Build output: [magic][salt_len][salt][cost][verify_len][verify][data]
    uint16_t salt_len   = static_cast<uint16_t>(salt.size());
    uint16_t verify_len = static_cast<uint16_t>(std::strlen(verify));
    size_t   header_sz  = 8 + 2 + salt_len + 2 + 2 + verify_len;
    size_t   total      = header_sz + enc_data.size();

    uint8_t *buf = static_cast<uint8_t *>(std::malloc(total));
    if (!buf) { std::free(hashed); std::free(verify); return -1; }

    size_t pos = 0;
    std::memcpy(buf + pos, MAGIC, 8);                        pos += 8;
    write_le16(buf + pos, salt_len);                          pos += 2;
    std::memcpy(buf + pos, salt.c_str(), salt_len);           pos += salt_len;
    write_le16(buf + pos, static_cast<uint16_t>(cost));       pos += 2;
    write_le16(buf + pos, verify_len);                        pos += 2;
    std::memcpy(buf + pos, verify, verify_len);               pos += verify_len;
    if (!enc_data.empty())
        std::memcpy(buf + pos, enc_data.data(), enc_data.size());

    *out     = buf;
    *out_len = total;

    std::free(hashed);
    std::free(verify);
    return 0;
}

/* ── decrypt_bytes ───────────────────────────────────────────────────────── */

int decrypt_bytes(const uint8_t *in, size_t in_len,
                  const char *password,
                  uint8_t **out, size_t *out_len)
{
    // Parse and validate magic
    if (in_len < 8 || std::memcmp(in, MAGIC, 8) != 0) {
        *out = nullptr; *out_len = 0;
        return -1;
    }
    size_t pos = 8;

    if (pos + 2 > in_len) return -1;
    uint16_t salt_len = read_le16(in + pos); pos += 2;
    if (pos + salt_len > in_len) return -1;
    std::string salt(reinterpret_cast<const char *>(in + pos), salt_len); pos += salt_len;

    if (pos + 2 > in_len) return -1;
    uint16_t cost = read_le16(in + pos); pos += 2;

    if (pos + 2 > in_len) return -1;
    uint16_t verify_len = read_le16(in + pos); pos += 2;
    if (pos + verify_len > in_len) return -1;
    std::string stored_verify(reinterpret_cast<const char *>(in + pos), verify_len);
    pos += verify_len;

    // Derive key and re-compute verification tag
    char *hashed = hash_password(password, static_cast<int>(cost), salt.c_str());
    if (!hashed) return -1;
    char *computed_verify = hash_password(hashed, static_cast<int>(cost), salt.c_str());
    if (!computed_verify) { std::free(hashed); return -1; }

    bool ok = (stored_verify == std::string(computed_verify));
    std::free(computed_verify);
    if (!ok) { std::free(hashed); return -2; }  // wrong password

    // Decrypt data
    size_t   data_len = in_len - pos;
    uint8_t *buf = static_cast<uint8_t *>(std::malloc(data_len ? data_len : 1));
    if (!buf) { std::free(hashed); return -1; }
    std::memcpy(buf, in + pos, data_len);

    for (size_t off = 0; off < data_len; off += SUB_CHUNK) {
        size_t chunk     = std::min(SUB_CHUNK, data_len - off);
        int    chunk_idx = static_cast<int>(off / SUB_CHUNK);
        byte_manipulations_reverse(buf + off, chunk,
                                   reinterpret_cast<const uint8_t *>(hashed),
                                   std::strlen(hashed), chunk_idx);
    }

    *out     = buf;
    *out_len = data_len;

    std::free(hashed);
    return 0;
}
