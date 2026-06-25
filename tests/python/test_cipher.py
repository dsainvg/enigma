"""
pytest tests for enigma.encrypt_bytes() / decrypt_bytes() and
enigma.encrypt_file() / decrypt_file()

Run: pytest tests/python/test_cipher.py -v
"""

import os
import tempfile
import pytest
import enigma


PASSWORD = "testPassword123!"


# ── encrypt_bytes / decrypt_bytes ────────────────────────────────────────────

class TestEncryptDecryptBytes:
    def test_basic_round_trip(self):
        plaintext = b"Hello, enigma world!"
        ciphertext = enigma.encrypt_bytes(plaintext, PASSWORD)
        assert isinstance(ciphertext, bytes)
        assert ciphertext != plaintext
        recovered = enigma.decrypt_bytes(ciphertext, PASSWORD)
        assert recovered == plaintext

    def test_output_longer_than_input(self):
        data = b"short"
        enc = enigma.encrypt_bytes(data, PASSWORD)
        assert len(enc) > len(data)

    def test_magic_header(self):
        enc = enigma.encrypt_bytes(b"test", PASSWORD)
        assert enc[:8] == b"ENIGMA01"

    def test_wrong_password_raises(self):
        enc = enigma.encrypt_bytes(b"secret data", PASSWORD)
        with pytest.raises((ValueError, RuntimeError)):
            enigma.decrypt_bytes(enc, "wrong_password")

    def test_empty_bytes_round_trip(self):
        enc = enigma.encrypt_bytes(b"", PASSWORD)
        dec = enigma.decrypt_bytes(enc, PASSWORD)
        assert dec == b""

    def test_single_byte_round_trip(self):
        enc = enigma.encrypt_bytes(b"\x42", PASSWORD)
        dec = enigma.decrypt_bytes(enc, PASSWORD)
        assert dec == b"\x42"

    def test_binary_data_round_trip(self):
        data = bytes(range(256))
        enc = enigma.encrypt_bytes(data, PASSWORD)
        dec = enigma.decrypt_bytes(enc, PASSWORD)
        assert dec == data

    def test_large_data_round_trip(self):
        # > 1 KB to exercise multi-chunk path
        data = (b"A" * 500 + b"B" * 500 + b"C" * 500) * 4
        enc = enigma.encrypt_bytes(data, PASSWORD)
        dec = enigma.decrypt_bytes(enc, PASSWORD)
        assert dec == data

    def test_very_large_data_round_trip(self):
        # > 4 KB — spans many SUB_CHUNKs
        data = bytes(i % 256 for i in range(8192))
        enc = enigma.encrypt_bytes(data, PASSWORD)
        dec = enigma.decrypt_bytes(enc, PASSWORD)
        assert dec == data

    def test_different_passwords_produce_different_ciphertext(self):
        data = b"same plaintext"
        enc1 = enigma.encrypt_bytes(data, "password1")
        enc2 = enigma.encrypt_bytes(data, "password2")
        assert enc1 != enc2

    def test_random_salt_produces_unique_ciphertext(self):
        data = b"same plaintext"
        enc1 = enigma.encrypt_bytes(data, PASSWORD)
        enc2 = enigma.encrypt_bytes(data, PASSWORD)
        # Ciphertexts may have different lengths (variable-length hash output),
        # but their ENIGMA01 headers embed unique salts.
        # Extract salt: bytes[10 : 10+salt_len] where salt_len = LE16 at bytes[8:10]
        def get_salt(enc):
            salt_len = int.from_bytes(enc[8:10], 'little')
            return enc[10:10 + salt_len]
        assert get_salt(enc1) != get_salt(enc2), "Salts should differ between two encryptions"
        # Both must decrypt correctly
        assert enigma.decrypt_bytes(enc1, PASSWORD) == data
        assert enigma.decrypt_bytes(enc2, PASSWORD) == data

    def test_cost_8_round_trip(self):
        data = b"test with cost 8"
        enc = enigma.encrypt_bytes(data, PASSWORD, cost=8)
        assert enigma.decrypt_bytes(enc, PASSWORD) == data

    def test_bad_bytes_raises(self):
        with pytest.raises((ValueError, RuntimeError)):
            enigma.decrypt_bytes(b"BADMAGIC00000000", PASSWORD)

    def test_unicode_password(self):
        data = b"unicode password test"
        pw = "p@$$wörD_héllo"
        enc = enigma.encrypt_bytes(data, pw)
        dec = enigma.decrypt_bytes(enc, pw)
        assert dec == data

    def test_special_characters_password(self):
        data = b"special chars"
        pw = "p@$$w0rd!#%^&*()"
        enc = enigma.encrypt_bytes(data, pw)
        dec = enigma.decrypt_bytes(enc, pw)
        assert dec == data


# ── encrypt_file / decrypt_file ──────────────────────────────────────────────

class TestEncryptDecryptFile:
    def _tmpfile(self, suffix=""):
        fd, path = tempfile.mkstemp(suffix=suffix)
        os.close(fd)
        return path

    def test_basic_file_round_trip(self):
        src  = self._tmpfile(".txt")
        enc  = self._tmpfile(".enc")
        dec  = self._tmpfile(".txt")
        try:
            content = b"File encryption round-trip test 1234567890"
            with open(src, "wb") as f:
                f.write(content)

            enigma.encrypt_file(src, enc, PASSWORD)
            enigma.decrypt_file(enc, dec, PASSWORD)

            with open(dec, "rb") as f:
                result = f.read()
            assert result == content
        finally:
            for p in (src, enc, dec):
                if os.path.exists(p): os.remove(p)

    def test_wrong_password_raises_on_file(self):
        src = self._tmpfile(".txt")
        enc = self._tmpfile(".enc")
        dec = self._tmpfile(".dec")
        try:
            with open(src, "wb") as f:
                f.write(b"secret content")
            enigma.encrypt_file(src, enc, PASSWORD)
            with pytest.raises((ValueError, RuntimeError)):
                enigma.decrypt_file(enc, dec, "wrong_password")
        finally:
            for p in (src, enc, dec):
                if os.path.exists(p): os.remove(p)

    def test_missing_input_file_raises(self):
        with pytest.raises(RuntimeError):
            enigma.encrypt_file("/nonexistent/file.txt", "/tmp/out.enc", PASSWORD)

    def test_empty_file_round_trip(self):
        src = self._tmpfile(".txt")
        enc = self._tmpfile(".enc")
        dec = self._tmpfile(".dec")
        try:
            open(src, "wb").close()  # Create empty file
            enigma.encrypt_file(src, enc, PASSWORD)
            enigma.decrypt_file(enc, dec, PASSWORD)
            with open(dec, "rb") as f:
                assert f.read() == b""
        finally:
            for p in (src, enc, dec):
                if os.path.exists(p): os.remove(p)

    def test_large_file_round_trip(self):
        src = self._tmpfile()
        enc = self._tmpfile(".enc")
        dec = self._tmpfile(".dec")
        try:
            content = bytes(i % 256 for i in range(16384))
            with open(src, "wb") as f:
                f.write(content)
            enigma.encrypt_file(src, enc, PASSWORD, cost=8)
            enigma.decrypt_file(enc, dec, PASSWORD)
            with open(dec, "rb") as f:
                result = f.read()
            assert result == content
        finally:
            for p in (src, enc, dec):
                if os.path.exists(p): os.remove(p)

    def test_encrypt_file_creates_enigma01_format(self):
        src = self._tmpfile(".txt")
        enc = self._tmpfile(".enc")
        try:
            with open(src, "wb") as f:
                f.write(b"check format")
            enigma.encrypt_file(src, enc, PASSWORD)
            with open(enc, "rb") as f:
                magic = f.read(8)
            assert magic == b"ENIGMA01"
        finally:
            for p in (src, enc):
                if os.path.exists(p): os.remove(p)


# ── Consistency: bytes and file APIs agree ────────────────────────────────────

class TestBytesFileConsistency:
    def test_bytes_and_file_produce_same_plaintext(self):
        content = b"consistency test payload"
        password = "consistencyPass"

        # Encrypt via bytes API
        enc_bytes = enigma.encrypt_bytes(content, password, cost=8)

        # Write to file and decrypt
        with tempfile.NamedTemporaryFile(delete=False, suffix=".enc") as f:
            f.write(enc_bytes)
            enc_path = f.name
        dec_path = enc_path + ".dec"
        try:
            enigma.decrypt_file(enc_path, dec_path, password)
            with open(dec_path, "rb") as f:
                result = f.read()
            assert result == content
        finally:
            for p in (enc_path, dec_path):
                if os.path.exists(p): os.remove(p)
