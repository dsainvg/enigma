# Installation Guide

Get up and running with Enigma as a Python library or as a standalone native command-line utility.

---

## 🐍 Python Library

The official Python package is published as **`enigma-encryption`**. It includes pre-compiled binary wheels for all major platforms (Windows, macOS, and Linux) running Python 3.10 to 3.13.

Install it directly via your package manager:

```bash
pip install enigma-encryption
```

Once installed, it can be imported in your Python scripts as `enigma`:

```python
import enigma
```

---

## 💻 C++ Command-Line Tool (CLI)

For command-line use, you can download the standalone, pre-compiled binary (`enigma` or `enigma.exe`) directly. 

This standalone tool requires no compiler or dependencies on your system path.

### Downloading the Executable

1. Go to the [GitHub Releases Page](https://github.com/dsainvg/enigma/releases).
2. Download the binary package corresponding to your operating system:
   * **Windows**: `enigma-windows-amd64.zip` (contains `enigma.exe`)
   * **macOS**: `enigma-macos-universal.tar.gz`
   * **Linux**: `enigma-linux-amd64.tar.gz`
3. Extract the archive and place the executable in a directory on your system path (e.g., `/usr/local/bin` on Linux/macOS or a custom folder added to your Windows Path environment variables).

To verify the installation, open your terminal and run:

```bash
enigma --help
```
