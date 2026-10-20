# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif image/control.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=cellular" "rseed=12345"
    "sound=off" "savedir=." "exitnoask=no")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "params=0/1010/21/100000000" "savename=render000.gif"
    "autokeyname=cellular_preparation.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Cellular preparation interruption failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "params=0/1010/21/0" "savename=control.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Cellular control rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_PREPARATION}"
    image/render000.gif image/render001.gif image/control.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Cellular preparation verification failed: ${RESULT}")
endif()
