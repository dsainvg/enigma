/**
 * bindings.cpp -- pybind11 bindings for the enigma Python library.
 *
 * Exposes the full enigma_core API (hash + cipher) to Python.
 * This is the only Python-facing code; all logic lives in enigma_core.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "enigma/hash.h"
#include "enigma/cipher.h"
#include "enigma/primitives.h"

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace py = pybind11;

/* -- Helpers ---------------------------------------------------------------- */

static std::string wrap_hash_password(const std::string &password,
                                      int cost,
                                      py::object salt_obj)
{
    const char *salt = nullptr;
    std::string salt_str;
    if (!salt_obj.is_none()) {
        salt_str = salt_obj.cast<std::string>();
        salt = salt_str.c_str();
    }
    char *result = hash_password(password.c_str(), cost, salt);
    if (!result)
        throw std::runtime_error("hash_password() returned NULL");
    std::string s(result);
    std::free(result);
    return s;
}

static std::string wrap_extract_salt(const std::string &hash) {
    char buf[256];
    if (extract_salt(hash.c_str(), buf, sizeof(buf)) != 0)
        throw std::invalid_argument("Malformed hash string -- cannot extract salt");
    return std::string(buf);
}

static py::bytes wrap_encrypt_bytes(py::bytes data,
                                    const std::string &password,
                                    int cost)
{
    std::string s = data;
    const auto *in = reinterpret_cast<const uint8_t *>(s.data());
    uint8_t *out = nullptr;
    size_t   out_len = 0;

    int rc = encrypt_bytes(in, s.size(), password.c_str(), cost, &out, &out_len);
    if (rc != 0) {
        std::free(out);
        throw std::runtime_error("encrypt_bytes() failed (code " + std::to_string(rc) + ")");
    }

    py::bytes result(reinterpret_cast<char *>(out), out_len);
    std::free(out);
    return result;
}

static py::bytes wrap_decrypt_bytes(py::bytes data, const std::string &password) {
    std::string s = data;
    const auto *in = reinterpret_cast<const uint8_t *>(s.data());
    uint8_t *out = nullptr;
    size_t   out_len = 0;

    int rc = decrypt_bytes(in, s.size(), password.c_str(), &out, &out_len);
    if (rc == -2) {
        std::free(out);
        throw std::invalid_argument("Wrong password");
    }
    if (rc != 0) {
        std::free(out);
        throw std::runtime_error("decrypt_bytes() failed (code " + std::to_string(rc) + ")");
    }

    py::bytes result(reinterpret_cast<char *>(out), out_len);
    std::free(out);
    return result;
}

static void wrap_encrypt_file(const std::string &input_file,
                               const std::string &output_file,
                               const std::string &password,
                               int cost,
                               int threads,
                               int io_buf_mb)
{
    int rc = encrypt_file(input_file.c_str(), output_file.c_str(),
                          password.c_str(), cost, threads, io_buf_mb);
    if (rc != 0)
        throw std::runtime_error("encrypt_file() failed (code " + std::to_string(rc) + ")");
}

static void wrap_decrypt_file(const std::string &input_file,
                               const std::string &output_file,
                               const std::string &password,
                               int threads,
                               int io_buf_mb)
{
    int rc = decrypt_file(input_file.c_str(), output_file.c_str(),
                          password.c_str(), threads, io_buf_mb);
    if (rc == -2)
        throw std::invalid_argument("Wrong password");
    if (rc != 0)
        throw std::runtime_error("decrypt_file() failed (code " + std::to_string(rc) + ")");
}

/* -- Module definition ------------------------------------------------------ */

PYBIND11_MODULE(_enigma, m) {
    m.doc() = "enigma -- custom password hashing and file encryption library";

    m.def("hash_password",
          &wrap_hash_password,
          py::arg("password"),
          py::arg("cost") = 10,
          py::arg("salt") = py::none(),
          R"doc(
Hash a password using the enigma custom algorithm.

Args:
    password: The password string to hash.
    cost:     Work factor; iterations = 2^cost (default 10 = 1024 rounds).
              Recommended range: 8-14.
    salt:     Optional 16-char salt string.  When None a cryptographically
              secure random salt is generated automatically.

Returns:
    Hash string in the format ``$<salt>$/$<hash>``.

Example::

    import enigma
    h = enigma.hash_password("mysecret", cost=10)
    # Verify later:
    salt = enigma.extract_salt(h)
    assert enigma.hash_password("mysecret", cost=10, salt=salt) == h
)doc");

    m.def("extract_salt",
          &wrap_extract_salt,
          py::arg("hash"),
          R"doc(
Extract the salt embedded in a hash string produced by hash_password().

Args:
    hash: Hash string from hash_password().

Returns:
    Salt string (str).

Raises:
    ValueError: If the hash string is malformed.
)doc");

    m.def("encrypt_bytes",
          &wrap_encrypt_bytes,
          py::arg("data"),
          py::arg("password"),
          py::arg("cost") = 10,
          R"doc(
Encrypt raw bytes.

Args:
    data:     Plaintext bytes.
    password: Encryption password.
    cost:     Work factor (default 10).

Returns:
    Ciphertext bytes in ENIGMA01 format.
)doc");

    m.def("decrypt_bytes",
          &wrap_decrypt_bytes,
          py::arg("data"),
          py::arg("password"),
          R"doc(
Decrypt bytes produced by encrypt_bytes().

Args:
    data:     Ciphertext bytes.
    password: Decryption password.

Returns:
    Plaintext bytes.

Raises:
    ValueError:   Wrong password.
    RuntimeError: Other decryption errors.
)doc");

    m.def("encrypt_file",
          &wrap_encrypt_file,
          py::arg("input_file"),
          py::arg("output_file"),
          py::arg("password"),
          py::arg("cost") = 10,
          py::arg("threads") = 1,
          py::arg("io_buf_mb") = 4,
          py::call_guard<py::gil_scoped_release>(),
          R"doc(
Encrypt a file.  Output is written in ENIGMA01 format.

Streams the file in `io_buf_mb`-MiB chunks — memory usage is bounded
regardless of file size, so files larger than RAM are supported.

Args:
    input_file:  Path to the plaintext file.
    output_file: Path for the encrypted output file.
    password:    Encryption password.
    cost:        Key-derivation work factor (default 10, iterations = 2^cost).
    threads:     Worker threads for block cipher passes (default 1).
                 Set to 0 to use all available CPU cores.
    io_buf_mb:   I/O buffer size in MiB (default 4).

Raises:
    RuntimeError: On I/O or encryption error.
)doc");

    m.def("decrypt_file",
          &wrap_decrypt_file,
          py::arg("input_file"),
          py::arg("output_file"),
          py::arg("password"),
          py::arg("threads") = 1,
          py::arg("io_buf_mb") = 4,
          py::call_guard<py::gil_scoped_release>(),
          R"doc(
Decrypt a file produced by encrypt_file().

Args:
    input_file:  Path to the encrypted file.
    output_file: Path for the decrypted output file.
    password:    Decryption password.
    threads:     Worker threads for block cipher passes (default 1).
                 Set to 0 to use all available CPU cores.
    io_buf_mb:   I/O buffer size in MiB (default 4).

Raises:
    ValueError:   Wrong password.
    RuntimeError: On I/O or decryption error.
)doc");

    // Version info
    m.attr("__version__") = "0.1.0";
}
