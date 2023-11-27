# Copyright (C) 2022 Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

cmake_minimum_required(VERSION 3.16.0)
project(ModernCLI VERSION 0.8.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

file(GLOB LINT_SOURCES RELATIVE ${PROJECT_SOURCE_DIR} src/*.hpp test/*.cpp)

include(cmake/custom.cmake OPTIONAL)
include(cmake/project.cmake)
include(cmake/features.cmake)
include(cmake/coverage.cmake)
include(cmake/linting.cmake)

file(GLOB markdown *.md)
file(GLOB headers src/*.hpp)

# Setup headers

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

configure_file(moderncli.pc.in moderncli.pc)

# Testing
enable_testing()
add_executable(test_shell  test/shell.cpp src/print.hpp src/args.hpp src/filesystem.hpp)
add_test(NAME test-shell COMMAND test_shell)
target_link_libraries(test_shell PRIVATE ${FMT_LIBRARIES})

add_executable(test_audiofile test/audiofile.cpp src/audiofile.hpp)
add_test(NAME test-audiofile COMMAND test_audiofile)

add_executable(test_keyfile  test/keyfile.cpp src/keyfile.hpp test/test.conf)
add_test(NAME test-keyfile COMMAND test_keyfile)

add_executable(test_strings  test/strings.cpp src/strings.hpp)
add_test(NAME test-strings COMMAND test_strings)

add_executable(test_sync  test/sync.cpp src/sync.hpp)
add_test(NAME test-sync COMMAND test_sync)
target_link_libraries(test_sync PRIVATE Threads::Threads)

add_executable(test_endian  test/endian.cpp src/endian.hpp)
add_test(NAME test-endian COMMAND test_endian)

if(NOT WIN32)
	add_executable(test_serial  test/serial.cpp src/serial.hpp)
	add_test(NAME test-serial COMMAND test_serial)
endif()

if(OPENSSL_FOUND)
    add_executable(test_digest  test/digest.cpp src/digest.hpp)
    target_link_libraries(test_digest PRIVATE ${CRYPTO_LIBS})
    target_include_directories(test_digest PRIVATE "${OPENSSL_INCLUDE_DIR}")
    add_test(NAME test-digest COMMAND test_digest)

    add_executable(test_random  test/random.cpp src/random.hpp)
    target_link_libraries(test_random PRIVATE ${CRYPTO_LIBS})
    target_include_directories(test_random PRIVATE "${OPENSSL_INCLUDE_DIR}")
    add_test(NAME test-random COMMAND test_random)
endif()

# Extras...

add_custom_target(header-files SOURCES ${headers})
add_custom_target(support-files SOURCES moderncli.pc.in ${markdown})

install(FILES ${headers} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/moderncli")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/moderncli.pc DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")

