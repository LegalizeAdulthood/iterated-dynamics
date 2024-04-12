# Copyright 2018 Richard Thomson

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

add_library(os_hc STATIC unix.cpp)
target_compile_definitions(os_hc PUBLIC ${OS_DEFINITIONS})
target_compile_options(os_hc PRIVATE ${ID_TARGET_OPTIONS})
target_include_directories(os_hc PRIVATE ../headers)
target_link_libraries(os_hc PUBLIC helpcom config)

add_library(os STATIC
    d_x11.cpp
    init_failure.cpp
    x11_frame.cpp
    x11_text.cpp
    x11_plot.cpp
    general.cpp
    stack_avail.cpp
    unixscr.cpp
    video.cpp)
target_link_libraries(os PUBLIC help_defs os_hc X11 m)
target_compile_definitions(os PUBLIC ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(os PRIVATE ${ID_TARGET_OPTIONS})
target_include_directories(os PRIVATE ../libid/include)

set_src_dir(unix.cpp)
add_dependencies(os native_help)

add_executable(id main.cpp)
target_link_libraries(id PUBLIC libid)
install(TARGETS id DESTINATION .)
