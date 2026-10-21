# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif image/control.gif
    debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=${FRACTAL_TYPE}"
    "viewwindows=1/0.75/no/64/48" "maxiter=128" "params=16/0.66/3"
    "sound=off" "savedir=." "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "savename=render000.gif" "autokeyname=standard_interrupt.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Bifurcation interruption/resume failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "savename=control.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Bifurcation control rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_RESUME}"
    image/render000.gif image/render001.gif image/control.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Bifurcation resume verification failed: ${RESULT}")
endif()
