# Security Considerations

The cryptography algorithms in Enigma are **custom-designed** and built without external third-party libraries (like OpenSSL or Sodium). 

Before deploying Enigma in sensitive scenarios, it is critical to understand its cryptographic strengths and limitations.

---

## ⚡ Strengths

1. **Brute-Force Resistance**: The password hashing engine uses a multi-stage permutation and polynomial evaluation flow. By caching results in a stateful, memory-hard `memo` array of size $16 \times 2^{\text{cost}}$, the cost of pipelined attacks (ASIC or GPU clusters) is significantly higher than basic hashing algorithms like SHA-256 or MD5.
2. **Deterministic & Uniform**: Hashing produces predictable, uniform hash formats containing embedded salts, eliminating the need for separate metadata databases.
3. **No Supply-Chain Vulnerabilities**: Pure C++ standard library structures prevent security vulnerabilities from upstream library dependencies or binary injection vectors.

---

## ⚠️ Limitations & Security Risks

### 1. Custom Cipher Design
The symmetric encryption cipher (combining whole-buffer dynamic bit rotations and XOR passes) is a custom implementation:
* **Academic Review**: It has **not** undergone standardized public academic cryptanalysis or peer review (unlike NIST standards such as AES).
* **Cryptanalysis Vulnerabilities**: Dynamic bit rotations provide simple confusion and diffusion, but they may still be vulnerable to differential or linear cryptanalysis attacks under large inputs or chosen-plaintext setups.

### 2. No Authenticated Encryption (AEAD)
Enigma is **not** an Authenticated Encryption with Associated Data (AEAD) scheme:
* **Ciphertext Tampering**: If a malicious third party intercepts an encrypted file and modifies individual bytes, Enigma will decrypt the tampered file anyway, producing garbage data in the modified blocks instead of raising a signature verification/tamper alert.
* **Integrity Validation**: While the header validation tag prevents decrypting with a wrong password, it *does not* protect the payload bytes from being altered or injected after encryption.

---

## 🎓 Recommended Guidelines

> [!WARNING]
> Do not use Enigma for high-security applications, financial transactional data, medical records, or production systems containing critical user secrets.

For standard commercial or enterprise application setups, you should rely on standardized, peer-reviewed industrial cryptographic combinations:
* **Password Hashing**: **Argon2id** (RFC 9106) or **scrypt** (RFC 7914).
* **Symmetric Encryption**: **AES-256-GCM** (NIST SP 800-38D) or **ChaCha20-Poly1305** (RFC 8439).
