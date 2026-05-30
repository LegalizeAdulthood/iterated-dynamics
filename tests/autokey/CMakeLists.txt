# SPDX-License-Identifier: GPL-3.0-only
#
# Until the unix port is complete, we can't run end-to-end UI tests there.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

include(IdTestHome)

id_make_test_home(makepar-mig-pieces MAKEPAR_MIG_PIECES_HOME)
id_make_test_home(3dlook-targa THREE_D_LOOK_TARGA_HOME PARS radar.par)
file(MAKE_DIRECTORY "${THREE_D_LOOK_TARGA_HOME}/image")
file(MAKE_DIRECTORY "${THREE_D_LOOK_TARGA_HOME}/key")
file(COPY_FILE "${CMAKE_SOURCE_DIR}/tests/images/gold-3dlook01.pot"
    "${THREE_D_LOOK_TARGA_HOME}/image/potntial.pot")
file(COPY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/3dlook_targa.key"
    "${THREE_D_LOOK_TARGA_HOME}/key/3dlook_targa.key")

add_test(NAME AutokeyTest.makepar-mig-pieces
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DINPUT_IMAGE=${CMAKE_SOURCE_DIR}/tests/images/make_mig_input.gif"
        "-DAUTO_KEY=${CMAKE_CURRENT_SOURCE_DIR}/make_mig_pieces.key"
        "-DGOLD_BAT=${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.bat"
        "-DGOLD_PAR=${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.par"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/make_mig_pieces_test.cmake"
    WORKING_DIRECTORY "${MAKEPAR_MIG_PIECES_HOME}"
)
set_tests_properties(AutokeyTest.makepar-mig-pieces
    PROPERTIES LABELS "autokey;ui")

add_test(NAME AutokeyTest.3dlook-targa
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
        "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-3dlook-targa.tga"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/3dlook_targa_test.cmake"
    WORKING_DIRECTORY "${THREE_D_LOOK_TARGA_HOME}"
)
set_tests_properties(AutokeyTest.3dlook-targa
    PROPERTIES LABELS "autokey;ui;targa")
