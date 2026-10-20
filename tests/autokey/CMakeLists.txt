# SPDX-License-Identifier: GPL-3.0-only
#

include(IdTestHome)
include(PlatformArgs)

set(MAKEPAR_MIG_SCRIPT_FILE "makemig.bat")
set(MAKEPAR_MIG_GOLD_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.bat")
set(MAKEPAR_MIG_NORMALIZE_SCRIPT_EXECUTABLE OFF)
set(MAKEPAR_MIG_RUN_SCRIPT OFF)
set(MAKEPAR_MIG_LABELS "autokey;ui")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(MAKEPAR_MIG_RUN_SCRIPT ON)
    set(MAKEPAR_MIG_LABELS "acceptance;autokey;ui")
else()
    set(MAKEPAR_MIG_SCRIPT_FILE "makemig.sh")
    set(MAKEPAR_MIG_GOLD_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.sh")
    set(MAKEPAR_MIG_NORMALIZE_SCRIPT_EXECUTABLE ON)
    set(MAKEPAR_MIG_RUN_SCRIPT ON)
    set(MAKEPAR_MIG_LABELS "acceptance;autokey;ui")
endif()

id_make_test_home(makepar-mig-pieces MAKEPAR_MIG_PIECES_HOME
    IMAGES make_mig_input.gif
    KEYS make_mig_pieces.key)
add_test(NAME AutoKeyTest.makepar-mig-pieces
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DID_DIR=$<SHELL_PATH:$<TARGET_FILE_DIR:id>>"
        "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
        "-DINPUT_IMAGE=image/make_mig_input.gif"
        "-DAUTO_KEY=key/make_mig_pieces.key"
        "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.gif"
        "-DTEST_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/test-make_mig_script.gif"
        "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/make_mig_script-diff.png"
        "-DTEST_ARTIFACT_DIR=${CMAKE_BINARY_DIR}/test-artifacts/AutoKeyTest.makepar-mig-pieces"
        "-DSCRIPT_FILE=${MAKEPAR_MIG_SCRIPT_FILE}"
        "-DGOLD_SCRIPT=${MAKEPAR_MIG_GOLD_SCRIPT}"
        "-DGOLD_PAR=${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig_pieces.par"
        "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
        "-DNORMALIZE_SCRIPT_EXECUTABLE=${MAKEPAR_MIG_NORMALIZE_SCRIPT_EXECUTABLE}"
        "-DRUN_SCRIPT=${MAKEPAR_MIG_RUN_SCRIPT}"
        "-DCMAKE_MODULE_PATH=${CMAKE_SOURCE_DIR}/tests/cmake"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/make_mig_pieces_test.cmake"
    WORKING_DIRECTORY "${MAKEPAR_MIG_PIECES_HOME}"
)
set_tests_properties(AutoKeyTest.makepar-mig-pieces
    PROPERTIES LABELS "${MAKEPAR_MIG_LABELS}")

id_make_test_home(3dlook-targa THREE_D_LOOK_TARGA_HOME
    IMAGES gold-3dlook01.pot=potntial.pot
    KEYS 3dlook_targa.key
    PARS radar.par)
add_test(NAME AutoKeyTest.3dlook-targa
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
        "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-3dlook-targa.tga"
        "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/3dlook-targa-diff.png"
        "-DTEST_ARTIFACT_DIR=${CMAKE_BINARY_DIR}/test-artifacts/AutoKeyTest.3dlook-targa"
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
        "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/rds-random-diff.png"
        "-DTEST_ARTIFACT_DIR=${CMAKE_BINARY_DIR}/test-artifacts/AutoKeyTest.rds-random"
        "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/rds_random_test.cmake"
    WORKING_DIRECTORY "${RDS_RANDOM_HOME}"
)
set_tests_properties(AutoKeyTest.rds-random
    PROPERTIES LABELS "autokey;ui;rds")

id_make_test_home(palette-cycle PALETTE_CYCLE_HOME
    KEYS palette_cycle.key)
add_test(NAME AutoKeyTest.palette-cycle
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/palette_cycle_test.cmake"
    WORKING_DIRECTORY "${PALETTE_CYCLE_HOME}"
)
set_tests_properties(AutoKeyTest.palette-cycle
    PROPERTIES LABELS "autokey;ui;colormap" TIMEOUT 90)

id_make_test_home(cellular-preparation CELLULAR_PREPARATION_HOME
    KEYS cellular_preparation.key)
add_test(NAME AutoKeyTest.cellular-preparation
    COMMAND ${CMAKE_COMMAND}
        "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
        "-DCHECK_PREPARATION=$<SHELL_PATH:$<TARGET_FILE:check-cellular-preparation>>"
        "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cellular_preparation_test.cmake"
    WORKING_DIRECTORY "${CELLULAR_PREPARATION_HOME}")
set_tests_properties(AutoKeyTest.cellular-preparation
    PROPERTIES LABELS "autokey;ui;cellular" TIMEOUT 130)

foreach(passes IN ITEMS 1 2)
    foreach(pixel IN ITEMS optimized general)
        set(name "standard-interrupt-${passes}-${pixel}")
        id_make_test_home("${name}" STANDARD_INTERRUPT_HOME
            KEYS standard_interrupt.key)
        add_test(NAME "AutoKeyTest.${name}"
            COMMAND ${CMAKE_COMMAND}
                "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
                "-DCHECK_RESUME=$<SHELL_PATH:$<TARGET_FILE:check-standard-resume>>"
                "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
                "-DPASSES=${passes}"
                "-DGENERAL_PIXEL=$<STREQUAL:${pixel},general>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/standard_interrupt_test.cmake"
            WORKING_DIRECTORY "${STANDARD_INTERRUPT_HOME}")
        set_tests_properties("AutoKeyTest.${name}"
            PROPERTIES LABELS "autokey;ui;passes" TIMEOUT 250)
    endforeach()
endforeach()

foreach(perturbation IN ITEMS auto no)
    set(name "perturbation-pass-switch-${perturbation}")
    id_make_test_home("${name}" PERTURBATION_PASS_SWITCH_HOME
        KEYS perturbation_pass_switch.key)
    add_test(NAME "AutoKeyTest.${name}"
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DCHECK_SWITCH=$<SHELL_PATH:$<TARGET_FILE:check-pass-switch>>"
            "-DID_EXTRA_ARGS=${ID_EXTRA_ARGS}"
            "-DPERTURBATION=${perturbation}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/perturbation_pass_switch_test.cmake"
        WORKING_DIRECTORY "${PERTURBATION_PASS_SWITCH_HOME}")
    set_tests_properties("AutoKeyTest.${name}"
        PROPERTIES LABELS "autokey;ui;passes;perturbation" TIMEOUT 250)
endforeach()
