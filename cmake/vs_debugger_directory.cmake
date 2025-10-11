# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
function(vs_debugger_directory target dir)
    set_property(TARGET "${target}" PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${dir}")
endfunction()
