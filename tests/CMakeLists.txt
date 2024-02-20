include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-id
    test_update_save_name.cpp
)
target_link_libraries(test-id PRIVATE GTest::gtest_main libid)

gtest_discover_tests(test-id)
