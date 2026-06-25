# Python Integration Guide

The `enigma` package provides a high-performance Python interface to Enigma's password hashing and symmetric file encryption.

---

## Hashing & Verification

### `hash_password`
```python
def hash_password(password: str, cost: int = 10, salt: str | None = None) -> str
```
| Parameter | Default | Description |
|---|---|---|
| `password` | required | The password string to hash. |
| `cost` | `10` | Work factor — iterations = 2^cost. Range: 0–31. |
| `salt` | `None` | Fixed 16-char salt. If `None`, a CSPRNG salt is generated. |

Returns `$<salt>$/$<hash>`. Raises `ValueError` for invalid `cost` or `salt` length.

### `extract_salt`
```python
def extract_salt(hash_str: str) -> str
```
Returns the 16-character salt embedded in a hash string. Raises `ValueError` for malformed input.

### Example
```python
import enigma

# Hash (random salt)
hashed = enigma.hash_password("MySecret123", cost=10)

# Verify login
salt = enigma.extract_salt(hashed)
ok = enigma.hash_password("MySecret123", cost=10, salt=salt) == hashed
assert ok
```

---

## In-Memory Byte Encryption

### `encrypt_bytes`
```python
def encrypt_bytes(data: bytes, password: str, cost: int = 10) -> bytes
```

### `decrypt_bytes`
```python
def decrypt_bytes(data: bytes, password: str) -> bytes
```
Raises `ValueError` on wrong password or malformed data.

### Example
```python
import enigma

payload = b"Top secret database connection string."
encrypted = enigma.encrypt_bytes(payload, "DBPassword123")
decrypted = enigma.decrypt_bytes(encrypted, "DBPassword123")
assert decrypted == payload
```

---

## File Encryption

Streams data in configurable chunks — **files larger than RAM are fully supported.** The GIL is released during file operations, so multiple `encrypt_file` calls run in true parallel from Python threads.

### `encrypt_file`
```python
def encrypt_file(
    input_file:  str,
    output_file: str,
    password:    str,
    cost:        int = 10,
    threads:     int = 1,
    io_buf_mb:   int = 4,
) -> None
```

| Parameter | Default | Description |
|---|---|---|
| `input_file` | required | Path to the plaintext file. |
| `output_file` | required | Path for the encrypted output. |
| `password` | required | Encryption password. |
| `cost` | `10` | Key-derivation work factor (iterations = 2^cost). |
| `threads` | `1` | Worker threads for cipher block passes. `0` = all CPU cores. |
| `io_buf_mb` | `4` | I/O buffer size in MiB. Only this much data lives in RAM at once. |

Raises `RuntimeError` on I/O failure.

### `decrypt_file`
```python
def decrypt_file(
    input_file:  str,
    output_file: str,
    password:    str,
    threads:     int = 1,
    io_buf_mb:   int = 4,
) -> None
```

Raises `ValueError` on wrong password, `RuntimeError` on I/O failure.

### Examples

```python
import enigma

# Basic (backward-compatible, same as before)
enigma.encrypt_file("config.json", "config.json.enc", "KeyPassphrase")
enigma.decrypt_file("config.json.enc", "config_restored.json", "KeyPassphrase")

# Encrypt a large file with 8 threads and a 16 MiB buffer
enigma.encrypt_file("huge.iso", "huge.iso.enc", "KeyPassphrase",
                    threads=8, io_buf_mb=16)

# Use all CPU cores (threads=0)
enigma.encrypt_file("huge.iso", "huge.iso.enc", "KeyPassphrase", threads=0)
```

### Parallel batch encryption via Python threads

Because the GIL is released during file operations, you can encrypt multiple files simultaneously:

```python
import enigma
import threading

files = [("a.mp4", "a.mp4.enc"), ("b.mp4", "b.mp4.enc"), ("c.mp4", "c.mp4.enc")]
threads = [
    threading.Thread(
        target=enigma.encrypt_file,
        args=(src, dst, "password"),
        kwargs={"threads": 4, "io_buf_mb": 8}
    )
    for src, dst in files
]
for t in threads: t.start()
for t in threads: t.join()
```
