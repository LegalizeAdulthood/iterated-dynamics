# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/ifs000.gif image/ifs001.gif image/ifs002.gif
    image/batch.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=ifs"
    "ifsfile=${IFS_FILE}" "viewwindows=1/0.75/no/64/48"
    "maxiter=2" "sound=off" "savedir=." "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "ifs=missing" "savename=ifs000.gif"
    "autokeyname=ifs_validation.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 70 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "IFS validation recovery failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_IFS}"
    image/ifs000.gif image/ifs001.gif image/ifs002.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "IFS validation checkpoints failed: ${RESULT}")
endif()
foreach(entry IN ITEMS missing bad empty)
    file(REMOVE image/batch.gif debug/stopmsg.txt)
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
        "ifs=${entry}" "savename=batch.gif" "batch=yes"
        RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "2")
        message(FATAL_ERROR "Invalid IFS '${entry}' returned ${RESULT}, expected 2")
    endif()
    if(EXISTS image/batch.gif)
        message(FATAL_ERROR "Invalid IFS '${entry}' saved an image")
    endif()
    file(STRINGS debug/stopmsg.txt DIAGNOSTICS)
    list(LENGTH DIAGNOSTICS COUNT)
    if(NOT COUNT EQUAL 1)
        message(FATAL_ERROR "Expected one diagnostic for '${entry}': ${DIAGNOSTICS}")
    endif()
    if(entry STREQUAL "missing")
        set(EXPECTED "'missing' file entry item not found")
    elseif(entry STREQUAL "bad")
        set(EXPECTED "invalid IFS definition")
    else()
        set(EXPECTED "Empty IFS definition")
    endif()
    if(NOT DIAGNOSTICS STREQUAL EXPECTED)
        message(FATAL_ERROR "Unexpected '${entry}' diagnostic: ${DIAGNOSTICS}")
    endif()
endforeach()
