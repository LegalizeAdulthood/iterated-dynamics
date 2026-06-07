# SPDX-License-Identifier: GPL-3.0-only
#
function(dump_var name)
    message(STATUS "${name}=${${name}}")
endfunction()

include(CompareGoldText)

set(DEBUG ON)
if(DEBUG)
    set(COMMAND_ECHO "STDOUT")
else()
    set(COMMAND_ECHO "NONE")
endif()

if(DEBUG)
    dump_var(ID)
    dump_var(IMAGE_COMPARE)
    dump_var(INPUT_IMAGE)
    dump_var(AUTO_KEY)
    dump_var(GOLD_IMAGE)
    dump_var(TEST_IMAGE)
    dump_var(SCRIPT_FILE)
    dump_var(GOLD_SCRIPT)
    dump_var(GOLD_PAR)
    dump_var(ID_EXTRA_ARGS)
    dump_var(NORMALIZE_SCRIPT_EXECUTABLE)
    dump_var(RUN_SCRIPT)
endif()

set(STOP_MESSAGE_FILE "debug/stopmsg.txt")
set(PAR_FILE "par/make_mig_pieces.par")

file(MAKE_DIRECTORY "par")
file(REMOVE
    "${SCRIPT_FILE}"
    "${PAR_FILE}")

execute_process(
    COMMAND "${ID}" ${ID_EXTRA_ARGS} "${INPUT_IMAGE}" "video=F6" "askvideo=no"
        "sound=off" "savedir=." "overwrite=yes" "exitnoask=yes"
        "autokeyname=${AUTO_KEY}" "autokey=play"
    RESULT_VARIABLE ID_RESULT
    COMMAND_ECHO ${COMMAND_ECHO})
if(ID_RESULT)
    if(EXISTS "${STOP_MESSAGE_FILE}")
        file(READ "${STOP_MESSAGE_FILE}" STOP_MESSAGE)
        message(STATUS "${STOP_MESSAGE_FILE}:\n${STOP_MESSAGE}")
    endif()
    message(FATAL_ERROR "Id execution failed with result ${ID_RESULT}.")
endif()
if(NOT EXISTS "${SCRIPT_FILE}")
    message(FATAL_ERROR "MIG script file '${SCRIPT_FILE}' does not exist.")
endif()
if(NOT EXISTS "${PAR_FILE}")
    message(FATAL_ERROR "MIG parameter file '${PAR_FILE}' does not exist.")
endif()

set(ACTUAL_SCRIPT "${SCRIPT_FILE}")
if(NORMALIZE_SCRIPT_EXECUTABLE)
    file(READ "${SCRIPT_FILE}" SCRIPT_TEXT)
    string(REGEX REPLACE "id_bin='[^']*'" "id_bin='ID_EXECUTABLE'"
        SCRIPT_TEXT "${SCRIPT_TEXT}")
    set(ACTUAL_SCRIPT "${SCRIPT_FILE}.normalized")
    file(WRITE "${ACTUAL_SCRIPT}" "${SCRIPT_TEXT}")
endif()

compare_gold_text_files("${GOLD_SCRIPT}" "${ACTUAL_SCRIPT}"
    "Generated MIG script file differs from '${GOLD_SCRIPT}'." IGNORE_EOL)
compare_gold_text_files("${GOLD_PAR}" "${PAR_FILE}"
    "Generated MIG parameter file differs from '${GOLD_PAR}'.")

if(RUN_SCRIPT)
    set(OUTPUT "image/fractmig.gif")
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
    execute_process(
        COMMAND "./${SCRIPT_FILE}"
        RESULT_VARIABLE SCRIPT_RESULT
        COMMAND_ECHO ${COMMAND_ECHO})
    if(SCRIPT_RESULT)
        if(EXISTS "${STOP_MESSAGE_FILE}")
            file(READ "${STOP_MESSAGE_FILE}" STOP_MESSAGE)
            message(STATUS "${STOP_MESSAGE_FILE}:\n${STOP_MESSAGE}")
        endif()
        message(FATAL_ERROR "MIG script failed with result ${SCRIPT_RESULT}.")
    endif()
    if(NOT EXISTS "${OUTPUT}")
        message(FATAL_ERROR "MIG output '${OUTPUT}' does not exist.")
    endif()
    foreach(component 00 01 10 11)
        if(EXISTS "image/frmig_${component}.gif")
            message(FATAL_ERROR
                "MIG component 'image/frmig_${component}.gif' was not removed.")
        endif()
    endforeach()

    file(COPY_FILE "${OUTPUT}" "${TEST_IMAGE}")
    execute_process(COMMAND "${IMAGE_COMPARE}" "${GOLD_IMAGE}" "${TEST_IMAGE}"
        COMMAND_ERROR_IS_FATAL ANY
        COMMAND_ECHO ${COMMAND_ECHO})
endif()
