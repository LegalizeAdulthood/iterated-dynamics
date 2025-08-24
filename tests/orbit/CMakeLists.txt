# SPDX-License-Identifier: GPL-3.0-only
#
# Until the unix port is complete, we can't run end-to-end orbit tests there
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

function(add_orbit_test name)
    cmake_parse_arguments(ORBIT_TEST "DISABLED" "" "PARAMETERS;LABELS" ${ARGN})
    if(ORBIT_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${ORBIT_TEST_UNPARSED_ARGUMENTS}")
    endif()
    set(TEST_ORBIT "test-${name}")
    list(APPEND ORBIT_TEST_PARAMETERS "batch=yes" "video=F6"
        "orbitsave=y" "orbitsavename=${TEST_ORBIT}" "savedir=."
        "maxiter=2" "type=${name}"
    )
    add_test(NAME OrbitTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DORBIT_NAME=${name}"
            "-DTEST_ORBIT=orbit/${TEST_ORBIT}.raw"
            "-DGOLD_ORBIT=${CMAKE_CURRENT_SOURCE_DIR}/gold-${name}.txt"
            "-DPARAMETERS=${ORBIT_TEST_PARAMETERS}"
            "-DTEST_KEEP_ORBIT=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/orbit_compare_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
    list(APPEND ORBIT_TEST_LABELS "orbit")
    if(ORBIT_TEST_DISABLED)
        set(DISABLED "TRUE")
    else()
        set(DISABELD "FALSE")
    endif()
    set_tests_properties(OrbitTest.${name} PROPERTIES
        DISABLED "${DISABLED}"
        LABELS "${ORBIT_TEST_LABELS}")
endfunction()

foreach(orbit dynamic)
    add_orbit_test("${orbit}")
endforeach()
