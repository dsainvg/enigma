"""
cross_check.py — Verify that the CLI and Python library produce identical outputs.

Since both link against the SAME enigma_core static library, they MUST agree on:
  1. hash_password()  — same hash for same password + salt + cost
  2. encrypt/decrypt  — CLI-encrypted file can be decrypted by Python and vice-versa

Usage (from enigma/ directory):
    python tests/python/cross_check.py [path/to/enigma.exe]

Defaults to: ./build/cli/enigma.exe  (Windows) or ./build/cli/encry  (Linux/macOS)
"""

from __future__ import annotations

import os
import sys

# Ensure UTF-8 output on Windows cp1252 consoles
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

import subprocess
import tempfile
import platform
import textwrap

# ── Locate CLI binary ────────────────────────────────────────────────────────

def find_cli(given: str | None = None) -> str:
    if given:
        return given
    exe_name = "enigma.exe" if platform.system() == "Windows" else "enigma"
    candidates = [
        os.path.join("build", "cli", exe_name),
        os.path.join("build", "cli", "Release", exe_name),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    raise FileNotFoundError(
        f"Cannot find enigma CLI binary. Build first:\n"
        f"  cmake -B build && cmake --build build --parallel\n"
        f"Or pass the path: python cross_check.py path/to/enigma"
    )

# ── Try to import the Python library ────────────────────────────────────────

try:
    import enigma as _enigma_check  # noqa: F401 -- just check it's importable
    HAS_PY = True
except ImportError:
    HAS_PY = False

# ── Test runner ──────────────────────────────────────────────────────────────

passed = 0
failed = 0

def check(name: str, ok: bool, detail: str = "") -> None:
    global passed, failed
    mark = "PASS" if ok else "FAIL"
    print(f"  [{mark}] {name}", flush=True)
    if not ok and detail:
        for line in detail.splitlines():
            print(f"      {line}")
    if ok:
        passed += 1
    else:
        failed += 1

def section(name: str) -> None:
    print(f"\n--- {name} ---")

# ── CLI helpers ───────────────────────────────────────────────────────────────

def cli_hash(cli: str, password: str, cost: int, salt: str) -> str:
    """Run: enigma hash <password> <cost> <salt>  -> strip trailing newline."""
    result = subprocess.run(
        [cli, "hash", password, str(cost), salt],
        capture_output=True, text=True
    )
    return result.stdout.strip()

def cli_encrypt(cli: str, infile: str, outfile: str, password: str, cost: int = 8) -> int:
    result = subprocess.run([cli, "encrypt", infile, password, str(cost), outfile],
                            capture_output=True)
    return result.returncode

def cli_decrypt(cli: str, infile: str, outfile: str, password: str) -> int:
    result = subprocess.run([cli, "decrypt", infile, password, outfile],
                            capture_output=True)
    return result.returncode

# ── Tests ─────────────────────────────────────────────────────────────────────

def test_hash_determinism(cli: str) -> None:
    """Both CLI and Python library must produce the same hash for the same inputs."""
    section("hash_password() cross-check")

    SALT = "testSalt12345678"
    CASES = [
        ("mypassword",   8,  SALT),
        ("test123",      8,  SALT),
        ("",             8,  SALT),
        ("p@$$w0rd!",    8,  SALT),
        ("unicode héllo", 8, SALT),
    ]

    for pw, cost, salt in CASES:
        h_cli = cli_hash(cli, pw, cost, salt)

        if HAS_PY:
            import enigma
            h_py = enigma.hash_password(pw, cost=cost, salt=salt)
            check(
                f"hash('{pw[:12]}…', cost={cost}) CLI==Python",
                h_cli == h_py,
                f"CLI: {h_cli[:60]}\nPy:  {h_py[:60]}"
            )
        else:
            # At least check CLI is deterministic with itself
            h_cli2 = cli_hash(cli, pw, cost, salt)
            check(
                f"hash('{pw[:12]}…', cost={cost}) CLI deterministic",
                h_cli == h_cli2,
                f"Run1: {h_cli[:60]}\nRun2: {h_cli2[:60]}"
            )

def test_encrypt_cli_decrypt_python(cli: str) -> None:
    """Encrypt with CLI, decrypt with Python library."""
    if not HAS_PY:
        print("  [skipped] Python library not installed — skipping CLI→Python test")
        return

    import enigma

    section("CLI encrypt → Python decrypt")

    cases = [
        (b"Hello from CLI encrypt!", "password123"),
        (b"" * 0,                    "emptyTest"),
        (bytes(range(256)) * 4,      "binaryData!"),
    ]

    for plaintext, pw in cases:
        with tempfile.NamedTemporaryFile(delete=False, suffix=".txt") as f:
            f.write(plaintext)
            src = f.name
        enc = src + ".enc"
        dec = src + ".dec"
        try:
            rc = cli_encrypt(cli, src, enc, pw)
            if rc != 0:
                check(f"CLI encrypt '{pw}' succeeds", False, "CLI encrypt returned non-zero")
                continue

            recovered = enigma.decrypt_bytes(open(enc, "rb").read(), pw)
            check(
                f"Python decrypts CLI-encrypted data (pw='{pw}')",
                recovered == plaintext,
                f"Expected {len(plaintext)} bytes, got {len(recovered)} bytes"
            )
        finally:
            for p in (src, enc, dec):
                if os.path.exists(p): os.remove(p)

def test_encrypt_python_decrypt_cli(cli: str) -> None:
    """Encrypt with Python library, decrypt with CLI."""
    if not HAS_PY:
        print("  [skipped] Python library not installed — skipping Python→CLI test")
        return

    import enigma

    section("Python encrypt → CLI decrypt")

    cases = [
        (b"Hello from Python encrypt!", "password123"),
        (b"Binary \x00\x01\x02\x03 data", "binaryPass"),
    ]

    for plaintext, pw in cases:
        enc_bytes = enigma.encrypt_bytes(plaintext, pw, cost=8)

        with tempfile.NamedTemporaryFile(delete=False, suffix=".enc") as f:
            f.write(enc_bytes)
            enc_path = f.name
        dec_path = enc_path + ".dec"
        try:
            rc = cli_decrypt(cli, enc_path, dec_path, pw)
            check(
                f"CLI decrypts Python-encrypted data (pw='{pw}')",
                rc == 0,
                f"CLI returned exit code {rc}"
            )
            if rc == 0:
                recovered = open(dec_path, "rb").read()
                check(
                    f"  Content matches",
                    recovered == plaintext,
                    f"Expected {plaintext!r}\nGot      {recovered!r}"
                )
        finally:
            for p in (enc_path, dec_path):
                if os.path.exists(p): os.remove(p)

def test_wrong_password(cli: str) -> None:
    """Both CLI and Python must reject wrong passwords."""
    section("Wrong-password rejection")

    with tempfile.NamedTemporaryFile(delete=False, suffix=".txt") as f:
        f.write(b"secret content")
        src = f.name
    enc = src + ".enc"
    dec = src + ".dec"
    pw = "correctPassword"

    try:
        cli_encrypt(cli, src, enc, pw)

        # CLI wrong password
        rc = cli_decrypt(cli, enc, dec, "wrongPassword")
        check("CLI rejects wrong password (non-zero exit)", rc != 0)

        if HAS_PY:
            import enigma
            try:
                enigma.decrypt_file(enc, dec, "wrongPassword")
                check("Python rejects wrong password (raises)", False, "No exception raised!")
            except (ValueError, RuntimeError):
                check("Python rejects wrong password (raises ValueError/RuntimeError)", True)
    finally:
        for p in (src, enc, dec):
            if os.path.exists(p): os.remove(p)

# ── Entry point ───────────────────────────────────────────────────────────────

def main() -> None:
    cli_path = sys.argv[1] if len(sys.argv) > 1 else None
    cli = find_cli(cli_path)

    print(f"=== encry cross-compatibility check ===")
    print(f"CLI:    {cli}")
    print(f"Python: {'OK encry installed' if HAS_PY else 'NOT INSTALLED (hash-only mode)'}")

    test_hash_determinism(cli)
    test_encrypt_cli_decrypt_python(cli)
    test_encrypt_python_decrypt_cli(cli)
    test_wrong_password(cli)

    print(f"\n=== Results: {passed} passed, {failed} failed ===")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
