/**
 * primitives.cpp -- Low-level bit-manipulation cipher primitives.
 *
 * Implements the rotate+XOR building blocks used by encrypt_bytes/decrypt_bytes.
 * Algorithm is identical to the Python reference in encryptionfileutils.py:
 *
 *   rotate_left / rotate_right:
 *     big-endian integer rotation (matches Python int.from_bytes(...,'big'))
 *
 *   byte_manipulations / byte_manipulations_reverse:
 *     iterations = 10 + (iterat % 6)
 *     n = key[(key[1]+key[2]) % (key_len-5)] % 7
 *     forward:  for i in range(iterations): rotate_left(1+(n+i)%7), xor
 *     reverse:  for i in range(iterations-1, -1, -1): xor, rotate_right(1+(n+i)%7)
 */

#include "enigma/primitives.h"

#include <algorithm>
#include <cstring>
#include <vector>

/* ── rotate_left ─────────────────────────────────────────────────────────── */

void rotate_left(uint8_t *data, size_t len, int k) {
    if (len == 0 || k == 0) return;
    k = k % static_cast<int>(len * 8);
    if (k < 0) k += static_cast<int>(len * 8);

    std::vector<uint8_t> temp(data, data + len);
    uint64_t int_val = 0;
    for (size_t i = 0; i < len && i < 8; i++)
        int_val = (int_val << 8) | temp[i];

    if (len <= 8) {
        size_t n = len * 8;
        uint64_t rotated = ((int_val << k) | (int_val >> (n - k))) & ((1ULL << n) - 1);
        for (int i = static_cast<int>(len) - 1; i >= 0; i--) {
            data[i] = rotated & 0xFF;
            rotated >>= 8;
        }
    } else {
        std::vector<uint8_t> result(len);
        size_t byte_shift = static_cast<size_t>(k) / 8;
        int    bit_shift  = k % 8;
        for (size_t i = 0; i < len; i++)
            result[i] = temp[(i + len - byte_shift) % len];
        if (bit_shift > 0) {
            uint8_t carry = 0;
            for (size_t i = 0; i < len; i++) {
                uint8_t new_carry = result[i] >> (8 - bit_shift);
                result[i] = (result[i] << bit_shift) | carry;
                carry = new_carry;
            }
            result[0] |= carry;
        }
        std::memcpy(data, result.data(), len);
    }
}

/* ── rotate_right ────────────────────────────────────────────────────────── */

void rotate_right(uint8_t *data, size_t len, int k) {
    if (len == 0 || k == 0) return;
    k = k % static_cast<int>(len * 8);
    if (k < 0) k += static_cast<int>(len * 8);

    std::vector<uint8_t> temp(data, data + len);
    uint64_t int_val = 0;
    for (size_t i = 0; i < len && i < 8; i++)
        int_val = (int_val << 8) | temp[i];

    if (len <= 8) {
        size_t n = len * 8;
        uint64_t rotated = ((int_val >> k) | (int_val << (n - k))) & ((1ULL << n) - 1);
        for (int i = static_cast<int>(len) - 1; i >= 0; i--) {
            data[i] = rotated & 0xFF;
            rotated >>= 8;
        }
    } else {
        std::vector<uint8_t> result(len);
        size_t byte_shift = static_cast<size_t>(k) / 8;
        int    bit_shift  = k % 8;
        for (size_t i = 0; i < len; i++)
            result[i] = temp[(i + byte_shift) % len];
        if (bit_shift > 0) {
            uint8_t carry = 0;
            for (int i = static_cast<int>(len) - 1; i >= 0; i--) {
                uint8_t new_carry = result[i] << (8 - bit_shift);
                result[i] = (result[i] >> bit_shift) | carry;
                carry = new_carry;
            }
            result[len - 1] |= carry;
        }
        std::memcpy(data, result.data(), len);
    }
}

/* ── xor_bytes ───────────────────────────────────────────────────────────── */

void xor_bytes(uint8_t *data, size_t data_len,
               const uint8_t *key,  size_t key_len) {
    for (size_t i = 0; i < data_len; i++)
        data[i] ^= key[i % key_len];
}

/* ── byte_manipulations (encrypt direction) ──────────────────────────────── */

void byte_manipulations(uint8_t *data, size_t data_len,
                        const uint8_t *key, size_t key_len, int iterat) {
    if (key_len < 6) return;
    iterat = 10 + (iterat % 6);
    int n = key[(key[1] + key[2]) % (key_len - 5)] % 7;
    for (int i = 0; i < iterat; i++) {
        rotate_left(data, data_len, 1 + ((n + i) % 7));
        xor_bytes(data, data_len, key, key_len);
    }
}

/* ── byte_manipulations_reverse (decrypt direction) ─────────────────────── */

void byte_manipulations_reverse(uint8_t *data, size_t data_len,
                                const uint8_t *key, size_t key_len, int iterat) {
    if (key_len < 6) return;
    iterat = 10 + (iterat % 6);
    int n = key[(key[1] + key[2]) % (key_len - 5)] % 7;
    for (int i = iterat - 1; i >= 0; i--) {
        xor_bytes(data, data_len, key, key_len);
        rotate_right(data, data_len, 1 + ((n + i) % 7));
    }
}
