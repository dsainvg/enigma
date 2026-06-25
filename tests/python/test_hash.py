"""
pytest tests for enigma.hash_password() and enigma.extract_salt()

Run: pytest tests/python/test_hash.py -v
"""

import pytest
import enigma


SALT = "testSalt12345678"
SALT2 = "otherSalt1234567"


# ── Determinism ─────────────────────────────────────────────────────────────

class TestDeterminism:
    def test_same_password_salt_produces_same_hash(self):
        h1 = enigma.hash_password("mypassword", cost=10, salt=SALT)
        h2 = enigma.hash_password("mypassword", cost=10, salt=SALT)
        assert h1 == h2

    def test_deterministic_at_cost_8(self):
        h1 = enigma.hash_password("test123", cost=8, salt=SALT)
        h2 = enigma.hash_password("test123", cost=8, salt=SALT)
        assert h1 == h2

    def test_empty_password_deterministic(self):
        h1 = enigma.hash_password("", cost=8, salt=SALT)
        h2 = enigma.hash_password("", cost=8, salt=SALT)
        assert h1 == h2

    def test_single_char_deterministic(self):
        h1 = enigma.hash_password("a", cost=8, salt=SALT)
        h2 = enigma.hash_password("a", cost=8, salt=SALT)
        assert h1 == h2


# ── Password Sensitivity ─────────────────────────────────────────────────────

class TestPasswordSensitivity:
    def test_different_passwords_differ(self):
        h1 = enigma.hash_password("password1", cost=8, salt=SALT)
        h2 = enigma.hash_password("password2", cost=8, salt=SALT)
        assert h1 != h2

    def test_case_sensitive(self):
        h1 = enigma.hash_password("Password", cost=8, salt=SALT)
        h2 = enigma.hash_password("password", cost=8, salt=SALT)
        assert h1 != h2

    def test_single_char_passwords_differ(self):
        h1 = enigma.hash_password("A", cost=8, salt=SALT)
        h2 = enigma.hash_password("B", cost=8, salt=SALT)
        assert h1 != h2

    def test_trailing_space_changes_hash(self):
        h1 = enigma.hash_password("abc", cost=8, salt=SALT)
        h2 = enigma.hash_password("abc ", cost=8, salt=SALT)
        assert h1 != h2

    def test_long_similar_passwords_differ(self):
        h1 = enigma.hash_password("VeryLongPassword123456789", cost=8, salt=SALT)
        h2 = enigma.hash_password("VeryLongPassword123456788", cost=8, salt=SALT)
        assert h1 != h2


# ── Salt Sensitivity ─────────────────────────────────────────────────────────

class TestSaltSensitivity:
    def test_different_salts_produce_different_hashes(self):
        h1 = enigma.hash_password("mypassword", cost=8, salt=SALT)
        h2 = enigma.hash_password("mypassword", cost=8, salt=SALT2)
        assert h1 != h2

    def test_single_char_salts_differ(self):
        h1 = enigma.hash_password("test", cost=8, salt="a")
        h2 = enigma.hash_password("test", cost=8, salt="b")
        assert h1 != h2

    def test_near_identical_salts_differ(self):
        h1 = enigma.hash_password("test", cost=8, salt="salt123456789012")
        h2 = enigma.hash_password("test", cost=8, salt="salt123456789013")
        assert h1 != h2


# ── Random Salts ─────────────────────────────────────────────────────────────

class TestRandomSalts:
    def test_random_salts_produce_different_hashes(self):
        h1 = enigma.hash_password("mypassword", cost=8)
        h2 = enigma.hash_password("mypassword", cost=8)
        assert h1 != h2

    def test_random_salt_hash_can_be_re_verified(self):
        h = enigma.hash_password("mypassword", cost=8)
        salt = enigma.extract_salt(h)
        h2 = enigma.hash_password("mypassword", cost=8, salt=salt)
        assert h == h2


# ── Cost Factor ──────────────────────────────────────────────────────────────

class TestCostFactor:
    def test_cost_8_vs_10_differ(self):
        h1 = enigma.hash_password("mypassword", cost=8,  salt=SALT)
        h2 = enigma.hash_password("mypassword", cost=10, salt=SALT)
        assert h1 != h2

    def test_cost_10_vs_12_differ(self):
        h1 = enigma.hash_password("mypassword", cost=10, salt=SALT)
        h2 = enigma.hash_password("mypassword", cost=12, salt=SALT)
        assert h1 != h2


# ── Output Format ────────────────────────────────────────────────────────────

class TestOutputFormat:
    def test_returns_string(self):
        h = enigma.hash_password("test", cost=8, salt=SALT)
        assert isinstance(h, str)

    def test_starts_with_dollar(self):
        h = enigma.hash_password("test", cost=8, salt=SALT)
        assert h.startswith("$")

    def test_contains_separator(self):
        h = enigma.hash_password("test", cost=8, salt=SALT)
        assert "$/$" in h

    def test_salt_embedded_in_output(self):
        h = enigma.hash_password("test", cost=8, salt=SALT)
        assert SALT in h


# ── extract_salt ─────────────────────────────────────────────────────────────

class TestExtractSalt:
    def test_extracts_known_salt(self):
        h = enigma.hash_password("mypassword", cost=8, salt=SALT)
        extracted = enigma.extract_salt(h)
        assert extracted == SALT

    def test_extracts_random_salt(self):
        h = enigma.hash_password("mypassword", cost=8)
        extracted = enigma.extract_salt(h)
        assert isinstance(extracted, str)
        assert len(extracted) > 0

    def test_bad_format_raises(self):
        with pytest.raises((ValueError, RuntimeError)):
            enigma.extract_salt("noDollarSign")

    def test_re_hash_with_extracted_salt_matches(self):
        h1 = enigma.hash_password("secret", cost=8, salt=SALT)
        salt = enigma.extract_salt(h1)
        h2 = enigma.hash_password("secret", cost=8, salt=salt)
        assert h1 == h2


# ── Special Characters ───────────────────────────────────────────────────────

class TestSpecialCharacters:
    def test_special_chars_deterministic(self):
        pw = "p@$$w0rd!#%^&*()"
        h1 = enigma.hash_password(pw, cost=8, salt=SALT)
        h2 = enigma.hash_password(pw, cost=8, salt=SALT)
        assert h1 == h2

    def test_unicode_deterministic(self):
        pw = "héllo wörld — 日本語"
        h1 = enigma.hash_password(pw, cost=8, salt=SALT)
        h2 = enigma.hash_password(pw, cost=8, salt=SALT)
        assert h1 == h2


# ── Version ──────────────────────────────────────────────────────────────────

def test_version_attribute():
    assert hasattr(enigma, "__version__")
    assert isinstance(enigma.__version__, str)
