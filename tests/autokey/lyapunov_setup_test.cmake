# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/lyap000.gif image/lyap001.gif image/control.gif
    debug/stopmsg.txt)
set(PARAMETERS "reset=1730" "video=F6" "askvideo=no" "type=lyapunov"
    "viewwindows=1/0.75/no/64/48" "maxiter=25" "params=65/0.05"
    "corners=2.719/3.178/3.26/3.604" "sound=off"
    "savedir=." "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "inside=bof60" "savename=lyap000.gif"
    "autokeyname=lyapunov_setup.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Lyapunov setup recovery failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "inside=2" "savename=control.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Lyapunov control render failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_SETUP}"
    image/lyap000.gif image/lyap001.gif image/control.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Lyapunov setup checkpoints failed: ${RESULT}")
endif()
