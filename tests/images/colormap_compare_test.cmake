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
    dump_var(COLORMAP_COMPARE)
    dump_var(TEST_SAVE_IMAGE)
    dump_var(TEST_KEEP_IMAGE)
    dump_var(MAP_FILE)
    dump_var(MAP_DIR)
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

execute_process(COMMAND "${COLORMAP_COMPARE}" "${MAP_DIR}/${MAP_FILE}" "${TEST_KEEP_IMAGE}/${TEST_SAVE_IMAGE}"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
