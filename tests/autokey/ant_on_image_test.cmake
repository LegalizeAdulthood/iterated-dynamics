# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image par)
file(REMOVE image/on-image000.gif image/on-image001.gif par/ant.par
    debug/stopmsg.txt)
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS}
    "video=F6" "askvideo=no" "type=mandel" "maxiter=32"
    "viewwindows=1/0.75/no/64/48" "sound=off" "savedir=."
    "exitnoask=yes" "savename=on-image000.gif"
    "autokeyname=ant_on_image.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Ant on existing image failed: ${RESULT}")
endif()
execute_process(COMMAND "${IMAGE_COMPARE}"
    image/on-image000.gif image/on-image001.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Zero-batch Ant changed the existing image: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS}
    "batch=yes" "savedir=." "image/on-image001.gif" "makepar=ant/after"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0" OR NOT EXISTS par/ant.par)
    message(FATAL_ERROR "Ant image parameter extraction failed: ${RESULT}")
endif()
file(READ par/ant.par PARAMETERS)
if(NOT PARAMETERS MATCHES "type=mandel" OR PARAMETERS MATCHES "type=ant")
    message(FATAL_ERROR "Ant did not restore the previous fractal type")
endif()
