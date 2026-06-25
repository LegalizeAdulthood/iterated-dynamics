# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson

if(WIN32 OR ID_USE_WX)
    return()
endif()

find_package(X11 REQUIRED)

add_library(os STATIC
    init_failure.cpp
    include/X11Driver/X11BaseDriver.h X11BaseDriver.cpp
    include/X11Driver/X11Connection.h X11Connection.cpp
    X11DiskDriver.cpp
    X11Driver.cpp
    include/X11Driver/X11Frame.h X11Frame.cpp
    include/X11Driver/X11Plot.h X11Plot.cpp
    include/X11Driver/X11Text.h X11Text.cpp
)

target_compile_definitions(os PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(os PRIVATE ${ID_TARGET_OPTIONS})
target_link_libraries(os PUBLIC X11::X11 os-hc help-defs)
target_include_directories(os PRIVATE ../libid/include PUBLIC include)
target_folder(os "Libraries")
add_dependencies(os native-help)

add_executable(id
    x11_main.cpp)
target_include_directories(id PUBLIC .)
target_link_libraries(id PUBLIC libid)
gui_executable(id RUNTIME_NAME xid)
vs_debugger_directory(id "${CMAKE_SOURCE_DIR}/home")
install(TARGETS id DESTINATION .)
