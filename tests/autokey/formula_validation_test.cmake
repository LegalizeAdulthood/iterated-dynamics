# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/form000.gif image/form001.gif image/form002.gif
    image/batch.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=formula"
    "formulafile=${FORMULA_FILE}" "viewwindows=1/0.75/no/64/48"
    "maxiter=150" "sound=off" "savedir=." "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "formulaname=missing" "savename=form000.gif"
    "autokeyname=formula_validation.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 70 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Formula validation recovery failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_FORMULA}"
    image/form000.gif image/form001.gif image/form002.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Formula validation checkpoints failed: ${RESULT}")
endif()
foreach(entry IN ITEMS missing bad empty)
    file(REMOVE image/batch.gif debug/stopmsg.txt)
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
        "formulaname=${entry}" "savename=batch.gif" "batch=yes"
        RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "2")
        message(FATAL_ERROR
            "Invalid formula '${entry}' returned ${RESULT}, expected 2")
    endif()
    if(EXISTS image/batch.gif)
        message(FATAL_ERROR "Invalid formula '${entry}' saved an image")
    endif()
    file(READ debug/stopmsg.txt DIAGNOSTICS)
    if(entry STREQUAL "missing")
        set(EXPECTED "'missing' file entry item not found")
    elseif(entry STREQUAL "bad")
        set(EXPECTED "at line 2:  Should be an Argument")
    else()
        set(EXPECTED "Formula has no executable instructions")
    endif()
    string(FIND "${DIAGNOSTICS}" "${EXPECTED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR
            "Missing '${entry}' diagnostic: ${DIAGNOSTICS}")
    endif()
endforeach()
