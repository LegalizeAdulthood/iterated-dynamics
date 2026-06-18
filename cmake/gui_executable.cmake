# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025-2026 Richard Thomson
#
function(gui_executable target)
    cmake_parse_arguments(ARG "" "RUNTIME_NAME" "" ${ARGN})
    set_target_properties("${target}" PROPERTIES WIN32_EXECUTABLE $<PLATFORM_ID:Windows>)
    if(DEFINED ARG_RUNTIME_NAME)
        set_target_properties("${target}" PROPERTIES RUNTIME_OUTPUT_NAME "${ARG_RUNTIME_NAME}")
    endif()
endfunction()
