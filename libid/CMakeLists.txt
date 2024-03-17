add_library(libid
    include/os.h

    3d.cpp include/3d.h
    line3d.cpp include/line3d.h
    plot3d.cpp include/plot3d.h

    calcfrac.cpp include/calcfrac.h
    calcmand.cpp include/calcmand.h
    calmanfp.cpp include/calmanfp.h
    fracsuba.cpp include/fracsuba.h
    fracsubr.cpp include/fracsubr.h
    fractalb.cpp include/fractalb.h
    fractalp.cpp include/fractalp.h
    fractals.cpp include/fractals.h
    frasetup.cpp include/frasetup.h
    soi.cpp include/soi.h
    soi1.cpp
    testpt.cpp include/testpt.h

    ant.cpp include/ant.h
    jb.cpp include/jb.h
    lorenz.cpp include/lorenz.h
    lsys.cpp include/lsys_fns.h
    lsysf.cpp
    miscfrac.cpp include/miscfrac.h

    cmdfiles.cpp include/cmdfiles.h
    decoder.cpp include/decoder.h
    diskvid.cpp include/diskvid.h
    editpal.cpp include/editpal.h
    encoder.cpp include/encoder.h
    evolve.cpp include/evolve.h
    gifview.cpp include/gifview.h
    loadfdos.cpp include/loadfdos.h
    loadfile.cpp include/loadfile.h
    loadmap.cpp include/loadmap.h
    parser.cpp include/parser.h
    parserfp.cpp
    rotate.cpp include/rotate.h
    slideshw.cpp include/slideshw.h
    stereo.cpp include/stereo.h

    bigflt.cpp
    biginit.cpp include/biginit.h
    bignum.cpp
    bignumc.cpp
    fpu087.cpp include/fpu087.h
    hcmplx.cpp include/hcmplx.h
    mpmath_c.cpp include/mpmath_c.h

    drivers.cpp
    memory.cpp include/memory.h

    abort_msg.cpp include/abort_msg.h
    include/choice_builder.h
    double_to_string.cpp include/double_to_string.h
    evolver_menu_switch.cpp include/evolver_menu_switch.h
    expand_dirname.cpp include/expand_dirname.h
    find_file.cpp include/find_file.h
    find_path.cpp include/find_path.h
    find_special_colors.cpp include/find_special_colors.h
    fix_dirname.cpp include/fix_dirname.h
    fractint.cpp include/fractint.h
    framain2.cpp include/framain2.h
    full_screen_prompt.cpp include/full_screen_prompt.h
    get_3d_params.cpp include/get_3d_params.h
    get_browse_params.cpp include/get_browse_params.h
    include/get_ifs_token.h
    get_key_no_help.cpp include/get_key_no_help.h
    get_rds_params.cpp include/get_rds_params.h
    get_toggles.cpp include/get_toggles.h
    help.cpp
    history.cpp include/history.h
    intro.cpp include/intro.h
    include/is_directory.h
    jiim.cpp include/jiim.h
    load_entry_text.cpp include/load_entry_text.h
    make_batch_file.cpp include/make_batch_file.h
    include/make_path.h
    merge_path_names.cpp include/merge_path_names.h
    miscovl.cpp include/miscovl.h
    miscres.cpp include/miscres.h
    path_match.cpp include/path_match.h
    prompts1.cpp include/prompts1.h
    prompts2.cpp include/prompts2.h
    realdos.cpp include/realdos.h
    search_path.cpp include/search_path.h
    spindac.cpp include/spindac.h
    split_path.cpp include/split_path.h
    update_save_name.cpp include/update_save_name.h
    zoom.cpp include/zoom.h

    include/big.h
    include/biginit.h
    include/cmplx.h
    include/drivers.h
    include/fractype.h
    include/id.h
    include/id_data.h
    include/lsys.h
    include/mpmath.h
    include/newton.h
    include/prototyp.h
    include/winprot.h
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
    include/calcfrac.h
    include/calcmand.h
    include/calmanfp.h
    include/fracsuba.h
    include/fracsubr.h
    include/fractalb.h
    include/fractalp.h
    include/fractals.h
    include/fractype.h
    include/frasetup.h
    include/id_data.h
    include/soi.h
    include/testpt.h
)
source_group("Source Files/engine" FILES
    calcfrac.cpp
    calcmand.cpp
    calmanfp.cpp
    fracsuba.cpp
    fracsubr.cpp
    fractalb.cpp
    fractalp.cpp
    fractals.cpp
    frasetup.cpp
    soi.cpp
    soi1.cpp
    testpt.cpp
)
source_group("Header Files/fractal specific" FILES
    include/ant.h
    include/jb.h
    include/lorenz.h
    include/lsys.h
    include/lsysf.h
    include/lsys_fns.h
    include/miscfrac.h
    include/newton.h
)
source_group("Source Files/fractal specific" FILES
    ant.cpp
    jb.cpp
    lorenz.cpp
    lsys.cpp
    lsysf.cpp
    miscfrac.cpp
)
source_group("Header Files/IO" FILES
    include/cmdfiles.h
    include/decoder.h
    include/diskvid.h
    include/editpal.h
    include/encoder.h
    include/evolve.h
    include/gifview.h
    include/id_io.h
    include/loadfdos.h
    include/loadfile.h
    include/loadmap.h
    include/parser.h
    include/rotate.h
    include/slideshw.h
    include/stereo.h
)
source_group("Source Files/IO" FILES
    cmdfiles.cpp
    decoder.cpp
    diskvid.cpp
    editpal.cpp
    encoder.cpp
    evolve.cpp
    gifview.cpp
    loadfdos.cpp
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
    include/fpu087.h
    include/hcmplx.h
    include/mpmath.h
    include/mpmath_c.h
)
source_group("Source Files/math" FILES
    bigflt.cpp
    biginit.cpp
    bignum.cpp
    bignumc.cpp
    fpu087.cpp
    hcmplx.cpp
    mpmath_c.cpp
)
source_group("Header Files/plumbing" FILES
    include/drivers.h
    include/memory.h
)
source_group("Source Files/plumbing" FILES
    drivers.cpp
    memory.cpp
)
source_group("Header Files/ui" FILES
    include/abort_msg.h
    include/choice_builder.h
    include/double_to_string.h
    include/evolver_menu_switch.h
    include/expand_dirname.h
    include/find_file.h
    include/find_path.h
    include/find_special_colors.h
    include/fix_dirname.h
    include/fractint.h
    include/framain2.h
    include/full_screen_prompt.h
    include/get_3d_params.h
    include/get_browse_params.h
    include/get_ifs_token.h
    include/get_key_no_help.h
    include/get_rds_params.h
    include/get_toggles.h
    include/help.h
    include/history.h
    include/id.h
    include/intro.h
    include/is_directory.h
    include/jiim.h
    include/load_entry_text.h
    include/make_batch_file.h
    include/make_path.h
    include/merge_path_names.h
    include/miscovl.h
    include/miscres.h
    include/path_match.h
    include/prompts1.h
    include/prompts2.h
    include/realdos.h
    include/search_path.h
    include/spindac.h
    include/split_path.h
    include/update_save_name.h
    include/zoom.h
)
source_group("Source Files/ui" FILES
    abort_msg.cpp
    double_to_string.cpp
    evolver_menu_switch.cpp
    expand_dirname.cpp
    find_file.cpp
    find_path.cpp
    find_special_colors.cpp
    fix_dirname.cpp
    fractint.cpp
    framain2.cpp
    full_screen_prompt.cpp
    get_3d_params.cpp
    get_browse_params.cpp
    get_key_no_help.cpp
    get_rds_params.cpp
    get_toggles.cpp
    help.cpp
    history.cpp
    intro.cpp
    jiim.cpp
    load_entry_text.cpp
    make_batch_file.cpp
    merge_path_names.cpp
    miscovl.cpp
    miscres.cpp
    path_match.cpp
    prompts1.cpp
    prompts2.cpp
    realdos.cpp
    search_path.cpp
    spindac.cpp
    split_path.cpp
    update_save_name.cpp
    zoom.cpp
)

set_src_dir(help.cpp)
set_src_dir(fractint.cpp)

target_compile_definitions(libid PUBLIC ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(libid PUBLIC ${ID_TARGET_OPTIONS})
target_include_directories(libid PUBLIC include)
target_link_libraries(libid PRIVATE help_defs helpcom os config Boost::boost)
target_link_libraries(libid PUBLIC config)
add_dependencies(libid native_help)
