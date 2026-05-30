# SPDX-License-Identifier: GPL-3.0-only
#
# Until the unix port is complete, we can't run end-to-end image tests there
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

include(IdTestHome)
include(AddImageTest)

function(add_par_test name)
    id_make_test_home(${name} PAR_TEST_HOME
        PARS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.par")
    add_image_test("${name}"
        PARAMETERS "@${name}.par/${name}"
        WORKING_DIRECTORY "${PAR_TEST_HOME}"
        LABELS par)
endfunction()

foreach(par Land_of_Loti Killer_Mosquito Broken_Arches Waves_in_the_Night
        Cosmic_Laser_Beams Mass_Ejection Faces_in_the_Night alien shockwave
        nameless Semi-Symmetrical monument clawden raindisk slithy_tove
        petals)
    add_par_test(${par})
endforeach()
