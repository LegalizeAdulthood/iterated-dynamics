# SPDX-License-Identifier: GPL-3.0-only
#
function(compare_gold_text_files expected_file actual_file failure_message)
    cmake_parse_arguments(COMPARE_TEXT "IGNORE_EOL" "" "" ${ARGN})
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

    execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${compare_args}
            "${expected_file}" "${actual_file}"
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
    message(STATUS "----- expected begin -----")
    execute_process(COMMAND ${CMAKE_COMMAND} -E cat "${expected_file}")
    message(STATUS "----- expected end -----")
    message(STATUS "Actual file: ${actual_file}")
    message(STATUS "----- actual begin -----")
    execute_process(COMMAND ${CMAKE_COMMAND} -E cat "${actual_file}")
    message(STATUS "----- actual end -----")
    message(FATAL_ERROR "${failure_message}")
endfunction()
