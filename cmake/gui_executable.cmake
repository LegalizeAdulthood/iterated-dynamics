# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025-2026 Richard Thomson
#
function(gui_executable target)
    set_target_properties("${target}" PROPERTIES WIN32_EXECUTABLE $<PLATFORM_ID:Windows>)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND target STREQUAL "id")
        set_target_properties("${target}" PROPERTIES RUNTIME_OUTPUT_NAME xid)
    endif()
endfunction()
