include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

set(ID_TEST_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_data")
configure_file(test_data.h.in include/test_data.h @ONLY)
configure_file(test.ifs.in "${ID_TEST_DATA_DIR}/test.ifs")

add_executable(test-id
    test.ifs.in
    "${ID_TEST_DATA_DIR}/test.ifs"
    "${CMAKE_CURRENT_BINARY_DIR}/include/test_data.h"
    test_data.h.in
    test_get_ifs_token.cpp
    test_update_save_name.cpp
)
source_group("CMake Templates" REGULAR_EXPRESSION ".*\\.in$")
source_group("Test Data/IFS" REGULAR_EXPRESSION ".*\\.ifs$")
target_include_directories(test-id PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test-id PRIVATE GTest::gtest_main libid)

gtest_discover_tests(test-id)
