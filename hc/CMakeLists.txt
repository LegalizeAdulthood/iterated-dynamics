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

set(ID_HELP_SOURCES help.src help2.src help3.src help4.src help5.src)

add_custom_target(native-help
    DEPENDS hc ${CMAKE_CURRENT_BINARY_DIR}/helpdefs.h "${CMAKE_CURRENT_BINARY_DIR}/id.hlp"
    SOURCES ${ID_HELP_SOURCES})
source_group("Help Files" REGULAR_EXPRESSION .*\\.src)
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/helpdefs.h
        ${CMAKE_CURRENT_BINARY_DIR}/id.hlp
        ${CMAKE_BINARY_DIR}/include/helpdefs.h
        ${home_dir}/id.hlp
        ${CMAKE_BINARY_DIR}/id.hlp
    COMMAND hc /i ${CMAKE_CURRENT_SOURCE_DIR} /c help.src
    COMMAND ${CMAKE_COMMAND} -E copy_if_different helpdefs.h ${CMAKE_BINARY_DIR}/include/helpdefs.h
    COMMAND ${CMAKE_COMMAND} -E copy_if_different id.hlp ${home_dir}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different id.hlp ${CMAKE_BINARY_DIR}
    DEPENDS hc
        help.src
        help2.src
        help3.src
        help4.src
        help5.src)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/id.hlp DESTINATION .)

add_library(help-defs INTERFACE)
target_sources(help-defs INTERFACE
    ${CMAKE_BINARY_DIR}/include/helpdefs.h)
target_include_directories(help-defs INTERFACE ${CMAKE_BINARY_DIR}/include)

add_custom_target(ascii-doc
    DEPENDS hc
    SOURCES ${ID_HELP_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/index.adoc)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/index.adoc
    COMMAND hc /i ${CMAKE_CURRENT_SOURCE_DIR} /adoc help.src
    DEPENDS hc
        help.src
        help2.src
        help3.src
        help4.src
        help5.src)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/index.adoc DESTINATION .)

add_subdirectory(tests)
