# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-algos
    test_string_algorithms.cpp)
target_link_libraries(test-algos PRIVATE GTest::gmock_main algos)
target_folder(test-algos "Tests")
gtest_discover_tests(test-algos PROPERTIES LABELS algos)
