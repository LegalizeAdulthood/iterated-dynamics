# SPDX-License-Identifier: GPL-3.0-only
#
include(CompareGoldText)

function(require_var name)
    if(NOT DEFINED ${name} OR "${${name}}" STREQUAL "")
        message(FATAL_ERROR "${name} must be specified")
    endif()
endfunction()

function(run_command working_dir)
    execute_process(
        COMMAND ${ARGN}
        WORKING_DIRECTORY "${working_dir}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(STATUS "----- stdout -----")
        message(STATUS "${output}")
        message(STATUS "----- stderr -----")
        message(STATUS "${error}")
        message(FATAL_ERROR "Command failed: ${ARGN}")
    endif()
endfunction()

function(stage_inputs)
    file(COPY "${INPUT_DIR}/source/" DESTINATION "${TEST_SOURCE_DIR}")
endfunction()

function(run_release_notes_script)
    run_command(
        "${TEST_WORK_DIR}"
        "${CMAKE_COMMAND}"
        "-DID_SOURCE_DIR:STRING=${TEST_SOURCE_DIR}"
        "-DRELEASE_VERSION:STRING=1.4.0"
        "-DOUTPUT_FILE:STRING=${ACTUAL_DIR}/release-notes.md"
        -P "${RELEASE_NOTES_SCRIPT}")
endfunction()

function(compare_results)
    compare_gold_text_files(
        "${GOLD_DIR}/release-notes.md"
        "${ACTUAL_DIR}/release-notes.md"
        "release notes did not match gold"
        IGNORE_EOL)
endfunction()

require_var(RELEASE_NOTES_SCRIPT)
require_var(INPUT_DIR)
require_var(GOLD_DIR)
require_var(TEST_WORK_DIR)

file(REMOVE_RECURSE "${TEST_WORK_DIR}")
file(MAKE_DIRECTORY "${TEST_WORK_DIR}")

set(TEST_SOURCE_DIR "${TEST_WORK_DIR}/source")
set(ACTUAL_DIR "${TEST_WORK_DIR}/actual")
file(MAKE_DIRECTORY "${ACTUAL_DIR}")

stage_inputs()
run_release_notes_script()
compare_results()
