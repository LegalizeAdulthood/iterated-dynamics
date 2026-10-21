# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/stepped.gif image/control.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=ant"
    "params=1100/2000/2/${ANT_TYPE}/1/12345" "rseed=12345"
    "viewwindows=1/0.75/no/64/48" "sound=off" "savedir=."
    "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "orbitdelay=1" "savename=stepped.gif"
    "autokeyname=ant_step_menu.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Ant stepping/menu rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "orbitdelay=0" "savename=control.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Ant control rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${IMAGE_COMPARE}" image/stepped.gif image/control.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Ant stepping/menu changed the rendered image: ${RESULT}")
endif()
