# Python API Reference

The `enigma` module is a compiled native C++ extension that provides high-speed cryptography wrappers.

---

## 🔑 Hashing Functions

### `hash_password`
Hashes a password string using the iterative, memory-hard hashing algorithm.

```python
def hash_password(password: str, cost: int = 10, salt: str = None) -> str:
```

* **Parameters**:
  * `password` (`str`): The credential to hash.
  * `cost` (`int`, optional): Work factor ($2^{\text{cost}}$ iterations). Default is `10`.
  * `salt` (`str`, optional): A 16-character salt. If omitted, a secure random salt is generated.
* **Returns**: `str` formatted as `$<salt>$/$<hash>`.
* **Raises**: `ValueError` if `cost` is out of bounds ($0 \dots 31$) or if the custom `salt` is not exactly 16 characters.

---

### `extract_salt`
Extracts the embedded 16-character salt from a formatted Enigma hash string.

```python
def extract_salt(hash_str: str) -> str:
```

* **Parameters**:
  * `hash_str` (`str`): The full Enigma hash string (e.g. `"$J&%Z1n_N/boCGJef$/$aBcD..."`).
* **Returns**: `str` of length 16 (the salt string).
* **Raises**: `ValueError` if the hash string does not conform to the expected format.

---

## 🔒 Encryption & Decryption Functions

### `encrypt_bytes`
Encrypts an in-memory byte buffer.

```python
def encrypt_bytes(data: bytes, password: str, cost: int = 10) -> bytes:
```

* **Parameters**:
  * `data` (`bytes` / `bytearray`): The raw plaintext buffer.
  * `password` (`str`): Password used to derive the encryption key.
  * `cost` (`int`, optional): Work factor used for key derivation. Default is `10`.
* **Returns**: `bytes` containing the `ENIGMA01` binary header and ciphertext.

---

### `decrypt_bytes`
Decrypts an in-memory byte buffer.

```python
def decrypt_bytes(data: bytes, password: str) -> bytes:
```

* **Parameters**:
  * `data` (`bytes` / `bytearray`): The encrypted byte buffer (must contain valid `ENIGMA01` header).
  * `password` (`str`): Password to verify and decrypt the buffer.
* **Returns**: `bytes` containing the decrypted plaintext.
* **Raises**: `ValueError` if password verification fails, or if the header format is corrupted.

---

### `encrypt_file`
Encrypts a file on disk.

```python
def encrypt_file(input_path: str, output_path: str, password: str, cost: int = 10) -> None:
```

* **Parameters**:
  * `input_path` (`str`): Path to the plaintext input file.
  * `output_path` (`str`): Path to save the encrypted output file.
  * `password` (`str`): Password for key derivation.
  * `cost` (`int`, optional): Key derivation work factor. Default is `10`.
* **Raises**: `IOError` if input/output files cannot be opened.

---

### `decrypt_file`
Decrypts a file on disk.

```python
def decrypt_file(input_path: str, output_path: str, password: str) -> None:
```

* **Parameters**:
  * `input_path` (`str`): Path to the encrypted file.
  * `output_path` (`str`): Path to save the decrypted output file.
  * `password` (`str`): Password for verification and decryption.
* **Raises**:
  * `ValueError` if password verification fails.
  * `IOError` if files cannot be opened.
