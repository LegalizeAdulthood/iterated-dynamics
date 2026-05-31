# SPDX-License-Identifier: GPL-3.0-only
#
# Until the unix port is complete, we can't run end-to-end UI tests there.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

include(IdTestHome)

id_make_test_home(makepar-mig-pieces MAKEPAR_MIG_PIECES_HOME
    IMAGES make_mig_input.gif
    KEYS make_mig_pieces.key)
add_test(NAME AutokeyTest.makepar-mig-pieces
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DINPUT_IMAGE=image/make_mig_input.gif"
        "-DAUTO_KEY=key/make_mig_pieces.key"
        "-DGOLD_BAT=${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.bat"
        "-DGOLD_PAR=${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.par"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/make_mig_pieces_test.cmake"
    WORKING_DIRECTORY "${MAKEPAR_MIG_PIECES_HOME}"
)
set_tests_properties(AutokeyTest.makepar-mig-pieces
    PROPERTIES LABELS "autokey;ui")

id_make_test_home(3dlook-targa THREE_D_LOOK_TARGA_HOME
    IMAGES gold-3dlook01.pot=potntial.pot
    KEYS 3dlook_targa.key
    PARS radar.par)
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

id_make_test_home(rds-random RDS_RANDOM_HOME
    KEYS rds_random.key)
add_test(NAME AutokeyTest.rds-random
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
        "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-rds-random.gif"
        "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/rds-random-diff.gif"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/rds_random_test.cmake"
    WORKING_DIRECTORY "${RDS_RANDOM_HOME}"
)
set_tests_properties(AutokeyTest.rds-random
    PROPERTIES LABELS "autokey;ui;rds")
