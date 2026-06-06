# SPDX-License-Identifier: GPL-3.0-only
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

if(DEBUG)
    dump_var(ID)
    dump_var(IMAGE_COMPARE)
    dump_var(GOLD_IMAGE)
    dump_var(DIFF_IMAGE)
    dump_var(ID_EXTRA_ARGS)
endif()

set(STOP_MESSAGE_FILE "debug/stopmsg.txt")
set(TEST_IMAGE "image/rds-random.gif")

file(MAKE_DIRECTORY "image")
file(REMOVE "${TEST_IMAGE}")
file(REMOVE "${DIFF_IMAGE}")

execute_process(
    COMMAND "${ID}" ${ID_EXTRA_ARGS} "video=F6" "askvideo=no" "type=mandel"
        "sound=off" "rseed=363" "savedir=." "savename=rds-random.gif" "overwrite=yes"
        "exitnoask=yes" "autokeyname=rds_random.key" "autokey=play"
    RESULT_VARIABLE ID_RESULT
    COMMAND_ECHO ${COMMAND_ECHO})
if(ID_RESULT)
    if(EXISTS "${STOP_MESSAGE_FILE}")
        file(READ "${STOP_MESSAGE_FILE}" STOP_MESSAGE)
        message(STATUS "${STOP_MESSAGE_FILE}:\n${STOP_MESSAGE}")
    endif()
    message(FATAL_ERROR "RDS execution failed with result ${ID_RESULT}.")
endif()
if(NOT EXISTS "${TEST_IMAGE}")
    message(FATAL_ERROR "RDS image file '${TEST_IMAGE}' does not exist.")
endif()

execute_process(
    COMMAND "${IMAGE_COMPARE}" "--diff-image" "${DIFF_IMAGE}"
        "${GOLD_IMAGE}" "${TEST_IMAGE}"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
