"""
test_algo_parity.py
-------------------
Verifies that the C++ enigma CLI produces IDENTICAL hash output to the
Python reference implementation (Archis/Enc/encryptionAlgorithm).

This test directly targets the hash3 bug that was fixed:
  Python does NOT reset `voil` between hash2 and hash3 loops.
  The C++ implementation must match this behaviour.

Run from enigma/ root:
    python tests/python/test_algo_parity.py
"""

from __future__ import annotations

import sys
import os
import subprocess
import platform

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

# ── Locate the reference Python implementation ────────────────────────────

SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
ENIGMA_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
REF_ROOT    = os.path.abspath(
    os.path.join(ENIGMA_ROOT, "..", "Archis", "Enc", "encryptionAlgorithm")
)

if not os.path.isdir(REF_ROOT):
    print(f"[SKIP] Reference implementation not found at {REF_ROOT}")
    sys.exit(0)

sys.path.insert(0, REF_ROOT)

try:
    from encryption_utils.hashing_utils import hashing_passcode as py_hash
except ImportError as e:
    print(f"[SKIP] Cannot import reference Python impl: {e}")
    sys.exit(0)

# ── Locate C++ CLI ────────────────────────────────────────────────────────

exe = "enigma.exe" if platform.system() == "Windows" else "enigma"
CLI_CANDIDATES = [
    os.path.join(ENIGMA_ROOT, "build", "cli", exe),
    os.path.join(ENIGMA_ROOT, "build", "cli", "Release", exe),
]
CLI = next((p for p in CLI_CANDIDATES if os.path.isfile(p)), None)

if CLI is None:
    print("[SKIP] enigma CLI not built. Run: cmake -B build && cmake --build build")
    sys.exit(0)

# ── Test runner ───────────────────────────────────────────────────────────

passed = 0
failed = 0

def check(name: str, ok: bool, detail: str = "") -> None:
    global passed, failed
    print(f"  [{'PASS' if ok else 'FAIL'}] {name}")
    if not ok and detail:
        for line in detail.splitlines():
            print(f"       {line}")
    if ok: passed += 1
    else:   failed += 1

def section(name: str) -> None:
    print(f"\n--- {name} ---")

def cli_hash(password: str, cost: int, salt: str) -> str:
    r = subprocess.run([CLI, "hash", password, str(cost), salt],
                       capture_output=True, text=True, encoding="utf-8")
    return r.stdout.strip()

# ── Parity tests ──────────────────────────────────────────────────────────

section("Hash output parity: C++ CLI vs Python reference")

CASES = [
    # (password,       cost, salt)
    ("password",        8,  "testSalt12345678"),
    ("",                8,  "aaaaaaaaaaaaaaaa"),
    ("hello world",     8,  "SaltSaltSaltSalt"),
    ("p@$$w0rd!123",    8,  "MyFixedSalt12345"),
    ("abc",             8,  "zzzzzzzzzzzzzzzz"),
    # Cost=1 (2 iterations) is fast and easy to verify
    ("password",        1,  "testSalt12345678"),
    ("singleIter",      0,  "aaaaaaaaaaaaaaaa"),  # cost=0 => 1 iteration
]

for pw, cost, salt in CASES:
    h_cpp = cli_hash(pw, cost, salt)
    h_py  = py_hash(pw, cost=cost, salt=salt)
    check(
        f"hash('{pw[:14]}', cost={cost}, salt='{salt[:8]}...') match",
        h_cpp == h_py,
        f"C++: {h_cpp}\nPy:  {h_py}"
    )

# ── Verify extracted salt is stable ──────────────────────────────────────

section("extract_salt + re-hash determinism (C++ side)")

SALT = "fixedSalt1234567"
h1 = cli_hash("mypassword", 8, SALT)
# Extract salt from the CLI output ($salt$/$hash)
embedded = h1[1:h1.index("$", 1)]
h2 = cli_hash("mypassword", 8, embedded)
check("Re-hash with extracted salt == original", h1 == h2,
      f"Original: {h1}\nRe-hash:  {h2}")

# ── Specific regression for the hash3 carry-over bug ─────────────────────

section("hash3 carry-over regression (Bug Fix Verification)")

# Before the fix, hash3 was computed with hv=0 (C++) instead of carrying
# the residual hv from hash2 (Python).  If the outputs match, the fix is good.
# We test multiple passwords to make it unlikely a coincidental match occurs.
REGRESSION_CASES = [
    ("test",     4, "regSalt000000001"),
    ("abc123",   4, "regSalt000000002"),
    ("!@#$%",    4, "regSalt000000003"),
    ("longpass",  4, "regSalt000000004"),
]

any_differ_pre = False  # just document: they would have differed before fix
for pw, cost, salt in REGRESSION_CASES:
    h_cpp = cli_hash(pw, cost, salt)
    h_py  = py_hash(pw, cost=cost, salt=salt)
    match = (h_cpp == h_py)
    check(f"[regression] hash('{pw}', cost={cost}) matches Python", match,
          f"C++: {h_cpp}\nPy:  {h_py}")

# ── Final report ──────────────────────────────────────────────────────────

print(f"\n=== Results: {passed} passed, {failed} failed ===")
sys.exit(1 if failed else 0)
