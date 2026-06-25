# Enigma Developer Portal

Welcome to the **Enigma Developer Portal**. This section contains architecture details, algorithm mathematics, build instructions, and testing frameworks designed for contributors and auditors.

---

## 🛠️ Development Philosophy

* **Single Source of Truth**: All cryptographic calculations (hashing, salting, encryption, decryption) live strictly inside the C++ core library target. CLI and Python bindings link this code statically.
* **Zero External Dependencies**: The repository must not depend on external dynamic binaries (such as OpenSSL) to ensure portability.
* **Strict Cross-Check Verification**: Tests must verify that CLI commands and Python binding returns match outputs identically on all inputs.

---

## 📖 Developer Guides

Explore the following sections to understand or contribute to the project:

1. **[Architecture Walkthrough](architecture.md)**: Diagrams the component interactions between C++, pybind11, pytest, and deployment pipelines. Includes a map of where source code files reside.
2. **[Cryptographic Algorithms](algorithms.md)**: Deep dive into the mathematical formulas of the sub-hashes, index permutations, memo lookup tables, and bitwise whole-buffer block rotations.
3. **[Compiling & Testing Setup](build.md)**: Detailed steps to set up local compilers (MSVC, GCC, Clang), configure CMake, compile binaries, and execute testing suites (`ctest` and `pytest`).
