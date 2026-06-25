# Enigma Documentation Portal

Welcome to the official documentation for **Enigma**, a modern, lightweight cryptography system for secure password hashing and symmetric byte/file encryption.

Enigma is built to deliver native, high-performance cryptographic operations through a secure C++ engine paired with standard Python interfaces.

---

## Key Capabilities

* 🔑 **Secure Password Hashing**: Strengthens user credentials with a memory-hard iteration workflow that defends against GPU-based offline dictionary attacks.
* 🔒 **Symmetric File Cipher**: Encrypts and decrypts files of any size using block-level bit rotation and XORing.
* 📦 **In-Memory Byte Encryption**: Secures variables and raw byte streams directly in-memory using fast Python bindings.
* 💻 **Cross-Platform CLI**: A portable command-line tool compiled for Windows, macOS, and Linux.
* 🐍 **Standard Python Extension**: Native speed combined with Pythonic interfaces and exceptions.
* 🔄 **Cross-Compatibility**: Decrypt files on the CLI that were encrypted using the Python package, and vice-versa.

---

## Project Purpose

Enigma was created to provide a lightweight, auditable cryptography tool that runs out of the box with zero runtime dependencies. 

> [!NOTE]
> Enigma relies on custom-designed encryption and hashing processes. It is designed for educational, utility, and obfuscation use cases, and is **not** recommended for high-security applications where standard AES-GCM or Argon2id systems are required. See the [Security Warnings](security.md) section for full details.
