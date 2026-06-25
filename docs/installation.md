# Installation Guide

You can run Enigma either as a high-performance Python package or as a standalone native command-line tool.

---

## 🐍 Python Package Installation

The official Python package is published to PyPI as **`enigma-encryption`** and includes pre-compiled binary wheels for Windows, macOS, and Linux (Python 3.10–3.13).

Install it via `pip`:

```bash
pip install enigma-encryption
```

### Build from Source (Source Distribution)

If you are on a platform without pre-built binary wheels, `pip` will automatically download the source distribution and compile it. 

To build it manually from a local clone of the repository:

```bash
# Clone the repository
git clone https://github.com/dsainvg/enigma.git
cd enigma

# Install dependencies and compile the package
pip install . -v
```

> [!NOTE]
> Building from source requires a C++17 compiler (GCC 8+, Clang 7+, or MSVC 2019+) and CMake 3.18+ installed on your system path.

---

## 💻 C++ Command-Line Tool (CLI)

To build the native CLI executable (`enigma` or `enigma.exe`) directly from source, use CMake.

### Prerequisites

Ensure you have the following installed:
* **CMake** (v3.18 or newer)
* **C++ Compiler** (supporting C++17)
* **Ninja** or **Make** (build generators)

### Build Steps

1. Configure the CMake build environment:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   ```
2. Compile the targets:
   ```bash
   cmake --build build --parallel
   ```
3. The built CLI executable will be generated at:
   * **Linux/macOS**: `build/cli/enigma`
   * **Windows**: `build/cli/enigma.exe`

### Running C++ Tests

To verify that the compiled C++ library and executable are functioning correctly on your machine, run `ctest` inside the build directory:

```bash
cd build
ctest --output-on-failure
```
