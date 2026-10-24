# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image checkpoints/image)
file(REMOVE image/source.gif checkpoints/image/restore000.gif
    checkpoints/image/restore001.gif checkpoints/image/accepted.gif
    checkpoints/image/fullview.gif)
set(COMMON "video=F6" "sound=off" "overwrite=yes" "exitnoask=yes"
    "librarydirs=." "fastrestore=no")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "type=mandel" "maxiter=150" "askvideo=no" "savedir=."
    "savename=source.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Restore source generation failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "source.gif" "askvideo=no" "savedir=checkpoints"
    "viewwindows=4/0.75/no/200/150"
    "savename=restore000.gif" "autokeyname=restore_cancel.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Restore cancellation failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_RESTORE}" "checkpoints/image/restore000.gif"
    "checkpoints/image/restore001.gif"
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Cancelled restore changed parameters: ${RESULT}")
endif()
execute_process(COMMAND "${IMAGE_COMPARE}" "checkpoints/image/restore000.gif"
    "checkpoints/image/restore001.gif"
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Cancelled restore changed pixels: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "source.gif" "askvideo=yes" "savedir=checkpoints"
    "savename=accepted.gif" "autokeyname=restore_accept.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Subsequent accepted restore failed: ${RESULT}")
endif()
execute_process(COMMAND "${IMAGE_COMPARE}" "image/source.gif"
    "checkpoints/image/accepted.gif"
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Accepted restore changed pixels: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "source.gif" "askvideo=no" "savedir=checkpoints"
    "viewwindows=4/0.75/no/200/150"
    "savename=fullview.gif" "autokeyname=restore_viewport.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Interactive restore from a reduced view failed: ${RESULT}")
endif()
execute_process(COMMAND "${IMAGE_COMPARE}" "image/source.gif"
    "checkpoints/image/fullview.gif"
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Interactive restore retained the reduced view: ${RESULT}")
endif()
