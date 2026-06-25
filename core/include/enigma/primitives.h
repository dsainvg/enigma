#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Rotate all bits in `data` left by `k` positions (big-endian bit order). */
void rotate_left(uint8_t *data, size_t len, int k);

/** Rotate all bits in `data` right by `k` positions (big-endian bit order). */
void rotate_right(uint8_t *data, size_t len, int k);

/** XOR `data` with the repeating `key` (key cycles if shorter than data). */
void xor_bytes(uint8_t *data, size_t data_len,
               const uint8_t *key, size_t key_len);

/**
 * Apply iterated (rotate-left then XOR) pass — encryption direction.
 * iterations = 10 + (iterat % 6), rotation derived from key content.
 * No-op if key_len < 6.
 */
void byte_manipulations(uint8_t *data, size_t data_len,
                        const uint8_t *key, size_t key_len, int iterat);

/**
 * Apply iterated (XOR then rotate-right) pass — decryption direction.
 * Exact inverse of byte_manipulations.
 * No-op if key_len < 6.
 */
void byte_manipulations_reverse(uint8_t *data, size_t data_len,
                                const uint8_t *key, size_t key_len, int iterat);

#ifdef __cplusplus
}
#endif
