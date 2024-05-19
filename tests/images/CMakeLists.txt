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
    tetrate
    unity
    volterra-lotka
)
add_image_test(btrace PARAMETERS type=mandel inside=zmag passes=b)
add_image_test(fractint-formula PARAMETERS type=formula formulafile=example2.frm formulaname=fractint)
