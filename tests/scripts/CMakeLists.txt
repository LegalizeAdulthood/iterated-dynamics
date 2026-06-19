# SPDX-License-Identifier: GPL-3.0-only
#
add_test(NAME ScriptTest.release-published
    COMMAND ${CMAKE_COMMAND}
        "-DRELEASE_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/release-published.cmake"
        "-DINPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/inputs"
        "-DGOLD_DIR=${CMAKE_CURRENT_SOURCE_DIR}/gold"
        "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/release-published"
        "-DCMAKE_MODULE_PATH=${CMAKE_SOURCE_DIR}/tests/cmake"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/release_published_test.cmake"
)
set_tests_properties(ScriptTest.release-published PROPERTIES LABELS "scripts")
