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

function(add_fractal_type_image_test)
    foreach(type ${ARGN})
        add_image_test(${type} PARAMETERS "type=${type}")
    endforeach()
endfunction()

function(add_fractal_type_legacy_image_test)
    foreach(type ${ARGN})
        add_image_test(${type} IGNORE_COLORMAP PARAMETERS "type=${type}")
    endforeach()
endfunction()

function(add_random_fractal_type_legacy_image_test)
    foreach(type ${ARGN})
        add_image_test(${type} IGNORE_COLORMAP PARAMETERS "rseed=4567" "type=${type}")
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
            "-DMAP_DIR=${CMAKE_SOURCE_DIR}/home/map"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/colormap_compare_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
    set_tests_properties(ColormapTest.${name} PROPERTIES LABELS "colormap")
endfunction()

function(add_passes_test name passes)
    cmake_parse_arguments(PASSES_TEST "IGNORE_COLORMAP" "" "PARAMETERS" ${ARGN})
    if(PASSES_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${PASSES_TEST_UNPARSED_ARGUMENTS}")
    endif()
    list(APPEND PASSES_TEST_PARAMETERS "type=mandel" "passes=${passes}")
    if(PASSES_TEST_IGNORE_COLORMAP)
        set(PASSES_TEST_IGNORE_COLORMAP "IGNORE_COLORMAP")
    else()
        set(PASSES_TEST_IGNORE_COLORMAP "")
    endif()
    add_image_test("passes-${name}" ${PASSES_TEST_IGNORE_COLORMAP}
        PARAMETERS ${PASSES_TEST_PARAMETERS} LABELS "passes")
endfunction()

foreach(type "bifurcation" "biflambda")
    add_image_test(${type} IGNORE_COLORMAP PARAMETERS "type=${type}" LABELS "bifurcation")
endforeach()
foreach(type "bifmay" "bifstewart")
    add_image_test(${type} PARAMETERS "type=${type}" LABELS "bifurcation")
endforeach()

add_fractal_type_legacy_image_test(
    "barnsleyj1" "barnsleyj2" "barnsleyj3" "barnsleym1" "barnsleym2" "barnsleym3" "burning-ship"
    "cellular" "circle" "cmplxmarksjul" "cmplxmarksmand" "complexbasin" "complexnewton"
    "dividebrot5"
    "escher_julia"
    "frothybasin"
    "gingerbreadman"
    "halley" "hypercomplex" "hypercomplexj"
    "julia" "julia4" "julia_inverse" "julzpower" "julzzpwr"
    "lambda" "lambdafn" "lyapunov"
    "magnet1j" "magnet1m" "magnet2j" "magnet2m"
    "mandel" "mandel4" "mandelbrotmix4" "mandelcloud" "mandelfn" "mandellambda" "mandphoenix" "mandphoenixclx"
    "manowar" "manowarj" "manzpower" "manzzpwr" "marksjulia" "marksmandel" "marksmandelpwr"
    "newtbasin" "newton"
    "phoenix" "phoenixcplx"
    "quat" "quatjul"
    "sierpinski" "spider"
    "tetrate" "tim's_error"
    "unity"
    "volterra-lotka"
)
add_random_fractal_type_legacy_image_test(
    "diffusion" "dynamic"
    "henon"
    "kamtorus" "kamtorus3d"
    "plasma"
)
add_image_test("bif-plus-sinpi" PARAMETERS "type=bif+sinpi")
add_image_test("bif-equal-sinpi" PARAMETERS "type=bif=sinpi")
add_image_test("fnz-plus-fnpixel" IGNORE_COLORMAP PARAMETERS "type=fn(z)+fn(pix)")
add_image_test("fn-zsqr" IGNORE_COLORMAP PARAMETERS "type=fn(z*z)")
add_image_test("fn-mul-fn" IGNORE_COLORMAP PARAMETERS "type=fn*fn")
add_image_test("fn-mul-z-plus-z" IGNORE_COLORMAP PARAMETERS "type=fn*z+z")
add_image_test("fn-plus-fn" IGNORE_COLORMAP PARAMETERS "type=fn+fn")
add_image_test("julfn-plus-exp" IGNORE_COLORMAP PARAMETERS "type=julfn+exp")
add_image_test("julfn-plus-zsqrd" IGNORE_COLORMAP PARAMETERS "type=julfn+zsqrd")
add_image_test("julia-fn-or-fn" IGNORE_COLORMAP PARAMETERS "type=julia(fn||fn)")
add_image_test("lambda-fn-or-fn" IGNORE_COLORMAP PARAMETERS "type=lambda(fn||fn)")
add_image_test("mandel-fn-or-fn" IGNORE_COLORMAP PARAMETERS "type=mandel(fn||fn)")
add_image_test("manfn-plus-exp" IGNORE_COLORMAP PARAMETERS "type=manfn+exp")
add_image_test("manfn-plus-zsqrd" IGNORE_COLORMAP PARAMETERS "type=manfn+zsqrd")
add_image_test("manlam-fn-or-fn" IGNORE_COLORMAP PARAMETERS "type=manlam(fn||fn)")
add_image_test("sqr-1-over-fn" IGNORE_COLORMAP PARAMETERS "type=sqr(1/fn)")
add_image_test("sqr-fn" IGNORE_COLORMAP PARAMETERS "type=sqr(fn)")
add_passes_test("one" "1")
add_passes_test("two" "2")
add_passes_test("three" "3")
add_passes_test("boundary-trace" "b" IGNORE_COLORMAP PARAMETERS "inside=zmag")
add_passes_test("diffusion" "d")
add_passes_test("guess" "g")
add_passes_test("guess-one" "g1")
add_passes_test("guess-two" "g2")
add_passes_test("guess-three" "g3")
add_passes_test("guess-four" "g4")
add_passes_test("guess-five" "g5")
add_passes_test("guess-six" "g6")
add_passes_test("orbit" "o")
add_passes_test("synchronous-orbits" "s")
add_passes_test("tesseral" "t")
add_image_test("mandel-perturb" IGNORE_COLORMAP PARAMETERS "type=mandel" "passes=p")
add_image_test("manzpower-3-perturb" IGNORE_COLORMAP PARAMETERS "type=manzpower" "params=0/0/3" "passes=p")
add_image_test("manzpower-4-perturb" IGNORE_COLORMAP PARAMETERS "type=manzpower" "params=0/0/4" "passes=p")
add_image_test("fractint-formula" IGNORE_COLORMAP PARAMETERS "type=formula" "formulafile=example2.frm" "formulaname=fractint")
foreach(bailout "and" "imag" "manh" "manr" "or" "real")
    add_image_test(bailout-${bailout} IGNORE_COLORMAP PARAMETERS "type=mandel" "bailoutest=${bailout}")
endforeach()
add_image_test("ifs-dragon-1" IGNORE_COLORMAP PARAMETERS "type=ifs" "ifs=dragon" "params=1" "maxiter=500")
add_image_test("ant1" IGNORE_COLORMAP PARAMETERS "type=ant"
    "center-mag=0/0/1/1.3333" "params=10100.0/999999.0/25.0/1.0/1.0/-400.0" "inside=0" LABELS "ant")
add_image_test("ant2" IGNORE_COLORMAP PARAMETERS "type=ant"
    "center-mag=0/0/1/1.3333" "params=11111.0/999999.0/25.0/2.0/1.0/-400.0" "inside=0" LABELS "ant")

add_colormap_test("grey" MAP "grey.map" PARAMETERS "type=mandel")

option(ID_RUN_SLOW_TESTS "Run slow image tests" OFF)
if(ID_RUN_SLOW_TESTS)
    add_image_test(mossy-garden PARAMETERS "@MossyFractalGarden.par/MossyFractalGarden" LABELS "slow")
endif()
