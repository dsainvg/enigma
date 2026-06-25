# Python Integration Guide

The `enigma` package provides a high-performance Python interface to Enigma's password hashing and symmetric encryption algorithms.

---

## 🔑 Hashing & Verification

Use hashing functions to stretch password credentials and verify user logins.

### Hashing API
```python
def hash_password(password: str, cost: int = 10, salt: str = None) -> str:
```
* **Returns**: A formatted string: `$<salt>$/$<hash>`.
* **Raises**: `ValueError` if `cost` is out of bounds or if `salt` length is not exactly 16.

### Salt Extraction API
```python
def extract_salt(hash_str: str) -> str:
```
* **Returns**: The extracted 16-character salt string.
* **Raises**: `ValueError` if the hash string format is invalid.

### Examples
```python
import enigma

# Hash a password (automatically generates a random salt)
hashed = enigma.hash_password("MySecret123", cost=10)
print("Hash:", hashed) # e.g. $J&%Z1n_N/boCGJef$/$aBcD...

# Verify a password login:
salt = enigma.extract_salt(hashed)
is_correct = enigma.hash_password("MySecret123", cost=10, salt=salt) == hashed
assert is_correct is True
```

---

## 🔒 In-Memory Byte Encryption

Secure byte arrays, variables, or transient memory streams directly in RAM.

### Encryption API
```python
def encrypt_bytes(data: bytes, password: str, cost: int = 10) -> bytes:
```
* **Returns**: `bytes` containing the binary header and the ciphertext payload.

### Decryption API
```python
def decrypt_bytes(data: bytes, password: str) -> bytes:
```
* **Returns**: Decrypted `bytes` buffer.
* **Raises**: `ValueError` if validation tag checks fail (wrong password) or formatting is invalid.

### Examples
```python
import enigma

payload = b"Top secret database connection string."

# Encrypt
encrypted = enigma.encrypt_bytes(payload, "DBPassword123")

# Decrypt
decrypted = enigma.decrypt_bytes(encrypted, "DBPassword123")
assert decrypted == payload
```

---

## 💾 File Encryption

Encrypt and decrypt files directly on disk. Large files are processed in chunks to limit memory footprint.

### Encryption API
```python
def encrypt_file(input_path: str, output_path: str, password: str, cost: int = 10) -> None:
```

### Decryption API
```python
def decrypt_file(input_path: str, output_path: str, password: str) -> None:
```
* **Raises**: `ValueError` if verification fails (wrong password) or `IOError` if files cannot be opened.

### Examples
```python
import enigma

# Secure file
enigma.encrypt_file("config.json", "config.json.enc", "KeyPassphrase")

# Restore file
enigma.decrypt_file("config.json.enc", "config_restored.json", "KeyPassphrase")
```
