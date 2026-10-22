# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY reference/image image)
file(REMOVE reference/image/orbit.gif image/orbit.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "sound=off" "overwrite=yes"
    "exitnoask=yes" "librarydirs=." "viewwindows=1/0.75/no/64/48"
    "type=${FRACTAL_TYPE}" "maxiter=64" "rseed=12345")
if(FRACTAL_TYPE STREQUAL "dynamic")
    list(APPEND PARAMETERS "params=2/0.01/1/3")
elseif(FRACTAL_TYPE STREQUAL "mandelcloud")
    list(APPEND PARAMETERS "params=2/0/0/0")
elseif(FRACTAL_TYPE STREQUAL "julibrot")
    list(APPEND PARAMETERS "julibrotfromto=-0.83/-0.83/0.25/-0.25"
        "julibroteyes=2.5" "center-mag=0/0/0.6666667" "params=0/0"
        "inside=0" "colors=@${ALTERN_MAP}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "savedir=reference" "savename=orbit.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Orbit reference rendering failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "savedir=." "savename=orbit.gif"
    "autokeyname=orbit_operation.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Orbit menu-return journey failed: ${RESULT}")
endif()
execute_process(COMMAND "${IMAGE_COMPARE}" reference/image/orbit.gif
    image/orbit.gif RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Orbit menu return changed the image: ${RESULT}")
endif()
