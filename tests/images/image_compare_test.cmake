# SPDX-License-Identifier: GPL-3.0-only
#
function(dump_var name)
    message(STATUS "${name}=${${name}}")
endfunction()

set(DEBUG ON)
if(DEBUG)
    set(COMMAND_ECHO "STDOUT")
else()
    set(COMMAND_ECHO "NONE")
endif()

list(APPEND PARAMETERS "savename=${TEST_SAVE_IMAGE}" "savedir=.")

if(DEBUG)
    dump_var(ID)
    dump_var(IMAGE_COMPARE)
    dump_var(GOLD_IMAGE)
    dump_var(PARAMETERS)
    dump_var(TEST_SAVE_IMAGE)
    dump_var(TEST_KEEP_IMAGE)
    dump_var(IMAGE_TEST_IGNORE_COLORMAP)
    message(STATUS "Parameters:")
    foreach(par ${PARAMETERS})
        message(STATUS "    ${par}")
    endforeach()
endif()

file(REMOVE "image/${TEST_SAVE_IMAGE}")
execute_process(COMMAND "${ID}" ${PARAMETERS}
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
if(NOT EXISTS "image/${TEST_SAVE_IMAGE}")
    message(FATAL_ERROR "Image file 'image/${TEST_SAVE_IMAGE}' does not exist.")
endif()
file(RENAME "image/${TEST_SAVE_IMAGE}" "${TEST_KEEP_IMAGE}/${TEST_SAVE_IMAGE}")

set(IMAGE_COMPARE_ARGS "")
if(IMAGE_TEST_IGNORE_COLORMAP)
    list(APPEND IMAGE_COMPARE_ARGS "--ignore-colormap")
endif()
list(APPEND IMAGE_COMPARE_ARGS "${GOLD_IMAGE}" "${TEST_KEEP_IMAGE}/${TEST_SAVE_IMAGE}")
execute_process(COMMAND "${IMAGE_COMPARE}" ${IMAGE_COMPARE_ARGS}
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
