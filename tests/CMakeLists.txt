include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

set(ID_TEST_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_data")

SET(ID_TEST_FIRST_IFS_NAME "binary")
set(ID_TEST_FIRST_IFS_PARAM1 ".5")
set(ID_TEST_SECOND_IFS_NAME "3dfern")
set(ID_TEST_IFS_FILE "test.ifs")
configure_file(test.ifs.in "${ID_TEST_DATA_DIR}/${ID_TEST_IFS_FILE}")

set(ID_TEST_DATA_SUBDIR_NAME "subdir")
set(ID_TEST_DATA_SUBDIR "${ID_TEST_DATA_DIR}/${ID_TEST_DATA_SUBDIR_NAME}")
set(ID_TEST_IFS_FILE2 "test2.ifs")
configure_file(test.ifs.in "${ID_TEST_DATA_SUBDIR}/${ID_TEST_IFS_FILE2}")

set(ID_TEST_FIND_FILE_SUBDIR test_data/find_file)
set(ID_TEST_DATA_FIND_FILE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ID_TEST_FIND_FILE_SUBDIR}")
configure_file(test_find_file.txt.in ${ID_TEST_FIND_FILE_SUBDIR}/find_file1.txt)
configure_file(test_find_file.txt.in ${ID_TEST_FIND_FILE_SUBDIR}/find_file2.txt)
configure_file(test_find_file.txt.in ${ID_TEST_FIND_FILE_SUBDIR}/subdir/find_file3.txt)

configure_file(test_data.h.in include/test_data.h @ONLY)

add_executable(test-id
    test.ifs.in
    "${ID_TEST_DATA_DIR}/test.ifs"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_data.h"
    current_path_saver.h
    test_data.h.in
    test_expand_dirname.cpp
    test_find_file.cpp
    test_find_path.cpp
    test_get_ifs_token.cpp
    test_is_directory.cpp
    test_make_path.cpp
    test_merge_path_names.cpp
    test_path_match.cpp
    test_search_path.cpp
    test_split_path.cpp
    test_update_save_name.cpp
    test_find_file.txt.in
)
source_group("CMake Templates" REGULAR_EXPRESSION ".*\\.in$")
source_group("Test Data/IFS" REGULAR_EXPRESSION ".*\\.ifs$")
target_include_directories(test-id PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test-id PRIVATE GTest::gtest_main libid)

gtest_discover_tests(test-id)
