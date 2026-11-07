# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
add_library(id-test-support OBJECT
    include/CommandTestFile.h test/CommandTestFile.cpp
    include/CommandParameterTest.h test/CommandParameterTest.cpp
    include/EnvVarSaver.h test/EnvVarSaver.cpp
    "include/expected_map.h"
    include/FailingOutputBuffer.h test/FailingOutputBuffer.cpp
    include/FindFractalInfoState.h
    include/FindFractalInfoTestSupport.h test/FindFractalInfoTestSupport.cpp
    include/FractalParamSaver.h test/FractalParamSaver.cpp
    include/GifReadSupport.h test/GifReadSupport.cpp
    include/ImageSaveTest.h test/ImageSaveTest.cpp
    include/MemoryDiskStorage.h test/MemoryDiskStorage.cpp
    include/NonStandardFractalTest.h test/NonStandardFractalTest.cpp
    "include/MockDriver.h"
    "include/MockSoundOutput.h"
    include/PassTestFrame.h test/PassTestFrame.cpp
    include/PendingDriverKey.h test/PendingDriverKey.cpp
    include/RestoreDecodeTestState.h
    include/StopMessageTestSupport.h test/StopMessageTestSupport.cpp
    "include/ValueUnchanged.h"
    "include/test_config_data.h.in"
    "include/test_data.h.in"
    "include/test_library.h.in"
    "include/test_migrate_gif.h.in"
    include/TemporaryDirectory.h test/TemporaryDirectory.cpp
    include/WorkListSnapshot.h test/WorkListSnapshot.cpp
    test/expected_map.cpp
)
target_compile_options(id-test-support PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")
target_include_directories(id-test-support PUBLIC
    include
    "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(id-test-support PUBLIC id::core GTest::gmock)
target_folder(id-test-support "Tests/Support")
if(ID_USE_WX)
    target_sources(id-test-support PRIVATE
        include/MockSpecialDirectories.h
        io/MockSpecialDirectories.cpp)
endif()

add_executable(test-core
    ../ConfigureData.cmake
    "home/save.cfg.in"
    "home/test.cfg.in"
    "home/test.frm.in"
    "home/test.ifs.in"
    "home/test.ini.in"
    "home/test.key.in"
    "home/test.l.in"
    "home/test.map.in"
    "home/test.par.in"
    "${ID_TEST_HOME_CONFIG_FILE}"
    "${ID_TEST_SAVE_CONFIG_FILE}"
    "${ID_TEST_FRM_DIR}/${ID_TEST_FRM_FILE}"
    "${ID_TEST_DATA_DIR}/${ID_TEST_IFS_FILE}"
    "${ID_TEST_DATA_SUBDIR}/${ID_TEST_IFS_FILE2}"
    "${ID_TEST_HOME_TOOLS_INI}"
    "${ID_TEST_MAP_DIR}/${ID_TEST_MAP_FILE}"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_check_write_file_data.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_config_data.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_data.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_library.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_migrate_gif.h"
    engine/test_bailout_formula.cpp
    engine/test_BoundaryTrace.cpp
    engine/test_calc_frac_init.cpp
    engine/test_color_utils.cpp
    engine/test_cmdfiles.cpp
    engine/test_Command.cpp
    engine/test_CommandHandler.cpp
    engine/test_CommandProcessor.cpp
    engine/test_Diffusion.cpp
    engine/test_get_prec_big_float.cpp
    engine/test_ImageRenderOperation.cpp
    engine/test_NonStandardFractal.cpp
    engine/test_InverseJulia.cpp
    engine/test_log_map.cpp
    engine/test_PertEngine.cpp
    engine/test_pixel.cpp
    engine/test_lowerize_parameter.cpp
    engine/test_random_seed.cpp
    engine/test_reset_version.cpp
    engine/test_resume.cpp
    engine/test_ScanlinePass.cpp
    engine/test_SolidGuess.cpp
    engine/test_SoundOutput.cpp
    engine/test_StandardPass.cpp
    engine/test_Tesseral.cpp
    engine/test_trig_fns.cpp
    fractals/test_ant.cpp
    fractals/test_bifurcation.cpp
    fractals/test_check_orbit_name.cpp
    fractals/test_frothy_basin.cpp
    fractals/test_fractalp.cpp
    fractals/test_get_ifs_token.cpp
    fractals/test_ifs.cpp
    fractals/test_interpreter.cpp
    fractals/test_julibrot.cpp
    fractals/test_lsystem.cpp
    fractals/test_lyapunov.cpp
    fractals/test_parser.cpp
    fractals/test_pickover_mandelbrot.cpp
    fractals/test_seeded_fractals.cpp
    geometry/test_StereoView.cpp
    io/test_check_write_file.cpp
    io/test_check_write_file_data.h.in
    io/test_disk_storage.cpp
    io/test_find_file.cpp
    io/test_find_file.txt.in
    io/test_find_path.cpp
    io/test_encoder.cpp
    io/test_EvolutionInfo.cpp
    io/test_GifRestore.cpp
    io/test_GifWriter.cpp
    io/test_gif_file.cpp
    io/test_gifview.cpp
    io/test_library.cpp
    io/test_loadfile.cpp
    io/test_loadmap.cpp
    io/test_load_config.cpp
    io/test_load_entry_text.cpp
    io/test_path_match.cpp
    io/test_RestoredImageOutput.cpp
    io/test_search_path.cpp
    io/test_trim_filename.cpp
    io/test_update_save_name.cpp
    math/test_bignum.cpp
    math/test_bigflt.cpp
    math/test_cmplx.cpp
    math/test_fpu087.cpp
    math/test_math.cpp
    math/test_round_float_double.cpp
    misc/test_driver.cpp
    misc/test_memory.cpp
    misc/test_ValueSaver.cpp
    misc/test_version.cpp
    test/test_CommandTestFile.cpp
    test/test_GifReadSupport.cpp
    test/test_TemporaryDirectory.cpp

)
file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/$<CONFIG>/test_executable_dir.h"
    CONTENT [[// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::test::data
{

constexpr const char *ID_TEST_PROGRAM_DIR{"$<TARGET_FILE_DIR:test-core>"};

} // namespace id::test::data
]])
if(NOT ID_USE_WX)
    target_sources(test-core PRIVATE io/test_special_dirs.cpp)
endif()
source_group("CMake Scripts" REGULAR_EXPRESSION ".*\\.cmake$")
source_group("CMake Templates" REGULAR_EXPRESSION ".*\\.in$")
source_group("Test Data/Color Maps" REGULAR_EXPRESSION ".*\\.map$")
source_group("Test Data/Config/Home" REGULAR_EXPRESSION ".*/home/.*\\.cfg$")
source_group("Test Data/Config/Save" REGULAR_EXPRESSION ".*/save/.*\\.cfg$")
source_group("Test Data/Formulas" REGULAR_EXPRESSION ".*\\.frm$")
source_group("Test Data/IFS" REGULAR_EXPRESSION ".*\\.ifs$")
source_group("Test Data/INI" REGULAR_EXPRESSION ".*\\.ini$")
source_group("Test Data/Library" REGULAR_EXPRESSION ".*/library/.*")
source_group_subdir("Engine" "engine")
source_group_subdir("Fractal Specific" "fractals")
source_group_subdir("IO" "io")
source_group_subdir("Math" "math")
source_group_subdir("Miscellaneous" "misc")
target_include_directories(test-core PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include/$<CONFIG>")
target_link_libraries(test-core PRIVATE id-test-support GTest::gmock_main)
target_folder(test-core "Tests")
gtest_discover_tests(test-core PROPERTIES LABELS id)
