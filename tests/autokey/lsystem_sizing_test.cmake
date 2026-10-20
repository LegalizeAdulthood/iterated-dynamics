# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif debug/stopmsg.txt)
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS}
    "video=F6" "askvideo=no" "type=lsystem" "lfile=${LSYSTEM_FILE}"
    "lname=Koch1" "params=9" "sound=off" "savedir=."
    "savename=render000.gif" "exitnoask=no"
    "autokeyname=lsystem_sizing.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "L-system sizing interruption/restart failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_SIZING}" image/render000.gif image/render001.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "L-system sizing verification failed: ${RESULT}")
endif()
