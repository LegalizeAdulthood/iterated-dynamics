# SPDX-License-Identifier: GPL-3.0-only
#
set(HC_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_data)

function(add_text_test name)
    cmake_parse_arguments(HC_TEST "" "" "PARAMETERS" ${ARGN})
    list(APPEND HC_TEST_PARAMETERS /p /r ${name} ${name}.src ${name}.txt)
    configure_file(${name}.src ${HC_TEST_DIR}/${name}.src COPYONLY)
    file(MAKE_DIRECTORY ${HC_TEST_DIR}/${name})
    add_test(NAME TestTextOutput.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DHC=$<SHELL_PATH:$<TARGET_FILE:hc>>"
            "-DPARAMETERS=${HC_TEST_PARAMETERS}"
            "-DGOLD_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${name}.txt"
            "-DTEXT_FILE=${HC_TEST_DIR}/${name}.txt"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/help_compare_test.cmake"
        WORKING_DIRECTORY "${HC_TEST_DIR}"
    )
    set_tests_properties(TestTextOutput.${name} PROPERTIES LABELS "hc;run-hc;text")
endfunction()

add_text_test(page-reference)
