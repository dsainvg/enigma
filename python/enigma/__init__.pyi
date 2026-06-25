"""Type stubs for the enigma package (PEP 561)."""

from __future__ import annotations

__version__: str

def hash_password(
    password: str,
    cost: int = 10,
    salt: str | None = None,
) -> str:
    """
    Hash a password using the enigma custom algorithm.

    Args:
        password: The password string to hash.
        cost:     Work factor; iterations = 2^cost (default 10 = 1024 rounds).
                  Recommended range: 8-14.
        salt:     Optional 16-char salt string.  When None a cryptographically
                  secure random salt is generated.

    Returns:
        Hash string in the format ``$<salt>$/$<hash>``.
    """
    ...

def extract_salt(hash: str) -> str:
    """
    Extract the salt embedded in a hash string produced by hash_password().

    Raises:
        ValueError: If the hash string is malformed.
    """
    ...

def encrypt_bytes(data: bytes, password: str, cost: int = 10) -> bytes:
    """
    Encrypt raw bytes.

    Returns:
        Ciphertext bytes in ENIGMA01 format.
    """
    ...

def decrypt_bytes(data: bytes, password: str) -> bytes:
    """
    Decrypt bytes produced by encrypt_bytes().

    Raises:
        ValueError:   Wrong password.
        RuntimeError: Other decryption errors.
    """
    ...

def encrypt_file(
    input_file: str,
    output_file: str,
    password: str,
    cost: int = 10,
) -> None:
    """
    Encrypt a file.  Output is written in ENIGMA01 format.

    Raises:
        RuntimeError: On I/O or encryption error.
    """
    ...

def decrypt_file(
    input_file: str,
    output_file: str,
    password: str,
) -> None:
    """
    Decrypt a file produced by encrypt_file().

    Raises:
        ValueError:   Wrong password.
        RuntimeError: On I/O or decryption error.
    """
    ...
