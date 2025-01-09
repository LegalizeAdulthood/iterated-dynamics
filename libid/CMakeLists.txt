# SPDX-License-Identifier: GPL-3.0-only
#
find_package(Boost REQUIRED)
find_package(GIF REQUIRED)
find_package(cpuid CONFIG REQUIRED)

add_library(libid
    include/os.h

    3d.cpp include/3d.h
    line3d.cpp include/line3d.h
    plot3d.cpp include/plot3d.h

    boundary_trace.cpp include/boundary_trace.h
    calc_frac_init.cpp include/calc_frac_init.h
    calcfrac.cpp include/calcfrac.h
    calmanfp.cpp include/calmanfp.h
    convert_center_mag.cpp include/convert_center_mag.h
    convert_corners.cpp include/convert_corners.h
    diffusion_scan.cpp include/diffusion_scan.h
    fracsuba.cpp include/fracsuba.h
    fractalb.cpp include/fractalb.h
    fractals.cpp include/fractals.h
    fractals/frasetup.cpp include/fractals/frasetup.h
    get_julia_attractor.cpp include/get_julia_attractor.h
    one_or_two_pass.cpp include/one_or_two_pass.h
    orbit.cpp include/orbit.h
    param_not_used.cpp include/param_not_used.h
    PertEngine.cpp include/PertEngine.h
    perturbation.cpp include/perturbation.h
    pixel_grid.cpp include/pixel_grid.h
    include/Point.h
    resume.cpp include/resume.h
    soi.cpp include/soi.h
    soi1.cpp
    solid_guess.cpp include/solid_guess.h
    sticky_orbits.cpp include/sticky_orbits.h
    tesseral.cpp include/tesseral.h
    type_has_param.cpp include/type_has_param.h
    wait_until.cpp include/wait_until.h
    work_list.cpp include/work_list.h

    fractals/ant.cpp include/fractals/ant.h
    fractals/barnsley.cpp include/fractals/barnsley.h
    fractals/bifurcation.cpp include/fractals/bifurcation.h
    fractals/burning_ship.cpp include/fractals/burning_ship.h
    fractals/cellular.cpp include/fractals/cellular.h
    fractals/circle_pattern.cpp include/fractals/circle_pattern.h
    fractals/diffusion.cpp include/fractals/diffusion.h
    fractals/divide_brot.cpp include/fractals/divide_brot.h
    fractals/escher.cpp include/fractals/escher.h
    fractals/fn_or_fn.cpp include/fractals/fn_or_fn.h
    fractals/fractalp.cpp include/fractals/fractalp.h
    fractals/frothy_basin.cpp include/fractals/frothy_basin.h
    fractals/halley.cpp include/fractals/halley.h
    fractals/hypercomplex_mandelbrot.cpp include/fractals/hypercomplex_mandelbrot.h
    fractals/ifs.cpp include/fractals/ifs.h
    fractals/jb.cpp include/fractals/jb.h
    fractals/lambda_fn.cpp include/fractals/lambda_fn.h
    fractals/lorenz.cpp include/fractals/lorenz.h
    fractals/lsys.cpp include/fractals/lsys_fns.h
    fractals/lsysf.cpp
    fractals/lyapunov.cpp include/fractals/lyapunov.h
    fractals/magnet.cpp include/fractals/magnet.h
    fractals/mandelbrot_mix.cpp include/fractals/mandelbrot_mix.h
    fractals/newton.cpp include/fractals/newton.h
    fractals/parser.cpp include/fractals/parser.h
    fractals/peterson_variations.cpp include/fractals/peterson_variations.h
    fractals/phoenix.cpp include/fractals/phoenix.h
    fractals/pickover_mandelbrot.cpp include/fractals/pickover_mandelbrot.h
    fractals/plasma.cpp include/fractals/plasma.h
    fractals/popcorn.cpp include/fractals/popcorn.h
    fractals/population.cpp include/fractals/population.h
    fractals/quartic_mandelbrot.cpp include/fractals/quartic_mandelbrot.h
    fractals/quaternion_mandelbrot.cpp include/fractals/quaternion_mandelbrot.h
    fractals/sierpinski_gasket.cpp include/fractals/sierpinski_gasket.h
    fractals/taylor_skinner_variations.cpp include/fractals/taylor_skinner_variations.h
    fractals/testpt.cpp include/fractals/testpt.h
    fractals/unity.cpp include/fractals/unity.h
    fractals/volterra_lotka.cpp include/fractals/volterra_lotka.h

    cmdfiles.cpp include/cmdfiles.h include/cmdfiles_test.h
    decode_info.cpp include/decode_info.h
    decoder.cpp include/decoder.h
    diskvid.cpp include/diskvid.h
    do_pause.cpp include/do_pause.h
    editpal.cpp include/editpal.h
    encoder.cpp include/encoder.h
    evolve.cpp include/evolve.h
    gif_file.cpp include/gif_file.h
    gifview.cpp include/gifview.h
    loadfile.cpp include/loadfile.h
    loadmap.cpp include/loadmap.h
    lowerize_parameter.cpp include/lowerize_parameter.h
    rotate.cpp include/rotate.h
    save_file.cpp include/save_file.h
    slideshw.cpp include/slideshw.h
    special_dirs.cpp include/special_dirs.h
    stereo.cpp include/stereo.h
    video.cpp include/video.h

    include/math/big.h
    math/bigflt.cpp
    math/biginit.cpp include/math/biginit.h
    math/bignum.cpp
    math/bignumc.cpp
    include/math/cmplx.h
    include/math/complex_fn.h
    math/fixed_pt.cpp include/math/fixed_pt.h
    math/fpu087.cpp include/math/fpu087.h
    math/hcmplx.cpp include/math/hcmplx.h
    include/math/mpmath.h
    math/mpmath_c.cpp include/math/mpmath_c.h
    include/math/rand15.h
    include/math/sign.h
    include/math/sqr.h

    debug_flags.cpp include/debug_flags.h
    drivers.cpp
    get_disk_space.cpp include/get_disk_space.h
    memory.cpp include/memory.h

    bailout_formula.cpp include/bailout_formula.h
    check_key.cpp include/check_key.h
    check_orbit_name.cpp include/check_orbit_name.h
    check_write_file.cpp include/check_write_file.h
    comments.cpp include/comments.h
    dir_file.cpp include/dir_file.h
    double_to_string.cpp include/double_to_string.h
    ends_with_slash.cpp include/ends_with_slash.h
    engine_timer.cpp include/engine_timer.h
    evolver_menu_switch.cpp include/evolver_menu_switch.h
    expand_dirname.cpp include/expand_dirname.h
    include/extract_filename.h
    file_gets.cpp include/file_gets.h
    find_extra_param.cpp include/find_extra_param.h
    find_file.cpp include/find_file.h
    find_path.cpp include/find_path.h
    find_special_colors.cpp include/find_special_colors.h
    fix_dirname.cpp include/fix_dirname.h
    flip_image.cpp include/flip_image.h
    framain2.cpp include/framain2.h
    get_calculation_time.cpp include/get_calculation_time.h
    get_commands.cpp include/get_commands.h
    get_key_no_help.cpp include/get_key_no_help.h
    get_prec_big_float.cpp include/get_prec_big_float.h
    has_ext.cpp include/has_ext.h
    history.cpp include/history.h
    id_main.cpp include/id_main.h
    jiim.cpp include/jiim.h
    load_config.cpp include/load_config.h
    load_entry_text.cpp include/load_entry_text.h
    load_params.cpp include/load_params.h
    main_menu_switch.cpp include/main_menu_switch.h
    include/main_state.h
    make_batch_file.cpp include/make_batch_file.h
    make_mig.cpp include/make_mig.h
    make_path.cpp include/make_path.h
    menu_handler.cpp include/menu_handler.h
    merge_path_names.cpp include/merge_path_names.h
    mouse.cpp include/mouse.h
    path_match.cpp include/path_match.h
    round_float_double.cpp include/round_float_double.h
    search_path.cpp include/search_path.h
    set_default_params.cpp include/set_default_params.h
    shell_sort.cpp include/shell_sort.h
    sound.cpp include/sound.h
    spindac.cpp include/spindac.h
    split_path.cpp include/split_path.h
    starfield.cpp include/starfield.h
    trig_fns.cpp include/trig_fns.h
    trim_filename.cpp include/trim_filename.h
    update_save_name.cpp include/update_save_name.h
    version.cpp include/version.h
    video_mode.cpp include/video_mode.h
    zoom.cpp include/zoom.h

    include/ui/ChoiceBuilder.h
    ui/get_video_mode.cpp include/ui/get_video_mode.h
    ui/text_screen.cpp include/ui/text_screen.h
    ui/field_prompt.cpp include/ui/field_prompt.h
    ui/file_item.cpp include/ui/file_item.h
    ui/full_screen_choice.cpp include/ui/full_screen_choice.h
    ui/full_screen_prompt.cpp include/ui/full_screen_prompt.h
    ui/get_3d_params.cpp include/ui/get_3d_params.h
    ui/get_a_filename.cpp include/ui/get_a_filename.h
    ui/get_a_number.cpp include/ui/get_a_number.h
    ui/get_browse_params.cpp include/ui/get_browse_params.h
    ui/get_cmd_string.cpp include/ui/get_cmd_string.h
    ui/get_corners.cpp include/ui/get_corners.h
    ui/get_fract_type.cpp include/ui/get_fract_type.h
    ui/get_rds_params.cpp include/ui/get_rds_params.h
    ui/get_sound_params.cpp include/ui/get_sound_params.h
    ui/get_toggles.cpp include/ui/get_toggles.h
    ui/get_toggles2.cpp include/ui/get_toggles2.h
    ui/get_view_params.cpp include/ui/get_view_params.h
    ui/goodbye.cpp include/ui/goodbye.h
    ui/help.cpp
    ui/help_title.cpp include/ui/help_title.h
    ui/intro.cpp include/ui/intro.h
    ui/input_field.cpp include/ui/input_field.h
    ui/main_menu.cpp include/ui/main_menu.h
    ui/not_disk_msg.cpp include/ui/not_disk_msg.h
    ui/passes_options.cpp include/ui/passes_options.h
    ui/put_string_center.cpp include/ui/put_string_center.h
    ui/select_video_mode.cpp include/ui/select_video_mode.h
    ui/stop_msg.cpp include/ui/stop_msg.h
    ui/tab_display.cpp include/ui/tab_display.h
    ui/temp_msg.cpp include/ui/temp_msg.h
    ui/thinking.cpp include/ui/thinking.h

    include/drivers.h
    include/fractype.h
    include/id.h
    include/id_data.h
    include/id_keys.h
    include/is_directory.h
    include/is_writeable.h
    include/fractals/lsys.h
    include/pixel_limits.h
    include/prototyp.h
    include/read_ticker.h
    include/sized_types.h
    include/stack_avail.h
    include/ValueSaver.h
)

source_group("Header Files/3d" FILES
    include/3d.h
    include/line3d.h
    include/plot3d.h
)
source_group("Source Files/3d" FILES
    3d.cpp
    line3d.cpp
    plot3d.cpp
)
source_group("Header Files/Engine" FILES
    include/bailout_formula.h
    include/boundary_trace.h
    include/calc_frac_init.h
    include/calcfrac.h
    include/calmanfp.h
    include/check_key.h
    include/convert_center_mag.h
    include/convert_corners.h
    include/diffusion_scan.h
    include/engine_timer.h
    include/fracsuba.h
    include/fractalb.h
    include/fractals.h
    include/fractype.h
    include/get_julia_attractor.h
    include/id_data.h
    include/jiim.h
    include/one_or_two_pass.h
    include/orbit.h
    include/param_not_used.h
    include/PertEngine.h
    include/perturbation.h
    include/pixel_grid.h
    include/pixel_limits.h
    include/Point.h
    include/resume.h
    include/soi.h
    include/solid_guess.h
    include/sticky_orbits.h
    include/tesseral.h
    include/type_has_param.h
    include/wait_until.h
    include/work_list.h
)
source_group("Source Files/Engine" FILES
    bailout_formula.cpp
    boundary_trace.cpp
    calc_frac_init.cpp
    calcfrac.cpp
    calmanfp.cpp
    check_key.cpp
    convert_center_mag.cpp
    convert_corners.cpp
    diffusion_scan.cpp
    engine_timer.cpp
    fracsuba.cpp
    fractalb.cpp
    fractals.cpp
    get_julia_attractor.cpp
    jiim.cpp
    one_or_two_pass.cpp
    orbit.cpp
    param_not_used.cpp
    PertEngine.cpp
    perturbation.cpp
    pixel_grid.cpp
    resume.cpp
    soi.cpp
    soi1.cpp
    solid_guess.cpp
    sticky_orbits.cpp
    tesseral.cpp
    type_has_param.cpp
    wait_until.cpp
    work_list.cpp
)
source_group("Header Files/Fractal Specific" FILES
    include/fractals/ant.h
    include/fractals/barnsley.h
    include/fractals/bifurcation.h
    include/fractals/burning_ship.h
    include/fractals/cellular.h
    include/fractals/circle_pattern.h
    include/fractals/diffusion.h
    include/fractals/divide_brot.h
    include/fractals/escher.h
    include/fractals/fn_or_fn.h
    include/fractals/fractalp.h
    include/fractals/frasetup.h
    include/fractals/frothy_basin.h
    include/fractals/halley.h
    include/fractals/hypercomplex_mandelbrot.h
    include/fractals/ifs.h
    include/fractals/jb.h
    include/fractals/lambda_fn.h
    include/fractals/lorenz.h
    include/fractals/lsys.h
    include/fractals/lsysf.h
    include/fractals/lsys_fns.h
    include/fractals/lyapunov.h
    include/fractals/magnet.h
    include/fractals/mandelbrot_mix.h
    include/fractals/newton.h
    include/fractals/parser.h
    include/fractals/peterson_variations.h
    include/fractals/phoenix.h
    include/fractals/pickover_mandelbrot.h
    include/fractals/plasma.h
    include/fractals/popcorn.h
    include/fractals/population.h
    include/fractals/quartic_mandelbrot.h
    include/fractals/quaternion_mandelbrot.h
    include/fractals/sierpinski_gasket.h
    include/fractals/taylor_skinner_variations.h
    include/fractals/testpt.h
    include/fractals/unity.h
    include/fractals/volterra_lotka.h
)
source_group("Source Files/Fractal Specific" FILES
    fractals/ant.cpp
    fractals/barnsley.cpp
    fractals/bifurcation.cpp
    fractals/burning_ship.cpp
    fractals/cellular.cpp
    fractals/circle_pattern.cpp
    fractals/diffusion.cpp
    fractals/divide_brot.cpp
    fractals/escher.cpp
    fractals/fn_or_fn.cpp
    fractals/fractalp.cpp
    fractals/frasetup.cpp
    fractals/frothy_basin.cpp
    fractals/halley.cpp
    fractals/hypercomplex_mandelbrot.cpp
    fractals/ifs.cpp
    fractals/jb.cpp
    fractals/lambda_fn.cpp
    fractals/lorenz.cpp
    fractals/lsys.cpp
    fractals/lsysf.cpp
    fractals/lyapunov.cpp
    fractals/magnet.cpp
    fractals/mandelbrot_mix.cpp
    fractals/newton.cpp
    fractals/parser.cpp
    fractals/peterson_variations.cpp
    fractals/phoenix.cpp
    fractals/pickover_mandelbrot.cpp
    fractals/plasma.cpp
    fractals/popcorn.cpp
    fractals/population.cpp
    fractals/quartic_mandelbrot.cpp
    fractals/quaternion_mandelbrot.cpp
    fractals/sierpinski_gasket.cpp
    fractals/taylor_skinner_variations.cpp
    fractals/testpt.cpp
    fractals/unity.cpp
    fractals/volterra_lotka.cpp
)
source_group("Header Files/IO" FILES
    include/cmdfiles.h
    include/cmdfiles_test.h
    include/comments.h
    include/decode_info.h
    include/decoder.h
    include/diskvid.h
    include/do_pause.h
    include/editpal.h
    include/encoder.h
    include/evolve.h
    include/gif_file.h
    include/gifview.h
    include/is_directory.h
    include/is_writeable.h
    include/loadfile.h
    include/loadmap.h
    include/lowerize_parameter.h
    include/rotate.h
    include/save_file.h
    include/slideshw.h
    include/sound.h
    include/special_dirs.h
    include/stereo.h
    include/video.h
)
source_group("Source Files/IO" FILES
    cmdfiles.cpp
    comments.cpp
    decode_info.cpp
    decoder.cpp
    diskvid.cpp
    do_pause.cpp
    editpal.cpp
    encoder.cpp
    evolve.cpp
    gif_file.cpp
    gifview.cpp
    loadfile.cpp
    loadmap.cpp
    lowerize_parameter.cpp
    rotate.cpp
    save_file.cpp
    slideshw.cpp
    sound.cpp
    special_dirs.cpp
    stereo.cpp
    video.cpp
)
source_group("Header Files/Math" FILES
    include/math/big.h
    include/math/biginit.h
    include/math/cmplx.h
    include/math/complex_fn.h
    include/math/fixed_pt.h
    include/math/fpu087.h
    include/math/hcmplx.h
    include/math/mpmath.h
    include/math/mpmath_c.h
    include/math/rand15.h
    include/math/sign.h
    include/math/sqr.h
)
source_group("Source Files/Math" FILES
    math/bigflt.cpp
    math/biginit.cpp
    math/bignum.cpp
    math/bignumc.cpp
    math/fixed_pt.cpp
    math/fpu087.cpp
    math/hcmplx.cpp
    math/mpmath_c.cpp
)
source_group("Header Files/Miscellaneous" FILES
    include/check_orbit_name.h
    include/check_write_file.h
    include/dir_file.h
    include/double_to_string.h
    include/ends_with_slash.h
    include/evolver_menu_switch.h
    include/expand_dirname.h
    include/extract_filename.h
    include/file_gets.h
    include/find_extra_param.h
    include/find_file.h
    include/find_path.h
    include/find_special_colors.h
    include/fix_dirname.h
    include/flip_image.h
    include/framain2.h
    include/get_calculation_time.h
    include/get_commands.h
    include/get_key_no_help.h
    include/get_prec_big_float.h
    include/has_ext.h
    include/history.h
    include/id.h
    include/id_keys.h
    include/id_main.h
    include/load_config.h
    include/load_entry_text.h
    include/load_params.h
    include/main_menu_switch.h
    include/main_state.h
    include/make_batch_file.h
    include/make_mig.h
    include/make_path.h
    include/menu_handler.h
    include/merge_path_names.h
    include/mouse.h
    include/path_match.h
    include/round_float_double.h
    include/search_path.h
    include/set_default_params.h
    include/shell_sort.h
    include/spindac.h
    include/split_path.h
    include/starfield.h
    include/trig_fns.h
    include/trim_filename.h
    include/update_save_name.h
    include/version.h
    include/video_mode.h
    include/zoom.h
)
source_group("Source Files/Miscellaneous" FILES
    check_orbit_name.cpp
    check_write_file.cpp
    dir_file.cpp
    double_to_string.cpp
    ends_with_slash.cpp
    evolver_menu_switch.cpp
    expand_dirname.cpp
    file_gets.cpp
    find_extra_param.cpp
    find_file.cpp
    find_path.cpp
    find_special_colors.cpp
    fix_dirname.cpp
    flip_image.cpp
    framain2.cpp
    get_calculation_time.cpp
    get_commands.cpp
    get_key_no_help.cpp
    get_prec_big_float.cpp
    has_ext.cpp
    history.cpp
    id_main.cpp
    load_config.cpp
    load_entry_text.cpp
    load_params.cpp
    main_menu_switch.cpp
    make_batch_file.cpp
    make_mig.cpp
    make_path.cpp
    menu_handler.cpp
    merge_path_names.cpp
    mouse.cpp
    path_match.cpp
    round_float_double.cpp
    search_path.cpp
    set_default_params.cpp
    shell_sort.cpp
    spindac.cpp
    split_path.cpp
    starfield.cpp
    trig_fns.cpp
    trim_filename.cpp
    update_save_name.cpp
    version.cpp
    video_mode.cpp
    zoom.cpp
)
source_group("Header Files/Plumbing" FILES
    include/debug_flags.h
    include/drivers.h
    include/get_disk_space.h
    include/init_failure.h
    include/memory.h
    include/os.h
    include/prototyp.h
    include/read_ticker.h
    include/sized_types.h
    include/special_dirs.h
    include/stack_avail.h
    include/ValueSaver.h
)
source_group("Source Files/Plumbing" FILES
    debug_flags.cpp
    drivers.cpp
    get_disk_space.cpp
    memory.cpp
)
source_group("Header Files/UI" FILES
    include/ui/ChoiceBuilder.h
    include/ui/field_prompt.h
    include/ui/file_item.h
    include/ui/full_screen_choice.h
    include/ui/full_screen_prompt.h
    include/ui/get_3d_params.h
    include/ui/get_a_number.h
    include/ui/get_browse_params.h
    include/ui/get_a_filename.h
    include/ui/get_cmd_string.h
    include/ui/get_corners.h
    include/ui/get_fract_type.h
    include/ui/get_rds_params.h
    include/ui/get_sound_params.h
    include/ui/get_toggles.h
    include/ui/get_toggles2.h
    include/ui/get_video_mode.h
    include/ui/get_view_params.h
    include/ui/goodbye.h
    include/ui/help.h
    include/ui/help_title.h
    include/ui/input_field.h
    include/ui/intro.h
    include/ui/main_menu.h
    include/ui/not_disk_msg.h
    include/ui/passes_options.h
    include/ui/put_string_center.h
    include/ui/select_video_mode.h
    include/ui/stop_msg.h
    include/ui/tab_display.h
    include/ui/temp_msg.h
    include/ui/text_screen.h
    include/ui/thinking.h
)
source_group("Source Files/UI" FILES
    ui/field_prompt.cpp
    ui/file_item.cpp
    ui/full_screen_choice.cpp
    ui/full_screen_prompt.cpp
    ui/get_3d_params.cpp
    ui/get_a_filename.cpp
    ui/get_a_number.cpp
    ui/get_browse_params.cpp
    ui/get_cmd_string.cpp
    ui/get_corners.cpp
    ui/get_fract_type.cpp
    ui/get_rds_params.cpp
    ui/get_sound_params.cpp
    ui/get_toggles.cpp
    ui/get_toggles2.cpp
    ui/get_view_params.cpp
    ui/get_video_mode.cpp
    ui/goodbye.cpp
    ui/help.cpp
    ui/help_title.cpp
    ui/input_field.cpp
    ui/intro.cpp
    ui/main_menu.cpp
    ui/not_disk_msg.cpp
    ui/passes_options.cpp
    ui/put_string_center.cpp
    ui/select_video_mode.cpp
    ui/stop_msg.cpp
    ui/tab_display.cpp
    ui/temp_msg.cpp
    ui/text_screen.cpp
    ui/thinking.cpp
)

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
