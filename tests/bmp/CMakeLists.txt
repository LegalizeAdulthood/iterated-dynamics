# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
include(IdTestHome)
include(PlatformArgs)
include(AddImageTest)

foreach(view IN ITEMS mandel-direct mandel-perturb julia-direct)
    id_make_test_home(${view} BMP_TEST_HOME
        PARS "${CMAKE_CURRENT_SOURCE_DIR}/${view}.par")
    add_image_test("bmp-${view}"
        GOLD "gold-${view}"
        PARAMETERS "@${view}.par/${view}"
        WORKING_DIRECTORY "${BMP_TEST_HOME}"
        LABELS bmp precision)
endforeach()
