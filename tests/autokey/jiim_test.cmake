# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif image/render002.gif
    image/render003.gif debug/stopmsg.txt)
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS}
    "video=F6" "askvideo=no" "type=mandel" "sound=off" "savedir=."
    "savename=render000.gif" "exitnoask=no"
    "autokeyname=jiim.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 60 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "JIIM interaction failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_JIIM}" image/render000.gif
    image/render001.gif image/render002.gif image/render003.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "JIIM verification failed: ${RESULT}")
endif()
