# SPDX-License-Identifier: GPL-3.0-only
#
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

set(image_files
    "type-mandel.png"
    "type-julia.png"
# TODO: The thumbnail is so dark as to appear wholly black.
#    "type-julia_inverse.png"
    "type-newtbasin.png"
    "type-newton.png"
    "type-complexnewton.png"
    "type-lambda.png"
    "type-mandellambda.png"
    "type-circle.png"
    "type-plasma.png"
    "type-lambdafn.png"
    "type-mandelfn.png"
    "type-barnsleym1.png"
    "type-barnsleym2.png"
    "type-barnsleym3.png"
    "type-barnsleyj1.png"
    "type-barnsleyj2.png"
    "type-barnsleyj3.png"
    "type-sierpinski.png"
    "type-mandel4.png"
    "type-julia4.png"
    "type-manfn-plus-zsqrd.png"
    "type-manzpower.png"
    "type-manzzpwr.png"
    "type-manfn-plus-exp.png"
    "type-julfn-plus-zsqrd.png"
    "type-julzpower.png"
    "type-julzzpwr.png"
    "type-julfn-plus-exp.png"
    "type-popcorn.png"
    "type-popcornjul.png"
# TODO: the default parameters just yield regular M-set
#    "type-marksmandel.png"
#    "type-cmplxmarksmand.png"
    "type-unity.png"
)
foreach(image ${image_files})
    configure_file(images/${image} help/images/${image} COPYONLY)
    install(FILES images/${image} DESTINATION help/images)
    configure_file(images/thumbnails/${image} help/images/thumbnails/${image} COPYONLY)
    install(FILES images/thumbnails/${image} DESTINATION help/images/thumbnails)
endforeach()
