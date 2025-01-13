# SPDX-License-Identifier: GPL-3.0-only
#
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-config
    test_string_case_compare.cpp
)
target_link_libraries(test-config PRIVATE GTest::gmock_main config)
set_target_properties(test-config PROPERTIES FOLDER Tests)
gtest_discover_tests(test-config PROPERTIES LABELS config)
