# SPDX-License-Identifier: GPL-3.0-only
#
function(normalize_versioned_text input_file output_file)
    file(READ "${input_file}" contents)
    string(REGEX REPLACE
        "Iterated Dynamics Version [0-9]+\\.[0-9]+(\\.[0-9]+)?(\\.[0-9]+)?[ ]+Page"
        "Iterated Dynamics Version <version> Page"
        contents "${contents}")
    string(REGEX REPLACE
        "Created by Iterated Dynamics Ver\\. [0-9]+\\.[0-9]+(\\.[0-9]+)?(\\.[0-9]+)?"
        "Created by Iterated Dynamics Ver. <version>"
        contents "${contents}")
    file(WRITE "${output_file}" "${contents}")
endfunction()

function(compare_gold_text_files expected_file actual_file failure_message)
    cmake_parse_arguments(COMPARE_TEXT "IGNORE_EOL;NORMALIZE_VERSION" "" "" ${ARGN})
    if(COMPARE_TEXT_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${COMPARE_TEXT_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT EXISTS "${expected_file}")
        message(FATAL_ERROR "Expected file '${expected_file}' does not exist.")
    endif()
    if(NOT EXISTS "${actual_file}")
        message(FATAL_ERROR "Actual file '${actual_file}' does not exist.")
    endif()

    set(compare_args)
    if(COMPARE_TEXT_IGNORE_EOL)
        list(APPEND compare_args "--ignore-eol")
    endif()
    set(compare_command_echo)
    if(DEFINED COMMAND_ECHO)
        list(APPEND compare_command_echo COMMAND_ECHO ${COMMAND_ECHO})
    endif()

    set(compare_expected "${expected_file}")
    set(compare_actual "${actual_file}")
    if(COMPARE_TEXT_NORMALIZE_VERSION)
        get_filename_component(actual_dir "${actual_file}" DIRECTORY)
        get_filename_component(actual_name "${actual_file}" NAME)
        set(compare_expected "${actual_dir}/${actual_name}.expected-normalized")
        set(compare_actual "${actual_dir}/${actual_name}.actual-normalized")
        normalize_versioned_text("${expected_file}" "${compare_expected}")
        normalize_versioned_text("${actual_file}" "${compare_actual}")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${compare_args}
            "${compare_expected}" "${compare_actual}"
        RESULT_VARIABLE compare_result
        ${compare_command_echo})
    if(compare_result EQUAL 0)
        if(DEBUG)
            message(STATUS "Files are equal")
        endif()
        return()
    elseif(NOT compare_result EQUAL 1)
        message(FATAL_ERROR "Error running cmake -E compare_files.")
    endif()

    message(STATUS "Expected file: ${expected_file}")
    if(COMPARE_TEXT_NORMALIZE_VERSION)
        message(STATUS "Compared expected file: ${compare_expected}")
    endif()
    message(STATUS "----- expected begin -----")
    execute_process(COMMAND ${CMAKE_COMMAND} -E cat "${compare_expected}")
    message(STATUS "----- expected end -----")
    message(STATUS "Actual file: ${actual_file}")
    if(COMPARE_TEXT_NORMALIZE_VERSION)
        message(STATUS "Compared actual file: ${compare_actual}")
    endif()
    message(STATUS "----- actual begin -----")
    execute_process(COMMAND ${CMAKE_COMMAND} -E cat "${compare_actual}")
    message(STATUS "----- actual end -----")
    message(FATAL_ERROR "${failure_message}")
endfunction()
