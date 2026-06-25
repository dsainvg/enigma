# Byte Buffer Encryption

Enigma allows you to perform in-memory symmetric encryption on arbitrary byte streams or strings using the `encrypt_bytes` and `decrypt_bytes` Python bindings.

---

## 🔬 How In-Memory Cryptography Works

When you encrypt a byte buffer, the library processes the memory buffer in sequential **1 KB blocks** (1024 bytes). For data smaller than 1 KB, the entire buffer is processed as a single block.

### Bitwise Stream Rotation
Unlike standard block ciphers (e.g., AES) that process data in fixed 16-byte blocks, Enigma performs a bitwise rotation over the *entire block*:

1. **Shift Calculation**: In each round $n$, the rotation shift amount is:
   $$\text{shift} = 1 + ((\text{key\_byte} + n) \pmod 7) \text{ bits}$$
2. **Left Rotation**: Every bit in the 1 KB block is shifted left by the calculated amount. Bits that overflow past byte boundaries are shifted into the neighboring byte. The leftmost bits of the first byte wrap around to the rightmost positions of the last byte.
3. **XOR Blend**: Following the bit rotation, every byte is XORed with the corresponding byte of the derived password hash key.

This double-action (Rotate + XOR) is repeated dynamically for $10 + (\text{block\_index} \pmod 6)$ rounds to ensure excellent avalanche effect across the plaintext.

---

## 🐍 Python Usage

The Python bindings work directly with Python `bytes` or `bytearray` types.

```python
import enigma

plaintext = b"This is a highly confidential string of bytes."

# 1. Encrypt the byte buffer
ciphertext = enigma.encrypt_bytes(plaintext, "MySecretKey", cost=8)
print("Ciphertext size:", len(ciphertext)) # Includes ENIGMA01 header

# 2. Decrypt the byte buffer
decrypted = enigma.decrypt_bytes(ciphertext, "MySecretKey")

assert decrypted == plaintext
print("Success!")
```

### Key Considerations
* **Memory Overhead**: `encrypt_bytes` prefix-prepends the binary header (`ENIGMA01` magic, salt, and verification tag) to the front of the ciphertext. The output size will always be larger than the input size by the size of the header (approximately 600–700 bytes depending on the salt and validation tag representation).
* **Speed**: The entire operation is written in C++ and compiled with compiler optimizations, making in-memory buffer transformations exceptionally fast.
