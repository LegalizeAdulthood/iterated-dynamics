# SPDX-License-Identifier: GPL-3.0-only
#
# Until the unix port is complete, we can't run end-to-end UI tests there.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

set(MAKEPAR_MIG_PIECES_HOME "${CMAKE_CURRENT_BINARY_DIR}/makepar-mig-pieces/home")
file(MAKE_DIRECTORY "${MAKEPAR_MIG_PIECES_HOME}")
file(COPY_FILE "${CMAKE_SOURCE_DIR}/home/sstools.ini"
    "${MAKEPAR_MIG_PIECES_HOME}/sstools.ini")
file(COPY_FILE "${CMAKE_SOURCE_DIR}/home/id.cfg"
    "${MAKEPAR_MIG_PIECES_HOME}/id.cfg")

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
