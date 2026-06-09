# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2018-2026 Richard Thomson

find_program(ASCII_DOCTOR asciidoctor)
find_program(ASCII_DOC_PDF asciidoctor-pdf)

option(ID_PACKAGE_REQUIRE_HTML_DOC "Require HTML documentation in packages" OFF)
option(ID_PACKAGE_REQUIRE_PDF_DOC "Require PDF documentation in packages" OFF)

set(ID_HELP_SOURCES help.src help2.src help3.src help4.src help5.src)
set(ID_HTML_DOC ${CMAKE_CURRENT_BINARY_DIR}/id.html)
set(ID_PDF_DOC ${CMAKE_CURRENT_BINARY_DIR}/id.pdf)
set(ID_INSTALL_HTML_DOC OFF)

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
target_folder(native-help "Docs")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/id.hlp DESTINATION .)

add_library(help-links 
    ${CMAKE_CURRENT_BINARY_DIR}/help_links.h
    ${CMAKE_CURRENT_BINARY_DIR}/help_links.cpp
)
target_include_directories(help-links PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(help-links PUBLIC help-defs)
target_folder(help-links "Libraries")

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
target_folder(ascii-doc "Docs")

if(ASCII_DOCTOR)
    set(ID_HTML_DOC_ALL)
    if(NOT ID_PACKAGE_REQUIRE_HTML_DOC)
        set(ID_HTML_DOC_ALL ALL)
    endif()
    add_custom_target(html-doc ${ID_HTML_DOC_ALL}
        DEPENDS hc ascii-doc
        SOURCES ${ID_HTML_DOC} ${CMAKE_CURRENT_BINARY_DIR}/id.adoc)
    add_custom_command(
        OUTPUT ${ID_HTML_DOC}
        COMMAND asciidoctor ${CMAKE_CURRENT_BINARY_DIR}/id.adoc
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/id.adoc)
    target_folder(html-doc "Docs")
endif()
if(ASCII_DOCTOR OR ID_PACKAGE_REQUIRE_HTML_DOC)
    set(ID_INSTALL_HTML_DOC ON)
    install(FILES ${ID_HTML_DOC} DESTINATION .)
    set_property(INSTALL "id.html" PROPERTY CPACK_START_MENU_SHORTCUTS "HTML Documentation")
elseif(EXISTS ${CMAKE_SOURCE_DIR}/id.html)
    set(ID_INSTALL_HTML_DOC ON)
    message(STATUS "Installing prebuilt id.html")
    install(FILES ${CMAKE_SOURCE_DIR}/id.html DESTINATION .)
    set_property(INSTALL "id.html" PROPERTY CPACK_START_MENU_SHORTCUTS "HTML Documentation")
endif()

if(ASCII_DOC_PDF)
    set(ID_PDF_DOC_ALL)
    if(NOT ID_PACKAGE_REQUIRE_PDF_DOC)
        set(ID_PDF_DOC_ALL ALL)
    endif()
    add_custom_target("pdf-doc" ${ID_PDF_DOC_ALL}
        DEPENDS hc "ascii-doc"
        SOURCES ${ID_PDF_DOC} ${CMAKE_CURRENT_BINARY_DIR}/id.adoc)
    add_custom_command(
        OUTPUT ${ID_PDF_DOC}
        COMMAND "asciidoctor-pdf" ${CMAKE_CURRENT_BINARY_DIR}/id.adoc
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/id.adoc)
    target_folder("pdf-doc" "Docs")
endif()
if(ASCII_DOC_PDF OR ID_PACKAGE_REQUIRE_PDF_DOC)
    install(FILES ${ID_PDF_DOC} DESTINATION .)
    set_property(INSTALL "id.pdf" PROPERTY CPACK_START_MENU_SHORTCUTS "PDF Documentation")
elseif(EXISTS ${CMAKE_SOURCE_DIR}/id.pdf)
    message(STATUS "Installing prebuilt id.pdf")
    install(FILES ${CMAKE_SOURCE_DIR}/id.pdf DESTINATION .)
    set_property(INSTALL "id.pdf" PROPERTY CPACK_START_MENU_SHORTCUTS "PDF Documentation")
endif()

add_custom_target(doc-file ALL
    SOURCES "${CMAKE_CURRENT_BINARY_DIR}/id.txt")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/id.txt"
    COMMAND hc /i "${CMAKE_CURRENT_SOURCE_DIR}" /p
    COMMENT "Building documentation text file"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS native-help
)
target_folder(doc-file "Docs")

set(image_files
    "type-mandel.png"
    "type-julia.png"
    # TODO: The thumbnail is so dark as to appear wholly black.
    # "type-julia_inverse.png"
    "type-newtbasin.png"
    "type-newton.png"
    "type-complexnewton.png"
    "type-complexbasin.png"
    "type-lambda.png"
    "type-mandellambda.png"
    "type-circle.png"
    "type-plasma.png"
    "type-lambdafn.png"
    "type-halley.png"
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
    "type-mandelbrotmix4.png"
    "type-dividebrot5.png"
    "type-burning-ship.png"
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
    "type-marksmandel.png"
    "type-marksjulia.png"
    "type-cmplxmarksmand.png"
    "type-cmplxmarksjul.png"
    "type-marksmandelpwr.png"
    "type-tims_error.png"
    "type-unity.png"
    "type-fn-zsqr.png"
    "type-fn-mul-fn.png"
    "type-fn-mul-z-plus-z.png"
    "type-fn-plus-fn.png"
    "type-fnz-plus-fnpixel.png"
    "type-sqr-1-over-fn.png"
    "type-sqr-fn.png"
    "type-spider.png"
    "type-tetrate.png"
    "type-manowar.png"
    "type-kamtorus.png"
    "type-kamtorus3d.png"
    "type-bifurcation.png"
    "type-bifmay.png"
    "type-bifstewart.png"
    "type-biflambda.png"
    "type-bif-eq-sinpi.png"
    "type-bif-plus-sinpi.png"
    "type-lorenz.png"
    "type-lorenz3d.png"
    "type-rossler3d.png"
    "type-henon.png"
    "type-pickover.png"
    "type-gingerbreadman.png"
    "type-hopalong.png"
    "type-martin.png"
    "type-icons.png"
    "type-icons3d.png"
    "type-latoocarfian.png"
    "type-quat.png"
    "type-quatjul.png"
    "type-hypercomplex.png"
    "type-hypercomplexj.png"
    "type-cellular.png"
    "type-frothybasin.png"
    "type-julibrot.png"
    # TODO: thumbnail is almost completely black
    # "type-diffusion.png"
    "type-lyapunov.png"
    "type-magnet1m.png"
    "type-magnet2m.png"
    "type-magnet1j.png"
    "type-magnet2j.png"
    "type-phoenix.png"
    "type-mandphoenix.png"
    "type-phoenixcplx.png"
    "type-mandphoenixclx.png"
    "type-lambda-fn-or-fn.png"
    "type-manlam-fn-or-fn.png"
    "type-julia-fn-or-fn.png"
    "type-mandel-fn-or-fn.png"
    "type-volterra-lotka.png"
    "type-escher_julia.png"
    "type-dynamic.png"
    "type-mandelcloud.png"
)
foreach(image ${image_files})
    configure_file(images/${image} help/images/${image} COPYONLY)
    configure_file(images/thumbnails/${image} help/images/thumbnails/${image} COPYONLY)
    if(ID_INSTALL_HTML_DOC)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/help/images/${image}
            DESTINATION help/images)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/help/images/thumbnails/${image}
            DESTINATION help/images/thumbnails)
    endif()
endforeach()
