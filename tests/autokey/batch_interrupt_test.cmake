# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/interrupted.gif image/resumed.gif image/control.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=mandel" "passes=1"
    "viewwindows=1/0.75/no/64/48" "maxiter=128" "periodicity=0"
    "sound=off" "savedir=." "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "batch=yes" "savename=interrupted.gif"
    "autokeyname=batch_interrupt.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "2" OR NOT EXISTS image/interrupted.gif)
    message(FATAL_ERROR "Batch interruption did not save unfinished work and exit: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS}
    "video=F6" "askvideo=no" "sound=off" "savedir=." "exitnoask=yes"
    "image/interrupted.gif" "batch=yes" "savename=resumed.gif"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Batch resume did not complete and exit: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "batch=yes" "savename=control.gif"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Batch control rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_RESUME}"
    image/interrupted.gif image/resumed.gif image/control.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Batch interruption/resume verification failed: ${RESULT}")
endif()
