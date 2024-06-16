include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-hc
    test_help_source.cpp
    test_parse_options.cpp
)
source_group("CMake Scripts" REGULAR_EXPRESSION ".*\\.cmake$")
source_group("CMake Templates" REGULAR_EXPRESSION ".*\\.in$")
source_group("Test Data/IFS" REGULAR_EXPRESSION ".*\\.ifs$")
source_group("Test Data/Config" REGULAR_EXPRESSION ".*\\.cfg$")
#target_include_directories(test-hc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test-hc PRIVATE GTest::gmock_main libhc)
gtest_discover_tests(test-hc
    PROPERTIES LABELS hc)
