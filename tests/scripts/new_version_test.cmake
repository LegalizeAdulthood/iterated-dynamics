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
    file(COPY "${INPUT_DIR}/" DESTINATION "${TEST_WORK_DIR}")
endfunction()

function(run_new_version_script source_dir)
    run_command(
        "${TEST_WORK_DIR}"
        "${CMAKE_COMMAND}"
        "-DID_SOURCE_DIR:STRING=${source_dir}"
        "-DCURRENT_VERSION:STRING=1.4.1"
        "-DNEXT_VERSION:STRING=1.5.0"
        -P "${NEW_VERSION_SCRIPT}")
endfunction()

function(compare_results source_dir)
    compare_gold_text_files(
        "${GOLD_DIR}/source/CMakeLists.txt"
        "${source_dir}/CMakeLists.txt"
        "CMakeLists.txt did not match gold"
        IGNORE_EOL)
    compare_gold_text_files(
        "${GOLD_DIR}/source/packaging/CMakeLists.txt"
        "${source_dir}/packaging/CMakeLists.txt"
        "packaging/CMakeLists.txt did not match gold"
        IGNORE_EOL)
endfunction()

require_var(NEW_VERSION_SCRIPT)
require_var(INPUT_DIR)
require_var(GOLD_DIR)
require_var(TEST_WORK_DIR)

file(REMOVE_RECURSE "${TEST_WORK_DIR}")
file(MAKE_DIRECTORY "${TEST_WORK_DIR}")

set(source_dir "${TEST_WORK_DIR}/source")

stage_inputs()
run_new_version_script("${source_dir}")
compare_results("${source_dir}")
