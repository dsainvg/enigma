# Compiling & Testing Guide

This guide describes how to configure your local development environment, compile the C++ binaries, run the testing suites, and manage python packaging.

---

## 🛠️ Build Prerequisites

Verify that your system has the following tools installed:
*   **CMake** (v3.18 or newer)
*   **C++ Compiler** (supporting standard C++17)
    *   **Windows**: MSVC 2019+ or MinGW-w64 (GCC 8+)
    *   **macOS**: Apple Clang (Xcode 10.3+)
    *   **Linux**: GCC 8+ or Clang 7+
*   **Python** (v3.8 or newer) with `pip` and `virtualenv`

---

## 💻 Local Compilation (C++)

To configure, compile, and run the C++ targets:

```bash
# 1. Configure CMake build tree
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Build CLI and tests in parallel
cmake --build build --parallel
```

The resulting binaries will be created at:
*   **CLI Executable**: `build/cli/enigma` (or `enigma.exe`)
*   **Test Executable**: `build/tests/cpp/hash_tests` and `cipher_tests`

---

## 🧪 Testing Suites

Enigma has parallel C++ and Python verification paths.

### 1. C++ CTest Suite
Runs standard regression tests for the core hashing arrays and block rotation states.

```bash
cd build
ctest --output-on-failure
```

### 2. Python Pytest Suite
Runs verification checks, parameter error boundaries, and algorithm parity checks.

```bash
# Install the package in editable mode
pip install -e . --no-build-isolation

# Run the pytest suite
python -m pytest tests/python/ -v
```

---

## 📦 Python Packaging & Cibuildwheel

Python packaging is managed by `scikit-build-core` as defined in [pyproject.toml](file:///r:/Coding/Projects/Encryption/enigma/pyproject.toml).

When publishing a tag `v*.*.*` to GitHub:
1.  **Cibuildwheel** executes across a matrix of `ubuntu-latest`, `macos-latest`, and `windows-latest` runners.
2.  It builds wheels targeting **Python 3.10, 3.11, 3.12, and 3.13** (`cp310-*` to `cp313-*`).
3.  On Windows, it dynamically builds via MinGW with flags to link C++ runtimes (`-static-libgcc`, `-static-libstdc++`, `-Wl,-Bstatic,--whole-archive -lwinpthread`) statically. This prevents runtime errors on client systems lacking GCC/MinGW runtimes.
4.  Once built, wheels are automatically uploaded to PyPI using Trusted Publishing.
