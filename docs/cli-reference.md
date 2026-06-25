# CLI Command Reference

The native C++ command-line utility provides direct access to all hashing and encryption capabilities of the `enigma_core` engine.

---

## 🛠️ Global Command Structure

```bash
enigma <command> [arguments...]
```

| Command | Purpose |
| :--- | :--- |
| **`hash`** | Compute a password hash with optional cost and salt values. |
| **`encrypt`** | Encrypt an input file using a password key. |
| **`decrypt`** | Decrypt a previously encrypted file. |

---

## 🔑 `hash`

Generate an Enigma-format hash from a password string.

### Syntax
```bash
enigma hash <password> [cost=10] [salt]
```

### Parameters
* **`<password>`** (Required): The password string to hash (UTF-8).
* **`[cost]`** (Optional, default `10`): Iteration work factor ($2^{\text{cost}}$ rounds). Must be between `0` and `31`.
* **`[salt]`** (Optional): A custom 16-character salt. If not provided, a random printable ASCII salt is generated.

### Examples
```bash
# Basic usage (cost=10, random salt)
enigma hash "SuperSecretString"

# Explicit cost (cost=12, random salt)
enigma hash "SuperSecretString" 12

# Explicit cost and fixed salt
enigma hash "SuperSecretString" 8 "myFixedSalt12345"
```

---

## 🔒 `encrypt`

Encrypt any file and save it in the `ENIGMA01` binary format.

### Syntax
```bash
enigma encrypt <file> <password> [cost=10] [output_file]
```

### Parameters
* **`<file>`** (Required): Path to the target file to encrypt.
* **`<password>`** (Required): Secret password string.
* **`[cost]`** (Optional, default `10`): Hashing cost factor for key derivation.
* **`[output_file]`** (Optional): The destination file path. Defaults to `<file>.enc` if omitted.

### Examples
```bash
# Encrypts notes.txt -> notes.txt.enc
enigma encrypt notes.txt "MyPassword123"

# Encrypts notes.txt -> secure.bin with custom cost 12
enigma encrypt notes.txt "MyPassword123" 12 secure.bin
```

---

## 🔓 `decrypt`

Decrypt an `ENIGMA01` formatted file.

### Syntax
```bash
enigma decrypt <file> <password> [output_file]
```

### Parameters
* **`<file>`** (Required): Path to the encrypted file.
* **`<password>`** (Required): Password used for decryption verification.
* **`[output_file]`** (Optional): The destination file path. Defaults to `<file>.dec` if omitted.

### Examples
```bash
# Decrypts notes.txt.enc -> notes.txt.enc.dec
enigma decrypt notes.txt.enc "MyPassword123"

# Decrypts secure.bin -> restored.txt
enigma decrypt secure.bin "MyPassword123" restored.txt
```
