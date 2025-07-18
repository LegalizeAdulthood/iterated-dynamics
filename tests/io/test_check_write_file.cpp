// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/check_write_file.h>

#include "test_check_write_file_data.h"

#include <gtest/gtest.h>

using namespace id::test::check_write_file;

TEST(TestCheckWriteFile, newFile)
{
    std::string name{ID_TEST_CHECK_WRITE_FILE_NEW};

    check_write_file(name, ID_TEST_CHECK_WRITE_FILE_EXT);

    EXPECT_EQ(id::test::check_write_file::ID_TEST_CHECK_WRITE_FILE_NEW, name);
}

TEST(TestCheckWriteFile, existingFile2)
{
    std::string name{ID_TEST_CHECK_WRITE_FILE_EXISTS2};

    check_write_file(name, ID_TEST_CHECK_WRITE_FILE_EXT);

    EXPECT_EQ(id::test::check_write_file::ID_TEST_CHECK_WRITE_FILE3, name);
}

TEST(TestCheckWriteFile, existingFile1)
{
    std::string name{ID_TEST_CHECK_WRITE_FILE_EXISTS1};

    check_write_file(name, ID_TEST_CHECK_WRITE_FILE_EXT);

    EXPECT_EQ(id::test::check_write_file::ID_TEST_CHECK_WRITE_FILE3, name);
}

TEST(TestCheckWriteFile, existingFile1NoExtension)
{
    std::string name{ID_TEST_CHECK_WRITE_FILE_BASE1};

    check_write_file(name, ID_TEST_CHECK_WRITE_FILE_EXT);

    EXPECT_EQ(ID_TEST_CHECK_WRITE_FILE3, name);
}
