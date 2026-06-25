#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hash a password using the custom iterative algorithm.
 *
 * @param password  Null-terminated password string.
 * @param cost      Work factor: iterations = 2^cost (recommended: 8-14).
 * @param salt      16-char null-terminated salt string, or NULL for random.
 * @return          Heap-allocated string "$salt$/$hash[...]", caller must free().
 *                  Returns NULL on allocation failure.
 */
char *hash_password(const char *password, int cost, const char *salt);

/**
 * Extract the salt embedded in a hash string produced by hash_password().
 *
 * @param hash      Hash string in the format "$salt$/$..."
 * @param out_salt  Buffer to receive the salt (at least 32 bytes recommended).
 * @param out_len   Size of out_salt buffer.
 * @return          0 on success, -1 on malformed input.
 */
int extract_salt(const char *hash, char *out_salt, size_t out_len);

#ifdef __cplusplus
}
#endif
