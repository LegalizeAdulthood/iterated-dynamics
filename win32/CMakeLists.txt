# Copyright 2018 Richard Thomson

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

add_library(os_hc INTERFACE)
target_link_libraries(os_hc INTERFACE helpcom config)

add_library(os STATIC
    d_disk.cpp
    d_gdi.cpp
    d_win32.h
    d_win32.cpp
    frame.h
    frame.cpp
    os_win32.cpp
    parsera.cpp
    plot.h
    plot.cpp
    win_text.h
    win_text.cpp
    create_minidump.h
    instance.h
    ods.h
    tos.h
)
target_compile_definitions(os PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(os PRIVATE ${ID_TARGET_OPTIONS})
target_link_libraries(os PUBLIC gdi32.lib user32.lib shlwapi.lib os_hc help_defs)
target_include_directories(os PRIVATE ../libid/include PUBLIC .)
add_dependencies(os native_help)

add_executable(id WIN32
    Id.ico
    id.rc
    resource.h
    win_main.cpp)
target_link_libraries(id PUBLIC libid)
install(TARGETS id DESTINATION .)
