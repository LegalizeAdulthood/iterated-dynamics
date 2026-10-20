# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif image/render002.gif
    image/control150.gif image/control300.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=mandel" "passes=1"
    "viewwindows=1/0.75/no/32/24" "periodicity=0" "sound=off"
    "savedir=." "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "maxiter=150" "showdot=b/20" "orbitdelay=20"
    "savename=render000.gif" "autokeyname=show_dot.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 90 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Show-dot interruption/restart failed: ${RESULT}")
endif()
foreach(iterations IN ITEMS 150 300)
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
        "maxiter=${iterations}" "savename=control${iterations}.gif" "batch=yes"
        RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "0")
        message(FATAL_ERROR "Show-dot control rendering failed: ${RESULT}")
    endif()
endforeach()
execute_process(COMMAND "${CHECK_SHOW_DOT}"
    image/render000.gif image/render001.gif image/render002.gif
    image/control150.gif image/control300.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Show-dot checkpoint verification failed: ${RESULT}")
endif()

