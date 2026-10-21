# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY map image)
file(REMOVE image/pal000.gif image/pal001.gif image/pal002.gif
    image/batch.gif debug/stopmsg.txt map/missing-palette.map)
string(REPEAT "10 20 30\n" 256 INITIAL_MAP)
string(REPEAT "70 80 90\n" 256 VALID_MAP)
string(REPEAT "40 40 40\n" 255 GRAY_TAIL)
file(WRITE map/initial.map "${INITIAL_MAP}")
file(WRITE map/valid.map "${VALID_MAP}")
file(WRITE map/malformed.map "1 2 3\n4 5\n")
file(WRITE map/partial.map "1 2 3\n${GRAY_TAIL}")
set(PARAMETERS "video=F6" "askvideo=no" "type=mandel"
    "viewwindows=1/0.75/no/64/48" "sound=off" "savedir=."
    "exitnoask=yes" "overwrite=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "colors=@initial.map" "savename=pal000.gif"
    "autokeyname=palette_validation.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Palette validation recovery failed: ${RESULT}")
endif()
foreach(checkpoint RANGE 0 2)
    if(checkpoint EQUAL 0)
        set(EXPECTED map/initial.map)
    elseif(checkpoint EQUAL 1)
        set(EXPECTED map/partial.map)
    else()
        set(EXPECTED map/valid.map)
    endif()
    execute_process(COMMAND "${COLORMAP_COMPARE}"
        "${EXPECTED}" "image/pal00${checkpoint}.gif"
        RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "0")
        message(FATAL_ERROR "Palette checkpoint ${checkpoint} failed: ${RESULT}")
    endif()
endforeach()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "batch=yes" "colors=@missing-palette.map" "savename=batch.gif"
    RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "1" OR EXISTS image/batch.gif)
    message(FATAL_ERROR "Missing startup palette did not fail without saving: ${RESULT}")
endif()
file(READ debug/stopmsg.txt DIAGNOSTIC)
string(FIND "${DIAGNOSTIC}" "Could not load color map missing-palette.map" POSITION)
if(POSITION EQUAL -1)
    message(FATAL_ERROR "Missing startup palette diagnostic: ${DIAGNOSTIC}")
endif()
