# Command Line Interface (CLI)

The native command-line utility provides terminal-based operations to hash passwords, encrypt files, and decrypt files.

---

## Global Syntax

```bash
enigma <command> [arguments...] [--flag=value ...]
```

Commands: **`hash`**, **`encrypt`**, **`decrypt`**

---

## `hash`

Generate an Enigma password hash.

### Syntax
```bash
enigma hash <password> [--cost=10] [--salt=<str>]
```

### Options
| Flag | Default | Description |
|---|---|---|
| `--cost=N` | `10` | Key-derivation work factor. Iterations = 2^N. Range: 0–31. |
| `--salt=S` | random | Fixed 16-character salt. If omitted, a CSPRNG salt is generated. |

### Examples
```bash
# Hash with defaults (random salt, cost 10)
enigma hash "mySecurePassword"

# Hash with cost 12 (4× slower, stronger)
enigma hash "mySecurePassword" --cost=12

# Hash with a fixed salt (for reproducibility)
enigma hash "mySecurePassword" --cost=10 --salt="fixedSalt1234567"
```

---

## `encrypt`

Encrypt any file. Streams data in configurable chunks — **files larger than RAM are fully supported.**

### Syntax
```bash
enigma encrypt <file> <password> [--cost=10] [--threads=1] [--buffer-mb=4] [--out=<path>]
```

### Options
| Flag | Default | Description |
|---|---|---|
| `--cost=N` | `10` | Key-derivation work factor (iterations = 2^N). |
| `--threads=N` | `1` | Worker threads for cipher block passes. `0` = all CPU cores. |
| `--buffer-mb=N` | `4` | I/O buffer size in MiB. Only this much data is held in RAM at once. |
| `--out=<path>` | `<file>.enc` | Output file path. |

### Examples
```bash
# Basic: encrypts data.json -> data.json.enc
enigma encrypt data.json "SecretPassphrase"

# Custom output, higher cost
enigma encrypt data.json "SecretPassphrase" --cost=12 --out=archive.bin

# Encrypt a 50 GB file using 8 threads and a 16 MiB buffer
enigma encrypt huge.iso "SecretPassphrase" --threads=8 --buffer-mb=16

# Use all CPU cores automatically
enigma encrypt huge.iso "SecretPassphrase" --threads=0
```

!!! tip "Choosing `--threads` and `--buffer-mb`"
    - For small files (< 1 MB), threads add overhead — keep `--threads=1`.
    - For large files (> 32 MB), `--threads=0` (all cores) delivers the best throughput.
    - `--buffer-mb` controls peak memory usage. On a 4 GB system encrypting a 100 GB file, `--buffer-mb=64` is safe and fast.

---

## `decrypt`

Decrypt any `ENIGMA01` formatted file.

### Syntax
```bash
enigma decrypt <file> <password> [--threads=1] [--buffer-mb=4] [--out=<path>]
```

### Options
| Flag | Default | Description |
|---|---|---|
| `--threads=N` | `1` | Worker threads for cipher block passes. `0` = all CPU cores. |
| `--buffer-mb=N` | `4` | I/O buffer size in MiB. |
| `--out=<path>` | `<file>.dec` | Output file path. |

### Examples
```bash
# Basic decrypt
enigma decrypt archive.bin "SecretPassphrase"

# Decrypt to specific path
enigma decrypt archive.bin "SecretPassphrase" --out=restored_data.json

# High-throughput decrypt with all cores
enigma decrypt huge.iso.enc "SecretPassphrase" --threads=0 --buffer-mb=16
```
