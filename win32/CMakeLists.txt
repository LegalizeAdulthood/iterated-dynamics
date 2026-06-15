# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2018-2026 Richard Thomson

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

string(TIMESTAMP CURRENT_YEAR "%Y")
configure_file(id.rc.in id.rc @ONLY)

find_package(fmt CONFIG REQUIRED)

set(ID_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "Id resource source directory")
set(ID_RESOURCE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "Id resource binary directory")
set(ID_RESOURCE_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/id.rc" CACHE INTERNAL "Id resource script")

function(target_id_resources target)
    target_sources("${target}" PRIVATE
        "${ID_RESOURCE_DIR}/Id.ico"
        "${ID_RESOURCE_DIR}/id.rc.in"
        "${ID_RESOURCE_SCRIPT}"
        "${ID_RESOURCE_DIR}/app-manifest.xml"
        "${ID_RESOURCE_DIR}/resource.h")
    target_link_options("${target}" PRIVATE
        "/manifest:embed"
        "/manifestinput:${ID_RESOURCE_DIR}/app-manifest.xml")
    target_include_directories("${target}" PRIVATE
        "${ID_RESOURCE_DIR}"
        "${ID_RESOURCE_BINARY_DIR}")
endfunction()

if(ID_USE_WX)
    # TODO: eliminate the need for os-wx
    add_library(os-wx STATIC
        create_minidump.h create_minidump.cpp
        instance.h instance.cpp
        win_defines.h
    )

    target_compile_definitions(os-wx PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
    target_compile_options(os-wx PRIVATE ${ID_TARGET_OPTIONS})
    target_link_libraries(os-wx PUBLIC fmt::fmt-header-only gdi32.lib user32.lib shlwapi.lib os-hc help-defs)
    target_include_directories(os-wx PRIVATE ../libid/include PUBLIC .)
    target_folder(os-wx "Libraries")
    add_dependencies(os-wx native-help)
else()
    add_library(os STATIC
        create_minidump.h create_minidump.cpp
        DiskDriver.cpp
        GDIDriver.cpp
        Frame.h Frame.cpp
        init_failure.cpp
        instance.h instance.cpp
        ods.h ods.cpp
        Plot.h Plot.cpp
        Win32BaseDriver.h Win32BaseDriver.cpp
        WinText.h WinText.cpp
        instance.h
        special_dirs.cpp
        win_defines.h
    )

    target_compile_definitions(os PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
    target_compile_options(os PRIVATE ${ID_TARGET_OPTIONS})
    target_link_libraries(os PUBLIC fmt::fmt-header-only gdi32.lib user32.lib shlwapi.lib os-hc help-defs)
    target_include_directories(os PRIVATE ../libid/include PUBLIC .)
    target_folder(os "Libraries")
    add_dependencies(os native-help)

    add_executable(id
        win_main.cpp)
    source_group("CMake Templates" REGULAR_EXPRESSION "^.*\.in$")
    target_include_directories(id PUBLIC .)
    target_link_libraries(id PUBLIC libid)
    target_id_resources(id)
    gui_executable(id)
    vs_debugger_directory(id "${CMAKE_SOURCE_DIR}/home")
    install(TARGETS id
        DESTINATION .
        RUNTIME_DEPENDENCIES
            PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
            POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    )

    add_custom_target(txt-doc ALL SOURCES "${CMAKE_CURRENT_BINARY_DIR}/id.txt")
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/id.txt"
        COMMAND id savedir=. makedoc
        COMMENT "Building documentation text file"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS native-help
    )
    target_folder(txt-doc "Docs")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/id.txt" DESTINATION .)
    set_property(INSTALL "id.txt" PROPERTY CPACK_START_MENU_SHORTCUTS "Documentation")
endif()
