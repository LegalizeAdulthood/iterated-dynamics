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

set(RAYTRACE_OUTPUT "raytrace/fract001.ray")
if(EXISTS "${RAYTRACE_OUTPUT}")
    file(REMOVE "${RAYTRACE_OUTPUT}")
endif()
if(EXISTS "image/fract001.gif")
    file(REMOVE "image/fract001.gif")
endif()

set(TEST_OUTPUT "${TEST_KEEP_RAYTRACE}/${RAYTRACE_NAME}.txt")

execute_process(COMMAND "${ID}" ${PARAMETERS}
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
file(RENAME "${RAYTRACE_OUTPUT}" "${TEST_OUTPUT}")
if(EXISTS "image/fract001.gif")
    file(REMOVE "image/fract001.gif")
endif()

execute_process(COMMAND ${CMAKE_COMMAND}
    -E compare_files "${GOLD_TRACE}" "${TEST_OUTPUT}"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
