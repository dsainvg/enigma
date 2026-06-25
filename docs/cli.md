# Command Line Interface (CLI)

The native command-line utility provides terminal-based operations to hash passwords, encrypt files, and decrypt files.

---

## 🛠️ Global Command Syntax

The executable (`enigma` or `enigma.exe`) is invoked as follows:

```bash
enigma <command> [arguments...]
```

Exposed commands:
* **`hash`**: Hash a password credentials string.
* **`encrypt`**: Encrypt any file.
* **`decrypt`**: Decrypt an encrypted file.

---

## 🔑 `hash`

Generate an Enigma password hash.

### Syntax
```bash
enigma hash <password> [cost] [salt]
```

### Arguments
* **`<password>`** (Required): The password string to hash.
* **`[cost]`** (Optional, default `10`): The work factor factor ($2^{\text{cost}}$ iterations). Must be between `0` and `31`.
* **`[salt]`** (Optional): A 16-character salt string. If omitted, a cryptographically secure random salt is generated.

### Examples
```bash
# Hash a password using default cost 10 and a random salt:
enigma hash "mySecurePassword"

# Hash a password with custom cost 12:
enigma hash "mySecurePassword" 12

# Hash a password using a fixed salt:
enigma hash "mySecurePassword" 10 "fixedSalt1234567"
```

---

## 🔒 `encrypt`

Encrypt any file. By default, it outputs a file with an `.enc` extension using the custom `ENIGMA01` binary format.

### Syntax
```bash
enigma encrypt <file> <password> [cost] [output_file]
```

### Arguments
* **`<file>`** (Required): Path to the input file to encrypt.
* **`<password>`** (Required): The encryption password.
* **`[cost]`** (Optional, default `10`): Hashing cost factor used for deriving the encryption key.
* **`[output_file]`** (Optional): Path to write the encrypted output. If omitted, defaults to `<file>.enc`.

### Examples
```bash
# Encrypts 'data.json' -> 'data.json.enc'
enigma encrypt data.json "SecretPassphrase"

# Encrypts 'data.json' -> 'archive.bin' with custom cost 12
enigma encrypt data.json "SecretPassphrase" 12 archive.bin
```

---

## 🔓 `decrypt`

Decrypt any `ENIGMA01` formatted file.

### Syntax
```bash
enigma decrypt <file> <password> [output_file]
```

### Arguments
* **`<file>`** (Required): Path to the encrypted file.
* **`<password>`** (Required): Decryption password.
* **`[output_file]`** (Optional): Path to save the decrypted output. If omitted, defaults to `<file>.dec`.

### Examples
```bash
# Decrypts 'archive.bin' -> 'archive.bin.dec'
enigma decrypt archive.bin "SecretPassphrase"

# Decrypts 'archive.bin' -> 'restored_data.json'
enigma decrypt archive.bin "SecretPassphrase" restored_data.json
```
