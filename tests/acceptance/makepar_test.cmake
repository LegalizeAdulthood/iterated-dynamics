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
set(TEST_WORK_DIR "${TEST_ROOT}/work")
set(TEST_SAVE_DIR "${TEST_ROOT}/save")
set(TEST_PAR_FILE "${TEST_SAVE_DIR}/par/from-any-dir.par")

if(DEBUG)
    dump_var(ID)
    dump_var(ID_EXTRA_ARGS)
    dump_var(INPUT_IMAGE)
    dump_var(TEST_ROOT)
    dump_var(TEST_HOME)
    dump_var(TEST_WORK_DIR)
    dump_var(TEST_SAVE_DIR)
    dump_var(TEST_PAR_FILE)
endif()

if(TEST_ROOT STREQUAL "" OR INPUT_IMAGE STREQUAL "")
    message(FATAL_ERROR "TEST_ROOT and INPUT_IMAGE must not be empty.")
endif()

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${TEST_HOME}")
file(MAKE_DIRECTORY "${TEST_WORK_DIR}")
file(MAKE_DIRECTORY "${TEST_SAVE_DIR}")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
        "HOME=${TEST_HOME}"
        "XDG_CONFIG_HOME="
        "XDG_DOCUMENTS_DIR="
        "${ID}" ${ID_EXTRA_ARGS} "batch=yes" "${INPUT_IMAGE}"
            "savedir=${TEST_SAVE_DIR}" "askvideo=no" "sound=off"
            "makepar=from-any-dir/from-any-dir" "exitnoask=yes"
    RESULT_VARIABLE ID_RESULT
    COMMAND_ECHO ${COMMAND_ECHO}
    WORKING_DIRECTORY "${TEST_WORK_DIR}")
if(ID_RESULT)
    set(STOP_MESSAGE_FILES
        "${TEST_SAVE_DIR}/debug/stopmsg.txt"
        "${TEST_WORK_DIR}/debug/stopmsg.txt")
    foreach(STOP_MESSAGE_FILE IN LISTS STOP_MESSAGE_FILES)
        if(EXISTS "${STOP_MESSAGE_FILE}")
            file(READ "${STOP_MESSAGE_FILE}" STOP_MESSAGE)
            message(STATUS "${STOP_MESSAGE_FILE}:\n${STOP_MESSAGE}")
        endif()
    endforeach()
    message(FATAL_ERROR "Id execution failed with result ${ID_RESULT}.")
endif()
if(NOT EXISTS "${TEST_PAR_FILE}")
    message(FATAL_ERROR "Parameter file '${TEST_PAR_FILE}' does not exist.")
endif()

file(READ "${TEST_PAR_FILE}" PAR_TEXT)
if(NOT PAR_TEXT MATCHES "from-any-dir[ ]*\\{")
    message(FATAL_ERROR
        "Parameter file '${TEST_PAR_FILE}' does not contain from-any-dir.")
endif()
