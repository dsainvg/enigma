/**
 * main.cpp -- enigma CLI tool
 *
 * Usage:
 *   enigma hash    <password> [--cost=10] [--salt=<str>]
 *   enigma encrypt <file> <password> [--cost=10] [--threads=1] [--buffer-mb=4] [--out=<path>]
 *   enigma decrypt <file> <password> [--threads=1] [--buffer-mb=4] [--out=<path>]
 */

#include "enigma/hash.h"
#include "enigma/cipher.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>

static std::string utf16_to_utf8(const wchar_t *wstr) {
    if (!wstr) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string str(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], size, nullptr, nullptr);
    return str;
}
#endif

/* ── Lightweight flag parser ─────────────────────────────────────────────── */
// Recognises:  --key=value   --key value   (no short flags)
// Positional args (no leading --) are collected into `positional`.

struct Flags {
    std::unordered_map<std::string, std::string> kv;
    std::vector<std::string> positional;

    void parse(int argc, char *argv[], int start = 0) {
        for (int i = start; i < argc; ++i) {
            std::string a = argv[i];
            if (a.size() > 2 && a[0] == '-' && a[1] == '-') {
                a = a.substr(2);
                auto eq = a.find('=');
                if (eq != std::string::npos) {
                    kv[a.substr(0, eq)] = a.substr(eq + 1);
                } else if (i + 1 < argc && argv[i + 1][0] != '-') {
                    kv[a] = argv[++i];
                } else {
                    kv[a] = "1";  // boolean flag
                }
            } else {
                positional.push_back(a);
            }
        }
    }

    std::string get(const std::string &key, const std::string &def = "") const {
        auto it = kv.find(key);
        return (it != kv.end()) ? it->second : def;
    }

    int get_int(const std::string &key, int def) const {
        auto it = kv.find(key);
        return (it != kv.end()) ? std::atoi(it->second.c_str()) : def;
    }

    bool has(const std::string &key) const { return kv.count(key) > 0; }
};

/* ── Usage ───────────────────────────────────────────────────────────────── */

static void print_usage(const char *prog) {
    std::printf(
        "Enigma — custom password hashing and symmetric file encryption\n\n"
        "Usage:\n"
        "  %s hash    <password> [--cost=10] [--salt=<str>]\n"
        "  %s encrypt <file> <password> [--cost=10] [--threads=1] [--buffer-mb=4] [--out=<path>]\n"
        "  %s decrypt <file> <password> [--threads=1] [--buffer-mb=4] [--out=<path>]\n\n"
        "Options:\n"
        "  --cost=N       Key-derivation work factor; iterations = 2^N  (range 0-31, default 10)\n"
        "  --salt=S       Fixed 16-character salt for hash command\n"
        "  --threads=N    Worker threads for cipher block passes (0 = all CPU cores, default 1)\n"
        "  --buffer-mb=N  I/O buffer size in MiB; only this much plaintext is held in RAM\n"
        "                 at once, so files larger than RAM are supported (default 4)\n"
        "  --out=<path>   Output file path (encrypt defaults to <file>.enc,\n"
        "                                   decrypt defaults to <file>.dec)\n",
        prog, prog, prog
    );
}

/* ── Hash command ─────────────────────────────────────────────────────────── */

static int cmd_hash(int argc, char *argv[]) {
    // argv[2] = password (positional, required)
    if (argc < 3) {
        std::printf("Usage: enigma hash <password> [--cost=10] [--salt=<str>]\n");
        return 1;
    }
    const char *password = argv[2];

    Flags f;
    f.parse(argc, argv, 3);

    int cost = f.get_int("cost", 10);
    if (cost < 0 || cost > 31) {
        std::fprintf(stderr, "Error: --cost must be between 0 and 31\n");
        return 1;
    }
    // Materialise the salt string before taking c_str() — f.get() returns by value.
    std::string salt_str = f.get("salt");
    const char *salt = f.has("salt") ? salt_str.c_str() : nullptr;

    char *hash = hash_password(password, cost, salt);
    if (!hash) { std::fprintf(stderr, "Error: hashing failed\n"); return 1; }
    std::printf("%s\n", hash);
    std::free(hash);
    return 0;
}

/* ── Encrypt command ──────────────────────────────────────────────────────── */

static int cmd_encrypt(int argc, char *argv[]) {
    // argv[2]=file  argv[3]=password  [flags...]
    if (argc < 4) {
        std::printf("Usage: enigma encrypt <file> <password> [--cost=10] [--threads=1] [--buffer-mb=4] [--out=<path>]\n");
        return 1;
    }
    const char *input    = argv[2];
    const char *password = argv[3];

    Flags f;
    f.parse(argc, argv, 4);

    int cost      = f.get_int("cost",      10);
    int threads   = f.get_int("threads",    1);
    int buffer_mb = f.get_int("buffer-mb",  4);
    std::string output = f.has("out") ? f.get("out") : (std::string(input) + ".enc");

    if (cost < 0 || cost > 31) {
        std::fprintf(stderr, "Error: --cost must be between 0 and 31\n");
        return 1;
    }
    if (buffer_mb < 1) {
        std::fprintf(stderr, "Error: --buffer-mb must be >= 1\n");
        return 1;
    }

    int rc = encrypt_file(input, output.c_str(), password, cost, threads, buffer_mb);
    if (rc == 0) {
        std::printf("Encrypted → %s\n", output.c_str());
    } else {
        std::fprintf(stderr, "Error: encryption failed (code %d)\n", rc);
    }
    return rc == 0 ? 0 : 1;
}

/* ── Decrypt command ──────────────────────────────────────────────────────── */

static int cmd_decrypt(int argc, char *argv[]) {
    // argv[2]=file  argv[3]=password  [flags...]
    if (argc < 4) {
        std::printf("Usage: enigma decrypt <file> <password> [--threads=1] [--buffer-mb=4] [--out=<path>]\n");
        return 1;
    }
    const char *input    = argv[2];
    const char *password = argv[3];

    Flags f;
    f.parse(argc, argv, 4);

    int threads   = f.get_int("threads",    1);
    int buffer_mb = f.get_int("buffer-mb",  4);
    std::string output = f.has("out") ? f.get("out") : (std::string(input) + ".dec");

    if (buffer_mb < 1) {
        std::fprintf(stderr, "Error: --buffer-mb must be >= 1\n");
        return 1;
    }

    int rc = decrypt_file(input, output.c_str(), password, threads, buffer_mb);
    if (rc == 0) {
        std::printf("Decrypted → %s\n", output.c_str());
    } else if (rc == -2) {
        std::fprintf(stderr, "Error: wrong password\n");
    } else {
        std::fprintf(stderr, "Error: decryption failed (code %d)\n", rc);
    }
    return rc == 0 ? 0 : 1;
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
#ifdef _WIN32
    int wargc;
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    std::vector<std::string> args_utf8;
    std::vector<char*> argv_utf8;
    if (wargv) {
        args_utf8.resize(wargc);
        argv_utf8.resize(wargc);
        for (int i = 0; i < wargc; ++i) {
            args_utf8[i] = utf16_to_utf8(wargv[i]);
            argv_utf8[i] = const_cast<char*>(args_utf8[i].c_str());
        }
        LocalFree(wargv);
        argc = wargc;
        argv = argv_utf8.data();
    }
#endif

    if (argc < 2) { print_usage(argv[0]); return 1; }

    const char *cmd = argv[1];
    if (std::strcmp(cmd, "hash")    == 0) return cmd_hash(argc, argv);
    if (std::strcmp(cmd, "encrypt") == 0) return cmd_encrypt(argc, argv);
    if (std::strcmp(cmd, "decrypt") == 0) return cmd_decrypt(argc, argv);
    if (std::strcmp(cmd, "-h")      == 0 || std::strcmp(cmd, "--help") == 0) {
        print_usage(argv[0]); return 0;
    }
    std::fprintf(stderr, "Unknown command: %s\n\n", cmd);
    print_usage(argv[0]);
    return 1;
}
