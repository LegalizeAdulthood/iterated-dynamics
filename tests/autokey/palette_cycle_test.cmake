# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#

set(BEFORE_MAP "map/palette-before.map")
set(AFTER_MAP "map/palette-after.map")
set(TEST_IMAGE "image/palette-cycle.gif")
set(STOP_MESSAGE_FILE "debug/stopmsg.txt")
file(MAKE_DIRECTORY "map" "image")
file(REMOVE "${BEFORE_MAP}" "${AFTER_MAP}" "${TEST_IMAGE}" "${STOP_MESSAGE_FILE}")

execute_process(
    COMMAND "${ID}" ${ID_EXTRA_ARGS} "video=F6" "askvideo=no" "type=mandel"
        "sound=off" "savedir=." "savename=palette-cycle.gif" "overwrite=yes"
        "cyclerange=1/255" "exitnoask=yes"
        "autokeyname=palette_cycle.key" "autokey=play"
    RESULT_VARIABLE ID_RESULT
    TIMEOUT 60
    COMMAND_ECHO STDOUT)
if(NOT ID_RESULT STREQUAL "0")
    if(EXISTS "${STOP_MESSAGE_FILE}")
        file(READ "${STOP_MESSAGE_FILE}" STOP_MESSAGE)
        message(STATUS "${STOP_MESSAGE_FILE}:\n${STOP_MESSAGE}")
    endif()
    message(FATAL_ERROR "Palette cycling failed with result ${ID_RESULT}.")
endif()

foreach(path IN ITEMS "${BEFORE_MAP}" "${AFTER_MAP}" "${TEST_IMAGE}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "Palette cycling did not produce '${path}'.")
    endif()
endforeach()

file(STRINGS "${BEFORE_MAP}" BEFORE_COLORS)
file(STRINGS "${AFTER_MAP}" AFTER_COLORS)
list(LENGTH BEFORE_COLORS BEFORE_COUNT)
list(LENGTH AFTER_COLORS AFTER_COUNT)
if(NOT BEFORE_COUNT EQUAL 256 OR NOT AFTER_COUNT EQUAL 256)
    message(FATAL_ERROR "Palette checkpoints must each contain 256 colors.")
endif()
if(BEFORE_COLORS STREQUAL AFTER_COLORS)
    message(FATAL_ERROR "The single-step key did not change the palette.")
endif()
foreach(index RANGE 0 255)
    if(index EQUAL 0)
        set(source 0)
    elseif(index EQUAL 255)
        set(source 1)
    else()
        math(EXPR source "${index} + 1")
    endif()
    list(GET BEFORE_COLORS ${source} EXPECTED)
    list(GET AFTER_COLORS ${index} ACTUAL)
    if(NOT ACTUAL STREQUAL EXPECTED)
        message(FATAL_ERROR "Single-step palette color ${index}: '${ACTUAL}', expected '${EXPECTED}'.")
    endif()
endforeach()

file(READ "${TEST_IMAGE}" GIF_HEADER LIMIT 6 HEX)
if(NOT GIF_HEADER MATCHES "^47494638(37|39)61$")
    message(FATAL_ERROR "Returning from palette cycling saved an invalid GIF header: ${GIF_HEADER}.")
endif()
