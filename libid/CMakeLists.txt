# SPDX-License-Identifier: GPL-3.0-only
#
find_package(Boost REQUIRED)
find_package(GIF REQUIRED)
find_package(cpuid CONFIG REQUIRED)

add_library(libid
    include/os.h

    include/3d/3d.h 3d/3d.cpp
    include/3d/line3d.h 3d/line3d.cpp
    include/3d/plot3d.h 3d/plot3d.cpp

    include/engine/boundary_trace.h engine/boundary_trace.cpp
    include/engine/calc_frac_init.h engine/calc_frac_init.cpp
    include/engine/calcfrac.h engine/calcfrac.cpp
    include/engine/calmanfp.h engine/calmanfp.cpp
    include/engine/convert_center_mag.h engine/convert_center_mag.cpp
    include/engine/convert_corners.h engine/convert_corners.cpp
    include/engine/diffusion_scan.h engine/diffusion_scan.cpp
    include/engine/engine_timer.h engine/engine_timer.cpp
    include/engine/fracsuba.h engine/fracsuba.cpp
    include/engine/fractalb.h engine/fractalb.cpp
    include/engine/fractals.h engine/fractals.cpp
    include/engine/get_julia_attractor.h engine/get_julia_attractor.cpp
    include/engine/id_data.h
    include/engine/jiim.h engine/jiim.cpp
    include/engine/one_or_two_pass.h engine/one_or_two_pass.cpp
    include/engine/orbit.h engine/orbit.cpp
    include/engine/param_not_used.h engine/param_not_used.cpp
    include/engine/PertEngine.h engine/PertEngine.cpp
    include/engine/perturbation.h engine/perturbation.cpp
    include/engine/pixel_grid.h engine/pixel_grid.cpp
    include/engine/pixel_limits.h
    include/engine/Point.h
    include/engine/resume.h engine/resume.cpp
    include/engine/soi.h engine/soi.cpp
    engine/soi1.cpp
    include/engine/solid_guess.h engine/solid_guess.cpp
    include/engine/sticky_orbits.h engine/sticky_orbits.cpp
    include/engine/tesseral.h engine/tesseral.cpp
    include/engine/type_has_param.h engine/type_has_param.cpp
    include/engine/wait_until.h engine/wait_until.cpp
    include/engine/work_list.h engine/work_list.cpp

    include/fractals/ant.h fractals/ant.cpp
    include/fractals/barnsley.h fractals/barnsley.cpp
    include/fractals/bifurcation.h fractals/bifurcation.cpp
    include/fractals/burning_ship.h fractals/burning_ship.cpp
    include/fractals/cellular.h fractals/cellular.cpp
    include/fractals/check_orbit_name.h fractals/check_orbit_name.cpp
    include/fractals/circle_pattern.h fractals/circle_pattern.cpp
    include/fractals/diffusion.h fractals/diffusion.cpp
    include/fractals/divide_brot.h fractals/divide_brot.cpp
    include/fractals/escher.h fractals/escher.cpp
    include/fractals/fn_or_fn.h fractals/fn_or_fn.cpp
    include/fractals/fractalp.h fractals/fractalp.cpp
    include/fractals/fractype.h
    include/fractals/frasetup.h fractals/frasetup.cpp
    include/fractals/frothy_basin.h fractals/frothy_basin.cpp
    include/fractals/halley.h fractals/halley.cpp
    include/fractals/hypercomplex_mandelbrot.h fractals/hypercomplex_mandelbrot.cpp
    include/fractals/ifs.h fractals/ifs.cpp
    include/fractals/jb.h fractals/jb.cpp
    include/fractals/lambda_fn.h fractals/lambda_fn.cpp
    include/fractals/lorenz.h fractals/lorenz.cpp
    include/fractals/lsys.h fractals/lsys.cpp
    include/fractals/lsys_fns.h
    fractals/lsysf.cpp
    include/fractals/lyapunov.h fractals/lyapunov.cpp
    include/fractals/magnet.h fractals/magnet.cpp
    include/fractals/mandelbrot_mix.h fractals/mandelbrot_mix.cpp
    include/fractals/newton.h fractals/newton.cpp
    include/fractals/parser.h fractals/parser.cpp
    include/fractals/peterson_variations.h fractals/peterson_variations.cpp
    include/fractals/phoenix.h fractals/phoenix.cpp
    include/fractals/pickover_mandelbrot.h fractals/pickover_mandelbrot.cpp
    include/fractals/plasma.h fractals/plasma.cpp
    include/fractals/popcorn.h fractals/popcorn.cpp
    include/fractals/population.h fractals/population.cpp
    include/fractals/quartic_mandelbrot.h fractals/quartic_mandelbrot.cpp
    include/fractals/quaternion_mandelbrot.h fractals/quaternion_mandelbrot.cpp
    include/fractals/sierpinski_gasket.h fractals/sierpinski_gasket.cpp
    include/fractals/taylor_skinner_variations.h fractals/taylor_skinner_variations.cpp
    include/fractals/testpt.h fractals/testpt.cpp
    include/fractals/unity.h fractals/unity.cpp
    include/fractals/volterra_lotka.h fractals/volterra_lotka.cpp

    include/cmdfiles.h cmdfiles.cpp
    include/cmdfiles_test.h
    include/decode_info.h decode_info.cpp
    include/decoder.h decoder.cpp
    include/diskvid.h diskvid.cpp
    include/do_pause.h do_pause.cpp
    include/editpal.h editpal.cpp
    include/encoder.h encoder.cpp
    include/evolve.h evolve.cpp
    include/gif_file.h gif_file.cpp
    include/gifview.h gifview.cpp
    include/loadfile.h loadfile.cpp
    include/loadmap.h loadmap.cpp
    include/lowerize_parameter.h lowerize_parameter.cpp
    include/rotate.h rotate.cpp
    include/slideshw.h slideshw.cpp
    include/stereo.h stereo.cpp
    include/video.h video.cpp

    include/io/check_write_file.h io/check_write_file.cpp
    include/io/dir_file.h io/dir_file.cpp
    include/io/ends_with_slash.h io/ends_with_slash.cpp
    include/io/expand_dirname.h io/expand_dirname.cpp
    include/io/get_disk_space.h io/get_disk_space.cpp
    include/io/extract_filename.h
    include/io/file_gets.h io/file_gets.cpp
    include/io/find_file.h io/find_file.cpp
    include/io/find_path.h io/find_path.cpp
    include/io/fix_dirname.h io/fix_dirname.cpp
    include/io/has_ext.h io/has_ext.cpp
    include/io/is_directory.h
    include/io/is_writeable.h
    include/io/make_path.h io/make_path.cpp
    include/io/merge_path_names.h io/merge_path_names.cpp
    include/io/path_match.h io/path_match.cpp
    include/io/save_file.h io/save_file.cpp
    include/io/search_path.h io/search_path.cpp
    include/io/special_dirs.h io/special_dirs.cpp
    include/io/split_path.h io/split_path.cpp
    include/io/trim_filename.h io/trim_filename.cpp
    include/io/update_save_name.h io/update_save_name.cpp

    include/math/big.h
    math/bigflt.cpp
    include/math/biginit.h math/biginit.cpp
    math/bignum.cpp
    math/bignumc.cpp
    include/math/cmplx.h
    include/math/complex_fn.h
    include/math/fixed_pt.h math/fixed_pt.cpp
    include/math/fpu087.h math/fpu087.cpp
    include/math/hcmplx.h math/hcmplx.cpp
    include/math/mpmath.h
    include/math/mpmath_c.h math/mpmath_c.cpp
    include/math/rand15.h
    include/math/sign.h
    include/math/sqr.h

    include/debug_flags.h debug_flags.cpp
    drivers.cpp
    include/memory.h memory.cpp

    include/engine/bailout_formula.h engine/bailout_formula.cpp
    include/engine/check_key.h engine/check_key.cpp
    include/comments.h comments.cpp
    include/double_to_string.h double_to_string.cpp
    include/evolver_menu_switch.h evolver_menu_switch.cpp
    include/find_extra_param.h find_extra_param.cpp
    include/find_special_colors.h find_special_colors.cpp
    include/flip_image.h flip_image.cpp
    include/framain2.h framain2.cpp
    include/get_calculation_time.h get_calculation_time.cpp
    include/get_commands.h get_commands.cpp
    include/get_key_no_help.h get_key_no_help.cpp
    include/get_prec_big_float.h get_prec_big_float.cpp
    include/history.h history.cpp
    include/id_main.h id_main.cpp
    include/load_config.h load_config.cpp
    include/load_entry_text.h load_entry_text.cpp
    include/load_params.h load_params.cpp
    include/main_menu_switch.h main_menu_switch.cpp
    include/main_state.h
    include/make_batch_file.h make_batch_file.cpp
    include/make_mig.h make_mig.cpp
    include/menu_handler.h menu_handler.cpp
    include/mouse.h mouse.cpp
    include/round_float_double.h round_float_double.cpp
    include/set_default_params.h set_default_params.cpp
    include/shell_sort.h shell_sort.cpp
    include/sound.h sound.cpp
    include/spindac.h spindac.cpp
    include/starfield.h starfield.cpp
    include/trig_fns.h trig_fns.cpp
    include/version.h version.cpp
    include/video_mode.h video_mode.cpp
    include/zoom.h zoom.cpp

    include/ui/ChoiceBuilder.h
    include/ui/get_video_mode.h ui/get_video_mode.cpp
    include/ui/text_screen.h ui/text_screen.cpp
    include/ui/field_prompt.h ui/field_prompt.cpp
    include/ui/file_item.h ui/file_item.cpp
    include/ui/full_screen_choice.h ui/full_screen_choice.cpp
    include/ui/full_screen_prompt.h ui/full_screen_prompt.cpp
    include/ui/get_3d_params.h ui/get_3d_params.cpp
    include/ui/get_a_filename.h ui/get_a_filename.cpp
    include/ui/get_a_number.h ui/get_a_number.cpp
    include/ui/get_browse_params.h ui/get_browse_params.cpp
    include/ui/get_cmd_string.h ui/get_cmd_string.cpp
    include/ui/get_corners.h ui/get_corners.cpp
    include/ui/get_fract_type.h ui/get_fract_type.cpp
    include/ui/get_rds_params.h ui/get_rds_params.cpp
    include/ui/get_sound_params.h ui/get_sound_params.cpp
    include/ui/get_toggles.h ui/get_toggles.cpp
    include/ui/get_toggles2.h ui/get_toggles2.cpp
    include/ui/get_view_params.h ui/get_view_params.cpp
    include/ui/goodbye.h ui/goodbye.cpp
    ui/help.cpp
    include/ui/help_title.h ui/help_title.cpp
    include/ui/intro.h ui/intro.cpp
    include/ui/input_field.h ui/input_field.cpp
    include/ui/main_menu.h ui/main_menu.cpp
    include/ui/not_disk_msg.h ui/not_disk_msg.cpp
    include/ui/passes_options.h ui/passes_options.cpp
    include/ui/put_string_center.h ui/put_string_center.cpp
    include/ui/select_video_mode.h ui/select_video_mode.cpp
    include/ui/stop_msg.h ui/stop_msg.cpp
    include/ui/tab_display.h ui/tab_display.cpp
    include/ui/temp_msg.h ui/temp_msg.cpp
    include/ui/thinking.h ui/thinking.cpp

    include/drivers.h
    include/id.h
    include/id_keys.h
    include/prototyp.h
    include/read_ticker.h
    include/sized_types.h
    include/stack_avail.h
    include/ValueSaver.h
)

source_group("Header Files/Miscellaneous" FILES
    include/cmdfiles.h
    include/cmdfiles_test.h
    include/comments.h
    include/decode_info.h
    include/decoder.h
    include/double_to_string.h
    include/diskvid.h
    include/do_pause.h
    include/editpal.h
    include/encoder.h
    include/evolve.h
    include/evolver_menu_switch.h
    include/find_extra_param.h
    include/find_special_colors.h
    include/flip_image.h
    include/framain2.h
    include/get_calculation_time.h
    include/get_commands.h
    include/get_key_no_help.h
    include/get_prec_big_float.h
    include/gif_file.h
    include/gifview.h
    include/history.h
    include/id.h
    include/id_keys.h
    include/id_main.h
    include/loadfile.h
    include/loadmap.h
    include/load_config.h
    include/load_entry_text.h
    include/load_params.h
    include/lowerize_parameter.h
    include/main_menu_switch.h
    include/main_state.h
    include/make_batch_file.h
    include/make_mig.h
    include/menu_handler.h
    include/mouse.h
    include/round_float_double.h
    include/rotate.h
    include/set_default_params.h
    include/shell_sort.h
    include/slideshw.h
    include/sound.h
    include/spindac.h
    include/starfield.h
    include/stereo.h
    include/trig_fns.h
    include/version.h
    include/video.h
    include/video_mode.h
    include/zoom.h
)
source_group("Source Files/Miscellaneous" FILES
    cmdfiles.cpp
    comments.cpp
    decode_info.cpp
    decoder.cpp
    diskvid.cpp
    double_to_string.cpp
    do_pause.cpp
    editpal.cpp
    encoder.cpp
    evolve.cpp
    evolver_menu_switch.cpp
    find_extra_param.cpp
    find_special_colors.cpp
    flip_image.cpp
    framain2.cpp
    get_calculation_time.cpp
    get_commands.cpp
    get_key_no_help.cpp
    get_prec_big_float.cpp
    history.cpp
    id_main.cpp
    gif_file.cpp
    gifview.cpp
    loadfile.cpp
    loadmap.cpp
    load_config.cpp
    load_entry_text.cpp
    load_params.cpp
    lowerize_parameter.cpp
    main_menu_switch.cpp
    make_batch_file.cpp
    make_mig.cpp
    menu_handler.cpp
    mouse.cpp
    round_float_double.cpp
    rotate.cpp
    set_default_params.cpp
    shell_sort.cpp
    slideshw.cpp
    sound.cpp
    spindac.cpp
    starfield.cpp
    stereo.cpp
    trig_fns.cpp
    version.cpp
    video.cpp
    video_mode.cpp
    zoom.cpp
)
source_group("Header Files/Plumbing" FILES
    include/debug_flags.h
    include/drivers.h
    include/init_failure.h
    include/memory.h
    include/os.h
    include/prototyp.h
    include/read_ticker.h
    include/sized_types.h
    include/stack_avail.h
    include/ValueSaver.h
)
source_group("Source Files/Plumbing" FILES
    debug_flags.cpp
    drivers.cpp
    memory.cpp
)
source_group("Header Files/3D" REGULAR_EXPRESSION "include/3d/.*")
source_group("Source Files/3D" REGULAR_EXPRESSION "3d/.*")
source_group("Header Files/Engine" REGULAR_EXPRESSION "include/engine/.*")
source_group("Source Files/Engine" REGULAR_EXPRESSION "engine/.*")
source_group("Header Files/Fractal Specific" REGULAR_EXPRESSION "include/fractals/.*")
source_group("Source Files/Fractal Specific" REGULAR_EXPRESSION "fractals/.*")
source_group("Header Files/IO" REGULAR_EXPRESSION "include/io/.*")
source_group("Source Files/IO" REGULAR_EXPRESSION "io/.*")
source_group("Header Files/Math" REGULAR_EXPRESSION "include/math/.*")
source_group("Source Files/Math" REGULAR_EXPRESSION "math/.*")
source_group("Header Files/UI" REGULAR_EXPRESSION "include/ui/.*")
source_group("Source Files/UI" REGULAR_EXPRESSION "ui/.*")

set_src_dir(ui/help.cpp)
set_src_dir(id_main.cpp)

target_compile_definitions(libid PUBLIC
    ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
# TODO: Figure out a better way to deal with converting packed structs on big-endian machines
set_source_files_properties(decode_info.cpp
    PROPERTIES
        COMPILE_FLAGS $<$<CXX_COMPILER_ID:GNU>:-Wno-address-of-packed-member>)
target_compile_options(libid PUBLIC ${ID_TARGET_OPTIONS})
target_include_directories(libid PUBLIC include)
target_link_libraries(libid PRIVATE helpcom os)
target_link_libraries(libid PUBLIC help-defs config Boost::boost GIF::GIF cpuid::cpuid)
set_target_properties(libid PROPERTIES FOLDER Libraries)
add_dependencies(libid native-help)
