# Copyright (C) 2022 Tycho Softworks.
#
# This file is free software; as a special exception the author gives unlimited
# permission to copy and/or distribute it, with or without modifications, as
# long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY, to the extent permitted by law; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

cmake_minimum_required(VERSION 3.16.0)
project(ModernCLI VERSION 0.9.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

file(
    GLOB LINT_SOURCES
    RELATIVE ${PROJECT_SOURCE_DIR}
    src/*.hpp test/*.cpp)

include(cmake/custom.cmake OPTIONAL)
include(cmake/project.cmake)
include(cmake/features.cmake)
include(cmake/coverage.cmake)
include(cmake/linting.cmake)
include(cmake/deploy.cmake OPTIONAL)

file(GLOB optional .git[a-z]* *.json *.in *.yml cmake/*)
file(GLOB markdown *.md)
file(GLOB headers src/*.hpp)

# Setup headers
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
configure_file(moderncli.pc.in moderncli.pc)

# Testing
enable_testing()
add_executable(test_shell test/shell.cpp src/print.hpp src/args.hpp src/filesystem.hpp)
add_test(NAME test-shell COMMAND test_shell)
target_link_libraries(test_shell PRIVATE fmt::fmt)

add_executable(test_keyfile test/keyfile.cpp src/keyfile.hpp test/test.conf)
set_target_properties(test_keyfile PROPERTIES COMPILE_DEFINITIONS "TEST_DATA=\"${CMAKE_SOURCE_DIR}/test\"")
add_test(NAME test-keyfile COMMAND test_keyfile)

add_executable(test_strings test/strings.cpp src/strings.hpp src/datetime.hpp src/print.hpp)
add_test(NAME test-strings COMMAND test_strings)
target_link_libraries(test_strings PRIVATE fmt::fmt)

add_executable(test_sync test/sync.cpp src/sync.hpp)
add_test(NAME test-sync COMMAND test_sync)
target_link_libraries(test_sync PRIVATE Threads::Threads)

add_executable(test_atomics test/atomics.cpp src/atomics.hpp)
add_test(NAME test-atomics COMMAND test_atomics)
target_link_libraries(test_atomics PRIVATE Threads::Threads)

add_executable(test_endian test/endian.cpp src/endian.hpp)
add_test(NAME test-endian COMMAND test_endian)

add_executable(test_output test/output.cpp src/output.hpp)
add_test(NAME test-output COMMAND test_output)

add_executable(test_socket test/socket.cpp src/socket.hpp)
add_test(NAME test-socket COMMAND test_socket)
if(WIN32)
    target_link_libraries(test_socket PRIVATE Threads::Threads fmt::fmt ws2_32)
else()
    target_link_libraries(test_socket PRIVATE Threads::Threads fmt::fmt)
endif()

add_executable(test_process test/process.cpp src/process.hpp)
add_test(NAME test-process COMMAND test_process)

add_executable(test_expected test/expected.cpp src/expected.hpp)
add_test(NAME test-expected COMMAND test_expected)

add_executable(test_digest test/digest.cpp src/digest.hpp)
target_link_libraries(test_digest PRIVATE OpenSSL::Crypto)
add_test(NAME test-digest COMMAND test_digest)

add_executable(test_random test/random.cpp src/random.hpp)
target_link_libraries(test_random PRIVATE OpenSSL::Crypto)
add_test(NAME test-random COMMAND test_random)

add_executable(test_cipher test/cipher.cpp src/cipher.hpp src/random.hpp)
target_link_libraries(test_cipher PRIVATE OpenSSL::Crypto)
add_test(NAME test-cipher COMMAND test_cipher)

if(NOT WIN32)
    add_executable(test_serial test/serial.cpp src/serial.hpp)
    add_test(NAME test-serial COMMAND test_serial)
endif()

# Extras...
add_custom_target(header-files SOURCES ${headers})
add_custom_target(support-files SOURCES .clang-tidy ${markdown} ${optional})

install(FILES ${headers} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/moderncli")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/moderncli.pc DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
