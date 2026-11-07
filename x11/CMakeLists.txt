# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson

if(WIN32 OR ID_USE_WX)
    return()
endif()

find_package(X11 REQUIRED)

add_library(driver-x11 STATIC
    include/X11Driver/X11BaseDriver.h X11BaseDriver.cpp
    include/X11Driver/X11Connection.h X11Connection.cpp
    X11DiskDriver.cpp
    X11Driver.cpp
    include/X11Driver/X11Frame.h X11Frame.cpp
    include/X11Driver/X11Plot.h X11Plot.cpp
    include/X11Driver/X11Text.h X11Text.cpp
)

target_link_libraries(driver-x11 PUBLIC id::ui X11::X11)
target_include_directories(driver-x11 PUBLIC include)
target_folder(driver-x11 "Libraries")
add_dependencies(driver-x11 native-help)

add_executable(id
    init_failure.cpp
    x11_main.cpp)
target_include_directories(id PUBLIC .)
target_link_libraries(id PUBLIC driver-x11)
gui_executable(id RUNTIME_NAME xid)
vs_debugger_directory(id "${CMAKE_SOURCE_DIR}/home")
install(TARGETS id DESTINATION .)
