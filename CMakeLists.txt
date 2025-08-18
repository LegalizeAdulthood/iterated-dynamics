# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2014-2025 Richard Thomson
#
cmake_minimum_required(VERSION 3.23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(misc_target)
include(policy_new)
include(set_cxx_standard)
include(target_folder)
include(vs_startup_project)

# v3.30: The FindBoost module is removed.
policy_new(CMP0167)
# v3.31: install() DESTINATION paths are normalized.
policy_new(CMP0177)

# <filesystem> is C++17 or later
set_cxx_standard(17)

# Set install folder
set(CMAKE_INSTALL_PREFIX install)

set(ID_PROGRAM_NAME "Iterated Dynamics")
set(ID_VENDOR_NAME  "Legalize Adulthood!")

set(ID_PROJECT_NAME "${ID_PROGRAM_NAME}" CACHE STRING "Project name for IDEs")
option(ID_USE_WX "Enable wxWidgets for the GUI" OFF)

project("${ID_PROJECT_NAME}"
    VERSION 1.3.2.0
    DESCRIPTION "${ID_PROGRAM_NAME} is a fractal generator"
    HOMEPAGE_URL "https://LegalizeAdulthood.github.io/iterated-dynamics"
    LANGUAGES CXX C)

include(CTest)

set(home_dir "${CMAKE_SOURCE_DIR}/home")
set(ID_DIR ${CMAKE_CURRENT_SOURCE_DIR})
execute_process(COMMAND "${GITCOMMAND}" log -1 --format=%h
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE ID_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

function(source_group_subdir group subdir)
    source_group("Header Files/${group}" REGULAR_EXPRESSION "include/${subdir}/.*")
    source_group("Source Files/${group}" REGULAR_EXPRESSION "${subdir}/.*")
endfunction()

set(ID_TARGET_DEFINITIONS "")
set(ID_TARGET_DEFINITIONS_DEBUG "")

set(ID_EXECUTABLE_TYPE "")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ID_EXECUTABLE_TYPE "WIN32")
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
add_subdirectory(doc)
add_subdirectory(hc)
add_subdirectory(helpcom)
add_subdirectory(home)
add_subdirectory(libid)
add_subdirectory(win32)
if(ID_USE_WX)
    add_subdirectory(wx)
else()
    add_subdirectory(unix)
endif()
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

vs_startup_project(id)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_subdirectory(packaging)
endif()
install(FILES LICENSE.txt DESTINATION .)

misc_target(FILES
    "CMakePresets.json")
misc_group_sources("Documentation" FILES
    "ReadMe.md"
    "Style.md")
misc_group_sources("CMake Scripts" FILES
    "cmake/misc_target.cmake"
    "cmake/policy_new.cmake"
    "cmake/set_cxx_standard.cmake"
    "cmake/target_folder.cmake"
    "cmake/vs_startup_project.cmake")
misc_group_sources("Editing" FILES
    ".clang-format"
    ".editorconfig")
misc_group_sources("Git" FILES
    ".gitattributes"
    ".gitignore"
    ".gitmodules"
    ".mailmap")
misc_group_sources("GitHub" FILES
    "LICENSE.txt"
    ".github/workflows/cmake.yml")
misc_group_sources("VcPkg" FILES
    "vcpkg.json")
