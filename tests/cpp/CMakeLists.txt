cmake_minimum_required(VERSION 3.18)

add_executable(test_hash   test_hash.cpp)
add_executable(test_cipher test_cipher.cpp)

target_link_libraries(test_hash   PRIVATE enigma_core)
target_link_libraries(test_cipher PRIVATE enigma_core)

add_test(NAME hash_tests   COMMAND test_hash)
add_test(NAME cipher_tests COMMAND test_cipher)
