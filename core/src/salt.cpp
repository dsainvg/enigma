/**
 * salt.cpp -- CSPRNG and salt generation utilities.
 *
 * Provides two flavours of salt:
 *   generate_salt()        -- simple random salt (used by hash_password when
 *                             no salt is supplied by the caller)
 *   generate_cipher_salt() -- counter + timestamp mixed salt that guarantees
 *                             two back-to-back calls always differ, even if
 *                             the underlying CSPRNG returns the same bytes
 *                             (used by encrypt_bytes)
 */

#include "salt.h"

#include <atomic>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#  include <windows.h>
#  include <wincrypt.h>
#else
#  include <fcntl.h>
#  include <unistd.h>
#endif

/* ── Platform CSPRNG ─────────────────────────────────────────────────────── */

void get_random_bytes(uint8_t *buf, size_t len) {
#ifdef _WIN32
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, static_cast<DWORD>(len), buf);
        CryptReleaseContext(hProv, 0);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t r = read(fd, buf, len);
        (void)r;
        close(fd);
    }
#endif
}

/* ── Salt alphabet ───────────────────────────────────────────────────────── */

static const char ALPHA[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* ── Basic salt (hash_password with salt=NULL) ───────────────────────────── */

std::string generate_salt() {
    uint8_t rnd[16];
    get_random_bytes(rnd, 16);
    std::string salt(16, '\0');
    // First char must be alphabetic so '$' delimiters parse unambiguously.
    salt[0] = ALPHA[rnd[0] % 52];
    for (int i = 1; i < 16; i++)
        salt[i] = static_cast<char>((rnd[i] % (126 - 37)) + 37);
    return salt;
}

/* ── Counter-mixed salt (encrypt_bytes) ──────────────────────────────────── */

std::string generate_cipher_salt() {
    static std::atomic<uint64_t> call_counter{0};

    uint8_t rnd[16] = {};
    get_random_bytes(rnd, 16);

    // Mix counter across all bytes so two consecutive calls always differ.
    uint64_t ctr = ++call_counter;
    for (int i = 0; i < 16; i++) {
        rnd[i] ^= static_cast<uint8_t>(ctr >> ((i % 8) * 8));
        rnd[i] ^= static_cast<uint8_t>(
            (ctr * static_cast<uint64_t>(i + 1) * 0x9e3779b97f4a7c15ULL) >> 56);
    }

    // Also mix in a high-resolution timestamp for extra entropy.
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    for (int i = 0; i < 8; i++)
        rnd[i + 8] ^= static_cast<uint8_t>(now >> (i * 8));

    char salt_buf[17];
    salt_buf[0] = ALPHA[rnd[0] % 52];
    // Embed counter directly at positions 1 & 2 to guarantee uniqueness.
    salt_buf[1] = static_cast<char>(((ctr)       % (126 - 37)) + 37);
    salt_buf[2] = static_cast<char>(((ctr >> 8)  % (126 - 37)) + 37);
    for (int i = 3; i < 16; i++)
        salt_buf[i] = static_cast<char>((rnd[i] % (126 - 37)) + 37);
    salt_buf[16] = '\0';
    return std::string(salt_buf);
}
