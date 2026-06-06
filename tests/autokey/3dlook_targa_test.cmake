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
    dump_var(GOLD_IMAGE)
    dump_var(ID_EXTRA_ARGS)
endif()

set(STOP_MESSAGE_FILE "debug/stopmsg.txt")
get_filename_component(TEST_HOME "." ABSOLUTE)
set(POT_FILE "image/potntial.pot")
set(TGA_FILE "image/3dlook-targa.tga")

file(MAKE_DIRECTORY "image")
file(REMOVE "${TGA_FILE}")
if(NOT EXISTS "${POT_FILE}")
    message(FATAL_ERROR "Potential file '${POT_FILE}' does not exist.")
endif()

execute_process(
    COMMAND "${ID}" ${ID_EXTRA_ARGS} "video=F6" "askvideo=no" "parmfile=radar.par"
        "sound=off" "librarydirs=." "savedir=." "overwrite=yes" "exitnoask=yes"
        "lightname=3dlook-targa" "autokeyname=3dlook_targa.key" "autokey=play"
        "@radar/3dlook"
    RESULT_VARIABLE ID_RESULT
    COMMAND_ECHO ${COMMAND_ECHO})
if(ID_RESULT)
    if(EXISTS "${STOP_MESSAGE_FILE}")
        file(READ "${STOP_MESSAGE_FILE}" STOP_MESSAGE)
        message(STATUS "${STOP_MESSAGE_FILE}:\n${STOP_MESSAGE}")
    endif()
    message(FATAL_ERROR "3dlook execution failed with result ${ID_RESULT}.")
endif()
if(NOT EXISTS "${TGA_FILE}")
    message(FATAL_ERROR "Targa file '${TGA_FILE}' does not exist.")
endif()

execute_process(
    COMMAND "${IMAGE_COMPARE}" "${GOLD_IMAGE}" "${TGA_FILE}"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
