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

#ifdef __cplusplus
}

/**
 * Streaming + multi-threaded file encryption.
 *
 * Writes the file in streaming `io_buf_mb`-MiB chunks so memory usage is
 * bounded regardless of file size.  Independent 1 KB cipher blocks inside
 * each buffer are distributed across `thread_count` worker threads.
 *
 * The on-disk ENIGMA01 format is unchanged — files are fully compatible
 * with earlier builds.
 *
 * @param input_file   Path to plaintext input file.
 * @param output_file  Path for encrypted output file.
 * @param password     Password string.
 * @param cost         Key-derivation work factor (iterations = 2^cost).
 * @param thread_count Worker threads for cipher block passes.
 *                     0 = std::thread::hardware_concurrency(); 1 = single.
 * @param io_buf_mb    I/O buffer size in MiB (minimum 1).
 * @return  0 success, -1 I/O error.
 */
int encrypt_file(const char *input_file, const char *output_file,
                 const char *password, int cost,
                 int thread_count = 1, int io_buf_mb = 4);

/**
 * Streaming + multi-threaded file decryption.
 *
 * @param input_file   Path to encrypted file (ENIGMA01 format).
 * @param output_file  Path for decrypted output file.
 * @param password     Password string.
 * @param thread_count Worker threads for cipher block passes.  0 = auto.
 * @param io_buf_mb    I/O buffer size in MiB (minimum 1).
 * @return  0 success, -1 I/O error, -2 wrong password.
 */
int decrypt_file(const char *input_file, const char *output_file,
                 const char *password,
                 int thread_count = 1, int io_buf_mb = 4);
#endif
