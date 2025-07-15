// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/save_file.h>

#include "test_data.h"

#include <io/special_dirs.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

using namespace id::test::data;

TEST(TestSaveFile, getSaveName)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_DATA_DIR};

    const std::filesystem::path result{get_save_name("foo.par")};

    EXPECT_EQ(ID_TEST_DATA_DIR, result.parent_path().string());
    EXPECT_EQ("foo.par", result.filename());
}
