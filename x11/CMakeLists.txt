# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson

if(WIN32 OR ID_USE_WX)
    return()
endif()

find_package(X11 QUIET)
if(NOT X11_FOUND)
    return()
endif()

add_library(os STATIC
    X11Driver.cpp
    X11SpecialDirectories.cpp
)
target_compile_definitions(os PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(os PRIVATE ${ID_TARGET_OPTIONS})
target_link_libraries(os PUBLIC X11::X11 os-hc help-defs)
target_include_directories(os PRIVATE ../libid/include PUBLIC .)
target_folder(os "Libraries")
add_dependencies(os native-help)

add_executable(id
    x11_main.cpp)
target_include_directories(id PUBLIC .)
target_link_libraries(id PUBLIC libid)
gui_executable(id)
vs_debugger_directory(id "${CMAKE_SOURCE_DIR}/home")
install(TARGETS id
    DESTINATION .
    RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)
