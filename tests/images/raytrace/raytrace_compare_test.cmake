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
    dump_var(RAYTRACE_NAME)
    dump_var(GOLD_TRACE)
    dump_var(PARAMETERS)
    dump_var(TEST_KEEP_RAYTRACE)
    message(STATUS "Parameters:")
    foreach(par ${PARAMETERS})
        message(STATUS "    ${par}")
    endforeach()
endif()

if(EXISTS "fract001.ray")
    file(REMOVE "fract001.ray")
endif()
if(EXISTS "fract001.gif")
    file(REMOVE "fract001.gif")
endif()

set(TEST_OUTPUT "${TEST_KEEP_RAYTRACE}/${RAYTRACE_NAME}.txt")

execute_process(COMMAND "${ID}" ${PARAMETERS}
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
file(RENAME "fract001.ray" "${TEST_OUTPUT}")

execute_process(COMMAND ${CMAKE_COMMAND}
    -E compare_files "${GOLD_TRACE}" "${TEST_OUTPUT}"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
