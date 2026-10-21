# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image par)
file(REMOVE image/retry.gif image/control.gif image/batch.gif debug/stopmsg.txt)
set(PARAMETERS "video=F6" "askvideo=no" "type=mandel"
    "viewwindows=1/0.75/no/64/48" "sound=off" "savedir=."
    "librarydirs=." "exitnoask=yes" "overwrite=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "maxiter=150" "savename=retry.gif"
    "autokeyname=command_diagnostics.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Interactive command retry failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_COMMAND}" "image/retry.gif"
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Command retry did not apply its settings: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "maxiter=321" "savename=control.gif" "batch=yes"
    RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Command retry control render failed: ${RESULT}")
endif()
execute_process(COMMAND "${IMAGE_COMPARE}" "image/retry.gif" "image/control.gif"
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Command retry rendered different pixels: ${RESULT}")
endif()
file(WRITE par/valid.par "valid { maxiter=150 }\n")
file(WRITE par/broken.par "broken { maxiter=321\\")
foreach(case RANGE 0 4)
    file(REMOVE image/batch.gif debug/stopmsg.txt)
    if(case EQUAL 0)
        set(ARGUMENTS "batch=yes" "maxiter=bad")
        set(EXPECTED "maxiter=bad")
    elseif(case EQUAL 1)
        set(ARGUMENTS "maxiter=bad" "batch=yes")
        set(EXPECTED "maxiter=bad")
    elseif(case EQUAL 2)
        set(ARGUMENTS "batch=yes" "@valid/missing")
        set(EXPECTED "'missing' file entry item not found")
    elseif(case EQUAL 3)
        set(ARGUMENTS "batch=yes" "@broken")
        set(EXPECTED "maxiter=321")
    else()
        set(ARGUMENTS "maxiter=bad" "batch=yes" "makedoc=unwanted.txt")
        set(EXPECTED "maxiter=bad")
    endif()
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
        "savename=batch.gif" ${ARGUMENTS}
        RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "1" OR EXISTS image/batch.gif)
        message(FATAL_ERROR "Invalid startup case ${case} did not fail before saving: ${RESULT}")
    endif()
    file(READ debug/stopmsg.txt DIAGNOSTIC)
    string(FIND "${DIAGNOSTIC}" "${EXPECTED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "Missing startup diagnostic: ${DIAGNOSTIC}")
    endif()
endforeach()
file(REMOVE image/batch.gif debug/stopmsg.txt)
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "batch=yes" "lfile=missing-warning.l" "savename=batch.gif"
    RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0" OR NOT EXISTS image/batch.gif)
    message(FATAL_ERROR "Nonfatal startup warning changed batch completion: ${RESULT}")
endif()
file(READ debug/stopmsg.txt DIAGNOSTIC)
string(FIND "${DIAGNOSTIC}" "Can't find lfile=missing-warning.l" POSITION)
if(POSITION EQUAL -1)
    message(FATAL_ERROR "Missing nonfatal batch warning: ${DIAGNOSTIC}")
endif()

file(REMOVE image/startup.gif)
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
    "maxiter=321" "lfile=missing-warning.l" "savename=startup.gif"
    "autokeyname=command_startup_continue.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Startup warning acknowledgement failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_COMMAND}" "image/startup.gif"
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Startup did not continue after warning: ${RESULT}")
endif()
foreach(error IN ITEMS no yes)
    file(REMOVE image/aborted.gif)
    if(error)
        set(ARGUMENT "maxiter=bad")
        set(EXPECTED_RESULT 1)
    else()
        set(ARGUMENT "lfile=missing-warning.l")
        set(EXPECTED_RESULT 0)
    endif()
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${PARAMETERS}
        "${ARGUMENT}" "savename=aborted.gif"
        "autokeyname=command_startup_abort.key" "autokey=play"
        RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "${EXPECTED_RESULT}" OR EXISTS image/aborted.gif)
        message(FATAL_ERROR "Startup abort returned incorrect status or saved an image: ${RESULT}")
    endif()
endforeach()
