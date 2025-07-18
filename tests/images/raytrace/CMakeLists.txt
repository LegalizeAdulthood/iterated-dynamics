# SPDX-License-Identifier: GPL-3.0-only
#
# Until the unix port is complete, we can't run end-to-end image tests there
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

set(RAYTRACE_TYPE_acrospin "6")
set(RAYTRACE_TYPE_dxf "7")
set(RAYTRACE_TYPE_mtv "4")
set(RAYTRACE_TYPE_povray "1")
set(RAYTRACE_TYPE_raw "3")
set(RAYTRACE_TYPE_rayshade "5")
set(RAYTRACE_TYPE_vivid "2")

function(add_raytrace_test name)
    cmake_parse_arguments(RAYTRACE_TEST "DISABLED" "" "PARAMETERS;LABELS" ${ARGN})
    if(RAYTRACE_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${RAYTRACE_TEST_UNPARSED_ARGUMENTS}")
    endif()
    list(APPEND RAYTRACE_TEST_PARAMETERS "batch=yes" "video=F6" "3d=yes"
        "scalexyz=90/90" "roughness=30" "waterline=0" "filltype=2"
        "ambient=20" "rotation=60/30/0" "perspective=0" "xyshift=0/0"
        "ray=${RAYTRACE_TYPE_${name}}"
        "savedir=."
        "${CMAKE_CURRENT_SOURCE_DIR}/raytrace.gif"
    )
    add_test(NAME RayTraceTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DRAYTRACE_NAME=${name}"
            "-DGOLD_TRACE=${CMAKE_CURRENT_SOURCE_DIR}/gold-${name}.txt"
            "-DPARAMETERS=${RAYTRACE_TEST_PARAMETERS}"
            "-DTEST_KEEP_RAYTRACE=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/raytrace_compare_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
    list(APPEND RAYTRACE_TEST_LABELS "raytrace")
    if(RAYTRACE_TEST_DISABLED)
        set(DISABLED "TRUE")
    else()
        set(DISABELD "FALSE")
    endif()
    set_tests_properties(RayTraceTest.${name} PROPERTIES
        PROCESSORS "10000"
        DISABLED "${DISABLED}"
        LABELS "${RAYTRACE_TEST_LABELS}")
endfunction()

foreach(raytrace acrospin dxf mtv povray raw rayshade vivid)
    add_raytrace_test("${raytrace}")
endforeach()
