# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif debug/stopmsg.txt)
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS}
    "video=F6" "askvideo=no" "type=diffusion" "params=10/${MODE}/3"
    "viewwindows=1/0.75/no/64/48" "rseed=12345" "showorbit=no"
    "sound=off" "savedir=." "exitnoask=yes" "savename=render000.gif"
    "autokeyname=standard_interrupt.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Diffusion interruption/resume failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_RESUME}" image/render000.gif image/render001.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Diffusion resume verification failed: ${RESULT}")
endif()
