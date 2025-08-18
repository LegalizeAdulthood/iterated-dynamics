# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2018-2024 Richard Thomson

find_package(wxWidgets CONFIG REQUIRED)

add_library(os STATIC
    WxDriver.h WxDriver.cpp
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

configure_file(id.rc.in id.rc @ONLY)
add_executable(id WIN32
    Id.ico
    id.rc.in
    "${CMAKE_CURRENT_BINARY_DIR}/id.rc"
    resource.h
    CGATextWindow.h
    IdApp.h IdApp.cpp
    IdFrame.h IdFrame.cpp
    init_failure.cpp
    #TextScreen.h TextScreen.cpp
    WxSpecialDirectories.cpp
)
source_group("CMake Templates" REGULAR_EXPRESSION "^.*\.in$")
target_include_directories(id PUBLIC .)
target_link_libraries(id PUBLIC libid)
install(TARGETS id
    DESTINATION .
    RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)
