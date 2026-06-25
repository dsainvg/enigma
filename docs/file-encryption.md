# File Encryption

Enigma provides file-based encryption that secures files of any size by breaking them down into small, manageable chunks and wrapping them in the custom `ENIGMA01` binary format.

---

## 💾 The `ENIGMA01` File Format

Encrypted files are structured with a custom binary header containing metadata for key derivation and integrity checks, followed by the encrypted payload.

| Offset (Bytes) | Size (Bytes) | Field Name | Description |
| :--- | :--- | :--- | :--- |
| `0` | `8` | **Magic Header** | Must match the ASCII bytes `"ENIGMA01"` |
| `8` | `2` (LE) | **Salt Length** | Length of the salt string (always 16) |
| `10` | `16` | **Salt** | Printable ASCII characters used for hashing |
| `26` | `2` (LE) | **Cost** | Hashing work factor (cost) |
| `28` | `2` (LE) | **Verify Length** | Length of the validation tag |
| `30` | Variable | **Verify Tag** | Hash-of-hash string for quick password checking |
| `30 + TagLen` | Remaining | **Encrypted Data** | Sequentially encrypted data chunks (1 KB each) |

> [!IMPORTANT]
> `LE` denotes Little-Endian byte order. The verification tag allows the cipher to immediately determine if the user provided the correct password before modifying files or allocating memory for decryption.

---

## 🐍 Python Usage

```python
import enigma

# Encrypt a file (default cost=10)
enigma.encrypt_file("secrets.txt", "secrets.enc", "MyPassword123")

# Decrypt the file back to plaintext
enigma.decrypt_file("secrets.enc", "secrets_decrypted.txt", "MyPassword123")

# Wrong password check:
try:
    enigma.decrypt_file("secrets.enc", "secrets_decrypted.txt", "WrongPassword")
except ValueError as e:
    print("Decryption failed:", e) # Wrong password
```

---

## 💻 CLI Usage

```bash
# Encrypt 'document.pdf' to 'document.pdf.enc' using password and cost 12
enigma encrypt document.pdf "MyPassword123" 12

# Encrypt specifying a custom output filename:
enigma encrypt document.pdf "MyPassword123" 12 secure_document.enc

# Decrypt the document:
enigma decrypt secure_document.enc "MyPassword123"

# Decrypt specifying a custom output filename:
enigma decrypt secure_document.enc "MyPassword123" restored_document.pdf
```
