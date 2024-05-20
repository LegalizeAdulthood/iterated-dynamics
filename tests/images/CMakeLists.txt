# Until the unix port is complete, we can't run end-to-end image tests there
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

function(add_image_test name)
    cmake_parse_arguments(IMAGE_TEST "" "" "PARAMETERS" ${ARGN})
    list(APPEND IMAGE_TEST_PARAMETERS "batch=yes" "video=F6")
    add_test(NAME ImageTest.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DID=$<SHELL_PATH:$<TARGET_FILE:id>>"
            "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
            "-DGOLD_IMAGE=${CMAKE_CURRENT_SOURCE_DIR}/gold-${name}.gif"
            "-DPARAMETERS=${IMAGE_TEST_PARAMETERS}"
            "-DTEST_SAVE_IMAGE=test-${name}.gif"
            "-DTEST_KEEP_IMAGE=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/image_compare_test.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/home"
    )
    set_tests_properties(ImageTest.${name} PROPERTIES LABELS "image")
endfunction()

function(add_fractal_type_image_test)
    foreach(type ${ARGN})
        add_image_test(${type} PARAMETERS type=${type})
    endforeach()
endfunction()

function(add_random_fractal_type_image_test)
    foreach(type ${ARGN})
        add_image_test(${type} PARAMETERS rseed=4567 type=${type})
    endforeach()
endfunction()

add_fractal_type_image_test(
    barnsleyj1 barnsleyj2 barnsleyj3 barnsleym1 barnsleym2 barnsleym3
    cellular circle cmplxmarksjul cmplxmarksmand complexbasin complexnewton
    dividebrot5
    escher_julia
    gingerbreadman
    halley hypercomplex hypercomplexj
    julia julia4 julia_inverse julzpower julzzpwr
    lambda lambdafn lyapunov
    magnet1j magnet1m magnet2j magnet2m
    mandel mandel4 mandelcloud mandelfn mandellambda mandphoenix mandphoenixclx
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
add_image_test(fnz-plus-fnpixel PARAMETERS "type=fn(z)+fn(pix)")
add_image_test(fn-zsqr PARAMETERS "type=fn(z*z)")
add_image_test(fn-mul-fn PARAMETERS "type=fn*fn")
add_image_test(fn-mul-z-plus-z PARAMETERS "type=fn*z+z")
add_image_test(fn-plus-fn PARAMETERS "type=fn+fn")
add_image_test(julfn-plus-exp PARAMETERS "type=julfn+exp")
add_image_test(julfn-plus-zsqrd PARAMETERS "type=julfn+zsqrd")
add_image_test(julia-fn-or-fn PARAMETERS "type=julia(fn||fn)")
add_image_test(lambda-fn-or-fn PARAMETERS "type=lambda(fn||fn)")
add_image_test(mandel-fn-or-fn PARAMETERS "type=mandel(fn||fn)")
add_image_test(manfn-plus-exp PARAMETERS "type=manfn+exp")
add_image_test(manfn-plus-zsqrd PARAMETERS "type=manfn+zsqrd")
add_image_test(manlam-fn-or-fn PARAMETERS "type=manlam(fn||fn)")
add_image_test(sqr-1-over-fn PARAMETERS "type=sqr(1/fn)")
add_image_test(sqr-fn PARAMETERS "type=sqr(fn)")
add_image_test(btrace PARAMETERS type=mandel inside=zmag passes=b)
add_image_test(fractint-formula PARAMETERS type=formula formulafile=example2.frm formulaname=fractint)
