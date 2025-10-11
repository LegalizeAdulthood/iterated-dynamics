# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
function(gui_executable target)
    set_target_properties("${target}" PROPERTIES WIN32_EXECUTABLE $<PLATFORM_ID:Windows>)
endfunction()
