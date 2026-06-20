# SPDX-License-Identifier: GPL-3.0-only
#
add_test(NAME ScriptTest.release-published
    COMMAND ${CMAKE_COMMAND}
        "-DRELEASE_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/release-published.cmake"
        "-DINPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/inputs/release-published"
        "-DGOLD_DIR=${CMAKE_CURRENT_SOURCE_DIR}/gold/release-published"
        "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/release-published"
        "-DCMAKE_MODULE_PATH=${CMAKE_SOURCE_DIR}/tests/cmake"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/release_published_test.cmake"
)
set_tests_properties(ScriptTest.release-published PROPERTIES LABELS "scripts")

add_test(NAME ScriptTest.new-version
    COMMAND ${CMAKE_COMMAND}
        "-DNEW_VERSION_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/new-version.cmake"
        "-DINPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/inputs/new-version"
        "-DGOLD_DIR=${CMAKE_CURRENT_SOURCE_DIR}/gold/new-version"
        "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/new-version"
        "-DCMAKE_MODULE_PATH=${CMAKE_SOURCE_DIR}/tests/cmake"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/new_version_test.cmake"
)
set_tests_properties(ScriptTest.new-version PROPERTIES LABELS "scripts")

add_test(NAME ScriptTest.release-notes
    COMMAND ${CMAKE_COMMAND}
        "-DRELEASE_NOTES_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/release-notes.cmake"
        "-DINPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/inputs/release-notes"
        "-DGOLD_DIR=${CMAKE_CURRENT_SOURCE_DIR}/gold/release-notes"
        "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/release-notes"
        "-DCMAKE_MODULE_PATH=${CMAKE_SOURCE_DIR}/tests/cmake"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/release_notes_test.cmake"
)
set_tests_properties(ScriptTest.release-notes PROPERTIES LABELS "scripts")
