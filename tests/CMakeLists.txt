# SPDX-License-Identifier: GPL-3.0-only
#
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(ConfigureData)

add_executable(test-id
    ConfigureData.cmake
    test.cfg.in
    test.ifs.in
    "${ID_TEST_CONFIG_FILE}"
    "${ID_TEST_DATA_DIR}/${ID_TEST_IFS_FILE}"
    "${ID_TEST_DATA_SUBDIR}/${ID_TEST_IFS_FILE2}"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_check_write_file_data.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_config_data.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_data.h"
    current_path_saver.h
    test_config_data.h.in
    test_data.h.in
    test_check_write_file.cpp
    test_check_write_file_data.h.in
    test_choice_builder.cpp
    test_cmdfiles.cpp
    test_cmplx.cpp
    test_comments.cpp
    test_expand_dirname.cpp
    test_find_file.cpp
    test_find_file.txt.in
    test_find_path.cpp
    test_get_ifs_token.cpp
    test_gif_file.cpp
    test_is_directory.cpp
    test_load_config.cpp
    test_lowerize_parameter.cpp
    test_make_path.cpp
    test_math.cpp
    test_math_lcmplx.cpp
    test_merge_path_names.cpp
    test_open_save_file.cpp
    test_path_match.cpp
    test_search_path.cpp
    test_split_path.cpp
    test_trim_filename.cpp
    test_update_save_name.cpp
    test_value_saver.cpp
    test_version.cpp
)
source_group("CMake Scripts" REGULAR_EXPRESSION ".*\\.cmake$")
source_group("CMake Templates" REGULAR_EXPRESSION ".*\\.in$")
source_group("Test Data/IFS" REGULAR_EXPRESSION ".*\\.ifs$")
source_group("Test Data/Config" REGULAR_EXPRESSION ".*\\.cfg$")
target_include_directories(test-id PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test-id PRIVATE GTest::gmock_main libid)
set_target_properties(test-id PROPERTIES FOLDER Tests)
gtest_discover_tests(test-id PROPERTIES LABELS id)

add_subdirectory(images)
add_subdirectory(tools)
