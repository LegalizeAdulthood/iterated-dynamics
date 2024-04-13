# Until the unix port is complete, we can't run end-to-end image tests there
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

function(add_image_test name)
    cmake_parse_arguments(IMAGE_TEST "" "" "PARAMETERS")

    list(APPEND IMAGE_TEST_PARAMETERS "batch=yes" "video=F6")
    add_test(NAME ImageTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
            "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-${name}.gif"
            "-DPARAMETERS=${IMAGE_TEST_PARAMETERS}"
            "-DTEST_SAVE_IMAGE=test-${name}.gif"
            "-DTEST_KEEP_IMAGE=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/image_compare_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
endfunction()

add_image_test(mandel PARAMETERS type=mandel)
