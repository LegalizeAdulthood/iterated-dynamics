# Copyright 2014 Richard Thomson
cmake_minimum_required(VERSION 3.23)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# <filesystem> is C++17 or later
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set install folder
set(CMAKE_INSTALL_PREFIX install)

set(ID_PROJECT_NAME "Iterated Dynamics" CACHE STRING "Project name for IDEs")

project("${ID_PROJECT_NAME}"
    VERSION 1.0.0.0
    DESCRIPTION "Iterated Dynamics is a fractal generator"
    HOMEPAGE_URL "https://LegalizeAdulthood.github.io/iterated-dynamics"
    LANGUAGES CXX)

include(CTest)

find_package(Boost REQUIRED)

set(home_dir "${CMAKE_SOURCE_DIR}/home")
set(ID_DIR ${CMAKE_CURRENT_SOURCE_DIR})

function(set_src_dir src_file)
    set_source_files_properties(${src_file}
        PROPERTIES COMPILE_DEFINITIONS SRCDIR="${home_dir}")
endfunction()

set(ID_TARGET_DEFINITIONS "")
set(ID_TARGET_DEFINITIONS_DEBUG "")
set(ID_EXECUTABLE_TYPE "")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ID_EXECUTABLE_TYPE "WIN32")
else()
    list(APPEND ID_TARGET_DEFINITIONS XFRACT LINUX)
endif()

set(ID_TARGET_OPTIONS "")
if(NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    list(APPEND ID_TARGET_OPTIONS "-Werror")
endif()

# In debug builds, tell MSVC to:
#   - not warn us about unchecked iterators
#   - not warn us about deprecated CRT functions
#   - not warn us about possibly unsafe CRT functions
#   - store source code location information for memory allocations
if(MSVC)
    list(APPEND ID_TARGET_DEFINITIONS_DEBUG
        "_SCL_SECURE_NO_WARNINGS"
        "_CRT_SECURE_NO_DEPRECATE"
        "_CRT_SECURE_NO_WARNINGS"
        "_CRTDBG_MAP_ALLOC")
endif()

# Enable parallel builds with MSVC
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

add_subdirectory(config)
add_subdirectory(hc)
add_subdirectory(helpcom)
add_subdirectory(home)
add_subdirectory(id)
add_subdirectory(libid)
add_subdirectory(unix)
add_subdirectory(win32)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT id)
