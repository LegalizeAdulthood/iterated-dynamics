# SPDX-License-Identifier: GPL-3.0-only
#
include(IdTestHome)
include(AddImageTest)

set(ID_TEST_MACOS_ARM64_GOLD OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(ID_TEST_MACOS_ARM64_GOLD ON)
endif()
set(ID_TEST_MACOS_X64_GOLD OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(ID_TEST_MACOS_X64_GOLD ON)
endif()

function(add_par_test name)
    cmake_parse_arguments(PAR_TEST "" "GOLD" "" ${ARGN})
    if(PAR_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${PAR_TEST_UNPARSED_ARGUMENTS}")
    endif()
    if(PAR_TEST_GOLD)
        set(PAR_TEST_GOLD_ARG GOLD "${PAR_TEST_GOLD}")
    endif()
    id_make_test_home(${name} PAR_TEST_HOME
        PARS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.par")
    add_image_test("${name}"
        ${PAR_TEST_GOLD_ARG}
        PARAMETERS "@${name}.par/${name}"
        WORKING_DIRECTORY "${PAR_TEST_HOME}"
        LABELS par)
endfunction()

foreach(par Land_of_Loti Broken_Arches Waves_in_the_Night
        Cosmic_Laser_Beams Mass_Ejection Faces_in_the_Night alien shockwave
        nameless Semi-Symmetrical monument clawden raindisk slithy_tove
        petals)
    add_par_test(${par})
endforeach()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(KILLER_MOSQUITO_GOLD GOLD "gold-linux-Killer_Mosquito")
elseif(ID_TEST_MACOS_ARM64_GOLD)
    set(KILLER_MOSQUITO_GOLD GOLD "gold-Killer_Mosquito-macOS-arm64")
elseif(ID_TEST_MACOS_X64_GOLD)
    set(KILLER_MOSQUITO_GOLD GOLD "gold-Killer_Mosquito-macOS-x64")
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_par_test(Killer_Mosquito ${KILLER_MOSQUITO_GOLD})
endif()
