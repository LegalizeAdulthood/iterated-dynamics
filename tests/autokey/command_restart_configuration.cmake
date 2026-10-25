# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
foreach(attempt RANGE 0 199)
    if(EXISTS image/restart-ready.gif)
        file(COPY_FILE restart.ini sstools.ini)
        return()
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.05)
endforeach()
message(FATAL_ERROR "Initial image was not saved before the restart")
