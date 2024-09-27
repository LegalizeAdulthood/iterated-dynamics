// SPDX-License-Identifier: GPL-3.0-only
//
#include <is_directory.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace fs = std::filesystem;

TEST(TestIsDirectory, affirmative)
{
    EXPECT_TRUE(isadirectory(ID_TEST_DATA_DIR));
    EXPECT_TRUE(isadirectory(ID_TEST_DATA_SUBDIR));
}

TEST(TestIsDirectory, negative)
{
    EXPECT_FALSE(isadirectory((fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).string().c_str()));
    EXPECT_FALSE(isadirectory((fs::path{ID_TEST_DATA_SUBDIR} / ID_TEST_IFS_FILE2).string().c_str()));
}
