"""
enigma — custom password hashing and file encryption.

This package wraps the enigma_core C++ library via pybind11.
"""

from __future__ import annotations

from enigma._enigma import (  # type: ignore[import]
    hash_password,
    extract_salt,
    encrypt_bytes,
    decrypt_bytes,
    encrypt_file,
    decrypt_file,
    __version__,
)

__all__ = [
    "hash_password",
    "extract_salt",
    "encrypt_bytes",
    "decrypt_bytes",
    "encrypt_file",
    "decrypt_file",
    "__version__",
]
