# Security Considerations & Warnings

The cryptography algorithms in Enigma are **custom-designed** and written without external, peer-reviewed industrial libraries like OpenSSL, Libsodium, or BoringSSL.

Before using Enigma, it is critical to understand its security properties and target use cases.

---

## ⚡ Strengths

* **Iterative Work Factor**: By utilizing a work factor (`cost`), key derivation requires $2^{\text{cost}}$ iterations. This creates computational delays, defending against quick automated dictionary/brute-force attacks.
* **Memory-Hard Hashing**: Intermediate rounds reference a stateful lookup matrix (`memo`). This forces the hashing runner to utilize RAM, reducing the efficiency of custom FPGA/GPU parallel cracking setups.
* **No Upstream Dependencies**: A zero-dependency model reduces supply chain threat vectors and eliminates binary loading or dynamic linking issues.

---

## ⚠️ Limitations & Cryptanalysis Warnings

### 1. Custom Cryptographic Algorithms
* **No Academic Peer Review**: Unlike standard algorithms (such as AES or ChaCha20), Enigma's bitwise whole-buffer rotation and XOR cipher has not undergone extensive academic cryptanalysis or peer evaluation. 
* **Potential Flaws**: Custom encryption schemes are highly susceptible to mathematical weaknesses, shortcut attacks, or linear/differential cryptanalysis that could compromise keys or plaintext.

### 2. Lack of Authenticated Encryption (AEAD)
* **No Integrity Signature**: Enigma does not support Authenticated Encryption (AEAD) or message authentication codes (MACs). 
* **Ciphertext Tampering**: If an attacker intercepts your encrypted file and alters bits within the ciphertext, Enigma's decryption engine will not raise a signature validation error. It will proceed to decrypt the altered bytes anyway, producing corrupted/garbage data in the modified segments. Only password verification tags at the header level are validated.

---

## 🎓 Recommended Guidelines

> [!WARNING]
> Do not use Enigma to secure high-value commercial systems, financial data, health records, or authentication backends.

For standard production systems requiring certified security, you should use industry-standard ciphers:
* **Symmetric Encryption**: Use **AES-GCM-256** or **ChaCha20-Poly1305**.
* **Password Hashing**: Use **Argon2id** or **scrypt**.
* **Library Options**: Standard production libraries include `PyNaCl` (Libsodium) or Python's native `cryptography` package.
