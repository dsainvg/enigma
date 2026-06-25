# enigma-encryption

Custom password hashing and file encryption — a C++ monorepo that builds:

- 🐍 **`enigma-encryption` Python library** → published to [PyPI](https://pypi.org/project/enigma-encryption/)
- 💻 **`enigma` CLI tool** → published to [GitHub Releases](../../releases)

All hash and cipher logic lives **exactly once** in `core/`, shared by both outputs.

---

## Architecture

```
enigma/
├── core/               # ✅ Single source of truth
│   ├── include/enigma/
│   │   ├── hash.h      # hash_password() API
│   │   ├── cipher.h    # encrypt/decrypt API
│   │   └── primitives.h # Shared internal byte primitives
│   └── src/
│       ├── hash.cpp    # Custom iterative hash algorithm
│       ├── salt.cpp    # CSPRNG + salt generation
│       ├── primitives.cpp # Shared bitwise rotation/manipulation
│       ├── cipher.cpp  # Symmetric encrypt/decrypt of byte buffers
│       └── file_io.cpp # Chunked file encryption/decryption
├── cli/                # C++ CLI → GitHub Releases
├── python/             # pybind11 bindings → PyPI
├── tests/
│   ├── cpp/            # C++ test suite (hash + cipher tests)
│   └── python/         # pytest suite (50+ tests)
├── CMakeLists.txt
├── pyproject.toml      # scikit-build-core configuration
└── .github/workflows/
    ├── ci.yml           # Test on push (Linux, macOS, Windows)
    ├── release-cli.yml  # CLI → GitHub Releases (on tag)
    └── publish-pypi.yml # Python wheel → PyPI (on tag)
```

---

## CLI

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Hash a password
./build/cli/enigma hash mypassword
./build/cli/enigma hash mypassword 12 myFixedSalt1234

# Encrypt / decrypt files
./build/cli/enigma encrypt secrets.txt mypassword
./build/cli/enigma decrypt secrets.txt.enc mypassword
```

**Cost factor** controls iterations (2^cost). Default is 10 (1024 rounds).  
Recommended range: 8–14.

**File format**: Custom binary `enigma001` — no system dependencies.

---

## Python Library

### Install

```bash
pip install enigma-encryption
```

### Usage

```python
import enigma

# Hash a password (random salt generated automatically)
h = enigma.hash_password("mysecret", cost=10)
print(h)  # $aBcD1234567890$/$XYZ…

# Verify later — extract salt and re-hash
salt = enigma.extract_salt(h)
assert enigma.hash_password("mysecret", cost=10, salt=salt) == h

# Encrypt / decrypt bytes
data = b"my secret data"
enc = enigma.encrypt_bytes(data, "password", cost=10)
dec = enigma.decrypt_bytes(enc, "password")
assert dec == data

# Encrypt / decrypt files
enigma.encrypt_file("secret.txt", "secret.enc", "password", cost=10)
enigma.decrypt_file("secret.enc", "secret.txt", "password")
```

---

## Algorithm

**Hashing** (`hash_password`):
- Multi-stage iterative algorithm: hash1 → hash2 → hash3 → hash4
- Nth-element byte cycling (custom permutation)
- Latin-1 → UTF-8 encoding conversion within the hash
- Memory-hard: each iteration reads from a 16×N memoization table
- Output: `$<salt>$/$<hash>` — salt is always embedded

**Cipher** (`encrypt_bytes` / `encrypt_file`):
- Password → `hash_password(password, cost, random_salt)` → key
- Key → `hash_password(key, cost, salt)` → verification hash
- Data: iterated `rotate_left + XOR` in 1 KB sub-chunks
- File format: `enigma001` magic + salt + cost + verification hash + ciphertext

---

## Testing

```bash
# C++ tests
cmake -B build && cmake --build build --parallel
cd build && ctest --output-on-failure

# Python tests
pip install . -v
python -m pytest tests/python/ -v
```

---

## Release

Tag a version to trigger both pipelines:

```bash
git tag v1.0.0
git push --tags
```

- **GitHub Actions** builds CLI binaries for Linux, macOS, Windows → GitHub Release
- **GitHub Actions** builds Python wheels for all platforms → PyPI

---

## PyPI Setup (one-time)

1. Register at [pypi.org](https://pypi.org)
2. Go to **Account → Publishing → Add a new pending publisher**
3. Fill in: GitHub owner, repo `enigma`, workflow `publish-pypi.yml`, env `pypi`
4. Push a tag — no secrets needed (Trusted Publishing via OIDC)

---

## License

MIT
