# SPDX-License-Identifier: GPL-3.0-only
#
set(HC_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_data)
find_program(ASCII_DOCTOR NAMES asciidoctor.bat asciidoctor)

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
            "-DCMAKE_MODULE_PATH=${CMAKE_SOURCE_DIR}/tests/cmake"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/help_compare_test.cmake"
        WORKING_DIRECTORY "${HC_TEST_DIR}"
    )
    set_tests_properties(TestAsciiDocOutput.${name} PROPERTIES LABELS "hc;run-hc;adoc")
endfunction()

function(add_asciidoc_render_test name)
    cmake_parse_arguments(HC_TEST "" "" "PARAMETERS" ${ARGN})
    set(render_name render_${name})
    list(APPEND HC_TEST_PARAMETERS /adoc /r ${render_name} /o ${render_name} ${name}.src)
    configure_file(${name}.src ${HC_TEST_DIR}/${name}.src COPYONLY)
    file(MAKE_DIRECTORY ${HC_TEST_DIR}/${render_name})
    add_test(NAME TestAsciiDocRendered.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DHC=$<SHELL_PATH:$<TARGET_FILE:hc>>"
            "-DASCIIDOCTOR=${ASCII_DOCTOR}"
            "-DPARAMETERS=${HC_TEST_PARAMETERS}"
            "-DADOC_FILE=${HC_TEST_DIR}/${render_name}/id.adoc"
            "-DHTML_FILE=${HC_TEST_DIR}/${render_name}/id.html"
            "-DCHECK_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${name}.render"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/render_check_test.cmake"
        WORKING_DIRECTORY "${HC_TEST_DIR}"
    )
    set_tests_properties(TestAsciiDocRendered.${name}
        PROPERTIES LABELS "hc;run-hc;adoc;rendered-adoc")
endfunction()

function(add_asciidoc_failure_test name)
    cmake_parse_arguments(HC_TEST "" "" "PARAMETERS" ${ARGN})
    list(APPEND HC_TEST_PARAMETERS /adoc /r ${name} /o ${name} ${name}.src)
    configure_file(${name}.src ${HC_TEST_DIR}/${name}.src COPYONLY)
    file(MAKE_DIRECTORY ${HC_TEST_DIR}/${name})
    add_test(NAME TestAsciiDocOutput.${name}
        COMMAND "$<TARGET_FILE:hc>" ${HC_TEST_PARAMETERS}
        WORKING_DIRECTORY "${HC_TEST_DIR}"
    )
    set_tests_properties(TestAsciiDocOutput.${name} PROPERTIES
        LABELS "hc;run-hc;adoc"
        WILL_FAIL TRUE
    )
endfunction()

add_asciidoc_test(toc)
add_asciidoc_test(sections)
add_asciidoc_test(keys)
add_asciidoc_test(bullets)
add_asciidoc_test(links)
add_asciidoc_test(special_link_online)
add_asciidoc_test(table)
add_asciidoc_test(stem_matrix)
add_asciidoc_test(raw_blocks)
if(ASCII_DOCTOR)
    add_asciidoc_render_test(stem_matrix)
    add_asciidoc_render_test(table)
    add_asciidoc_render_test(keys)
    add_asciidoc_render_test(links)
else()
    message(STATUS "Skipping rendered AsciiDoc tests; asciidoctor not found.")
endif()
add_asciidoc_failure_test(link_outside_doc)
add_asciidoc_failure_test(missing_link)
add_asciidoc_failure_test(special_link)
add_asciidoc_failure_test(duplicate_anchor)
