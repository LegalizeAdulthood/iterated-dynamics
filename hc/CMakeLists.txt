# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2018-2024 Richard Thomson
#
find_package(Boost REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_library(libhc
    AsciiDocCompiler.h
    AsciiDocCompiler.cpp
    Compiler.h
    Compiler.cpp
    HelpCompiler.h
    HelpCompiler.cpp
    HelpSource.h
    HelpSource.cpp
    messages.h
    messages.cpp
    modes.h
    Options.h
    Options.cpp
)
target_compile_definitions(libhc PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_include_directories(libhc PUBLIC .)
target_link_libraries(libhc PUBLIC helpcom os-hc fmt::fmt-header-only Boost::boost)
target_folder(libhc "Libraries")

add_executable(hc hc.cpp)
target_link_libraries(hc PUBLIC libhc)
set_property(TARGET hc PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(help-defs INTERFACE)
target_sources(help-defs INTERFACE
    ${CMAKE_BINARY_DIR}/include/helpdefs.h)
target_include_directories(help-defs INTERFACE ${CMAKE_BINARY_DIR}/include)

add_subdirectory(src)
add_subdirectory(tests)
