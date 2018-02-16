# Copyright 2018 Richard Thomson

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    add_library(os_hc INTERFACE)
    add_library(os STATIC
        d_win32.cpp
        d_win32_disk.cpp
        d_win32_gdi.cpp
        frame.cpp
        os_win32.cpp
        parsera.cpp
        plot.cpp
        WinText.cpp
        fractint.rc
        d_win32.h
        frame.h
        ods.h
        plot.h
        resource.h
        WinText.h)
    target_link_libraries(os PUBLIC gdi32.lib user32.lib shlwapi.lib)
    target_include_directories(os PRIVATE ../headers PUBLIC .)
    add_dependencies(os native_help)
endif()
