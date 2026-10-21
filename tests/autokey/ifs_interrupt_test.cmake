# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/render000.gif image/render001.gif image/control.gif
    debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "ifsfile=${IFS_FILE}"
    "viewwindows=1/0.75/no/64/48" "rseed=12345"
    "sound=off" "savedir=." "exitnoask=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "type=mandel" "maxiter=64" "savename=render000.gif"
    "autokeyname=ifs_interrupt.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "IFS interruption/restart failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "type=${IFS_TYPE}" "ifs=${IFS_NAME}" "maxiter=65"
    "savename=control.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "IFS control rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_IFS}" "${IFS_TYPE}" "${IFS_NAME}"
    image/render000.gif image/render001.gif image/control.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "IFS interaction verification failed: ${RESULT}")
endif()
