# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image checkpoints/image)
file(REMOVE image/source.gif checkpoints/image/stereo000.gif
    checkpoints/image/stereo001.gif debug/stopmsg.txt)
set(COMMON "video=F6" "askvideo=no" "sound=off" "overwrite=yes"
    "exitnoask=yes" "librarydirs=.")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "type=mandel" "maxiter=64" "viewwindows=1/0.75/no/64/48"
    "savedir=." "savename=source.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Stereo restoration source failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "source.gif" "3d=yes" "stereo=3" "filltype=1" "interocular=20"
    "savedir=checkpoints" "savename=stereo000.gif"
    "autokeyname=stereo_restore.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Stereo restoration journey failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_STEREO}" restore complete
    checkpoints/image/stereo000.gif checkpoints/image/stereo001.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Stereo restoration checkpoints failed: ${RESULT}")
endif()
