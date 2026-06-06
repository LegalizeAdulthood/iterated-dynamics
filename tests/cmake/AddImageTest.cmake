# SPDX-License-Identifier: GPL-3.0-only
#

function(add_image_test name)
    cmake_parse_arguments(IMAGE_TEST "DISABLED;IGNORE_COLORMAP;POTENTIAL" "GOLD;WORKING_DIRECTORY"
        "PARAMETERS;LABELS" ${ARGN})
    if(IMAGE_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${IMAGE_TEST_UNPARSED_ARGUMENTS}")
    endif()
    set(IMAGE_TEST_SUFFIX "gif")
    if(IMAGE_TEST_POTENTIAL)
        set(IMAGE_TEST_SUFFIX "pot")
    endif()
    if(NOT IMAGE_TEST_WORKING_DIRECTORY)
        set(IMAGE_TEST_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home")
    endif()
    if(NOT IMAGE_TEST_GOLD)
        set(IMAGE_TEST_GOLD "gold-${name}")
    endif()
    set(IMAGE_TEST_IGNORE_COLORMAP_VALUE OFF)
    if(IMAGE_TEST_IGNORE_COLORMAP)
        set(IMAGE_TEST_IGNORE_COLORMAP_VALUE ON)
        list(APPEND IMAGE_TEST_LABELS "ignore-colormap")
    endif()
    list(APPEND IMAGE_TEST_PARAMETERS "batch=yes" "video=F6" "sound=off")
    add_test(NAME ImageTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
            "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/${IMAGE_TEST_GOLD}.${IMAGE_TEST_SUFFIX}"
            "-DPARAMETERS=${IMAGE_TEST_PARAMETERS}"
            "-DTEST_SAVE_IMAGE=test-${name}.${IMAGE_TEST_SUFFIX}"
            "-DTEST_KEEP_IMAGE=${CMAKE_CURRENT_BINARY_DIR}"
            "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/${name}-diff.gif"
            "-DIMAGE_TEST_IGNORE_COLORMAP=${IMAGE_TEST_IGNORE_COLORMAP_VALUE}"
            "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/image_compare_test.cmake"
        WORKING_DIRECTORY "${IMAGE_TEST_WORKING_DIRECTORY}"
    )
    list(APPEND IMAGE_TEST_LABELS "image")
    set_tests_properties(ImageTest.${name} PROPERTIES LABELS "${IMAGE_TEST_LABELS}")
    if(IMAGE_TEST_DISABLED)
        set_tests_properties(ImageTest.${name} PROPERTIES DISABLED TRUE)
    endif()
endfunction()
