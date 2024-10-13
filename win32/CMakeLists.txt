# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2018 Richard Thomson

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

add_library(os-win32 STATIC
    d_disk.cpp
    d_gdi.cpp
    d_win32.h
    d_win32.cpp
    frame.h
    frame.cpp
    os_win32.cpp
    plot.h
    plot.cpp
    win_text.h
    win_text.cpp
    create_minidump.h
    instance.h
    ods.h
    tos.h
    special_dirs.cpp
    win_defines.h
)
target_compile_definitions(os-win32 PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(os-win32 PRIVATE ${ID_TARGET_OPTIONS})
target_link_libraries(os-win32 PUBLIC gdi32.lib user32.lib shlwapi.lib os-hc help-defs)
target_include_directories(os-win32 PRIVATE ../libid/include PUBLIC .)
set_target_properties(os-win32 PROPERTIES FOLDER Libraries)
add_dependencies(os-win32 native-help)
