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
    cmake_parse_arguments(PAR_TEST "" "GOLD;TIMEOUT" "" ${ARGN})
    if(PAR_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${PAR_TEST_UNPARSED_ARGUMENTS}")
    endif()
    set(PAR_TEST_ARGS)
    if(PAR_TEST_GOLD)
        list(APPEND PAR_TEST_ARGS GOLD "${PAR_TEST_GOLD}")
    endif()
    if(DEFINED PAR_TEST_TIMEOUT)
        list(APPEND PAR_TEST_ARGS TIMEOUT "${PAR_TEST_TIMEOUT}")
    endif()
    id_make_test_home(${name} PAR_TEST_HOME
        PARS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.par")
    add_image_test("${name}"
        ${PAR_TEST_ARGS}
        PARAMETERS "@${name}.par/${name}"
        WORKING_DIRECTORY "${PAR_TEST_HOME}"
        LABELS par)
endfunction()

foreach(par Broken_Arches Cosmic_Laser_Beams Faces_in_the_Night alien
        nameless Semi-Symmetrical clawden slithy_tove petals)
    add_par_test(${par})
endforeach()

# Full-size reference images need longer render budgets in Debug.
foreach(par Land_of_Loti Waves_in_the_Night Mass_Ejection shockwave
        monument raindisk)
    add_par_test(${par} TIMEOUT 360)
endforeach()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(KILLER_MOSQUITO_GOLD GOLD "gold-linux-Killer_Mosquito")
elseif(ID_TEST_MACOS_ARM64_GOLD)
    set(KILLER_MOSQUITO_GOLD GOLD "gold-Killer_Mosquito-macOS-arm64")
elseif(ID_TEST_MACOS_X64_GOLD)
    set(KILLER_MOSQUITO_GOLD GOLD "gold-Killer_Mosquito-macOS-x64")
endif()
if(NOT CMAKE_SYSTEM_NAME MATCHES "^.*BSD$")
    add_par_test(Killer_Mosquito ${KILLER_MOSQUITO_GOLD} TIMEOUT 360)
endif()
