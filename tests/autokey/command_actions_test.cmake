# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
file(MAKE_DIRECTORY image formula)
file(REMOVE image/accepted.gif image/cancelled.gif image/fractmig.gif
    action-help.txt par/action.par debug/stopmsg.txt)
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
