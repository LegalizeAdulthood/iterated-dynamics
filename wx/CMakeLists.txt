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

add_library(TextScreen STATIC
    include/gui/TextScreen.h TextScreen.cpp
)
target_include_directories(TextScreen PUBLIC include)
target_link_libraries(TextScreen PUBLIC wx::wxstc)
target_folder(TextScreen "Libraries/wx")

add_library(Plot STATIC
    include/gui/Plot.h Plot.cpp
)
target_include_directories(Plot PUBLIC include)
target_link_libraries(Plot PUBLIC wx::core config)
target_folder(Plot "Libraries/wx")

add_library(IdFrame STATIC
    include/gui/IdFrame.h IdFrame.cpp)
target_include_directories(IdFrame PUBLIC include)
target_link_libraries(IdFrame PUBLIC TextScreen Plot wx::core)
target_folder(IdFrame "Libraries/wx")

add_library(gui STATIC
    include/gui/CGATextWindow.h
    include/gui/IdApp.h IdApp.cpp
)
target_include_directories(gui PUBLIC include)
target_link_libraries(gui PUBLIC libid IdFrame wx::core wx::base)
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
