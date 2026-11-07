# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2018-2026 Richard Thomson

find_package(fmt CONFIG REQUIRED)
find_package(wxWidgets CONFIG REQUIRED)

add_library(gui STATIC
    include/gui/App.h App.cpp
    include/gui/Frame.h Frame.cpp
    include/gui/Plot.h Plot.cpp
    include/gui/Screen.h
    include/gui/TextScreen.h TextScreen.cpp
)
target_include_directories(gui PRIVATE . PUBLIC include)
target_link_libraries(gui PUBLIC id::ui fmt::fmt-header-only wx::stc wx::core wx::base)
target_folder(gui "Libraries/wx")

add_library(driver-wx STATIC
    WxBaseDriver.h WxBaseDriver.cpp
    WxDiskDriver.h WxDiskDriver.cpp
    WxDriver.h WxDriver.cpp
)
target_link_libraries(driver-wx PUBLIC gui id::ui)
target_include_directories(driver-wx PUBLIC .)
target_folder(driver-wx "Libraries/wx")
if(WIN32)
    target_sources(driver-wx PRIVATE ../win32/create_minidump.h ../win32/create_minidump.cpp)
    target_include_directories(driver-wx PRIVATE ../win32)
    target_link_libraries(driver-wx PRIVATE user32.lib)
endif()

add_executable(id wx_id.h wx_id.cpp init_failure.cpp)
source_group("CMake Templates" REGULAR_EXPRESSION "^.*\.in$")
target_include_directories(id PUBLIC .)
target_link_libraries(id PUBLIC driver-wx)
if(WIN32)
    target_id_resources(id)
    target_link_libraries(id PRIVATE os)
endif()
gui_executable(id RUNTIME_NAME "$<IF:$<PLATFORM_ID:Windows>,id,xid>")
vs_debugger_directory(id "${CMAKE_SOURCE_DIR}/home")
if(WIN32)
    install(TARGETS id
        DESTINATION .
        RUNTIME_DEPENDENCIES
            PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
            POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    )
else()
    install(TARGETS id DESTINATION .)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
