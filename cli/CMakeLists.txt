cmake_minimum_required(VERSION 3.18)

# ── enigma CLI executable ───────────────────────────────────────────────────
add_executable(enigma src/main.cpp)

target_link_libraries(enigma PRIVATE enigma_core)

# Strip release builds for smaller binaries
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(NOT MSVC)
        target_link_options(enigma PRIVATE -s)
    endif()
endif()

install(TARGETS enigma RUNTIME DESTINATION bin)
