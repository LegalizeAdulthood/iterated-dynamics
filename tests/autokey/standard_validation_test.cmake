# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/valid000.gif image/valid001.gif
    image/batch.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=mandel" "passes=b"
    "viewwindows=1/0.75/no/32/24" "inside=0" "sound=off"
    "savedir=." "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "savename=valid000.gif" "autokeyname=standard_validation.key"
    "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Standard validation recovery failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_VALIDATION}"
    image/valid000.gif image/valid001.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Standard validation checkpoints failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "savename=batch.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "2")
    message(FATAL_ERROR "Invalid batch render returned ${RESULT}, expected 2")
endif()
if(EXISTS image/batch.gif)
    message(FATAL_ERROR "Invalid batch calculation saved an image")
endif()
file(READ debug/stopmsg.txt DIAGNOSTIC)
if(NOT "${DIAGNOSTIC}" MATCHES "Boundary tracing cannot be used with inside=0 or outside=0")
    message(FATAL_ERROR "Missing batch validation diagnostic: ${DIAGNOSTIC}")
endif()
