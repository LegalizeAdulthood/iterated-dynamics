# Copyright 2018 Richard Thomson

add_library(helpcom STATIC
    helpcom.cpp
    ../headers/helpcom.h)
target_compile_definitions(helpcom PUBLIC ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_include_directories(helpcom PUBLIC ../headers)
target_link_libraries(helpcom PUBLIC config)

add_executable(hc
    hc.cpp
    ${OS_HC_FILES})
target_include_directories(hc PRIVATE ../headers)
target_compile_definitions(hc PRIVATE ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_link_libraries(hc PRIVATE helpcom os_hc)

add_custom_target(native_help
    DEPENDS hc ${CMAKE_CURRENT_BINARY_DIR}/helpdefs.h ${CMAKE_CURRENT_BINARY_DIR}/fractint.hlp
    SOURCES help.src help2.src help3.src help4.src help5.src)
source_group("Help Files" REGULAR_EXPRESSION .*\\.src)
add_dependencies(native_help hc)
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/helpdefs.h
        ${CMAKE_CURRENT_BINARY_DIR}/fractint.hlp
        ${ID_DIR}/headers/helpdefs.h
        ${home_dir}/fractint.hlp
    COMMAND hc /i ${CMAKE_CURRENT_SOURCE_DIR} /c help.src
    COMMAND ${CMAKE_COMMAND} -E copy helpdefs.h ${ID_DIR}/headers
    COMMAND ${CMAKE_COMMAND} -E copy fractint.hlp ${home_dir}
    DEPENDS hc
        help.src
        help2.src
        help3.src
        help4.src
        help5.src)
