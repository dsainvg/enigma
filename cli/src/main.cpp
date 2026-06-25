/**
 * main.cpp -- enigma CLI tool
 *
 * Links against the enigma_core library (single source of truth for
 * all hash and cipher logic).
 *
 * Usage:
 *   enigma hash    <password> [cost] [salt]
 *   enigma encrypt <file> <password> [cost] [output]
 *   enigma decrypt <file> <password> [output]
 */

#include "enigma/hash.h"
#include "enigma/cipher.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
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

static void print_usage(const char *prog) {
    std::printf(
        "Usage:\n"
        "  %s hash    <password> [cost=10] [salt]\n"
        "  %s encrypt <file>     <password> [cost=10] [output_file]\n"
        "  %s decrypt <file>     <password> [output_file]\n"
        "\n"
        "  cost: work factor -- iterations = 2^cost (default 10 = 1024 rounds)\n"
        "        recommended range: 8-14\n",
        prog, prog, prog
    );
}

/* ── Hash command ─────────────────────────────────────────────────────────── */

static int cmd_hash(int argc, char *argv[]) {
    // argv: [0]=encry [1]=hash [2]=password [3]=cost? [4]=salt?
    if (argc < 3) {
        std::printf("Usage: encry hash <password> [cost=10] [salt]\n");
        return 1;
    }
    const char *password = argv[2];
    int cost = (argc >= 4) ? std::atoi(argv[3]) : 10;
    const char *salt = (argc >= 5) ? argv[4] : nullptr;

    if (cost < 0 || cost > 31) {
        std::fprintf(stderr, "Error: cost must be between 0 and 31\n");
        return 1;
    }

    char *hash = hash_password(password, cost, salt);
    if (!hash) {
        std::fprintf(stderr, "Error: hashing failed\n");
        return 1;
    }
    std::printf("%s\n", hash);
    std::free(hash);
    return 0;
}

/* ── Encrypt command ──────────────────────────────────────────────────────── */

static int cmd_encrypt(int argc, char *argv[]) {
    // argv: [0]=encry [1]=encrypt [2]=file [3]=password [4]=cost? [5]=output?
    if (argc < 4) {
        std::printf("Usage: encry encrypt <file> <password> [cost=10] [output_file]\n");
        return 1;
    }
    const char *input    = argv[2];
    const char *password = argv[3];
    int cost             = (argc >= 5) ? std::atoi(argv[4]) : 10;
    std::string output   = (argc >= 6) ? argv[5] : (std::string(input) + ".enc");

    if (cost < 0 || cost > 31) {
        std::fprintf(stderr, "Error: cost must be between 0 and 31\n");
        return 1;
    }

    int rc = encrypt_file(input, output.c_str(), password, cost);
    if (rc == 0) {
        std::printf("Encrypted: %s\n", output.c_str());
    } else {
        std::fprintf(stderr, "Error: encryption failed (code %d)\n", rc);
    }
    return rc;
}

/* ── Decrypt command ──────────────────────────────────────────────────────── */

static int cmd_decrypt(int argc, char *argv[]) {
    // argv: [0]=encry [1]=decrypt [2]=file [3]=password [4]=output?
    if (argc < 4) {
        std::printf("Usage: encry decrypt <file> <password> [output_file]\n");
        return 1;
    }
    const char *input    = argv[2];
    const char *password = argv[3];
    std::string output   = (argc >= 5) ? argv[4] : (std::string(input) + ".dec");

    int rc = decrypt_file(input, output.c_str(), password);
    if (rc == 0) {
        std::printf("Decrypted: %s\n", output.c_str());
    } else if (rc == -2) {
        std::fprintf(stderr, "Error: wrong password\n");
    } else {
        std::fprintf(stderr, "Error: decryption failed (code %d)\n", rc);
    }
    return (rc == 0) ? 0 : 1;
}

/* ── Entry point ──────────────────────────────────────────────────────────── */

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

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (std::strcmp(cmd, "hash")    == 0) return cmd_hash(argc, argv);
    if (std::strcmp(cmd, "encrypt") == 0) return cmd_encrypt(argc, argv);
    if (std::strcmp(cmd, "decrypt") == 0) return cmd_decrypt(argc, argv);
    if (std::strcmp(cmd, "-h") == 0 || std::strcmp(cmd, "--help") == 0 || std::strcmp(cmd, "--usage") == 0 || std::strcmp(cmd, "-?") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    std::fprintf(stderr, "Unknown command: %s\n", cmd);
    print_usage(argv[0]);
    return 1;
}
