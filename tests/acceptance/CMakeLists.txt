# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2014-2026 Richard Thomson
#
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    return()
endif()

include(PlatformArgs)

function(add_savedir_acceptance_test name)
    cmake_parse_arguments(SAVEDIR_TEST "USER_DIRS" "" "" ${ARGN})
    if(SAVEDIR_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${SAVEDIR_TEST_UNPARSED_ARGUMENTS}")
    endif()

    add_test(NAME AcceptanceTest.savedir-${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
            "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/savedir-${name}"
            "-DTEST_USER_DIRS=${SAVEDIR_TEST_USER_DIRS}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/savedir_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
    set_tests_properties(AcceptanceTest.savedir-${name}
        PROPERTIES LABELS "acceptance;linux;savedir")
endfunction()

add_savedir_acceptance_test(xdg-user-dirs USER_DIRS)
add_savedir_acceptance_test(documents-fallback)
