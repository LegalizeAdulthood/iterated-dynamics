# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif image/render002.gif
    image/render003.gif image/control.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=mandel" "passes=1"
    "viewwindows=1/0.75/no/8/6" "sound=off" "savedir=."
    "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "savename=render000.gif" "autokeyname=jiim_viewport.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Reduced-viewport orbit interaction failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "maxiter=151" "savename=control.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Reduced-viewport control rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_JIIM}" --viewport
    image/render000.gif image/render001.gif image/render002.gif
    image/render003.gif image/control.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Reduced-viewport orbit verification failed: ${RESULT}")
endif()
