# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2018 Richard Thomson

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

find_package(X11)

add_library(os-hc INTERFACE)
target_compile_options(os-hc INTERFACE ${ID_TARGET_OPTIONS})
target_link_libraries(os-hc INTERFACE helpcom config)
target_folder(os-hc "Libraries")

add_library(os STATIC
    d_x11.cpp
    init_failure.cpp
    x11_frame.cpp
    x11_text.cpp
    x11_plot.cpp
    general.cpp
    special_dirs.cpp
    stack_avail.cpp
    unixscr.cpp
    video.cpp)
target_link_libraries(os PUBLIC help-defs os-hc X11::X11 m)
target_compile_definitions(os PUBLIC ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(os PRIVATE ${ID_TARGET_OPTIONS})
target_include_directories(os PRIVATE ../libid/include)
target_folder(os "Libraries")

add_dependencies(os native-help)

add_executable(id main.cpp)
target_link_libraries(id PUBLIC libid)
install(TARGETS id
    DESTINATION .
)
