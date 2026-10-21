# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
set(PARAMETERS "video=F6" "askvideo=no" "type=mandel"
    "viewwindows=1/0.75/no/32/24" "sound=off" "savedir=."
    "savename=failed.gif" "exitnoask=yes" "batch=yes" "bfdigits=2000")

foreach(PHASE IN ITEMS startup calculation)
    file(REMOVE image/failed.gif debug/stopmsg.txt)
    if(PHASE STREQUAL "startup")
        set(PRECISION_ARGUMENT "center-mag=-0.5/0/1e30")
    else()
        set(PRECISION_ARGUMENT "debugflag=3200")
    endif()
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
        "${PRECISION_ARGUMENT}"
        RESULT_VARIABLE RESULT TIMEOUT 25 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "1")
        message(FATAL_ERROR
            "${PHASE} precision failure returned ${RESULT}, expected 1")
    endif()
    if(EXISTS image/failed.gif)
        message(FATAL_ERROR "${PHASE} precision failure saved an image")
    endif()
    file(READ debug/stopmsg.txt DIAGNOSTIC)
    if(NOT DIAGNOSTIC MATCHES "Requested precision of 2000 too high")
        message(FATAL_ERROR "Missing ${PHASE} precision diagnostic: ${DIAGNOSTIC}")
    endif()
endforeach()
