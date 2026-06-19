# SPDX-License-Identifier: GPL-3.0-only
#
include(CompareGoldText)

find_program(GIT_EXECUTABLE git REQUIRED)

function(require_var name)
    if(NOT DEFINED ${name} OR "${${name}}" STREQUAL "")
        message(FATAL_ERROR "${name} must be specified")
    endif()
endfunction()

function(write_text path content)
    get_filename_component(parent_dir "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${parent_dir}")
    file(WRITE "${path}" "${content}")
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

function(run_command_capture out_var working_dir)
    execute_process(
        COMMAND ${ARGN}
        WORKING_DIRECTORY "${working_dir}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT result EQUAL 0)
        message(STATUS "----- stdout -----")
        message(STATUS "${output}")
        message(STATUS "----- stderr -----")
        message(STATUS "${error}")
        message(FATAL_ERROR "Command failed: ${ARGN}")
    endif()
    set(${out_var} "${output}" PARENT_SCOPE)
endfunction()

function(stage_inputs)
    file(COPY "${INPUT_DIR}/" DESTINATION "${TEST_WORK_DIR}")
endfunction()

function(initialize_pages_repo pages_dir)
    run_command("${pages_dir}" "${GIT_EXECUTABLE}" init)
    run_command("${pages_dir}" "${GIT_EXECUTABLE}" checkout -b gh-pages)
    run_command("${pages_dir}" "${GIT_EXECUTABLE}" add .)
    run_command(
        "${pages_dir}"
        "${GIT_EXECUTABLE}"
        -c user.email=release-test@example.com
        -c user.name=release-test
        commit -m "initial gh-pages")
endfunction()

function(run_release_script source_dir pages_dir)
    run_command(
        "${TEST_WORK_DIR}"
        "${CMAKE_COMMAND}"
        "-DID_SOURCE_DIR:STRING=${source_dir}"
        "-DRELEASE_VERSION:STRING=1.4.0"
        "-DNEXT_VERSION:STRING=1.4.1"
        "-DRELEASE_PAGES_DIR:STRING=${pages_dir}"
        -P "${RELEASE_SCRIPT}")
endfunction()

function(write_git_summary pages_dir summary_file)
    run_command_capture(
        inside_work_tree
        "${pages_dir}"
        "${GIT_EXECUTABLE}" rev-parse --is-inside-work-tree)
    run_command_capture(
        branch_name
        "${pages_dir}"
        "${GIT_EXECUTABLE}" branch --show-current)
    run_command_capture(
        status
        "${pages_dir}"
        "${GIT_EXECUTABLE}" status --short --untracked-files=all)

    write_text("${summary_file}" "inside-work-tree=${inside_work_tree}
branch=${branch_name}
status:
${status}
")
endfunction()

function(compare_results source_dir pages_dir actual_dir)
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
    compare_gold_text_files(
        "${GOLD_DIR}/source/hc/src/help.src"
        "${source_dir}/hc/src/help.src"
        "help.src did not match gold"
        IGNORE_EOL)
    compare_gold_text_files(
        "${GOLD_DIR}/source/hc/src/help5.src"
        "${source_dir}/hc/src/help5.src"
        "help5.src did not match gold"
        IGNORE_EOL)
    compare_gold_text_files(
        "${GOLD_DIR}/pages/index.html"
        "${pages_dir}/index.html"
        "promoted pages/index.html did not match gold"
        IGNORE_EOL)
    compare_gold_text_files(
        "${GOLD_DIR}/pages/help/topic.txt"
        "${pages_dir}/help/topic.txt"
        "promoted pages/help/topic.txt did not match gold"
        IGNORE_EOL)

    if(EXISTS "${pages_dir}/help/old.txt")
        message(FATAL_ERROR "Old current help file was not removed.")
    endif()

    write_git_summary("${pages_dir}" "${actual_dir}/git.txt")
    compare_gold_text_files(
        "${GOLD_DIR}/git.txt"
        "${actual_dir}/git.txt"
        "git query output did not match gold"
        IGNORE_EOL)
endfunction()

require_var(RELEASE_SCRIPT)
require_var(INPUT_DIR)
require_var(GOLD_DIR)
require_var(TEST_WORK_DIR)

file(REMOVE_RECURSE "${TEST_WORK_DIR}")
file(MAKE_DIRECTORY "${TEST_WORK_DIR}")

set(source_dir "${TEST_WORK_DIR}/source")
set(pages_dir "${TEST_WORK_DIR}/pages")
set(actual_dir "${TEST_WORK_DIR}/actual")
file(MAKE_DIRECTORY "${actual_dir}")

stage_inputs()
initialize_pages_repo("${pages_dir}")
run_release_script("${source_dir}" "${pages_dir}")
compare_results("${source_dir}" "${pages_dir}" "${actual_dir}")
