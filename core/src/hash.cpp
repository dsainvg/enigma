/**
 * hash.cpp -- Custom iterative password hashing algorithm.
 *
 * Faithful C++ port of the Python reference implementation:
 *   Archis/Enc/encryptionAlgorithm/encryption_utils/hashing_utils.py
 *
 * Key Python-matching details:
 *   1. combined.encode('utf-8') — encoded via latin1_to_utf8_bytes()
 *   2. memo[1] stores str(sorted_hash_bytes) — Python's repr of a bytearray
 *   3. hash2/hash3/hash4 use int64_t to avoid overflow (Python bigint)
 *   4. hash3 does NOT reset hv from hash2 (Python doesn't reset voil)
 *   5. sort_index uses Python floor-modulo (always non-negative)
 *   6. hash1 formula uses Python floor division/modulo for negative hv1
 */

#include "enigma/hash.h"
#include "salt.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <cstdlib>

using Byte = uint8_t;

/* ── Helpers ─────────────────────────────────────────────────────────────── */

/** Python floor-modulo: result always has same sign as b. */
static int64_t py_mod(int64_t a, int64_t b) {
    return ((a % b) + b) % b;
}

/** Python floor-division. */
static int64_t py_div(int64_t a, int64_t b) {
    return (a - py_mod(a, b)) / b;
}

/**
 * Destructive nth-element cycling — matches Python _sort_by_nth_element.
 * Repeatedly picks element at (index + n - 1) % len, appends, removes.
 */
static std::vector<Byte> sort_by_nth_element(const std::vector<Byte> &data, size_t n) {
    if (data.empty()) return {};
    std::vector<Byte> arr = data;
    std::vector<Byte> result;
    result.reserve(data.size());
    size_t index = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        long long idx = static_cast<long long>(index) + static_cast<long long>(n) - 1;
        long long mod = idx % static_cast<long long>(arr.size());
        if (mod < 0) mod += static_cast<long long>(arr.size());
        index = static_cast<size_t>(mod);
        result.push_back(arr[index]);
        arr.erase(arr.begin() + index);
    }
    return result;
}

/**
 * Convert a Latin-1 string to its UTF-8 byte sequence.
 * Matches Python str.encode('utf-8') for strings produced by chr() calls.
 */
static std::vector<Byte> latin1_to_utf8_bytes(const std::string &s) {
    std::vector<Byte> out;
    out.reserve(s.size() * 2);
    for (unsigned char uc : s) {
        if (uc < 0x80) {
            out.push_back(uc);
        } else if (uc < 0xC0) {
            out.push_back(0xC2);
            out.push_back(uc);
        } else {
            out.push_back(0xC3);
            out.push_back(static_cast<Byte>(uc - 0x40));
        }
    }
    return out;
}

/**
 * Produce the Python str() representation of a bytearray.
 *
 * Python stores memo[16*i + 1] = str(sorted_hash_bytes) where
 * sorted_hash_bytes is a bytearray.  Python's str(bytearray(...)) gives:
 *   "bytearray(b'...')"
 * where printable ASCII chars appear as-is and non-printable/non-ASCII
 * chars appear as escape sequences (\xNN, \n, \r, \t, \\, \').
 *
 * This must be reproduced exactly in C++ so that memo lookups in subsequent
 * iterations produce identical accumulator values.
 */
static std::string bytearray_repr(const std::vector<Byte> &bytes) {
    std::string s = "bytearray(b'";
    for (Byte b : bytes) {
        if (b == '\\') {
            s += "\\\\";
        } else if (b == '\'') {
            s += "\\'";
        } else if (b == '\n') {
            s += "\\n";
        } else if (b == '\r') {
            s += "\\r";
        } else if (b == '\t') {
            s += "\\t";
        } else if (b >= 0x20 && b < 0x7f) {
            s += static_cast<char>(b);
        } else {
            // \xNN format (lowercase hex, exactly 2 digits)
            char buf[5];
            std::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned>(b));
            s += buf;
        }
    }
    s += "')";
    return s;
}

/* ── Core hash function (one iteration) ─────────────────────────────────── */

/**
 * One round of hashing — port of Python's _hash_password().
 *
 * Returns a 6-tuple matching Python's return:
 *   [0] hashed_str   ($salt$/$hash2 or $salt$/$hash2$hash4)
 *   [1] sorted_repr  str(sorted_hash_bytes) — bytearray repr
 *   [2] hash1
 *   [3] hash2
 *   [4] hash3  (or hash4 when memo_length > 47)
 *   [5] hash4  (or hash3 when memo_length > 47)
 */
using HashTuple = std::tuple<std::string, std::string, std::string,
                             std::string, std::string, std::string>;

static HashTuple internal_hash_password(
    const std::string &password,
    const std::string *salt_ptr,
    const std::vector<std::string> *memo)
{
    const std::string &salt = salt_ptr ? *salt_ptr : std::string("");
    std::string combined = salt + "$" + password;

    // Encode as UTF-8 — matches Python: combined.encode('utf-8')
    std::vector<Byte> combined_bytes = latin1_to_utf8_bytes(combined);

    std::string hash1, hash2, hash3, hash4;
    size_t memo_length = memo ? memo->size() : 0;
    int accumulator;

    if (memo && memo_length > 0) {
        // hash4 seeded from last memo entry (matches Python: hash4 = memo[length-1])
        hash4 = (*memo)[memo_length - 1];

        // Choose string_memo: memo[a] where a = (cb[-2]*97) % length
        int index = static_cast<int>(combined_bytes[combined_bytes.size() - 2]) * 97;
        index = index % static_cast<int>(memo_length);
        const std::string &string_memo = (*memo)[index];
        size_t string_memo_length = string_memo.size();

        // Choose accumulator byte from string_memo
        index = static_cast<int>(combined_bytes[combined_bytes.size() - 7]) * 113;
        index = index % static_cast<int>(string_memo_length);

        // Python: voila = string_memo[a].encode('utf-8')[0]
        // string_memo is a Python str; encode gives UTF-8 bytes; take [0].
        // In C++ string_memo is already stored as raw bytes (Latin-1 representation).
        // latin1_to_utf8_bytes of the single char gives its UTF-8 first byte.
        unsigned char sc = static_cast<unsigned char>(string_memo[index]);
        if (sc < 0x80) {
            accumulator = static_cast<int>(sc);
        } else if (sc < 0xC0) {
            accumulator = 0xC2; // first byte of 2-byte UTF-8 sequence
        } else {
            accumulator = 0xC3;
        }
    } else {
        int index = static_cast<int>(combined_bytes[0]) % 90;
        accumulator = static_cast<int>(static_cast<Byte>(index + 37));
    }

    if (accumulator > 96)
        accumulator -= 70;
    else
        accumulator -= 64;

    // ── hash1 ─────────────────────────────────────────────────────────────
    for (size_t i = 0; i < combined_bytes.size(); ++i) {
        int64_t hv1 = static_cast<int64_t>(accumulator) * 113 + combined_bytes[i];
        accumulator += 2;
        if (accumulator > 48) accumulator -= 23;
        // Python floor modulo/division to match negative-hv1 edge cases
        int64_t part1 = py_mod(hv1, 90);
        int64_t part2 = py_mod(py_div(hv1, 90), 90) + 37;
        hash1 += static_cast<char>(part1 & part2);
    }

    // hash1_bytes = hash1.encode('utf-8') — using latin1→utf-8
    std::vector<Byte> hash1_bytes = latin1_to_utf8_bytes(hash1);
    // sort_index = voila % 5 (Python floor modulo — always non-negative)
    int64_t sort_index = py_mod(static_cast<int64_t>(accumulator), 5);
    std::vector<Byte> sorted = sort_by_nth_element(hash1_bytes,
                                                    static_cast<size_t>(sort_index));

    // sorted_repr = str(sorted_hash_bytes) — Python bytearray repr
    std::string sorted_repr = bytearray_repr(sorted);

    // ── hash2: multiplier=71, starts at 0 ────────────────────────────────
    int64_t hv = 0;
    for (Byte b : sorted) {
        hv = hv * 71 + b;
        while (hv > 128) { hash2 += static_cast<char>((hv % 90) + 37); hv /= 90; }
    }
    hash2 += static_cast<char>((hv % 90) + 37);

    // ── hash3: multiplier=997, hv carries over from hash2 ────────────────
    for (Byte b : sorted) {
        hv = hv * 997 + b;
        while (hv > 128) { hash3 += static_cast<char>((hv % 90) + 37); hv /= 90; }
    }
    hash3 += static_cast<char>((hv % 90) + 37);

    // ── hash4: multiplier depends on memo_length, hv carries over ────────
    std::vector<Byte> h4_bytes;
    if (!hash4.empty()) {
        h4_bytes.assign(hash4.begin(), hash4.end());
    } else {
        h4_bytes = sorted;
        hash4.clear();
    }
    int64_t multiplier = (memo_length > 73) ? 1997 : 23;
    for (Byte b : h4_bytes) {
        hv = hv * multiplier + b;
        while (hv > 128) { hash4 += static_cast<char>((hv % 90) + 37); hv /= 90; }
    }

    // Return matches Python's tuple layout exactly:
    // (hashed, str(sorted), hash1, hash2, hash3_or_4, hash4_or_3)
    if (memo_length > 47) {
        return { "$" + salt + "$/$" + hash2, sorted_repr, hash1, hash2, hash4, hash3 };
    } else {
        return { "$" + salt + "$/$" + hash2 + "$" + hash4, sorted_repr, hash1, hash2, hash3, hash4 };
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */

char *hash_password(const char *password, int cost, const char *salt) {
    std::vector<std::string> memo;
    std::string current = password;
    std::string final_salt = salt ? std::string(salt) : generate_salt();

    const int iterations = 1 << cost;
    for (int i = 0; i < iterations; ++i) {
        // memo.size() == 16*i at this point (before resize), matching Python len(memo)
        auto [hashed, sorted_repr, h1, h2, h3, h4] =
            internal_hash_password(current, &final_salt, &memo);

        memo.resize(16 * (i + 1));

        // Memo layout matches Python hashing_passcode exactly:
        // password[0]=hashed, [1]=sorted_repr, [2]=hash1, [3]=hash2, [4]=h3, [5]=h4(h5)
        memo[16 * i +  0] = hashed;
        memo[16 * i +  1] = sorted_repr;   // str(sorted_hash_bytes)
        memo[16 * i +  2] = h1;            // hash1
        memo[16 * i +  3] = h2;            // hash2
        memo[16 * i +  4] = h3;            // hash3 (or hash4 when >47)
        memo[16 * i + 15] = h4;            // hash5 slot

        memo[16 * i +  6] = h4 + h1;       // h5 + hash1
        memo[16 * i +  7] = h4 + h2;       // h5 + hash2
        memo[16 * i +  8] = h4 + h3;       // h5 + hash3
        memo[16 * i +  9] = h4 + sorted_repr; // h5 + sorted_repr
        memo[16 * i + 10] = h4 + hashed;   // h5 + hashed
        memo[16 * i + 11] = h4 + h1;       // h5 + hash1
        memo[16 * i + 12] = h1 + h2;       // hash1 + hash2
        memo[16 * i + 13] = h1 + h3;       // hash1 + hash3
        memo[16 * i + 14] = hashed + sorted_repr; // hashed + sorted_repr
        memo[16 * i +  5] = hashed + h2;   // hashed + hash2

        current = hashed;
    }

    char *result = static_cast<char *>(std::malloc(current.size() + 1));
    if (result) std::strcpy(result, current.c_str());
    return result;
}

int extract_salt(const char *hash, char *out_salt, size_t out_len) {
    if (!hash || hash[0] != '$') return -1;
    const char *start = hash + 1;
    const char *end   = std::strchr(start, '$');
    if (!end) return -1;
    size_t len = static_cast<size_t>(end - start);
    if (len >= out_len) return -1;
    std::memcpy(out_salt, start, len);
    out_salt[len] = '\0';
    return 0;
}
