# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
find_package(boost_endian CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
prefer_vcpkg_gif()
find_package(cpuid CONFIG REQUIRED)

set(ID_SPECIAL_DIRECTORIES_SOURCE io/SpecialDirectoriesLinux.cpp)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ID_SPECIAL_DIRECTORIES_SOURCE io/SpecialDirectoriesWin.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(ID_SPECIAL_DIRECTORIES_SOURCE io/SpecialDirectoriesMac.cpp)
endif()

add_library(id-core
    include/engine/bailout_formula.h engine/bailout_formula.cpp
    include/engine/Browse.h engine/Browse.cpp
    include/engine/BoundaryTrace.h engine/BoundaryTrace.cpp
    include/engine/calc_frac_init.h engine/calc_frac_init.cpp
    include/engine/calcfrac.h engine/calcfrac.cpp
    include/engine/calmanfp.h engine/calmanfp.cpp
    include/engine/cmdfiles.h engine/cmdfiles.cpp
    include/engine/command_comments.h engine/command_comments.cpp
    include/engine/color_state.h engine/color_state.cpp
    include/engine/color_utils.h
    include/engine/Command.h engine/Command.cpp
    include/engine/CommandHandler.h engine/CommandHandler.cpp
    include/engine/CommandProcessor.h engine/CommandProcessor.cpp
    include/engine/convert_center_mag.h engine/convert_center_mag.cpp
    include/engine/convert_corners.h engine/convert_corners.cpp
    include/engine/Diffusion.h engine/Diffusion.cpp
    include/engine/engine_timer.h engine/engine_timer.cpp
    include/engine/find_extra_param.h engine/find_extra_param.cpp
    include/engine/fractalb.h engine/fractalb.cpp
    include/engine/fractals.h engine/fractals.cpp
    include/engine/get_julia_attractor.h engine/get_julia_attractor.cpp
    include/engine/get_prec_big_float.h engine/get_prec_big_float.cpp
    include/engine/ImageRegion.h engine/ImageRegion.cpp
    include/engine/ImageRenderOperation.h engine/ImageRenderOperation.cpp
    include/engine/Inversion.h engine/Inversion.cpp
    include/engine/jiim.h engine/jiim.cpp
    include/engine/load_params.h engine/load_params.cpp
    include/engine/LogicalScreen.h engine/LogicalScreen.cpp
    include/engine/log_map.h engine/log_map.cpp
    include/engine/lowerize_parameter.h engine/lowerize_parameter.cpp
    include/engine/NullSoundOutput.h engine/NullSoundOutput.cpp
    include/engine/NonStandardFractal.h engine/NonStandardFractal.cpp
    include/engine/ScanlinePass.h engine/ScanlinePass.cpp
    include/engine/OrbitPlot.h engine/OrbitPlot.cpp
    include/engine/param_not_used.h engine/param_not_used.cpp
    include/engine/PertEngine.h engine/PertEngine.cpp
    engine/perturbation.cpp
    include/engine/pixel.h engine/pixel.cpp
    include/engine/pixel_grid.h engine/pixel_grid.cpp
    include/engine/pixel_limits.h
    include/engine/Point.h
    include/engine/Potential.h engine/Potential.cpp
    include/engine/random_seed.h engine/random_seed.cpp
    include/engine/reset_version.h engine/reset_version.cpp
    include/engine/resume.h engine/resume.cpp
    include/engine/show_dot.h engine/show_dot.cpp
    include/engine/SynchronousOrbit.h engine/SynchronousOrbit.cpp
    include/engine/SolidGuess.h engine/SolidGuess.cpp
    include/engine/sound.h engine/sound.cpp
    include/engine/SoundOutput.h engine/SoundOutput.cpp
    include/engine/spindac.h engine/spindac.cpp
    include/engine/StandardFractal.h engine/StandardFractal.cpp
    include/engine/StandardPass.h engine/StandardPass.cpp
    include/engine/StickyOrbits.h engine/StickyOrbits.cpp
    include/engine/tesseral.h engine/tesseral.cpp
    include/engine/text_color.h engine/text_color.cpp
    include/engine/trig_fns.h engine/trig_fns.cpp
    include/engine/type_has_param.h engine/type_has_param.cpp
    include/engine/UserData.h engine/UserData.cpp
    include/engine/video_mode.h engine/video_mode.cpp
    include/engine/VideoInfo.h engine/VideoInfo.cpp
    include/engine/Viewport.h engine/Viewport.cpp
    include/engine/work_list.h engine/work_list.cpp

    include/fractals/Ant.h fractals/Ant.cpp
    include/fractals/barnsley.h fractals/barnsley.cpp
    include/fractals/bif_may.h fractals/bif_may.cpp
    include/fractals/Bifurcation.h fractals/Bifurcation.cpp
    include/fractals/burning_ship.h fractals/burning_ship.cpp
    include/fractals/Cellular.h fractals/Cellular.cpp
    include/fractals/check_orbit_name.h fractals/check_orbit_name.cpp
    include/fractals/circle_pattern.h fractals/circle_pattern.cpp
    include/fractals/Diffusion.h fractals/Diffusion.cpp
    include/fractals/divide_brot.h fractals/divide_brot.cpp
    include/fractals/escher.h fractals/escher.cpp
    include/fractals/fn_or_fn.h fractals/fn_or_fn.cpp
    include/fractals/formula.h fractals/formula.cpp
    include/fractals/fractalp.h fractals/fractalp.cpp
    include/fractals/fractype.h fractals/fractype.cpp
    include/fractals/frasetup.h fractals/frasetup.cpp
    include/fractals/FrothyBasin.h fractals/FrothyBasin.cpp
    include/fractals/halley.h fractals/halley.cpp
    include/fractals/hypercomplex_mandelbrot.h fractals/hypercomplex_mandelbrot.cpp
    include/fractals/ifs.h fractals/ifs.cpp
    include/fractals/interpreter.h fractals/interpreter.cpp
    include/fractals/julibrot.h fractals/julibrot.cpp
    include/fractals/lambda_fn.h fractals/lambda_fn.cpp
    include/fractals/lorenz.h fractals/lorenz.cpp
    include/fractals/lsystem.h fractals/lsystem.cpp
    include/fractals/lyapunov.h fractals/lyapunov.cpp
    include/fractals/magnet.h fractals/magnet.cpp
    include/fractals/mandelbrot_mix.h fractals/mandelbrot_mix.cpp
    include/fractals/newton.h fractals/newton.cpp
    include/fractals/parser.h fractals/parser.cpp
    include/fractals/peterson_variations.h fractals/peterson_variations.cpp
    include/fractals/phoenix.h fractals/phoenix.cpp
    include/fractals/pickover_mandelbrot.h fractals/pickover_mandelbrot.cpp
    include/fractals/Plasma.h fractals/Plasma.cpp
    include/fractals/popcorn.h fractals/popcorn.cpp
    include/fractals/population.h fractals/population.cpp
    include/fractals/quartic_mandelbrot.h fractals/quartic_mandelbrot.cpp
    include/fractals/quaternion_mandelbrot.h fractals/quaternion_mandelbrot.cpp
    include/fractals/sierpinski_gasket.h fractals/sierpinski_gasket.cpp
    include/fractals/taylor_skinner_variations.h fractals/taylor_skinner_variations.cpp
    include/fractals/TestPoint.h fractals/TestPoint.cpp
    include/fractals/unity.h fractals/unity.cpp
    include/fractals/volterra_lotka.h fractals/volterra_lotka.cpp

    include/geometry/3d.h geometry/3d.cpp
    include/geometry/line3d.h geometry/line3d.cpp
    include/geometry/plot3d.h geometry/plot3d.cpp
    include/geometry/stereo_params.h geometry/stereo_params.cpp
    include/geometry/StereoView.h geometry/StereoView.cpp

    include/io/check_write_file.h io/check_write_file.cpp
    include/io/CurrentPathSaver.h
    include/io/decoder.h io/decoder.cpp
    include/io/disk_storage.h io/disk_storage.cpp
    include/io/dir_file.h
    include/io/encoder.h io/encoder.cpp
    include/io/ImageSave.h io/ImageSave.cpp
    include/io/ends_with_slash.h io/ends_with_slash.cpp
    include/io/EvolutionInfo.h io/EvolutionInfo.cpp
    include/io/get_disk_space.h io/get_disk_space.cpp
    include/io/file_gets.h io/file_gets.cpp
    include/io/file_item.h io/file_item.cpp
    include/io/find_file.h io/find_file.cpp
    include/io/find_path.h io/find_path.cpp
    include/io/GifRestore.h io/GifRestore.cpp
    include/io/GifWriter.h io/GifWriter.cpp
    include/io/gifview.h io/gifview.cpp
    include/io/gif_extensions.h
    include/io/gif_file.h io/gif_file.cpp
    include/io/has_ext.h
    include/io/is_writeable.h
    include/io/library.h io/library.cpp
    include/io/loadfile.h io/loadfile.cpp
    include/io/loadmap.h io/loadmap.cpp
    include/io/load_config.h io/load_config.cpp
    include/io/load_entry_text.h io/load_entry_text.cpp
    include/io/make_mig.h io/make_mig.cpp
    include/io/path_match.h io/path_match.cpp
    include/io/RestoredImageOutput.h io/RestoredImageOutput.cpp
    include/io/save_timer.h io/save_timer.cpp
    include/io/search_path.h io/search_path.cpp
    include/io/special_dirs.h io/special_dirs.cpp
    ${ID_SPECIAL_DIRECTORIES_SOURCE}
    include/io/trim_filename.h io/trim_filename.cpp
    include/io/update_save_name.h io/update_save_name.cpp

    include/math/arg.h math/arg.cpp
    include/math/big.h
    math/bigflt.cpp
    include/math/biginit.h math/biginit.cpp
    math/bignum.cpp
    math/bignumc.cpp
    include/math/cmplx.h math/cmplx.cpp
    include/math/complex_fn.h
    include/math/fixed_pt.h math/fixed_pt.cpp
    include/math/fpu087.h math/fpu087.cpp
    include/math/hcmplx.h math/hcmplx.cpp
    include/math/Point.h
    include/math/round_float_double.h math/round_float_double.cpp
    include/math/sign.h
    include/math/sqr.h

    include/misc/debug_flags.h misc/debug_flags.cpp
    include/misc/Driver.h misc/Driver.cpp
    include/misc/id.h
    include/misc/memory.h misc/memory.cpp
    include/misc/run_cleanup.h
    include/misc/sized_types.h
    include/misc/stack_avail.h misc/stack_avail.cpp
    include/misc/ValueSaver.h
    include/misc/version.h misc/version.cpp

    "doc/EngineGlobals.md"
    "doc/FractalGlobals.md"
    "doc/GeometryGlobals.md"
    "doc/GlobalsIndex.md"
    "doc/inversion-of-control.md"
    "doc/IOGlobals.md"
    "doc/MathGlobals.md"
    "doc/MiscGlobals.md"
    "doc/NetworkArchitecture.md"
    "doc/parser-bignum.md"
    "doc/simd-highway.md"
    "doc/UIGlobals.md"
)
add_library(id::core ALIAS id-core)
source_group("Docs" REGULAR_EXPRESSION ".*\\.md$")
source_group_subdir("Engine" "engine")
source_group_subdir("Fractal Specific" "fractals")
source_group_subdir("Geometry" "geometry")
source_group_subdir("IO" "io")
source_group_subdir("Math" "math")
source_group_subdir("Miscellaneous" "misc")

target_compile_definitions(id-core PUBLIC
    ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(id-core PUBLIC ${ID_TARGET_OPTIONS})
target_include_directories(id-core PUBLIC include)
prefer_vcpkg_gif_includes(id-core)
target_link_libraries(id-core PRIVATE fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(id-core PRIVATE shell32.lib)
endif()
target_link_libraries(id-core PUBLIC help-defs config algos Boost::endian GIF::GIF cpuid::cpuid)
target_folder(id-core "Libraries")
add_dependencies(id-core native-help)
