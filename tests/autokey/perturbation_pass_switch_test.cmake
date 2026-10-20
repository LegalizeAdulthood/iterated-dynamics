# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif image/render002.gif
    image/render003.gif image/control-b.gif image/control-g.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=mandel" "bfdigits=30"
    "center-mag=0/1/1e20" "maxiter=300" "periodicity=0" "inside=7"
    "perturbation=${PERTURBATION}" "sound=off" "savedir=." "exitnoask=yes")

foreach(pass IN ITEMS b g)
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
        "passes=${pass}" "savename=control-${pass}.gif" "batch=yes"
        RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "0")
        message(FATAL_ERROR "Control rendering (${pass}) failed: ${RESULT}")
    endif()
endforeach()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "passes=b" "savename=render000.gif"
    "autokeyname=perturbation_pass_switch.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 120 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Pass switching failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_SWITCH}"
    image/render000.gif image/render001.gif image/render002.gif
    image/render003.gif image/control-b.gif image/control-g.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Pass-switch verification failed: ${RESULT}")
endif()
