# Copyright 2018 Richard Thomson

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

add_library(os-hc INTERFACE)
target_link_libraries(os-hc INTERFACE helpcom config)
set_target_properties(os-hc PROPERTIES FOLDER Libraries)

add_library(os STATIC
    d_disk.cpp
    d_gdi.cpp
    d_win32.h
    d_win32.cpp
    frame.h
    frame.cpp
    os_win32.cpp
    plot.h
    plot.cpp
    win_text.h
    win_text.cpp
    create_minidump.h
    instance.h
    ods.h
    tos.h
    special_dirs.cpp
    win_defines.h
)
target_compile_definitions(os PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(os PRIVATE ${ID_TARGET_OPTIONS})
target_link_libraries(os PUBLIC gdi32.lib user32.lib shlwapi.lib os-hc help-defs)
target_include_directories(os PRIVATE ../libid/include PUBLIC .)
set_target_properties(os PROPERTIES FOLDER Libraries)
add_dependencies(os native-help)

configure_file(id.rc.in id.rc @ONLY)
add_executable(id WIN32
    Id.ico
    id.rc.in
    "${CMAKE_CURRENT_BINARY_DIR}/id.rc"
    resource.h
    win_main.cpp)
source_group("CMake Templates" REGULAR_EXPRESSION "^.*\.in$")
target_include_directories(id PUBLIC .)
target_link_libraries(id PUBLIC libid)
install(TARGETS id
    DESTINATION .
    RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)

add_custom_target(doc-file ALL SOURCES "${CMAKE_CURRENT_BINARY_DIR}/id.txt")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/id.txt"
    COMMAND id savedir=. makedoc
    COMMENT "Building documentation text file"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS native-help
)
set_target_properties(doc-file PROPERTIES FOLDER Docs)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/id.txt" DESTINATION .)
set_property(INSTALL "id.txt" PROPERTY CPACK_START_MENU_SHORTCUTS "Documentation")
