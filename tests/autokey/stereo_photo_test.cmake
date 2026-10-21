# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image)
file(REMOVE image/stereo000.gif image/stereo001.gif debug/stopmsg.txt)
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS}
    "video=F6" "askvideo=no" "type=mandel" "maxiter=64"
    "ifsfile=${IFS_FILE}" "viewwindows=1/0.75/no/64/48"
    "rseed=12345" "sound=off" "savedir=." "savename=stereo000.gif"
    "exitnoask=yes" "autokeyname=stereo_photo.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Stereo photographer journey failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_STEREO}" "${PHOTO_TYPE}" "${PHOTO_RESULT}"
    image/stereo000.gif image/stereo001.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Stereo photographer checkpoints failed: ${RESULT}")
endif()
