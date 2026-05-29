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
    dump_var(INPUT_IMAGE)
    dump_var(AUTO_KEY)
    dump_var(GOLD_BAT)
    dump_var(GOLD_PAR)
endif()

set(STOP_MESSAGE_FILE "debug/stopmsg.txt")
set(PAR_FILE "par/make_mig_pieces.par")
set(BAT_FILE "makemig.bat")

file(MAKE_DIRECTORY "par")
file(REMOVE
    "${BAT_FILE}"
    "${PAR_FILE}")

execute_process(
    COMMAND "${ID}" "${INPUT_IMAGE}" "video=F6" "askvideo=no"
        "savedir=." "overwrite=yes" "exitnoask=yes"
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
if(NOT EXISTS "${BAT_FILE}")
    message(FATAL_ERROR "MIG batch file '${BAT_FILE}' does not exist.")
endif()
if(NOT EXISTS "${PAR_FILE}")
    message(FATAL_ERROR "MIG parameter file '${PAR_FILE}' does not exist.")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${GOLD_BAT}" "${BAT_FILE}"
    RESULT_VARIABLE BAT_COMPARE_RESULT
    COMMAND_ECHO ${COMMAND_ECHO})
if(BAT_COMPARE_RESULT)
    message(FATAL_ERROR "Generated MIG batch file differs from '${GOLD_BAT}'.")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${GOLD_PAR}" "${PAR_FILE}"
    RESULT_VARIABLE PAR_COMPARE_RESULT
    COMMAND_ECHO ${COMMAND_ECHO})
if(PAR_COMPARE_RESULT)
    message(FATAL_ERROR "Generated MIG parameter file differs from '${GOLD_PAR}'.")
endif()
