# SPDX-License-Identifier: GPL-3.0-only
#
find_package(boost_algorithm CONFIG REQUIRED)
find_package(boost_endian CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(GIF REQUIRED)
find_package(cpuid CONFIG REQUIRED)

add_library(libid
    include/engine/bailout_formula.h engine/bailout_formula.cpp
    include/engine/Browse.h engine/Browse.cpp
    include/engine/boundary_trace.h engine/boundary_trace.cpp
    include/engine/calc_frac_init.h engine/calc_frac_init.cpp
    include/engine/calcfrac.h engine/calcfrac.cpp
    include/engine/calmanfp.h engine/calmanfp.cpp
    include/engine/cmdfiles.h engine/cmdfiles.cpp
    include/engine/cmdfiles_test.h
    include/engine/color_state.h engine/color_state.cpp
    include/engine/convert_center_mag.h engine/convert_center_mag.cpp
    include/engine/convert_corners.h engine/convert_corners.cpp
    include/engine/diffusion_scan.h engine/diffusion_scan.cpp
    include/engine/engine_timer.h engine/engine_timer.cpp
    include/engine/find_extra_param.h engine/find_extra_param.cpp
    include/engine/fractalb.h engine/fractalb.cpp
    include/engine/fractals.h engine/fractals.cpp
    include/engine/get_julia_attractor.h engine/get_julia_attractor.cpp
    include/engine/get_prec_big_float.h engine/get_prec_big_float.cpp
    include/engine/ImageRegion.h engine/ImageRegion.cpp
    include/engine/Inversion.h engine/Inversion.cpp
    include/engine/jiim.h engine/jiim.cpp
    include/engine/load_params.h engine/load_params.cpp
    include/engine/LogicalScreen.h engine/LogicalScreen.cpp
    include/engine/log_map.h engine/log_map.cpp
    include/engine/lowerize_parameter.h engine/lowerize_parameter.cpp
    include/engine/one_or_two_pass.h engine/one_or_two_pass.cpp
    include/engine/orbit.h engine/orbit.cpp
    include/engine/param_not_used.h engine/param_not_used.cpp
    include/engine/PertEngine.h engine/PertEngine.cpp
    include/engine/perturbation.h engine/perturbation.cpp
    include/engine/pixel_grid.h engine/pixel_grid.cpp
    include/engine/pixel_limits.h
    include/engine/Point.h
    include/engine/Potential.h engine/Potential.cpp
    include/engine/random_seed.h engine/random_seed.cpp
    include/engine/resume.h engine/resume.cpp
    include/engine/show_dot.h engine/show_dot.cpp
    include/engine/soi.h engine/soi.cpp
    include/engine/solid_guess.h engine/solid_guess.cpp
    include/engine/sound.h engine/sound.cpp
    include/engine/spindac.h engine/spindac.cpp
    include/engine/sticky_orbits.h engine/sticky_orbits.cpp
    include/engine/tesseral.h engine/tesseral.cpp
    include/engine/text_color.h engine/text_color.cpp
    include/engine/trig_fns.h engine/trig_fns.cpp
    include/engine/type_has_param.h engine/type_has_param.cpp
    include/engine/UserData.h engine/UserData.cpp
    include/engine/video_mode.h engine/video_mode.cpp
    include/engine/VideoInfo.h engine/VideoInfo.cpp
    include/engine/Viewport.h engine/Viewport.cpp
    include/engine/wait_until.h engine/wait_until.cpp
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
    include/fractals/fractalp.h fractals/fractalp.cpp
    include/fractals/fractype.h fractals/fractype.cpp
    include/fractals/frasetup.h fractals/frasetup.cpp
    include/fractals/FrothyBasin.h fractals/FrothyBasin.cpp
    include/fractals/halley.h fractals/halley.cpp
    include/fractals/hypercomplex_mandelbrot.h fractals/hypercomplex_mandelbrot.cpp
    include/fractals/ifs.h fractals/ifs.cpp
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

    include/io/check_write_file.h io/check_write_file.cpp
    include/io/CurrentPathSaver.h
    include/io/decoder.h io/decoder.cpp
    include/io/decode_info.h io/decode_info.cpp
    include/io/dir_file.h
    include/io/encoder.h io/encoder.cpp
    include/io/ends_with_slash.h io/ends_with_slash.cpp
    include/io/expand_dirname.h io/expand_dirname.cpp
    include/io/get_disk_space.h io/get_disk_space.cpp
    include/io/file_gets.h io/file_gets.cpp
    include/io/file_item.h io/file_item.cpp
    include/io/find_file.h io/find_file.cpp
    include/io/find_path.h io/find_path.cpp
    include/io/fix_dirname.h io/fix_dirname.cpp
    include/io/gifview.h io/gifview.cpp
    include/io/gif_extensions.h
    include/io/gif_file.h io/gif_file.cpp
    include/io/has_ext.h
    include/io/is_directory.h
    include/io/is_writeable.h
    include/io/library.h io/library.cpp
    include/io/loadfile.h io/loadfile.cpp
    include/io/loadmap.h io/loadmap.cpp
    include/io/load_config.h io/load_config.cpp
    include/io/load_entry_text.h io/load_entry_text.cpp
    include/io/locate_input_file.h io/locate_input_file.cpp
    include/io/make_mig.h io/make_mig.cpp
    include/io/make_path.h io/make_path.cpp
    include/io/merge_path_names.h io/merge_path_names.cpp
    include/io/path_match.h io/path_match.cpp
    include/io/save_timer.h io/save_timer.cpp
    include/io/search_path.h io/search_path.cpp
    include/io/special_dirs.h io/special_dirs.cpp
    include/io/split_path.h io/split_path.cpp
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
    include/math/rand15.h
    include/math/round_float_double.h math/round_float_double.cpp
    include/math/sign.h
    include/math/sqr.h

    include/misc/debug_flags.h misc/debug_flags.cpp
    include/misc/Driver.h misc/Driver.cpp
    include/misc/id.h
    include/misc/memory.h misc/memory.cpp
    include/misc/sized_types.h
    include/misc/stack_avail.h misc/stack_avail.cpp
    include/misc/ValueSaver.h
    include/misc/version.h misc/version.cpp

    include/ui/ant.h ui/ant.cpp
    include/ui/bifurcation.h ui/bifurcation.cpp
    include/ui/big_while_loop.h ui/big_while_loop.cpp
    include/ui/cellular.h ui/cellular.cpp
    include/ui/check_key.h ui/check_key.cpp
    include/ui/ChoiceBuilder.h
    include/ui/comments.h ui/comments.cpp
    include/ui/diffusion.h ui/diffusion.cpp
    include/ui/diskvid.h ui/diskvid.cpp
    include/ui/double_to_string.h ui/double_to_string.cpp
    include/ui/do_pause.h ui/do_pause.cpp
    include/ui/dynamic2d.h ui/dynamic2d.cpp
    include/ui/editpal.h ui/editpal.cpp
    include/ui/evolve.h ui/evolve.cpp
    include/ui/evolver_menu_switch.h ui/evolver_menu_switch.cpp
    include/ui/field_prompt.h ui/field_prompt.cpp
    include/ui/file_get_window.h ui/file_get_window.cpp
    include/ui/find_special_colors.h ui/find_special_colors.cpp
    include/ui/flip_image.h ui/flip_image.cpp
    include/ui/frothy_basin.h ui/frothy_basin.cpp
    include/ui/full_screen_choice.h ui/full_screen_choice.cpp
    include/ui/full_screen_prompt.h ui/full_screen_prompt.cpp
    include/ui/get_3d_params.h ui/get_3d_params.cpp
    include/ui/get_a_number.h ui/get_a_number.cpp
    include/ui/get_browse_params.h ui/get_browse_params.cpp
    include/ui/get_calculation_time.h ui/get_calculation_time.cpp
    include/ui/get_cmd_string.h ui/get_cmd_string.cpp
    include/ui/get_commands.h ui/get_commands.cpp
    include/ui/get_corners.h ui/get_corners.cpp
    include/ui/get_file_entry.h ui/get_file_entry.cpp
    include/ui/get_fract_type.h ui/get_fract_type.cpp
    include/ui/get_key_no_help.h ui/get_key_no_help.cpp
    include/ui/get_rds_params.h ui/get_rds_params.cpp
    include/ui/get_sound_params.h ui/get_sound_params.cpp
    include/ui/get_toggles.h ui/get_toggles.cpp
    include/ui/get_toggles2.h ui/get_toggles2.cpp
    include/ui/get_video_mode.h ui/get_video_mode.cpp
    include/ui/get_view_params.h ui/get_view_params.cpp
    include/ui/goodbye.h ui/goodbye.cpp
    include/ui/help.h ui/help.cpp
    include/ui/help_title.h ui/help_title.cpp
    include/ui/history.h ui/history.cpp
    include/ui/id_keys.h
    include/ui/id_main.h ui/id_main.cpp
    include/ui/ifs2d.h ui/ifs2d.cpp
    include/ui/ifs3d.h ui/ifs3d.cpp
    include/ui/init_failure.h
    include/ui/intro.h ui/intro.cpp
    include/ui/input_field.h ui/input_field.cpp
    include/ui/inverse_julia.h ui/inverse_julia.cpp
    include/ui/make_batch_file.h ui/make_batch_file.cpp
    include/ui/main_menu.h ui/main_menu.cpp
    include/ui/main_menu_switch.h ui/main_menu_switch.cpp
    include/ui/main_state.h
    include/ui/menu_handler.h ui/menu_handler.cpp
    include/ui/mouse.h ui/mouse.cpp
    include/ui/not_disk_msg.h ui/not_disk_msg.cpp
    include/ui/orbit2d.h ui/orbit2d.cpp
    include/ui/orbit3d.h ui/orbit3d.cpp
    include/ui/passes_options.h ui/passes_options.cpp
    include/ui/plasma.h ui/plasma.cpp
    include/ui/put_string_center.h ui/put_string_center.cpp
    include/ui/read_ticker.h ui/read_ticker.cpp
    include/ui/rotate.h ui/rotate.cpp
    include/ui/select_video_mode.h ui/select_video_mode.cpp
    include/ui/set_default_params.h ui/set_default_params.cpp
    include/ui/shell_sort.h ui/shell_sort.cpp
    include/ui/slideshw.h ui/slideshw.cpp
    include/ui/standard_4d.h ui/standard_4d.cpp
    include/ui/starfield.h ui/starfield.cpp
    include/ui/stereo.h ui/stereo.cpp
    include/ui/stop_msg.h ui/stop_msg.cpp
    include/ui/tab_display.h ui/tab_display.cpp
    include/ui/temp_msg.h ui/temp_msg.cpp
    include/ui/testpt.h ui/testpt.cpp
    include/ui/text_screen.h ui/text_screen.cpp
    include/ui/thinking.h ui/thinking.cpp
    include/ui/video.h ui/video.cpp
    include/ui/zoom.h ui/zoom.cpp
)
source_group_subdir("Engine" "engine")
source_group_subdir("Fractal Specific" "fractals")
source_group_subdir("Geometry" "geometry")
source_group_subdir("IO" "io")
source_group_subdir("Math" "math")
source_group_subdir("Miscellaneous" "misc")
source_group_subdir("UI" "ui")

target_compile_definitions(libid PUBLIC
    ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
# TODO: Figure out a better way to deal with converting packed structs on big-endian machines
set_source_files_properties(io/decode_info.cpp
    PROPERTIES
        COMPILE_FLAGS $<$<CXX_COMPILER_ID:GNU>:-Wno-address-of-packed-member>)
target_compile_options(libid PUBLIC ${ID_TARGET_OPTIONS})
target_include_directories(libid PUBLIC include)
if(ID_USE_WX)
    set(ID_OS_LIBS os-wx)
else()
    set(ID_OS_LIBS "")
endif()
target_link_libraries(libid PRIVATE helpcom os ${ID_OS_LIBS} fmt::fmt-header-only)
target_link_libraries(libid PUBLIC help-defs config Boost::algorithm Boost::endian GIF::GIF cpuid::cpuid)
target_folder(libid "Libraries")
add_dependencies(libid native-help)
