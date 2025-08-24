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
    dump_var(ORBIT_NAME)
    dump_var(TEST_ORBIT)
    dump_var(GOLD_ORBIT)
    dump_var(PARAMETERS)
    dump_var(TEST_KEEP_ORBIT)
    message(STATUS "Parameters:")
    foreach(par ${PARAMETERS})
        message(STATUS "    ${par}")
    endforeach()
endif()

if(EXISTS "${TEST_ORBIT}")
    file(REMOVE "${TEST_ORBIT}")
endif()

set(TEST_OUTPUT "${TEST_KEEP_ORBIT}/test-${ORBIT_NAME}.txt")

execute_process(COMMAND "${ID}" ${PARAMETERS}
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
file(RENAME "${TEST_ORBIT}" "${TEST_OUTPUT}")

execute_process(COMMAND ${CMAKE_COMMAND}
    -E compare_files "${GOLD_ORBIT}" "${TEST_OUTPUT}"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
