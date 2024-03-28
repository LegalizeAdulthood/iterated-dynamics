add_library(libid
    include/os.h

    3d.cpp include/3d.h
    line3d.cpp include/line3d.h
    plot3d.cpp include/plot3d.h

    calc_frac_init.cpp include/calc_frac_init.h
    calcfrac.cpp include/calcfrac.h
    calcmand.cpp include/calcmand.h
    calmanfp.cpp include/calmanfp.h
    fracsuba.cpp include/fracsuba.h
    fractalb.cpp include/fractalb.h
    fractalp.cpp include/fractalp.h
    fractals.cpp include/fractals.h
    frasetup.cpp include/frasetup.h
    get_julia_attractor.cpp include/get_julia_attractor.h
    orbit.cpp include/orbit.h
    param_not_used.cpp include/param_not_used.h
    pixel_grid.cpp include/pixel_grid.h
    resume.cpp include/resume.h
    soi.cpp include/soi.h
    soi1.cpp
    ssg_block_size.cpp include/ssg_block_size.h
    type_has_param.cpp include/type_has_param.h
    wait_until.cpp include/wait_until.h
    work_list.cpp include/work_list.h

    ant.cpp include/ant.h
    barnsley.cpp include/barnsley.h
    cellular.cpp include/cellular.h
    circle_pattern.cpp include/circle_pattern.h
    escher.cpp include/escher.h
    fn_or_fn.cpp include/fn_or_fn.h
    frothy_basin.cpp include/frothy_basin.h
    halley.cpp include/halley.h
    hypercomplex_mandelbrot.cpp include/hypercomplex_mandelbrot.h
    ifs.cpp include/ifs.h
    jb.cpp include/jb.h
    lambda_fn.cpp include/lambda_fn.h
    lorenz.cpp include/lorenz.h
    lsys.cpp include/lsys_fns.h
    lsysf.cpp
    magnet.cpp include/magnet.h
    mandelbrot_mix.cpp include/mandelbrot_mix.h
    miscfrac.cpp include/miscfrac.h
    newton.cpp include/newton.h
    peterson_variations.cpp include/peterson_variations.h
    phoenix.cpp include/phoenix.h
    pickover_mandelbrot.cpp include/pickover_mandelbrot.h
    popcorn.cpp include/popcorn.h
    quartic_mandelbrot.cpp include/quartic_mandelbrot.h
    quaternion_mandelbrot.cpp include/quaternion_mandelbrot.h
    sierpinski_gasket.cpp include/sierpinski_gasket.h
    taylor_skinner_variations.cpp include/taylor_skinner_variations.h
    testpt.cpp include/testpt.h
    unity.cpp include/unity.h
    volterra_lotka.cpp include/volterra_lotka.h

    cmdfiles.cpp include/cmdfiles.h
    decode_info.cpp include/decode_info.h
    decoder.cpp include/decoder.h
    diskvid.cpp include/diskvid.h
    do_pause.cpp include/do_pause.h
    editpal.cpp include/editpal.h
    encoder.cpp include/encoder.h
    evolve.cpp include/evolve.h
    get_video_mode.cpp include/get_video_mode.h
    gifview.cpp include/gifview.h
    loadfile.cpp include/loadfile.h
    loadmap.cpp include/loadmap.h
    parser.cpp include/parser.h
    parserfp.cpp
    rotate.cpp include/rotate.h
    slideshw.cpp include/slideshw.h
    stereo.cpp include/stereo.h
    text_screen.cpp include/text_screen.h

    bigflt.cpp
    biginit.cpp include/biginit.h
    bignum.cpp
    bignumc.cpp
    fixed_pt.cpp include/fixed_pt.h
    fpu087.cpp include/fpu087.h
    hcmplx.cpp include/hcmplx.h
    mpmath_c.cpp include/mpmath_c.h

    debug_flags.cpp include/debug_flags.h
    drivers.cpp
    get_disk_space.cpp include/get_disk_space.h
    memory.cpp include/memory.h

    abort_msg.cpp include/abort_msg.h
    bailout_formula.cpp include/bailout_formula.h
    check_key.cpp include/check_key.h
    check_orbit_name.cpp include/check_orbit_name.h
    check_write_file.cpp include/check_write_file.h
    comments.cpp include/comments.h
    convert_center_mag.cpp include/convert_center_mag.h
    convert_corners.cpp include/convert_corners.h
    include/choice_builder.h
    dir_file.cpp include/dir_file.h
    double_to_string.cpp include/double_to_string.h
    ends_with_slash.cpp include/ends_with_slash.h
    engine_timer.cpp include/engine_timer.h
    evolver_menu_switch.cpp include/evolver_menu_switch.h
    expand_dirname.cpp include/expand_dirname.h
    include/extract_filename.h
    field_prompt.cpp include/field_prompt.h
    file_gets.cpp include/file_gets.h
    find_extra_param.cpp include/find_extra_param.h
    find_file.cpp include/find_file.h
    find_file_item.cpp include/find_file_item.h
    find_path.cpp include/find_path.h
    find_special_colors.cpp include/find_special_colors.h
    fix_dirname.cpp include/fix_dirname.h
    flip_image.cpp include/flip_image.h
    framain2.cpp include/framain2.h
    full_screen_choice.cpp include/full_screen_choice.h
    full_screen_prompt.cpp include/full_screen_prompt.h
    get_3d_params.cpp include/get_3d_params.h
    get_a_filename.cpp include/get_a_filename.h
    get_a_number.cpp include/get_a_number.h
    get_browse_params.cpp include/get_browse_params.h
    get_calculation_time.cpp include/get_calculation_time.h
    get_cmd_string.cpp include/get_cmd_string.h
    get_commands.cpp include/get_commands.h
    get_corners.cpp include/get_corners.h
    get_file_entry.cpp include/get_file_entry.h
    get_fract_type.cpp include/get_fract_type.h
    get_key_no_help.cpp include/get_key_no_help.h
    get_prec_big_float.cpp include/get_prec_big_float.h
    get_rds_params.cpp include/get_rds_params.h
    get_sound_params.cpp include/get_sound_params.h
    get_toggles.cpp include/get_toggles.h
    get_toggles2.cpp include/get_toggles2.h
    get_view_params.cpp include/get_view_params.h
    goodbye.cpp include/goodbye.h
    has_ext.cpp include/has_ext.h
    help.cpp
    help_title.cpp include/help_title.h
    history.cpp include/history.h
    id_main.cpp include/id_main.h
    intro.cpp include/intro.h
    input_field.cpp include/input_field.h
    jiim.cpp include/jiim.h
    load_config.cpp include/load_config.h
    load_entry_text.cpp include/load_entry_text.h
    load_params.cpp include/load_params.h
    main_menu.cpp include/main_menu.h
    main_menu_switch.cpp include/main_menu_switch.h
    include/main_state.h
    make_batch_file.cpp include/make_batch_file.h
    make_mig.cpp include/make_mig.h
    make_path.cpp include/make_path.h
    merge_path_names.cpp include/merge_path_names.h
    not_disk_msg.cpp include/not_disk_msg.h
    passes_options.cpp include/passes_options.h
    path_match.cpp include/path_match.h
    put_string_center.cpp include/put_string_center.h
    round_float_double.cpp include/round_float_double.h
    search_path.cpp include/search_path.h
    select_video_mode.cpp include/select_video_mode.h
    set_default_parms.cpp include/set_default_parms.h
    shell_sort.cpp include/shell_sort.h
    sound.cpp include/sound.h
    spindac.cpp include/spindac.h
    split_path.cpp include/split_path.h
    starfield.cpp include/starfield.h
    stop_msg.cpp include/stop_msg.h
    tab_display.cpp include/tab_display.h
    temp_msg.cpp include/temp_msg.h
    thinking.cpp include/thinking.h
    trig_fns.cpp include/trig_fns.h
    update_save_name.cpp include/update_save_name.h
    version.cpp include/version.h
    video_mode.cpp include/video_mode.h
    zoom.cpp include/zoom.h

    include/big.h
    include/biginit.h
    include/cmplx.h
    include/drivers.h
    include/fractype.h
    include/get_color.h
    include/get_line.h
    include/id.h
    include/id_data.h
    include/id_keys.h
    include/is_directory.h
    include/lsys.h
    include/mpmath.h
    include/out_line.h
    include/prototyp.h
    include/put_color_a.h
    include/read_ticker.h
    include/set_null_video.h
    include/sign.h
    include/stack_avail.h
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
source_group("Header Files/engine" FILES
    include/calc_frac_init.h
    include/calcfrac.h
    include/calcmand.h
    include/calmanfp.h
    include/fracsuba.h
    include/fractalb.h
    include/fractalp.h
    include/fractals.h
    include/fractype.h
    include/frasetup.h
    include/get_julia_attractor.h
    include/id_data.h
    include/orbit.h
    include/param_not_used.h
    include/pixel_grid.h
    include/resume.h
    include/soi.h
    include/ssg_block_size.h
    include/type_has_param.h
    include/wait_until.h
    include/work_list.h
)
source_group("Source Files/engine" FILES
    calc_frac_init.cpp
    calcfrac.cpp
    calcmand.cpp
    calmanfp.cpp
    fracsuba.cpp
    fractalb.cpp
    fractalp.cpp
    fractals.cpp
    frasetup.cpp
    get_julia_attractor.cpp
    orbit.cpp
    param_not_used.cpp
    pixel_grid.cpp
    resume.cpp
    soi.cpp
    soi1.cpp
    ssg_block_size.cpp
    type_has_param.cpp
    wait_until.cpp
    work_list.cpp
)
source_group("Header Files/fractal specific" FILES
    include/ant.h
    include/barnsley.h
    include/cellular.h
    include/circle_pattern.h
    include/escher.h
    include/fn_or_fn.h
    include/frothy_basin.h
    include/halley.h
    include/hypercomplex_mandelbrot.h
    include/ifs.h
    include/jb.h
    include/lambda_fn.h
    include/lorenz.h
    include/lsys.h
    include/lsysf.h
    include/lsys_fns.h
    include/magnet.h
    include/mandelbrot_mix.h
    include/miscfrac.h
    include/newton.h
    include/peterson_variations.h
    include/phoenix.h
    include/pickover_mandelbrot.h
    include/popcorn.h
    include/quartic_mandelbrot.h
    include/quaternion_mandelbrot.h
    include/sierpinski_gasket.h
    include/taylor_skinner_variations.h
    include/testpt.h
    include/unity.h
    include/volterra_lotka.h
)
source_group("Source Files/fractal specific" FILES
    ant.cpp
    barnsley.cpp
    cellular.cpp
    circle_pattern.cpp
    escher.cpp
    fn_or_fn.cpp
    frothy_basin.cpp
    halley.cpp
    hypercomplex_mandelbrot.cpp
    ifs.cpp
    jb.cpp
    lambda_fn.cpp
    lorenz.cpp
    lsys.cpp
    lsysf.cpp
    magnet.cpp
    mandelbrot_mix.cpp
    miscfrac.cpp
    newton.cpp
    peterson_variations.cpp
    phoenix.cpp
    pickover_mandelbrot.cpp
    popcorn.cpp
    quartic_mandelbrot.cpp
    quaternion_mandelbrot.cpp
    sierpinski_gasket.cpp
    taylor_skinner_variations.cpp
    testpt.cpp
    unity.cpp
    volterra_lotka.cpp
)
source_group("Header Files/IO" FILES
    include/cmdfiles.h
    include/decode_info.h
    include/decoder.h
    include/diskvid.h
    include/do_pause.h
    include/editpal.h
    include/encoder.h
    include/evolve.h
    include/get_video_mode.h
    include/gifview.h
    include/id_io.h
    include/loadfile.h
    include/loadmap.h
    include/parser.h
    include/rotate.h
    include/slideshw.h
    include/stereo.h
)
source_group("Source Files/IO" FILES
    cmdfiles.cpp
    decode_info.cpp
    decoder.cpp
    diskvid.cpp
    do_pause.cpp
    editpal.cpp
    encoder.cpp
    evolve.cpp
    get_video_mode.cpp
    gifview.cpp
    loadfile.cpp
    loadmap.cpp
    parser.cpp
    parserfp.cpp
    rotate.cpp
    slideshw.cpp
    stereo.cpp
)
source_group("Header Files/math" FILES
    include/big.h
    include/biginit.h
    include/cmplx.h
    include/fixed_pt.h
    include/fpu087.h
    include/hcmplx.h
    include/mpmath.h
    include/mpmath_c.h
    include/sign.h
)
source_group("Source Files/math" FILES
    bigflt.cpp
    biginit.cpp
    bignum.cpp
    bignumc.cpp
    fixed_pt.cpp
    fpu087.cpp
    hcmplx.cpp
    mpmath_c.cpp
)
source_group("Header Files/plumbing" FILES
    include/debug_flags.h
    include/drivers.h
    include/get_disk_space.h
    include/init_failure.h
    include/memory.h
)
source_group("Source Files/plumbing" FILES
    debug_flags.cpp
    drivers.cpp
    get_disk_space.cpp
    memory.cpp
)
source_group("Header Files/ui" FILES
    include/abort_msg.h
    include/bailout_formula.h
    include/check_key.h
    include/check_orbit_name.h
    include/check_write_file.h
    include/choice_builder.h
    include/comments.h
    include/convert_center_mag.h
    include/convert_corners.h
    include/dir_file.h
    include/double_to_string.h
    include/ends_with_slash.h
    include/engine_timer.h
    include/evolver_menu_switch.h
    include/expand_dirname.h
    include/extract_filename.h
    include/field_prompt.h
    include/file_gets.h
    include/find_extra_param.h
    include/find_file.h
    include/find_file_item.h
    include/find_path.h
    include/find_special_colors.h
    include/fix_dirname.h
    include/flip_image.h
    include/framain2.h
    include/full_screen_choice.h
    include/full_screen_prompt.h
    include/get_3d_params.h
    include/get_a_number.h
    include/get_browse_params.h
    include/get_a_filename.h
    include/get_calculation_time.h
    include/get_cmd_string.h
    include/get_commands.h
    include/get_corners.h
    include/get_file_entry.h
    include/get_file_entry.h
    include/get_fract_type.h
    include/get_key_no_help.h
    include/get_prec_big_float.h
    include/get_rds_params.h
    include/get_sound_params.h
    include/get_toggles.h
    include/get_toggles2.h
    include/get_view_params.h
    include/goodbye.h
    include/has_ext.h
    include/help.h
    include/help_title.h
    include/history.h
    include/id.h
    include/id_keys.h
    include/id_main.h
    include/input_field.h
    include/intro.h
    include/is_directory.h
    include/jiim.h
    include/load_config.h
    include/load_entry_text.h
    include/load_params.h
    include/main_menu.h
    include/main_menu_switch.h
    include/main_state.h
    include/make_batch_file.h
    include/make_mig.h
    include/make_path.h
    include/merge_path_names.h
    include/not_disk_msg.h
    include/passes_options.h
    include/path_match.h
    include/put_string_center.h
    include/round_float_double.h
    include/search_path.h
    include/select_video_mode.h
    include/set_default_parms.h
    include/shell_sort.h
    include/spindac.h
    include/split_path.h
    include/starfield.h
    include/stop_msg.h
    include/tab_display.h
    include/temp_msg.h
    include/text_screen.h
    include/thinking.h
    include/trig_fns.h
    include/update_save_name.h
    include/version.h
    include/video_mode.h
    include/zoom.h
)
source_group("Source Files/ui" FILES
    abort_msg.cpp
    bailout_formula.cpp
    check_key.cpp
    check_orbit_name.cpp
    check_write_file.cpp
    comments.cpp
    convert_center_mag.cpp
    convert_corners.cpp
    dir_file.cpp
    double_to_string.cpp
    ends_with_slash.cpp
    engine_timer.cpp
    evolver_menu_switch.cpp
    expand_dirname.cpp
    field_prompt.cpp
    file_gets.cpp
    find_extra_param.cpp
    find_file.cpp
    find_file_item.cpp
    find_path.cpp
    find_special_colors.cpp
    fix_dirname.cpp
    flip_image.cpp
    framain2.cpp
    full_screen_choice.cpp
    full_screen_prompt.cpp
    get_3d_params.cpp
    get_a_filename.cpp
    get_a_number.cpp
    get_browse_params.cpp
    get_calculation_time.cpp
    get_cmd_string.cpp
    get_commands.cpp
    get_corners.cpp
    get_file_entry.cpp
    get_file_entry.cpp
    get_fract_type.cpp
    get_key_no_help.cpp
    get_prec_big_float.cpp
    get_rds_params.cpp
    get_sound_params.cpp
    get_toggles.cpp
    get_toggles2.cpp
    get_view_params.cpp
    goodbye.cpp
    has_ext.cpp
    help.cpp
    help_title.cpp
    history.cpp
    id_main.cpp
    input_field.cpp
    intro.cpp
    jiim.cpp
    load_config.cpp
    load_entry_text.cpp
    load_params.cpp
    main_menu.cpp
    main_menu_switch.cpp
    make_batch_file.cpp
    make_mig.cpp
    make_path.cpp
    merge_path_names.cpp
    not_disk_msg.cpp
    passes_options.cpp
    path_match.cpp
    put_string_center.cpp
    round_float_double.cpp
    search_path.cpp
    select_video_mode.cpp
    set_default_parms.cpp
    shell_sort.cpp
    spindac.cpp
    split_path.cpp
    starfield.cpp
    stop_msg.cpp
    tab_display.cpp
    temp_msg.cpp
    text_screen.cpp
    thinking.cpp
    trig_fns.cpp
    update_save_name.cpp
    version.cpp
    video_mode.cpp
    zoom.cpp
)

set_src_dir(help.cpp)
set_src_dir(id_main.cpp)

target_compile_definitions(libid PUBLIC
    ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
# TODO: Figure out a better way to deal with converting packed structs on big-endian machines
set_source_files_properties(decode_info.cpp
    PROPERTIES
        COMPILE_FLAGS $<$<CXX_COMPILER_ID:GNU>:-Wno-address-of-packed-member>)
target_compile_options(libid PUBLIC ${ID_TARGET_OPTIONS})
target_include_directories(libid PUBLIC include)
target_link_libraries(libid PRIVATE help_defs helpcom os config Boost::boost)
target_link_libraries(libid PUBLIC config)
add_dependencies(libid native_help)
