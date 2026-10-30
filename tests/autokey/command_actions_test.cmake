# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image formula)
file(REMOVE image/accepted.gif image/cancelled.gif image/fractmig.gif
    action-help.txt par/action.par debug/stopmsg.txt)
file(READ sstools.ini DEFAULT_CONFIGURATION)
file(WRITE formula/selection.frm "one {\n z = pixel:\n z = sqr(z) + pixel\n |z| <= 4\n}\n")
set(COMMON "video=F6" "askvideo=no" "sound=off" "savedir=."
    "librarydirs=." "exitnoask=yes" "overwrite=yes")
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "batch=yes" "lfile=missing-action-warning.l"
    "makedoc=action-help.txt" "invalid-after-action=yes"
    RESULT_VARIABLE RESULT TIMEOUT 40 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0" OR NOT EXISTS action-help.txt)
    message(FATAL_ERROR "Deferred help-document action failed: ${RESULT}")
endif()
file(SIZE action-help.txt HELP_SIZE)
if(HELP_SIZE LESS 1000)
    message(FATAL_ERROR "Generated help document is incomplete")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "batch=yes" "makemig=2/2"
    RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "1")
    message(FATAL_ERROR "MIG failure did not return through the UI: ${RESULT}")
endif()
file(READ debug/stopmsg.txt DIAGNOSTIC)
string(FIND "${DIAGNOSTIC}" "Can't locate file frmig_00.gif!" POSITION)
if(POSITION EQUAL -1)
    message(FATAL_ERROR "Missing MIG failure diagnostic: ${DIAGNOSTIC}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "type=formula" "formulafile=selection.frm" "maxiter=321"
    "savename=accepted.gif" "autokeyname=command_select_accept.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 35 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Startup formula selection failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "type=formula" "formulafile=selection.frm" "maxiter=321"
    "savename=cancelled.gif" "autokeyname=command_select_cancel.key" "autokey=play"
    RESULT_VARIABLE RESULT TIMEOUT 35 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Startup selection cancellation failed: ${RESULT}")
endif()
execute_process(COMMAND "${CHECK_COMMAND}" image/accepted.gif image/cancelled.gif
    RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0")
    message(FATAL_ERROR "Startup selection metadata check failed: ${RESULT}")
endif()
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "batch=yes" "image/accepted.gif" "makepar=action/selected"
    RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
if(NOT RESULT STREQUAL "0" OR NOT EXISTS par/action.par)
    message(FATAL_ERROR "Deferred parameter-file action failed: ${RESULT}")
endif()
file(READ par/action.par PARAMETERS)
if(NOT PARAMETERS MATCHES "selected +{" OR NOT PARAMETERS MATCHES "type=formula")
    message(FATAL_ERROR "Generated parameter set lost its requested name or image type")
endif()

file(REMOVE debug/stopmsg.txt)
file(WRITE par/blocked.par "existing { type=mandel }\n")
# Block temporary-file creation without relying on file permissions.
file(MAKE_DIRECTORY par/id.tmp)
execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
    "batch=yes" "image/accepted.gif" "makepar=blocked/entry"
    RESULT_VARIABLE RESULT TIMEOUT 20 COMMAND_ECHO STDOUT)
file(REMOVE_RECURSE par/id.tmp)
if(NOT RESULT STREQUAL "1")
    message(FATAL_ERROR "Startup parameter-file write failure did not exit: ${RESULT}")
endif()
file(READ debug/stopmsg.txt DIAGNOSTIC)
if(NOT "${DIAGNOSTIC}" MATCHES "Can't create .*id[.]tmp")
    message(FATAL_ERROR "Missing parameter-file write failure diagnostic: ${DIAGNOSTIC}")
endif()
file(READ par/blocked.par PARAMETERS)
if(NOT PARAMETERS STREQUAL "existing { type=mandel }\n")
    message(FATAL_ERROR "Failed parameter-file action changed the existing file")
endif()

foreach(startup_image IN ITEMS no yes)
    file(REMOVE image/startup-initial.gif image/startup-unexpected.gif
        image/startup-expected.gif debug/stopmsg.txt)
    set(CONFIGURATION "${DEFAULT_CONFIGURATION}\nvideo=F6\naskvideo=no\nsound=off\nsavedir=.\nlibrarydirs=.\nexitnoask=yes\noverwrite=yes\nformulafile=selection.frm\n")
    if(startup_image)
        string(APPEND CONFIGURATION "filename=accepted.gif\n")
    endif()
    file(WRITE sstools.ini "${CONFIGURATION}savename=startup-initial.gif\n")
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
        "type=formula" "formulafile=selection.frm" "maxiter=321"
        "autokeyname=command_initial_cancel.key" "autokey=play"
        RESULT_VARIABLE RESULT TIMEOUT 35 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "0" OR NOT EXISTS image/startup-initial.gif)
        message(FATAL_ERROR "Initial cancellation lost startup requests (image=${startup_image}): ${RESULT}")
    endif()
    if(startup_image)
        set(CHECK_IMAGES image/startup-initial.gif image/cancelled.gif)
    else()
        set(CHECK_IMAGES image/accepted.gif image/startup-initial.gif)
    endif()
    execute_process(COMMAND "${CHECK_COMMAND}" ${CHECK_IMAGES}
        RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "0")
        message(FATAL_ERROR "Initial cancellation restored the wrong image (image=${startup_image})")
    endif()

    file(WRITE sstools.ini "${CONFIGURATION}savename=startup-unexpected.gif\n")
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
        "type=formula" "formulafile=selection.frm" "maxiter=321"
        "autokeyname=command_restart_cancel.key" "autokey=play"
        RESULT_VARIABLE RESULT TIMEOUT 45 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "0" OR EXISTS image/startup-unexpected.gif
        OR NOT EXISTS image/startup-expected.gif)
        message(FATAL_ERROR "Restart cancellation reinstated startup requests (image=${startup_image}): ${RESULT}")
    endif()
    execute_process(COMMAND "${CHECK_COMMAND}" image/accepted.gif image/startup-expected.gif
        RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "0")
        message(FATAL_ERROR "Restart cancellation did not render default Mandelbrot (image=${startup_image})")
    endif()
endforeach()
file(WRITE sstools.ini "${DEFAULT_CONFIGURATION}")

file(WRITE restart.ini "${DEFAULT_CONFIGURATION}\nunknown=yes\n")
foreach(selection IN ITEMS accept cancel)
    file(WRITE sstools.ini "${DEFAULT_CONFIGURATION}")
    file(REMOVE image/restart-ready.gif image/restart-accepted.gif
        image/restart-cancelled.gif debug/stopmsg.txt)
    execute_process(COMMAND "${ID}" ${ID_EXTRA_ARGS} ${COMMON}
        "type=formula" "formulafile=selection.frm" "maxiter=321" "debugflag=1"
        "savename=restart-ready.gif"
        "autokeyname=command_restart_error_${selection}.key" "autokey=play"
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_LIST_DIR}/command_restart_configuration.cmake"
        RESULTS_VARIABLE RESULTS TIMEOUT 40 COMMAND_ECHO STDOUT)
    if(NOT RESULTS STREQUAL "0;0")
        message(FATAL_ERROR "Restart entry ${selection} after a nonfatal error failed: ${RESULTS}")
    endif()
    file(READ debug/stopmsg.txt DIAGNOSTIC)
    if(NOT "${DIAGNOSTIC}" MATCHES "unknown=yes")
        message(FATAL_ERROR "Restart entry ${selection} lost its original diagnostic: ${DIAGNOSTIC}")
    endif()
    if(selection STREQUAL "accept")
        set(CHECK_IMAGES image/restart-accepted.gif image/cancelled.gif)
    else()
        set(CHECK_IMAGES image/accepted.gif image/restart-cancelled.gif)
    endif()
    execute_process(COMMAND "${CHECK_COMMAND}" ${CHECK_IMAGES}
        RESULT_VARIABLE RESULT COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "0")
        message(FATAL_ERROR "Restart entry ${selection} did not produce the expected image")
    endif()
endforeach()
file(WRITE sstools.ini "${DEFAULT_CONFIGURATION}")
