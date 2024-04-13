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
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_config_data.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_data.h"
    current_path_saver.h
    test_config_data.h.in
    test_data.h.in
    test_choice_builder.cpp
    test_cmplx.cpp
    test_expand_dirname.cpp
    test_find_file.cpp
    test_find_path.cpp
    test_get_ifs_token.cpp
    test_gif_file.cpp
    test_is_directory.cpp
    test_load_config.cpp
    test_make_path.cpp
    test_merge_path_names.cpp
    test_path_match.cpp
    test_search_path.cpp
    test_split_path.cpp
    test_update_save_name.cpp
    test_version.cpp
    test_find_file.txt.in
)
source_group("CMake Scripts" REGULAR_EXPRESSION ".*\\.cmake$")
source_group("CMake Templates" REGULAR_EXPRESSION ".*\\.in$")
source_group("Test Data/IFS" REGULAR_EXPRESSION ".*\\.ifs$")
source_group("Test Data/Config" REGULAR_EXPRESSION ".*\\.cfg$")
target_include_directories(test-id PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test-id PRIVATE GTest::gmock_main libid)
gtest_discover_tests(test-id)

add_subdirectory(tools)
