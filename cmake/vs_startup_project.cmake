# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
function(vs_startup_project target)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${target})
endfunction()
