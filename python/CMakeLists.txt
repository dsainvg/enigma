cmake_minimum_required(VERSION 3.18)

# ── Fetch pybind11 ──────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.12.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(pybind11)

# ── Python extension module: _enigma ───────────────────────────────────────
pybind11_add_module(_enigma bindings.cpp)

target_link_libraries(_enigma PRIVATE enigma_core)

# Install the compiled extension into the enigma/ package directory
install(TARGETS _enigma LIBRARY DESTINATION enigma)
