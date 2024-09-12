# Copyright 2018 Richard Thomson

find_program(ASCII_DOCTOR asciidoctor)

set(ID_HELP_SOURCES help.src help2.src help3.src help4.src help5.src)

add_custom_target(native-help
    DEPENDS hc ${CMAKE_CURRENT_BINARY_DIR}/helpdefs.h "${CMAKE_CURRENT_BINARY_DIR}/id.hlp"
    SOURCES ${ID_HELP_SOURCES})
source_group("Help Files" REGULAR_EXPRESSION .*\\.src)
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/helpdefs.h
        ${CMAKE_CURRENT_BINARY_DIR}/help_links.h
        ${CMAKE_CURRENT_BINARY_DIR}/help_links.cpp
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
set_target_properties(native-help PROPERTIES FOLDER Docs)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/id.hlp DESTINATION .)

add_library(help-links 
    ${CMAKE_CURRENT_BINARY_DIR}/help_links.h
    ${CMAKE_CURRENT_BINARY_DIR}/help_links.cpp
)
target_include_directories(help-links PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(help-links PUBLIC help-defs)
set_target_properties(help-links PROPERTIES FOLDER Libraries)

add_custom_target(ascii-doc
    DEPENDS hc
    SOURCES ${ID_HELP_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/id.adoc)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/id.adoc
    COMMAND hc /i ${CMAKE_CURRENT_SOURCE_DIR} /adoc help.src
    DEPENDS hc
        help.src
        help2.src
        help3.src
        help4.src
        help5.src)
set_target_properties(ascii-doc PROPERTIES FOLDER Docs)

if(ASCII_DOCTOR)
    add_custom_target(html-doc
        DEPENDS hc ascii-doc
        SOURCES ${CMAKE_CURRENT_BINARY_DIR}/id.html ${CMAKE_CURRENT_BINARY_DIR}/id.adoc)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/id.html
        COMMAND asciidoctor ${CMAKE_CURRENT_BINARY_DIR}/id.adoc
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/id.adoc)
    set_target_properties(html-doc PROPERTIES FOLDER Docs)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/id.html DESTINATION .)
    set_property(INSTALL "id.html" PROPERTY CPACK_START_MENU_SHORTCUTS "HTML Documentation")
elseif(EXISTS ${CMAKE_SOURCE_DIR}/id.html)
    message(STATUS "Installing prebuilt id.html")
    install(FILES ${CMAKE_SOURCE_DIR}/id.html DESTINATION .)
    set_property(INSTALL "id.html" PROPERTY CPACK_START_MENU_SHORTCUTS "HTML Documentation")
endif()
