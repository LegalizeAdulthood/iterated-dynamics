# SPDX-License-Identifier: GPL-3.0-only
#
# Until the unix port is complete, we can't run end-to-end image tests there
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

include(IdTestHome)

function(add_image_test name)
    cmake_parse_arguments(IMAGE_TEST "DISABLED;IGNORE_COLORMAP" "WORKING_DIRECTORY" "PARAMETERS;LABELS" ${ARGN})
    if(IMAGE_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${IMAGE_TEST_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT IMAGE_TEST_WORKING_DIRECTORY)
        set(IMAGE_TEST_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home")
    endif()
    set(IMAGE_TEST_IGNORE_COLORMAP_VALUE OFF)
    if(IMAGE_TEST_IGNORE_COLORMAP)
        set(IMAGE_TEST_IGNORE_COLORMAP_VALUE ON)
        list(APPEND IMAGE_TEST_LABELS "ignore-colormap")
    endif()
    list(APPEND IMAGE_TEST_PARAMETERS "batch=yes" "video=F6")
    add_test(NAME ImageTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
            "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-${name}.gif"
            "-DPARAMETERS=${IMAGE_TEST_PARAMETERS}"
            "-DTEST_SAVE_IMAGE=test-${name}.gif"
            "-DTEST_KEEP_IMAGE=${CMAKE_CURRENT_BINARY_DIR}"
            "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/${name}-diff.gif"
            "-DIMAGE_TEST_IGNORE_COLORMAP=${IMAGE_TEST_IGNORE_COLORMAP_VALUE}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/image_compare_test.cmake"
        WORKING_DIRECTORY "${IMAGE_TEST_WORKING_DIRECTORY}"
    )
    list(APPEND IMAGE_TEST_LABELS "image")
    set_tests_properties(ImageTest.${name} PROPERTIES LABELS "${IMAGE_TEST_LABELS}")
    if(IMAGE_TEST_DISABLED)
        set_tests_properties(ImageTest.${name} PROPERTIES DISABLED TRUE)
    endif()
endfunction()

function(add_fractal_type_image_test)
    foreach(type ${ARGN})
        add_image_test(${type} PARAMETERS "type=${type}")
    endforeach()
endfunction()

function(add_random_fractal_type_image_test)
    foreach(type ${ARGN})
        add_image_test(${type} PARAMETERS "rseed=4567" "type=${type}")
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

function(add_image_dump_test name)
    add_test(NAME ImageDumpTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DIMAGE_DUMP=$<SHELL_PATH:$<TARGET_FILE:image-dump>>"
            "-DINPUT_IMAGE=gold-${name}.gif"
            "-DGOLD_DUMP=${CMAKE_CURRENT_SOURCE_DIR}/gold-${name}-dump.txt"
            "-DTEST_DUMP=${CMAKE_CURRENT_BINARY_DIR}/test-${name}-dump.txt"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/image_dump_test.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    set_tests_properties(ImageDumpTest.${name} PROPERTIES LABELS "image-dump")
endfunction()

function(add_potential_image_test name)
    cmake_parse_arguments(POTENTIAL_TEST "" "" "PARAMETERS" ${ARGN})
    if(POTENTIAL_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${POTENTIAL_TEST_UNPARSED_ARGUMENTS}")
    endif()
    list(APPEND POTENTIAL_TEST_PARAMETERS "batch=yes" "video=F6")
    add_test(NAME ImageTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
            "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-${name}.pot"
            "-DPARAMETERS=${POTENTIAL_TEST_PARAMETERS}"
            "-DTEST_SAVE_IMAGE=test-${name}.pot"
            "-DTEST_KEEP_IMAGE=${CMAKE_CURRENT_BINARY_DIR}"
            "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/${name}-diff.gif"
            "-DIMAGE_TEST_IGNORE_COLORMAP=OFF"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/image_compare_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
    set_tests_properties(ImageTest.${name} PROPERTIES LABELS "image;potential")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gold-${name}.pot")
        set_tests_properties(ImageTest.${name}
            PROPERTIES DISABLED TRUE)
    endif()
endfunction()

function(add_make_mig_test)
    add_test(NAME ImageTest.make-mig
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
            "-DCOMPONENT_IMAGE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-make_mig.gif"
            "-DTEST_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/test-make_mig.gif"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/make_mig_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
    set_tests_properties(ImageTest.make-mig PROPERTIES LABELS "image;mig")
endfunction()

function(add_passes_test name passes)
    cmake_parse_arguments(PASSES_TEST "IGNORE_COLORMAP" "" "PARAMETERS" ${ARGN})
    if(PASSES_TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${PASSES_TEST_UNPARSED_ARGUMENTS}")
    endif()
    if(PASSES_TEST_IGNORE_COLORMAP)
        set(PASSES_TEST_IGNORE_COLORMAP_ARG IGNORE_COLORMAP)
    endif()
    list(APPEND PASSES_TEST_PARAMETERS "type=mandel" "passes=${passes}")
    add_image_test("passes-${name}"
        ${PASSES_TEST_IGNORE_COLORMAP_ARG} PARAMETERS ${PASSES_TEST_PARAMETERS} LABELS "passes")
endfunction()

foreach(type "bifurcation" "biflambda" "bifmay" "bifstewart")
    add_image_test(${type} PARAMETERS "type=${type}" LABELS "bifurcation")
endforeach()

add_fractal_type_image_test(
    "barnsleyj1" "barnsleyj2" "barnsleyj3" "barnsleym1" "barnsleym2" "barnsleym3" "burning-ship"
    "circle" "cmplxmarksjul" "cmplxmarksmand" "complexbasin" "complexnewton"
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
add_random_fractal_type_image_test(
    "diffusion" "dynamic"
    "henon"
    "kamtorus" "kamtorus3d"
    "plasma"
)
add_image_test("cellular" IGNORE_COLORMAP PARAMETERS "type=cellular")
add_image_test("bif-plus-sinpi" PARAMETERS "type=bif+sinpi")
add_image_test("bif-equal-sinpi" PARAMETERS "type=bif=sinpi")
add_image_test("fnz-plus-fnpixel" PARAMETERS "type=fn(z)+fn(pix)")
add_image_test("fn-zsqr" PARAMETERS "type=fn(z*z)")
add_image_test("fn-mul-fn" PARAMETERS "type=fn*fn")
add_image_test("fn-mul-z-plus-z" PARAMETERS "type=fn*z+z")
add_image_test("fn-plus-fn" PARAMETERS "type=fn+fn")
add_image_test("julfn-plus-exp" PARAMETERS "type=julfn+exp")
add_image_test("julfn-plus-zsqrd" PARAMETERS "type=julfn+zsqrd")
add_image_test("julia-fn-or-fn" PARAMETERS "type=julia(fn||fn)")
add_image_test("lambda-fn-or-fn" PARAMETERS "type=lambda(fn||fn)")
add_image_test("mandel-fn-or-fn" PARAMETERS "type=mandel(fn||fn)")
add_image_test("manfn-plus-exp" PARAMETERS "type=manfn+exp")
add_image_test("manfn-plus-zsqrd" PARAMETERS "type=manfn+zsqrd")
add_image_test("manlam-fn-or-fn" PARAMETERS "type=manlam(fn||fn)")
add_image_test("sqr-1-over-fn" PARAMETERS "type=sqr(1/fn)")
add_image_test("sqr-fn" PARAMETERS "type=sqr(fn)")
add_passes_test("one" "1")
add_passes_test("two" "2")
add_passes_test("three" "3")
add_passes_test("boundary-trace" "b" PARAMETERS "inside=zmag")
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
add_image_test("mandel-perturb" PARAMETERS "type=mandel" "passes=p")
add_image_test("manzpower-3-perturb" PARAMETERS "type=manzpower" "params=0/0/3" "passes=p")
add_image_test("manzpower-4-perturb" PARAMETERS "type=manzpower" "params=0/0/4" "passes=p")
add_image_test("fractint-formula" PARAMETERS "type=formula" "formulafile=example2.frm" "formulaname=fractint")
foreach(bailout "and" "imag" "manh" "manr" "or" "real")
    add_image_test(bailout-${bailout} PARAMETERS "type=mandel" "bailoutest=${bailout}")
endforeach()
add_image_test("ifs-dragon-1" PARAMETERS "type=ifs" "ifs=dragon" "params=1" "maxiter=500")
add_image_test("ifs-3dfern" PARAMETERS "type=ifs3d" "ifsfile=id.ifs" "ifs=3dfern"
    "center-mag=1.77636e-15/-1.77636e-15/0.3725782/1.3333" "params=1"
    "maxiter=500" "inside=0" "rotation=60/30/0" "perspective=0" "xyshift=0/0")
add_image_test("ant1" PARAMETERS "type=ant"
    "center-mag=0/0/1/1.3333" "params=10100000000000000000000/999999.0/25.0/1.0/1.0/-400.0" "inside=0" LABELS "ant")
add_image_test("ant2" PARAMETERS "type=ant"
    "center-mag=0/0/1/1.3333" "params=11111000000000000000000/999999.0/25.0/2.0/1.0/-400.0" "inside=0" LABELS "ant")
add_image_test("mandel-chroma" PARAMETERS "type=mandel" "map=chroma")
add_image_test("Egg" PARAMETERS "@id.par/Egg")
add_image_test("EvilFrog" PARAMETERS "@id.par/EvilFrog")
id_make_test_home(Land_of_Loti LAND_OF_LOTI_HOME
    PARS "${CMAKE_CURRENT_SOURCE_DIR}/Land_of_Loti.par")
add_image_test("Land_of_Loti"
    PARAMETERS "@Land_of_Loti.par/Land_of_Loti"
    WORKING_DIRECTORY "${LAND_OF_LOTI_HOME}")
add_image_test("mandala" PARAMETERS "@id.par/mandala")
add_image_test("filament" PARAMETERS "@id")
add_image_test("SWT-029" PARAMETERS "@id.par/SWT-029")
add_image_test("triangles5" PARAMETERS "@cellular.par/triangles5")
add_image_test("x-mas-tree" PARAMETERS "@cellular.par/X-mas_tree")
add_image_test("Achute" PARAMETERS "@example2.par/Achute")
add_image_test("Ant" PARAMETERS "@example2.par/Ant")
add_image_test("Barnsleyj2-manh" PARAMETERS "@example2.par/Barnsleyj2-manh")
add_image_test("Barnsleyj2-manr" PARAMETERS "@example2.par/Barnsleyj2-manr")
add_image_test("lyapunov-Blob1" PARAMETERS "@lyapunov/Blob1")
add_image_test("MandelTile" PARAMETERS "@example2.par/TileMandel")
add_image_test("julibrot" PARAMETERS "type=julibrot" "julibrotfromto=-0.83/-0.83/0.25/-0.25"
    "julibroteyes=2.5" "center-mag=0/0/0.6666667" "params=0/0" "inside=0"
    "colors=@altern.map")
add_image_test("mandel-orbit-line" PARAMETERS "type=mandel" "passes=o" "orbitdrawmode=l" "maxiter=999")
add_image_test("plasma-new" PARAMETERS "rseed=4567" "type=plasma" "params=1/1/1/0")
add_image_test("testpt" PARAMETERS "type=test")
add_image_test("decomp" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=0" "decomp=16")
add_image_test("biomorph" IGNORE_COLORMAP PARAMETERS "type=mandel" "center-mag=-0.500002/2.205e-006/0.6666676"
    "params=0/0" "inside=0" "biomorph=1" "cyclerange=1/15")
# TODO: center-mag parameter fails
add_image_test("skew" PARAMETERS "type=mandel"
    "center-mag=-0.645182/1e-007/0.9363297/1/0/-14.9314098822673937" "params=0/0" "inside=0")
# TODO: center-mag parameter fails
add_image_test("rotate" PARAMETERS "type=mandel"
    "center-mag=-0.5/7.45e-008/0.8503403/1/-14.999994501248608/-1.34634920815265846e-005" "params=0/0" "inside=0"
    "cyclerange=1/15")
add_image_test("invert" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=0" "invert=0.500000008333333/-0.500000150000002/0")
add_image_test("inside-1" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=1")
add_image_test("inside-maxiter" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=maxiter")
add_image_test("inside-bof60" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=bof60")
add_image_test("inside-bof61" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=bof61")
add_image_test("inside-zmag" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=zmag")
add_image_test("inside-epsiloncross" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=epsiloncross")
add_image_test("inside-startrail" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=startrail")
add_image_test("inside-period" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=period")
add_image_test("inside-atan" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=atan" "periodicity=0")
add_image_test("inside-fmod" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "inside=fmod")
add_image_test("outside-1" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "outside=1")
add_image_test("outside-real" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "outside=real")
add_image_test("outside-imag" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "outside=imag")
add_image_test("outside-summ" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "outside=summ")
add_image_test("outside-mult" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "outside=mult")
add_image_test("outside-atan" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "outside=atan")
# SKIPPED Due to the nature of the algorithm, the pixel variance is too high ~13%
#add_image_test("outside-fmod" DISABLED PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
#    "params=0/0" "outside=fmod" "cyclerange=1/1")
add_image_test("outside-tdis" PARAMETERS "type=mandel" "center-mag=-0.5/-2.5e-008/0.6666667"
    "params=0/0" "outside=tdis")

function(add_par_test name)
    id_make_test_home(${name} PAR_TEST_HOME
        PARS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.par")
    add_image_test("${name}"
        PARAMETERS "@${name}.par/${name}"
        WORKING_DIRECTORY "${PAR_TEST_HOME}"
        LABELS par)
endfunction()

foreach(par Killer_Mosquito Broken_Arches Waves_in_the_Night Cosmic_Laser_Beams)
    add_par_test(${par})
endforeach()

add_colormap_test("grey" MAP "grey.map" PARAMETERS "type=mandel")
add_image_dump_test("halley")
add_potential_image_test("3dlook01" PARAMETERS "@radar/3dlook01")
add_make_mig_test()

option(ID_RUN_SLOW_TESTS "Run slow image tests" OFF)
if(ID_RUN_SLOW_TESTS)
    add_image_test("mossy-garden" PARAMETERS "@MossyFractalGarden.par/MossyFractalGarden" LABELS "slow")
    add_image_test("orbits-demonic_moose" PARAMETERS "@orbits/demonic_moose" LABELS "slow")
endif()
