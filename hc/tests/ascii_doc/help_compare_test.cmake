# SPDX-License-Identifier: GPL-3.0-only
#
function(dump_var name)
    message(STATUS "${name}=${${name}}")
endfunction()

include(CompareGoldText)

set(DEBUG ON)
if(DEBUG)
    set(COMMAND_ECHO "STDOUT")
else()
    set(COMMAND_ECHO "NONE")
endif()

if(DEBUG)
    dump_var(HC)
    dump_var(GOLD_FILE)
    dump_var(ADOC_FILE)
    dump_var(PARAMETERS)
    message(STATUS "Parameters:")
    foreach(par ${PARAMETERS})
        message(STATUS "    ${par}")
    endforeach()
endif()

execute_process(COMMAND "${HC}" ${PARAMETERS}
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})

file(REAL_PATH "${ADOC_FILE}" adoc)
compare_gold_text_files("${GOLD_FILE}" "${adoc}" "The files are different."
    IGNORE_EOL)
