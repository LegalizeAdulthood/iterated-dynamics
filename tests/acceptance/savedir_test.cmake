# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2014-2026 Richard Thomson
#
function(dump_var name)
    message(STATUS "${name}=${${name}}")
endfunction()

set(DEBUG ON)
if(DEBUG)
    set(COMMAND_ECHO "STDOUT")
else()
    set(COMMAND_ECHO "NONE")
endif()

set(TEST_HOME "${TEST_ROOT}/home")
set(TEST_IMAGE_NAME "savedir-test.gif")
if(TEST_USER_DIRS)
    set(TEST_DOCUMENTS_DIR "${TEST_HOME}/MyDocs")
else()
    set(TEST_DOCUMENTS_DIR "${TEST_HOME}/Documents")
endif()
set(TEST_IMAGE
    "${TEST_DOCUMENTS_DIR}/Iterated Dynamics/image/${TEST_IMAGE_NAME}")

if(DEBUG)
    dump_var(ID)
    dump_var(ID_EXTRA_ARGS)
    dump_var(TEST_ROOT)
    dump_var(TEST_HOME)
    dump_var(TEST_USER_DIRS)
    dump_var(TEST_IMAGE)
endif()

if(TEST_ROOT STREQUAL "" OR TEST_HOME STREQUAL "")
    message(FATAL_ERROR "TEST_ROOT and TEST_HOME must not be empty.")
endif()

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${TEST_HOME}")
if(TEST_USER_DIRS)
    file(MAKE_DIRECTORY "${TEST_HOME}/.config")
    file(WRITE "${TEST_HOME}/.config/user-dirs.dirs"
        "XDG_DOCUMENTS_DIR=\"$HOME/MyDocs\"\n")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
        "HOME=${TEST_HOME}"
        "XDG_CONFIG_HOME="
        "XDG_DOCUMENTS_DIR="
        "${ID}" ${ID_EXTRA_ARGS} "batch=yes" "video=F6" "askvideo=no"
            "sound=off" "type=mandel" "savename=${TEST_IMAGE_NAME}"
            "overwrite=yes"
    RESULT_VARIABLE ID_RESULT
    COMMAND_ECHO ${COMMAND_ECHO})
if(ID_RESULT)
    set(STOP_MESSAGE_FILE "debug/stopmsg.txt")
    if(EXISTS "${STOP_MESSAGE_FILE}")
        file(READ "${STOP_MESSAGE_FILE}" STOP_MESSAGE)
        message(STATUS "${STOP_MESSAGE_FILE}:\n${STOP_MESSAGE}")
    endif()
    message(FATAL_ERROR "Id execution failed with result ${ID_RESULT}.")
endif()
if(NOT EXISTS "${TEST_IMAGE}")
    message(FATAL_ERROR "Image file '${TEST_IMAGE}' does not exist.")
endif()
