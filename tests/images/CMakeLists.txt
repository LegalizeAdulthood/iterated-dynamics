# SPDX-License-Identifier: GPL-3.0-only
#
# Until the unix port is complete, we can't run end-to-end image tests there
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

function(add_image_test name)
    cmake_parse_arguments(IMAGE_TEST "IGNORE_COLORMAP" "" "PARAMETERS;LABELS" ${ARGN})
    if(IMAGE_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${IMAGE_TEST_UNPARSED_ARGUMENTS}")
    endif()
    list(APPEND IMAGE_TEST_PARAMETERS "batch=yes" "video=F6")
    if(IMAGE_TEST_IGNORE_COLORMAP)
        set(IMAGE_TEST_IGNORE_COLORMAP ON)
    else()
        set(IMAGE_TEST_IGNORE_COLORMAP OFF)
    endif()        
    add_test(NAME ImageTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
            "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-${name}.gif"
            "-DPARAMETERS=${IMAGE_TEST_PARAMETERS}"
            "-DTEST_SAVE_IMAGE=test-${name}.gif"
            "-DTEST_KEEP_IMAGE=${CMAKE_CURRENT_BINARY_DIR}"
            "-DIMAGE_TEST_IGNORE_COLORMAP=${IMAGE_TEST_IGNORE_COLORMAP}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/image_compare_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
    list(APPEND IMAGE_TEST_LABELS "image")
    set_tests_properties(ImageTest.${name} PROPERTIES LABELS "${IMAGE_TEST_LABELS}")
endfunction()

function(add_legacy_image_test name)
    add_image_test(${name} IGNORE_COLORMAP ${ARGN})
endfunction()

function(add_fractal_type_image_test)
    foreach(type ${ARGN})
        add_legacy_image_test(${type} PARAMETERS type=${type})
    endforeach()
endfunction()

function(add_random_fractal_type_image_test)
    foreach(type ${ARGN})
        add_legacy_image_test(${type} PARAMETERS rseed=4567 type=${type})
    endforeach()
endfunction()

# Render an image with a map file and compare the image colormap to the map file
function(add_colormap_test name)
    cmake_parse_arguments(COLORMAP_TEST "" "MAP" "PARAMETERS" ${ARGN})
    if(COLORMAP_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${COLORMAP_TEST_UNPARSED_ARGUMENTS}")
    endif()
    list(APPEND COLORMAP_TEST_PARAMETERS "batch=yes" "video=F6" "colors=@${COLORMAP_TEST_MAP}")
    add_test(NAME ColormapTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DCOLORMAP_COMPARE=$<SHELL_PATH:$<TARGET_FILE:colormap-compare>>"
            "-DPARAMETERS=${COLORMAP_TEST_PARAMETERS}"
            "-DTEST_SAVE_IMAGE=test-${name}.gif"
            "-DTEST_KEEP_IMAGE=${CMAKE_CURRENT_BINARY_DIR}"
            "-DMAP_FILE=${COLORMAP_TEST_MAP}"
            "-DMAP_DIR=${CMAKE_SOURCE_DIR}/home/maps"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/colormap_compare_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
    set_tests_properties(ColormapTest.${name} PROPERTIES LABELS "colormap")
endfunction()

add_fractal_type_image_test(
    barnsleyj1 barnsleyj2 barnsleyj3 barnsleym1 barnsleym2 barnsleym3 burning-ship
    cellular circle cmplxmarksjul cmplxmarksmand complexbasin complexnewton
    dividebrot5
    escher_julia
    frothybasin
    gingerbreadman
    halley hypercomplex hypercomplexj
    julia julia4 julia_inverse julzpower julzzpwr
    lambda lambdafn lyapunov
    magnet1j magnet1m magnet2j magnet2m
    mandel mandel4 mandelbrotmix4 mandelcloud mandelfn mandellambda mandphoenix mandphoenixclx
    manowar manowarj manzpower manzzpwr marksjulia marksmandel marksmandelpwr
    newtbasin newton
    phoenix phoenixcplx
    quat quatjul
    sierpinski spider
    tetrate tim's_error
    unity
    volterra-lotka
)
add_random_fractal_type_image_test(
    biflambda
    diffusion dynamic
    henon
    kamtorus kamtorus3d
    plasma
)
add_legacy_image_test(fnz-plus-fnpixel PARAMETERS "type=fn(z)+fn(pix)")
add_legacy_image_test(fn-zsqr PARAMETERS "type=fn(z*z)")
add_legacy_image_test(fn-mul-fn PARAMETERS "type=fn*fn")
add_legacy_image_test(fn-mul-z-plus-z PARAMETERS "type=fn*z+z")
add_legacy_image_test(fn-plus-fn PARAMETERS "type=fn+fn")
add_legacy_image_test(julfn-plus-exp PARAMETERS "type=julfn+exp")
add_legacy_image_test(julfn-plus-zsqrd PARAMETERS "type=julfn+zsqrd")
add_legacy_image_test(julia-fn-or-fn PARAMETERS "type=julia(fn||fn)")
add_legacy_image_test(lambda-fn-or-fn PARAMETERS "type=lambda(fn||fn)")
add_legacy_image_test(mandel-fn-or-fn PARAMETERS "type=mandel(fn||fn)")
add_legacy_image_test(manfn-plus-exp PARAMETERS "type=manfn+exp")
add_legacy_image_test(manfn-plus-zsqrd PARAMETERS "type=manfn+zsqrd")
add_legacy_image_test(manlam-fn-or-fn PARAMETERS "type=manlam(fn||fn)")
add_legacy_image_test(sqr-1-over-fn PARAMETERS "type=sqr(1/fn)")
add_legacy_image_test(sqr-fn PARAMETERS "type=sqr(fn)")
add_legacy_image_test(btrace PARAMETERS "type=mandel" "inside=zmag" "passes=b")
add_legacy_image_test(mandel-perturb PARAMETERS "type=mandel" "passes=p")
add_legacy_image_test(manzpower-3-perturb PARAMETERS "type=manzpower" "params=0/0/3" "passes=p")
add_legacy_image_test(manzpower-4-perturb PARAMETERS "type=manzpower" "params=0/0/4" "passes=p")
add_legacy_image_test(fractint-formula PARAMETERS "type=formula" "formulafile=example2.frm" "formulaname=fractint")
foreach(bailout and imag manh manr or real)
    add_legacy_image_test(bailout-${bailout} PARAMETERS "type=mandel" "bailoutest=${bailout}")
endforeach()
add_legacy_image_test(ifs-dragon-1 PARAMETERS "type=ifs" "ifs=dragon" "params=1" "maxiter=500")
add_legacy_image_test(ant1 PARAMETERS "type=ant"
    "passes=1" "center-mag=0/0/1/1.3333" "params=10100.0/999999.0/25.0/1.0/1.0/-400.0" "inside=0" LABELS "ant")
add_legacy_image_test(ant2 PARAMETERS "type=ant"
    "passes=1" "center-mag=0/0/1/1.3333" "params=11111.0/999999.0/25.0/2.0/1.0/-400.0" "inside=0" LABELS "ant")

add_colormap_test(grey MAP grey.map PARAMETERS "type=mandel")

option(ID_RUN_SLOW_TESTS "Run slow image tests" OFF)
if(ID_RUN_SLOW_TESTS)
    add_image_test(mossy-garden PARAMETERS "@MossyFractalGarden.par/MossyFractalGarden" LABELS "slow")
endif()
