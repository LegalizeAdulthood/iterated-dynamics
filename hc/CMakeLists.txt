# Copyright 2018 Richard Thomson

add_library(helpcom STATIC
    helpcom.cpp
    ../headers/helpcom.h)
target_include_directories(helpcom PUBLIC ../headers)
target_compile_definitions(helpcom PUBLIC ${OS_DEFINITIONS})

add_executable(hc
    hc.cpp
    ${OS_HC_FILES})
target_include_directories(hc PRIVATE ../headers)
if(${HAVE_OS_DEFINITIONS})
    target_compile_definitions(hc PRIVATE ${OS_DEFINITIONS})
endif()
if(WIN32)
    target_compile_definitions(hc PRIVATE _SCL_SECURE_NO_WARNINGS)
endif()
target_link_libraries(hc PRIVATE helpcom os_hc)

add_custom_target(native_help
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/helpdefs.h ${CMAKE_CURRENT_SOURCE_DIR}/fractint.hlp
    SOURCES help.src help2.src help3.src help4.src help5.src)
source_group("Help Files" REGULAR_EXPRESSION .*\\.src)
add_dependencies(native_help hc)
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/helpdefs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fractint.hlp
        ${ID_DIR}/headers/helpdefs.h
        ${home_dir}/fractint.hlp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND hc /c ${CMAKE_CURRENT_SOURCE_DIR}/help.src
    COMMAND ${CMAKE_COMMAND} -E copy helpdefs.h ${ID_DIR}/headers
    COMMAND ${CMAKE_COMMAND} -E copy fractint.hlp ${home_dir}
    DEPENDS help.src
        help2.src
        help3.src
        help4.src
        help5.src)
