# SPDX-License-Identifier: GPL-3.0-only
#
include(GoogleTest)

find_package(boost_algorithm CONFIG REQUIRED)
find_package(boost_endian CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(ConfigureData)

# A note about test cases for Id:
#
# Id has a lot of global and static data.  Static data is just
# another name for global data that is inaccessible outside its
# declared scope.
#
# Because of this, it is unreliable to run some test cases
# concurrently within a single process.  This isn't a problem
# if the test cases are run via CTest, as ctest invokes the
# test executable once per test case and therefore each test
# case has it's own copy of the global and static data.
#
# When running test cases from within an IDE, such as Visual
# Studio, it can't be guaranteed that a separate process will
# be used for each test case.  Therefore you may see a situation
# whereby tests fail when run as a group, but pass when run
# individually.
#
# When in doubt, use CTest to run the test cases to verify
# that they are indeed passing as expected.
#
# Over time, it is hoped that the dependence on global and
# static data will lessen and it will be possible to run
# tests (and production code) concurrently on multiple threads
# within a single process without interference.
#
add_executable(test-id
    ConfigureData.cmake
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
    "include/expected_map.h"
    "include/MockDriver.h"
    "include/ValueUnchanged.h"
    "include/test_config_data.h.in"
    "include/test_data.h.in"
    "include/test_library.h.in"
    "include/test_migrate_gif.h.in"
    expected_map.cpp
    engine/test_cmdfiles.cpp
    engine/test_get_prec_big_float.cpp
    engine/test_log_map.cpp
    engine/test_resume.cpp
    fractals/test_check_orbit_name.cpp
    fractals/test_fractalp.cpp
    fractals/test_get_ifs_token.cpp
    io/test_check_write_file.cpp
    io/test_check_write_file_data.h.in
    io/test_expand_dirname.cpp
    io/test_find_file.cpp
    io/test_find_file.txt.in
    io/test_find_path.cpp
    io/test_gif_file.cpp
    io/test_is_directory.cpp
    io/test_library.cpp
    io/test_loadfile.cpp
    io/test_loadmap.cpp
    io/test_load_config.cpp
    io/test_load_entry_text.cpp
    io/test_locate_input_file.cpp
    io/test_make_path.cpp
    io/test_merge_path_names.cpp
    io/test_path_match.cpp
    io/test_search_path.cpp
    io/test_split_path.cpp
    io/test_trim_filename.cpp
    io/test_update_save_name.cpp
    math/test_cmplx.cpp
    math/test_fpu087.cpp
    math/test_math.cpp
    misc/driver_types.cpp
    misc/test_driver.cpp
    misc/test_ValueSaver.cpp
    misc/test_version.cpp
    ui/test_ChoiceBuilder.cpp
    ui/test_comments.cpp
    ui/test_file_item.cpp
    ui/test_lowerize_parameter.cpp
    ui/test_main_menu_switch.cpp
    ui/test_make_batch_file.cpp
    ui/test_video_mode.cpp
)
if(ID_USE_WX)
    target_sources(test-id PRIVATE
        include/MockSpecialDirectories.h
        io/MockSpecialDirectories.cpp
        ui/init_failure.cpp)
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
source_group_subdir("UI" "ui")
target_include_directories(test-id PUBLIC include "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test-id PRIVATE GTest::gmock_main Boost::algorithm Boost::endian libid)
target_folder(test-id "Tests")
gtest_discover_tests(test-id PROPERTIES LABELS id)
