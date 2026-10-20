# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif image/control.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=mandel" "passes=${PASSES}"
    "maxiter=5000" "periodicity=0" "sound=off" "savedir=." "exitnoask=yes")
if(GENERAL_PIXEL)
    list(APPEND PARAMETERS "inside=zmag")
endif()

execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "savename=render000.gif" "autokeyname=standard_interrupt.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 120 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Interrupted rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "savename=control.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 120 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Control rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_RESUME}"
    image/render000.gif image/render001.gif image/control.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Interrupted/resumed image verification failed: ${RESULT}")
endif()
