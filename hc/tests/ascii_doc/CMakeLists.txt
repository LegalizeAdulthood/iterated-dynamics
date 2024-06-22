set(HC_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_data)

function(add_hc_test name)
    cmake_parse_arguments(HC_TEST "" "" "PARAMETERS" ${ARGN})
    add_test(NAME TestHelpCompiler.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DHC=$<SHELL_PATH:$<TARGET_FILE:hc>>"
            "-DPARAMETERS=${HC_TEST_PARAMETERS}"
            "-DGOLD_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${name}.adoc"
            "-DADOC_FILE=${HC_TEST_DIR}/${name}/index.adoc"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/help_compare_test.cmake"
        WORKING_DIRECTORY "${HC_TEST_DIR}"
    )
    set_tests_properties(TestHelpCompiler.${name} PROPERTIES LABELS "hc;run-hc")
endfunction()

configure_file(toc.src ${HC_TEST_DIR}/toc.src COPYONLY)
file(MAKE_DIRECTORY ${HC_TEST_DIR}/toc)
add_hc_test(toc PARAMETERS /adoc /r toc /o toc toc.src)

configure_file(sections.src ${HC_TEST_DIR}/sections.src COPYONLY)
file(MAKE_DIRECTORY ${HC_TEST_DIR}/sections)
add_hc_test(sections PARAMETERS /adoc /r sections /o sections sections.src)

configure_file(keys.src ${HC_TEST_DIR}/keys.src COPYONLY)
file(MAKE_DIRECTORY ${HC_TEST_DIR}/keys)
add_hc_test(keys PARAMETERS /adoc /r keys /o keys keys.src)
