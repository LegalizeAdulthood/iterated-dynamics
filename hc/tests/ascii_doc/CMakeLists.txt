# SPDX-License-Identifier: GPL-3.0-only
#
set(HC_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_data)

function(add_asciidoc_test name)
    cmake_parse_arguments(HC_TEST "" "" "PARAMETERS" ${ARGN})
    list(APPEND HC_TEST_PARAMETERS /adoc /r ${name} /o ${name} ${name}.src)
    configure_file(${name}.src ${HC_TEST_DIR}/${name}.src COPYONLY)
    file(MAKE_DIRECTORY ${HC_TEST_DIR}/${name})
    add_test(NAME TestAsciiDocOutput.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DHC=$<SHELL_PATH:$<TARGET_FILE:hc>>"
            "-DPARAMETERS=${HC_TEST_PARAMETERS}"
            "-DGOLD_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${name}.adoc"
            "-DADOC_FILE=${HC_TEST_DIR}/${name}/id.adoc"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/help_compare_test.cmake"
        WORKING_DIRECTORY "${HC_TEST_DIR}"
    )
    set_tests_properties(TestAsciiDocOutput.${name} PROPERTIES LABELS "hc;run-hc;adoc")
endfunction()

add_asciidoc_test(toc)
add_asciidoc_test(sections)
add_asciidoc_test(keys)
add_asciidoc_test(bullets)
add_asciidoc_test(links)
add_asciidoc_test(table)
