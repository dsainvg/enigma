/**
 * salt.h -- internal header for CSPRNG and salt generation.
 *
 * NOT part of the public API (not installed). Used only by:
 *   hash.cpp    -- generate_salt() for hash_password(salt=NULL)
 *   cipher.cpp  -- generate_cipher_salt() for encrypt_bytes()
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>

/** Fill `buf` with `len` cryptographically random bytes (platform CSPRNG). */
void get_random_bytes(uint8_t *buf, size_t len);

/**
 * Generate a random 16-char printable salt.
 * First char is always an ASCII letter; remaining chars are in [37,126).
 */
std::string generate_salt();

/**
 * Generate a 16-char salt with extra uniqueness guarantee.
 * Mixes CSPRNG bytes with an atomic counter and high-resolution timestamp
 * so two back-to-back calls always produce distinct salts.
 */
std::string generate_cipher_salt();
