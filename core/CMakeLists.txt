cmake_minimum_required(VERSION 3.18)

# ── enigma_core static library ──────────────────────────────────────────────
add_library(enigma_core STATIC
    src/hash.cpp         # Password hashing algorithm
    src/salt.cpp         # CSPRNG + salt generation (shared by hash + cipher)
    src/primitives.cpp   # rotate/XOR/byte_manipulations primitives
    src/cipher.cpp       # ENIGMA01 format: encrypt_bytes / decrypt_bytes
    src/file_io.cpp      # File I/O: encrypt_file / decrypt_file
)

set_target_properties(enigma_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(enigma_core
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src   # for salt.h (internal header)
)

target_compile_features(enigma_core PUBLIC cxx_std_17)

if(WIN32)
    target_link_libraries(enigma_core PRIVATE advapi32)
endif()

# Install rules (used when building Python wheel)
install(TARGETS enigma_core EXPORT enigmaTargets)
install(DIRECTORY include/ DESTINATION include)
