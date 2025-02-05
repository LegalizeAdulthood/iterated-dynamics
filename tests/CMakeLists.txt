# SPDX-License-Identifier: GPL-3.0-only
#
include(GoogleTest)

find_package(Boost REQUIRED)
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
    home/test.cfg.in
    home/test.ifs.in
    "${ID_TEST_CONFIG_FILE}"
    "${ID_TEST_DATA_DIR}/${ID_TEST_IFS_FILE}"
    "${ID_TEST_DATA_SUBDIR}/${ID_TEST_IFS_FILE2}"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_check_write_file_data.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_config_data.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_data.h"
    include/CurrentPathSaver.h
    include/MockDriver.h
    include/ValueUnchanged.h
    include/test_config_data.h.in
    include/test_data.h.in
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
    io/test_load_config.cpp
    io/test_load_entry_text.cpp
    io/test_make_path.cpp
    io/test_merge_path_names.cpp
    io/test_open_save_file.cpp
    io/test_path_match.cpp
    io/test_save_file.cpp
    io/test_search_path.cpp
    io/test_split_path.cpp
    io/test_trim_filename.cpp
    io/test_update_save_name.cpp
    math/test_cmplx.cpp
    math/test_math.cpp
    math/test_math_lcmplx.cpp
    misc/test_driver.cpp
    misc/test_ValueSaver.cpp
    misc/test_version.cpp
    ui/test_ChoiceBuilder.cpp
    ui/test_cmdfiles.cpp
    ui/test_comments.cpp
    ui/test_lowerize_parameter.cpp
    ui/test_main_menu_switch.cpp
    ui/test_video_mode.cpp
)
source_group("CMake Scripts" REGULAR_EXPRESSION ".*\\.cmake$")
source_group("CMake Templates" REGULAR_EXPRESSION ".*\\.in$")
source_group("Test Data/IFS" REGULAR_EXPRESSION ".*\\.ifs$")
source_group("Test Data/Config" REGULAR_EXPRESSION ".*\\.cfg$")
source_group_subdir("Engine" "engine")
source_group_subdir("Fractal Specific" "fractals")
source_group_subdir("IO" "io")
source_group_subdir("Math" "math")
source_group_subdir("Miscellaneous" "misc")
source_group_subdir("UI" "ui")
target_include_directories(test-id PUBLIC include "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test-id PRIVATE GTest::gmock_main Boost::boost libid)
set_target_properties(test-id PROPERTIES FOLDER Tests)
gtest_discover_tests(test-id PROPERTIES LABELS id)

add_subdirectory(images)
add_subdirectory(tools)
