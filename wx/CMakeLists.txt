# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2018-2024 Richard Thomson

find_package(wxWidgets CONFIG REQUIRED)

add_library(os STATIC
    read_ticker.cpp
    stack_avail.cpp
)
target_compile_definitions(os PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(os PRIVATE ${ID_TARGET_OPTIONS})
target_link_libraries(os PUBLIC wx::core wx::base config)
target_include_directories(os PRIVATE ../libid/include)
target_include_directories(os PUBLIC .)
set_target_properties(os PROPERTIES FOLDER Libraries)
add_dependencies(os native-help)

add_library(gui STATIC
    include/gui/App.h App.cpp
    include/gui/Frame.h Frame.cpp
    include/gui/Plot.h Plot.cpp
    include/gui/Screen.h
    include/gui/TextScreen.h TextScreen.cpp
)
target_include_directories(gui PUBLIC include)
target_link_libraries(gui PUBLIC libid wx::stc wx::core wx::base)
target_folder(gui "Libraries/wx")

configure_file(id.rc.in id.rc @ONLY)
add_executable(id
    Id.ico
    id.rc.in
    "${CMAKE_CURRENT_BINARY_DIR}/id.rc"
    resource.h
    init_failure.cpp
    WxDriver.h WxDriver.cpp
    WxSpecialDirectories.cpp
)
source_group("CMake Templates" REGULAR_EXPRESSION "^.*\.in$")
target_include_directories(id PUBLIC .)
target_link_libraries(id PUBLIC gui)
gui_executable(id)
vs_debugger_directory(id "${CMAKE_SOURCE_DIR}/home")
install(TARGETS id
    DESTINATION .
    RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)

add_subdirectory(tests)
