# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
include(GoogleTest)

find_package(boost_endian CONFIG REQUIRED)
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
add_subdirectory(core)
add_subdirectory(ui)
