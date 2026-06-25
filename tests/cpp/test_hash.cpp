/**
 * test_hash.cpp -- C++ unit tests for enigma_core hash_password()
 *
 * Lightweight custom test runner (no external dependencies).
 * Run via CMake CTest or directly.
 */

#include "enigma/hash.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

/* ── Test cases ───────────────────────────────────────────────────────────── */

static void test_determinism() {
    section("Determinism");
    const char *salt = "testSalt12345678";

    char *h1 = hash_password("mypassword", 10, salt);
    char *h2 = hash_password("mypassword", 10, salt);
    test("Same password+salt → same hash",  h1 && h2 && std::strcmp(h1, h2) == 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("test123", 8, salt);
    h2 = hash_password("test123", 8, salt);
    test("Deterministic at cost=8",          h1 && h2 && std::strcmp(h1, h2) == 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("", 8, salt);
    h2 = hash_password("", 8, salt);
    test("Empty password deterministic",     h1 && h2 && std::strcmp(h1, h2) == 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("a", 8, salt);
    h2 = hash_password("a", 8, salt);
    test("Single-char password deterministic", h1 && h2 && std::strcmp(h1, h2) == 0);
    std::free(h1); std::free(h2);
}

static void test_different_passwords() {
    section("Password Sensitivity");
    const char *salt = "testSalt12345678";

    char *h1 = hash_password("password1", 10, salt);
    char *h2 = hash_password("password2", 10, salt);
    test("Different passwords → different hashes", h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("A", 10, salt);
    h2 = hash_password("B", 10, salt);
    test("Single-char passwords differ",          h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("completely_different_pw", 10, salt);
    h2 = hash_password("another_very_different_pw", 10, salt);
    test("Long distinct passwords differ", h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("AlphaPassword", 10, salt);
    h2 = hash_password("BetaPassword1", 10, salt);
    test("Different prefixes differ",            h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("abc", 8, salt);
    h2 = hash_password("abc ", 8, salt);
    test("Trailing space changes hash",          h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);
    /* Note: In extreme, highly unlikely cases, distinct inputs may collide (Birthday Paradox). */
}

static void test_different_salts() {
    section("Salt Sensitivity");

    char *h1 = hash_password("mypassword", 10, "testSalt12345678");
    char *h2 = hash_password("mypassword", 10, "otherSalt1234567");
    test("Different salts → different hashes",  h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("test", 10, "a");
    h2 = hash_password("test", 10, "b");
    test("Single-char salt differs",            h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("test", 10, "salt123456789012");
    h2 = hash_password("test", 10, "salt123456789013");
    test("Near-identical salts differ",         h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);
}

static void test_random_salts() {
    section("Random Salts");

    char *h1 = hash_password("mypassword", 10, nullptr);
    char *h2 = hash_password("mypassword", 10, nullptr);
    test("Random salts → different hashes",    h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("test", 8, nullptr);
    h2 = hash_password("test", 8, nullptr);
    test("Random salts at cost=8",             h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);
}

static void test_cost_factors() {
    section("Cost Factor");
    const char *salt = "testSalt12345678";

    char *h1 = hash_password("mypassword", 8,  salt);
    char *h2 = hash_password("mypassword", 10, salt);
    test("Cost 8 vs 10 differ",   h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("mypassword", 10, salt);
    h2 = hash_password("mypassword", 12, salt);
    test("Cost 10 vs 12 differ",  h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("mypassword", 1,  salt);
    h2 = hash_password("mypassword", 2,  salt);
    test("Cost 1 vs 2 differ",    h1 && h2 && std::strcmp(h1, h2) != 0);
    std::free(h1); std::free(h2);
}

static void test_format() {
    section("Output Format");
    const char *salt = "testSalt12345678";

    char *h = hash_password("mypassword", 8, salt);
    test("Non-null result",            h != nullptr);
    test("Starts with '$'",            h && h[0] == '$');
    test("Contains '$/$'",             h && std::strstr(h, "$/$") != nullptr);
    test("Contains salt in output",    h && std::strstr(h, salt) != nullptr);
    std::free(h);
}

static void test_extract_salt() {
    section("extract_salt()");
    const char *salt = "testSalt12345678";

    char *h = hash_password("mypassword", 8, salt);
    char out[64];
    int rc = extract_salt(h, out, sizeof(out));
    test("extract_salt returns 0",          rc == 0);
    test("Extracted salt matches original", std::strcmp(out, salt) == 0);

    // Re-hash with extracted salt should be deterministic
    char *h2 = hash_password("mypassword", 8, out);
    test("Re-hash with extracted salt matches", h && h2 && std::strcmp(h, h2) == 0);
    std::free(h); std::free(h2);

    // Bad input
    test("extract_salt NULL returns -1",   extract_salt(nullptr, out, sizeof(out)) == -1);
    test("extract_salt bad fmt returns -1", extract_salt("noDollar", out, sizeof(out)) == -1);
}

static void test_special_characters() {
    section("Special Characters");
    const char *salt = "testSalt12345678";

    char *h1 = hash_password("p@$$w0rd!", 8, salt);
    char *h2 = hash_password("p@$$w0rd!", 8, salt);
    test("Special chars deterministic",      h1 && h2 && std::strcmp(h1, h2) == 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("abc\x01\x02\x03", 8, salt);
    h2 = hash_password("abc\x01\x02\x03", 8, salt);
    test("Binary chars deterministic",       h1 && h2 && std::strcmp(h1, h2) == 0);
    std::free(h1); std::free(h2);

    h1 = hash_password("unicode: héllo wörld", 8, salt);
    h2 = hash_password("unicode: héllo wörld", 8, salt);
    test("Unicode bytes deterministic",      h1 && h2 && std::strcmp(h1, h2) == 0);
    std::free(h1); std::free(h2);
}

static void test_avalanche() {
    section("Avalanche Effect");
    const char *salt = "testSalt12345678";

    // Count differing characters between two hashes
    auto count_diff = [](const char *a, const char *b) {
        int diff = 0;
        size_t len = std::min(std::strlen(a), std::strlen(b));
        for (size_t i = 0; i < len; i++) if (a[i] != b[i]) diff++;
        return diff;
    };

    char *h1 = hash_password("password",  8, salt);
    char *h2 = hash_password("password1", 8, salt);
    int diff = h1 && h2 ? count_diff(h1, h2) : 0;
    test("One extra char → many different output chars (avalanche)", diff > 3);
    std::free(h1); std::free(h2);
}

/* ── Entry point ──────────────────────────────────────────────────────────── */

int main() {
    std::printf("=== enigma hash tests ===\n");

    test_determinism();
    test_different_passwords();
    test_different_salts();
    test_random_salts();
    test_cost_factors();
    test_format();
    test_extract_salt();
    test_special_characters();
    test_avalanche();

    std::printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
