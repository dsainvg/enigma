cmake_minimum_required(VERSION 3.18)

project(enigma
    VERSION 0.1.0
    DESCRIPTION "Custom password hashing and file encryption"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Options ────────────────────────────────────────────────────────────────
option(ENIGMA_BUILD_CLI     "Build the enigma CLI tool"           ON)
option(ENIGMA_BUILD_PYTHON  "Build the Python extension module"   OFF)
option(ENIGMA_BUILD_TESTS   "Build C++ test suite"                ON)

# ── Core library (always built) ────────────────────────────────────────────
add_subdirectory(core)

# ── CLI ───────────────────────────────────────────────────────────────────
if(ENIGMA_BUILD_CLI)
    add_subdirectory(cli)
endif()

# ── Python bindings ────────────────────────────────────────────────────────
# scikit-build-core sets ENIGMA_BUILD_PYTHON=ON and calls CMake directly.
# For manual builds: cmake -DENIGMA_BUILD_PYTHON=ON ..
if(ENIGMA_BUILD_PYTHON)
    add_subdirectory(python)
endif()

# ── Tests ──────────────────────────────────────────────────────────────────
if(ENIGMA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/cpp)
endif()
