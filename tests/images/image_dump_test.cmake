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
    dump_var(IMAGE_DUMP)
    dump_var(INPUT_IMAGE)
    dump_var(GOLD_DUMP)
    dump_var(TEST_DUMP)
endif()

execute_process(COMMAND "${IMAGE_DUMP}" --no-scanlines "${INPUT_IMAGE}"
    OUTPUT_FILE "${TEST_DUMP}"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO ${COMMAND_ECHO})

compare_gold_text_files("${GOLD_DUMP}" "${TEST_DUMP}"
    "Generated image dump differs from '${GOLD_DUMP}'.")
