# SPDX-License-Identifier: GPL-3.0-only
#
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-hc
    test_compiler.cpp
    test_help_source.cpp
    test_parse_options.cpp
)
source_group("CMake Scripts" REGULAR_EXPRESSION ".*\\.cmake$")
source_group("CMake Templates" REGULAR_EXPRESSION ".*\\.in$")
target_include_directories(test-hc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test-hc PRIVATE GTest::gmock_main libhc)
set_target_properties(test-hc PROPERTIES FOLDER Tests)
gtest_discover_tests(test-hc PROPERTIES LABELS hc)

add_subdirectory(ascii_doc)
