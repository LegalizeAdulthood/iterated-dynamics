function(dump_var name)
    message(STATUS "${name}=${${name}}")
endfunction()

set(DEBUG ON)
if(DEBUG)
    set(COMMAND_ECHO "STDOUT")
else()
    set(COMMAND_ECHO "NONE")
endif()

list(APPEND PARAMETERS "savename=${TEST_SAVE_IMAGE}")

if(DEBUG)
    dump_var(ID)
    dump_var(IMAGE_COMPARE)
    dump_var(GOLD_IMAGE)
    dump_var(PARAMETERS)
    dump_var(TEST_SAVE_IMAGE)
    dump_var(TEST_KEEP_IMAGE)
    message(STATUS "Parameters:")
    foreach(par ${PARAMETERS})
        message(STATUS "    ${par}")
    endforeach()
endif()

file(REMOVE "${TEST_SAVE_IMAGE}")
execute_process(COMMAND "${ID}" ${PARAMETERS}
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
file(RENAME "${TEST_SAVE_IMAGE}" "${TEST_KEEP_IMAGE}/${TEST_SAVE_IMAGE}")

execute_process(COMMAND "${IMAGE_COMPARE}" "${GOLD_IMAGE}" "${TEST_KEEP_IMAGE}/${TEST_SAVE_IMAGE}"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})
