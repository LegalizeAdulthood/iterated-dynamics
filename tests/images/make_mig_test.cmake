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

if(DEBUG)
    dump_var(ID)
    dump_var(IMAGE_COMPARE)
    dump_var(COMPONENT_IMAGE_DIR)
    dump_var(GOLD_IMAGE)
    dump_var(TEST_IMAGE)
endif()

set(OUTPUT "image/fractmig.gif")
set(STOP_MESSAGE_FILE "debug/stopmsg.txt")

file(MAKE_DIRECTORY "image")
file(REMOVE
    "image/frmig_00.gif"
    "image/frmig_01.gif"
    "image/frmig_10.gif"
    "image/frmig_11.gif"
    "${OUTPUT}"
    "frmig_00.gif"
    "frmig_01.gif"
    "frmig_10.gif"
    "frmig_11.gif"
    "fractmig.gif"
    "${TEST_IMAGE}")
foreach(component 00 01 10 11)
    set(COMPONENT_IMAGE "${COMPONENT_IMAGE_DIR}/make_mig_${component}.gif")
    if(NOT EXISTS "${COMPONENT_IMAGE}")
        message(FATAL_ERROR "MIG component '${COMPONENT_IMAGE}' does not exist.")
    endif()
    file(COPY_FILE "${COMPONENT_IMAGE}" "image/frmig_${component}.gif")
endforeach()

execute_process(COMMAND "${ID}" "batch=yes" "video=F6" "savedir=." "makemig=2/2"
    RESULT_VARIABLE ID_RESULT
    COMMAND_ECHO ${COMMAND_ECHO})
if(ID_RESULT)
    if(EXISTS "${STOP_MESSAGE_FILE}")
        file(READ "${STOP_MESSAGE_FILE}" STOP_MESSAGE)
        message(STATUS "${STOP_MESSAGE_FILE}:\n${STOP_MESSAGE}")
    endif()
    message(FATAL_ERROR "Id execution failed with result ${ID_RESULT}.")
endif()
if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "MIG output '${OUTPUT}' does not exist.")
endif()
foreach(component 00 01 10 11)
    if(EXISTS "image/frmig_${component}.gif")
        message(FATAL_ERROR "MIG component 'image/frmig_${component}.gif' was not removed.")
    endif()
endforeach()

file(COPY_FILE "${OUTPUT}" "${TEST_IMAGE}")
execute_process(COMMAND "${IMAGE_COMPARE}" "${GOLD_IMAGE}" "${TEST_IMAGE}"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
