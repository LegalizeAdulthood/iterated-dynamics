# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/lsys000.gif image/lsys001.gif image/lsys002.gif
    image/batch.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=lsystem"
    "lfile=${LSYSTEM_FILE}" "viewwindows=1/0.75/no/64/48"
    "params=2" "sound=off" "savedir=." "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "lname=missing" "savename=lsys000.gif"
    "autokeyname=lsystem_validation.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 70 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "L-system validation recovery failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_LSYSTEM}"
    image/lsys000.gif image/lsys001.gif image/lsys002.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "L-system validation checkpoints failed: ${RESULT}")
endif()
foreach(entry IN ITEMS missing bad)
    file(REMOVE image/batch.gif debug/stopmsg.txt)
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
        "lname=${entry}" "savename=batch.gif" "batch=yes"
        RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "2")
        message(FATAL_ERROR
            "Invalid L-system '${entry}' returned ${RESULT}, expected 2")
    endif()
    if(EXISTS image/batch.gif)
        message(FATAL_ERROR "Invalid L-system '${entry}' saved an image")
    endif()
    file(STRINGS debug/stopmsg.txt DIAGNOSTICS)
    list(LENGTH DIAGNOSTICS COUNT)
    if(NOT COUNT EQUAL 1)
        message(FATAL_ERROR
            "Expected one diagnostic for '${entry}': ${DIAGNOSTICS}")
    endif()
    if(entry STREQUAL "missing")
        set(EXPECTED "'missing' file entry item not found")
    else()
        set(EXPECTED "Error:  illegal or missing angle")
    endif()
    if(NOT DIAGNOSTICS STREQUAL EXPECTED)
        message(FATAL_ERROR
            "Unexpected '${entry}' diagnostic: ${DIAGNOSTICS}")
    endif()
endforeach()
