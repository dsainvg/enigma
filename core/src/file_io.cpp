/**
 * file_io.cpp -- File I/O helpers and high-level file encrypt/decrypt.
 *
 * Implements:
 *   read_file()      -- slurp a file into a byte vector
 *   write_file()     -- write a byte buffer to a file
 *   encrypt_file()   -- thin wrapper: read → encrypt_bytes → write
 *   decrypt_file()   -- thin wrapper: read → decrypt_bytes → write
 *
 * All crypto logic lives in cipher.cpp; this file only handles I/O.
 */

#include "enigma/cipher.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

/* ── Internal file helpers ───────────────────────────────────────────────── */

static std::vector<uint8_t> read_file(const char *path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    size_t sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> buf(sz);
    f.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(sz));
    return buf;
}

static bool write_file(const char *path, const uint8_t *data, size_t len) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(len));
    return f.good();
}

/* ── Public API ──────────────────────────────────────────────────────────── */

int encrypt_file(const char *input_file, const char *output_file,
                 const char *password, int cost)
{
    // Verify the input file is readable before doing any work.
    std::ifstream check(input_file, std::ios::binary);
    if (!check) return -1;
    check.close();

    auto in = read_file(input_file);
    uint8_t *out = nullptr;
    size_t   out_len = 0;
    int rc = encrypt_bytes(in.empty() ? nullptr : in.data(), in.size(),
                           password, cost, &out, &out_len);
    if (rc != 0) return rc;
    bool ok = write_file(output_file, out, out_len);
    std::free(out);
    return ok ? 0 : -1;
}

int decrypt_file(const char *input_file, const char *output_file,
                 const char *password)
{
    auto in = read_file(input_file);
    if (in.empty()) return -1;
    uint8_t *out = nullptr;
    size_t   out_len = 0;
    int rc = decrypt_bytes(in.data(), in.size(), password, &out, &out_len);
    if (rc != 0) return rc;
    bool ok = write_file(output_file, out, out_len);
    std::free(out);
    return ok ? 0 : -1;
}
