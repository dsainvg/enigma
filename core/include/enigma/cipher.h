#pragma once
#include <stddef.h>
#include <stdint.h>

// Convenience: include primitives so callers only need cipher.h
#include "enigma/primitives.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Encrypt a raw byte buffer into ENIGMA01 format.
 *
 * Output layout (all lengths little-endian uint16):
 *   [8]          magic "ENIGMA01"
 *   [2]          salt_len
 *   [salt_len]   salt (random per call)
 *   [2]          cost
 *   [2]          verify_len
 *   [verify_len] hash-of-hash (password verification tag)
 *   [rest]       encrypted data in 1 KB sub-chunks
 *
 * @param in        Plaintext bytes (may be NULL for empty input).
 * @param in_len    Length of input.
 * @param password  Password string.
 * @param cost      Work factor for hash_password (iterations = 2^cost).
 * @param out       Receives pointer to heap-allocated ciphertext (caller free()s).
 * @param out_len   Receives length of *out.
 * @return  0 on success, -1 on error.
 */
int encrypt_bytes(const uint8_t *in, size_t in_len,
                  const char *password, int cost,
                  uint8_t **out, size_t *out_len);

/**
 * Decrypt a buffer produced by encrypt_bytes().
 *
 * @param in        Ciphertext bytes.
 * @param in_len    Length of input.
 * @param password  Password string.
 * @param out       Receives pointer to heap-allocated plaintext (caller free()s).
 * @param out_len   Receives length of *out.
 * @return  0 on success, -1 on format/I/O error, -2 on wrong password.
 */
int decrypt_bytes(const uint8_t *in, size_t in_len,
                  const char *password,
                  uint8_t **out, size_t *out_len);

/**
 * Encrypt a file.  Output is written to `output_file` in ENIGMA01 format.
 * @return  0 success, -1 I/O error.
 */
int encrypt_file(const char *input_file, const char *output_file,
                 const char *password, int cost);

/**
 * Decrypt a file produced by encrypt_file().
 * @return  0 success, -1 I/O error, -2 wrong password.
 */
int decrypt_file(const char *input_file, const char *output_file,
                 const char *password);

#ifdef __cplusplus
}
#endif
