/**
 * file_io.cpp -- Streaming, optionally multi-threaded file encrypt/decrypt.
 *
 * Design goals:
 *   1. Constant memory — only `io_buf_mb` MiB of plaintext/ciphertext are
 *      held in RAM at any moment, regardless of file size.
 *   2. Intra-buffer parallelism — the independent 1 KB cipher blocks inside
 *      each buffer are distributed across `thread_count` worker threads.
 *
 * The ENIGMA01 format on disk is completely unchanged:
 *   [8]          magic "ENIGMA01"
 *   [2 LE]       salt_len
 *   [salt_len]   salt
 *   [2 LE]       cost
 *   [2 LE]       verify_len
 *   [verify_len] verify tag (hash-of-hash)
 *   [rest]       encrypted data in 1 KB sub-chunks (sequential, global index)
 *
 * encrypt_bytes / decrypt_bytes in cipher.cpp are NOT touched.
 */

#include "enigma/cipher.h"
#include "enigma/hash.h"
#include "salt.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

/* ── Constants ────────────────────────────────────────────────────────────── */

static constexpr size_t SUB_CHUNK   = 1024;         // 1 KB cipher block
static const char       MAGIC[8]    = {'E','N','I','G','M','A','0','1'};

/* ── Little-endian uint16 helpers ─────────────────────────────────────────── */

static void write_le16(uint8_t *p, uint16_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
}
static uint16_t read_le16(const uint8_t *p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

/* ── Thread-parallel cipher pass over a flat buffer ──────────────────────── */

/**
 * Apply byte_manipulations (or its reverse) to every 1 KB block inside `buf`,
 * distributing blocks evenly across `nthreads` worker threads.
 *
 * @param buf          Buffer to process in-place.
 * @param buf_len      Length of buf.
 * @param key          Derived key bytes (shared read-only across threads).
 * @param key_len      Key length.
 * @param first_chunk  Global chunk index of the first 1 KB block in buf.
 *                     Each block uses (first_chunk + local_index) as iterat.
 * @param decrypt      false → encrypt direction; true → decrypt direction.
 * @param nthreads     Number of threads to use (1 = no threading overhead).
 */
static void parallel_cipher(uint8_t *buf, size_t buf_len,
                             const uint8_t *key, size_t key_len,
                             int first_chunk, bool decrypt, int nthreads)
{
    // Count blocks in this buffer
    size_t nblocks = (buf_len + SUB_CHUNK - 1) / SUB_CHUNK;

    // Divide blocks across threads
    auto worker = [&](size_t block_start, size_t block_end) {
        for (size_t b = block_start; b < block_end; ++b) {
            size_t off   = b * SUB_CHUNK;
            size_t chunk = std::min(SUB_CHUNK, buf_len - off);
            int    idx   = first_chunk + static_cast<int>(b);
            if (!decrypt)
                byte_manipulations(buf + off, chunk, key, key_len, idx);
            else
                byte_manipulations_reverse(buf + off, chunk, key, key_len, idx);
        }
    };

    if (nthreads <= 1 || static_cast<size_t>(nthreads) >= nblocks) {
        // Single-threaded path — no thread-creation overhead
        worker(0, nblocks);
        return;
    }

    // Spawn nthreads workers
    std::vector<std::thread> workers;
    workers.reserve(static_cast<size_t>(nthreads));
    size_t per_thread = (nblocks + static_cast<size_t>(nthreads) - 1)
                         / static_cast<size_t>(nthreads);

    for (int t = 0; t < nthreads; ++t) {
        size_t bstart = static_cast<size_t>(t) * per_thread;
        size_t bend   = std::min(bstart + per_thread, nblocks);
        if (bstart >= nblocks) break;
        workers.emplace_back(worker, bstart, bend);
    }
    for (auto &th : workers) th.join();
}

/* ── Key derivation (shared by encrypt and decrypt) ──────────────────────── */

struct DerivedKey {
    std::string hashed;   // key bytes (printable ASCII)
    std::string verify;   // hash-of-hash for wrong-password check
    std::string salt;
    uint16_t    cost;
};

/* ── encrypt_file (streaming + parallel) ─────────────────────────────────── */

int encrypt_file(const char *input_file, const char *output_file,
                 const char *password, int cost,
                 int thread_count, int io_buf_mb)
{
    // Resolve thread count
    int nthreads = thread_count;
    if (nthreads <= 0)
        nthreads = static_cast<int>(std::thread::hardware_concurrency());
    if (nthreads < 1) nthreads = 1;

    // Open input
    std::ifstream fin(input_file, std::ios::binary);
    if (!fin) return -1;

    // 1. Key derivation
    std::string salt = generate_cipher_salt();
    char *hashed_c = hash_password(password, cost, salt.c_str());
    if (!hashed_c) return -1;
    char *verify_c = hash_password(hashed_c, cost, salt.c_str());
    if (!verify_c) { std::free(hashed_c); return -1; }

    std::string hashed(hashed_c);
    std::string verify(verify_c);
    std::free(hashed_c);
    std::free(verify_c);

    const auto *key     = reinterpret_cast<const uint8_t *>(hashed.c_str());
    size_t      key_len = hashed.size();

    // 2. Open output and write header
    std::ofstream fout(output_file, std::ios::binary);
    if (!fout) return -1;

    uint16_t salt_len16   = static_cast<uint16_t>(salt.size());
    uint16_t verify_len16 = static_cast<uint16_t>(verify.size());
    uint16_t cost16       = static_cast<uint16_t>(cost);

    uint8_t hdr[8 + 2 + 16 + 2 + 2]; // magic + salt_len + salt + cost + verify_len
    size_t hpos = 0;
    std::memcpy(hdr + hpos, MAGIC, 8);                    hpos += 8;
    write_le16(hdr + hpos, salt_len16);                    hpos += 2;
    std::memcpy(hdr + hpos, salt.c_str(), salt_len16);     hpos += salt_len16;
    write_le16(hdr + hpos, cost16);                        hpos += 2;
    write_le16(hdr + hpos, verify_len16);                  hpos += 2;

    fout.write(reinterpret_cast<const char *>(hdr), static_cast<std::streamsize>(hpos));
    fout.write(verify.c_str(), static_cast<std::streamsize>(verify.size()));
    if (!fout) return -1;

    // 3. Stream-encrypt
    size_t buf_bytes    = static_cast<size_t>(io_buf_mb < 1 ? 4 : io_buf_mb) * 1024 * 1024;
    std::vector<uint8_t> buf(buf_bytes);
    int global_chunk = 0;

    while (fin) {
        fin.read(reinterpret_cast<char *>(buf.data()),
                 static_cast<std::streamsize>(buf_bytes));
        std::streamsize n = fin.gcount();
        if (n <= 0) break;

        size_t len = static_cast<size_t>(n);
        parallel_cipher(buf.data(), len, key, key_len,
                        global_chunk, /*decrypt=*/false, nthreads);
        global_chunk += static_cast<int>((len + SUB_CHUNK - 1) / SUB_CHUNK);

        fout.write(reinterpret_cast<const char *>(buf.data()),
                   static_cast<std::streamsize>(len));
        if (!fout) return -1;
    }
    return 0;
}

/* ── decrypt_file (streaming + parallel) ─────────────────────────────────── */

int decrypt_file(const char *input_file, const char *output_file,
                 const char *password,
                 int thread_count, int io_buf_mb)
{
    int nthreads = thread_count;
    if (nthreads <= 0)
        nthreads = static_cast<int>(std::thread::hardware_concurrency());
    if (nthreads < 1) nthreads = 1;

    std::ifstream fin(input_file, std::ios::binary);
    if (!fin) return -1;

    // 1. Read and validate magic
    char magic_buf[8];
    fin.read(magic_buf, 8);
    if (fin.gcount() != 8 || std::memcmp(magic_buf, MAGIC, 8) != 0) return -1;

    // 2. Parse header fields
    auto read16 = [&](uint16_t &v) -> bool {
        uint8_t b[2];
        fin.read(reinterpret_cast<char *>(b), 2);
        if (fin.gcount() != 2) return false;
        v = read_le16(b);
        return true;
    };

    uint16_t salt_len16 = 0, cost16 = 0, verify_len16 = 0;
    if (!read16(salt_len16)) return -1;

    std::string salt(salt_len16, '\0');
    fin.read(&salt[0], salt_len16);
    if (fin.gcount() != static_cast<std::streamsize>(salt_len16)) return -1;

    if (!read16(cost16))       return -1;
    if (!read16(verify_len16)) return -1;

    std::string stored_verify(verify_len16, '\0');
    fin.read(&stored_verify[0], verify_len16);
    if (fin.gcount() != static_cast<std::streamsize>(verify_len16)) return -1;

    // 3. Key derivation + password verification
    char *hashed_c = hash_password(password, static_cast<int>(cost16), salt.c_str());
    if (!hashed_c) return -1;
    char *computed_c = hash_password(hashed_c, static_cast<int>(cost16), salt.c_str());
    if (!computed_c) { std::free(hashed_c); return -1; }

    bool ok = (stored_verify == std::string(computed_c));
    std::free(computed_c);
    if (!ok) { std::free(hashed_c); return -2; }  // wrong password

    std::string hashed(hashed_c);
    std::free(hashed_c);

    const auto *key     = reinterpret_cast<const uint8_t *>(hashed.c_str());
    size_t      key_len = hashed.size();

    // 4. Open output
    std::ofstream fout(output_file, std::ios::binary);
    if (!fout) return -1;

    // 5. Stream-decrypt
    size_t buf_bytes = static_cast<size_t>(io_buf_mb < 1 ? 4 : io_buf_mb) * 1024 * 1024;
    std::vector<uint8_t> buf(buf_bytes);
    int global_chunk = 0;

    while (fin) {
        fin.read(reinterpret_cast<char *>(buf.data()),
                 static_cast<std::streamsize>(buf_bytes));
        std::streamsize n = fin.gcount();
        if (n <= 0) break;

        size_t len = static_cast<size_t>(n);
        parallel_cipher(buf.data(), len, key, key_len,
                        global_chunk, /*decrypt=*/true, nthreads);
        global_chunk += static_cast<int>((len + SUB_CHUNK - 1) / SUB_CHUNK);

        fout.write(reinterpret_cast<const char *>(buf.data()),
                   static_cast<std::streamsize>(len));
        if (!fout) return -1;
    }
    return 0;
}



