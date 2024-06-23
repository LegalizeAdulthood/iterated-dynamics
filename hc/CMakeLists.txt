# Copyright 2018 Richard Thomson

add_library(libhc
    adoc_compiler.h
    adoc_compiler.cpp
    compiler.h
    compiler.cpp
    help_compiler.h
    help_compiler.cpp
    help_source.h
    help_source.cpp
    html_processor.h
    html_processor.cpp
    messages.h
    messages.cpp
    modes.h
    options.h
    options.cpp
)
target_compile_definitions(libhc PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_include_directories(libhc PUBLIC .)
target_link_libraries(libhc PUBLIC helpcom os-hc)

add_executable(hc hc.cpp)
target_link_libraries(hc PUBLIC libhc)

add_library(help-defs INTERFACE)
target_sources(help-defs INTERFACE
    ${CMAKE_BINARY_DIR}/include/helpdefs.h)
target_include_directories(help-defs INTERFACE ${CMAKE_BINARY_DIR}/include)

add_subdirectory(src)
add_subdirectory(tests)
