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
    dump_var(HC)
    dump_var(GOLD_FILE)
    dump_var(TEXT_FILE)
    dump_var(PARAMETERS)
    message(STATUS "Parameters:")
    foreach(par ${PARAMETERS})
        message(STATUS "    ${par}")
    endforeach()
endif()

execute_process(COMMAND "${HC}" ${PARAMETERS}
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})

file(REAL_PATH "${TEXT_FILE}" text)

if(NOT EXISTS ${GOLD_FILE})
    message(FATAL_ERROR "File ${GOLD_FILE} does not exist")
endif()
if(NOT EXISTS ${text})
    message(FATAL_ERROR "File ${text} does not exist")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol ${GOLD_FILE} ${text}
    RESULT_VARIABLE cmp_result
    COMMAND_ECHO ${COMMAND_ECHO})
if(cmp_result EQUAL 0 AND DEBUG)
    message(STATUS "Files are equal")
elseif(cmp_result EQUAL 1)
    message(STATUS "Expected: <<<<")
    execute_process(COMMAND ${CMAKE_COMMAND} -E cat ${GOLD_FILE})
    message(STATUS "==============")
    execute_process(COMMAND ${CMAKE_COMMAND} -E cat ${text})
    message(STATUS "Got >>>>>>>>>>")
    message(FATAL_ERROR "The files are different.")
else()
    message(FATAL_ERORR "Error running cmake -E compare_files")
endif()
