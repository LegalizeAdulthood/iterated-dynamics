# Copyright 2018 Richard Thomson

add_executable(hc
    hc.cpp
    ${OS_HC_FILES})
target_include_directories(hc PRIVATE ../headers)
target_compile_definitions(hc PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_link_libraries(hc PRIVATE helpcom os_hc)

add_custom_target(native_help
    DEPENDS hc ${CMAKE_CURRENT_BINARY_DIR}/helpdefs.h "${CMAKE_CURRENT_BINARY_DIR}/id.hlp"
    SOURCES help.src help2.src help3.src help4.src help5.src)
source_group("Help Files" REGULAR_EXPRESSION .*\\.src)
add_dependencies(native_help hc)
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

add_library(help_defs INTERFACE)
target_sources(help_defs INTERFACE
    ${CMAKE_BINARY_DIR}/include/helpdefs.h)
target_include_directories(help_defs INTERFACE ${CMAKE_BINARY_DIR}/include)
