# Copyright (C) 2021 Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

if(LINT_SOURCES)
    find_program(CLANG_TIDY_EXEC "clang-tidy")
    find_program(CPPCHECK_EXEC "cppcheck")

    if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/cppcheck.arg" AND CPPCHECK_EXEC)
        list(APPEND LINT_DEPENDS cppcheck)
        file(READ "${CMAKE_SOURCE_DIR}/cmake/cppcheck.arg" cppcheckArgs)
        string(STRIP "${cppcheckArgs}" cppcheckArgs)
        separate_arguments(cppcheckArgs)
        add_custom_target(cppcheck
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            USES_TERMINAL
            COMMAND ${CPPCHECK_EXEC} -DLINT=1 -I${PROJECT_BINARY_DIR} ${cppcheckArgs} --force --std=c++${CMAKE_CXX_STANDARD} --quiet ${LINT_SOURCES}
        )
    endif()

    if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/tidy.arg" AND CLANG_TIDY_EXEC)
        list(APPEND LINT_DEPENDS lint_tidy)
        file(READ "${CMAKE_SOURCE_DIR}/cmake/tidy.arg" tidyArgs)
        string(STRIP "${LINT_INCLUDES}" TIDY_INCLUDES)
        string(STRIP "${tidyArgs}" tidyArgs)
        separate_arguments(tidyArgs)
        add_custom_target(lint_tidy
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            USES_TERMINAL
            COMMAND ${CLANG_TIDY_EXEC} ${LINT_SOURCES} -quiet -- -std=c++${CMAKE_CXX_STANDARD} -I${PROJECT_BINARY_DIR} -DLINT=1 ${TIDY_INCLUDES} ${tidyArgs}
        )
    endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/.rubocop.yml")
    find_program(RUBOCOP_EXEC "rubocop")
    if(RUBOCOP_EXEC)
        list(APPEND LINT_DEPENDS lint_ruby)
        add_custom_target(lint_ruby
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            USES_TERMINAL
            COMMAND ${RUBOCOP_EXEC}
        )
    endif()
endif()

if(LINT_DEPENDS)
    add_custom_target(lint
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS ${LINT_DEPENDS}
    )
endif()

