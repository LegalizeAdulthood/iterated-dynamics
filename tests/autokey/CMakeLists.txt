# SPDX-License-Identifier: GPL-3.0-only
#

include(IdTestHome)
include(PlatformArgs)

id_make_test_home(makepar-mig-pieces MAKEPAR_MIG_PIECES_HOME
    IMAGES make_mig_input.gif
    KEYS make_mig_pieces.key)
add_test(NAME AutoKeyTest.makepar-mig-pieces
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DINPUT_IMAGE=image/make_mig_input.gif"
        "-DAUTO_KEY=key/make_mig_pieces.key"
        "-DGOLD_BAT=${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.bat"
        "-DGOLD_PAR=${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.par"
        "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
        "-DCMAKE_MODULE_PATH=${CMAKE_SOURCE_DIR}/tests/cmake"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/make_mig_pieces_test.cmake"
    WORKING_DIRECTORY "${MAKEPAR_MIG_PIECES_HOME}"
)
set_tests_properties(AutoKeyTest.makepar-mig-pieces
    PROPERTIES LABELS "autokey;ui")

id_make_test_home(3dlook-targa THREE_D_LOOK_TARGA_HOME
    IMAGES gold-3dlook01.pot=potntial.pot
    KEYS 3dlook_targa.key
    PARS radar.par)
add_test(NAME AutoKeyTest.3dlook-targa
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
        "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-3dlook-targa.tga"
        "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/3dlook_targa_test.cmake"
    WORKING_DIRECTORY "${THREE_D_LOOK_TARGA_HOME}"
)
set_tests_properties(AutoKeyTest.3dlook-targa
    PROPERTIES LABELS "autokey;ui;targa")

id_make_test_home(rds-random RDS_RANDOM_HOME
    KEYS rds_random.key)
add_test(NAME AutoKeyTest.rds-random
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
        "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-rds-random.gif"
        "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/rds-random-diff.gif"
        "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/rds_random_test.cmake"
    WORKING_DIRECTORY "${RDS_RANDOM_HOME}"
)
set_tests_properties(AutoKeyTest.rds-random
    PROPERTIES LABELS "autokey;ui;rds")
