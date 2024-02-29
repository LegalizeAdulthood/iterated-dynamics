#include <find_file.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

TEST(TestFindFile, firstTextFile)
{
    const int result = fr_findfirst((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());

    EXPECT_EQ(0, result);
    EXPECT_EQ("find_file1.txt", DTA.filename);
    EXPECT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, secondTextFile)
{
    fr_findfirst((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());

    const int second = fr_findnext();

    EXPECT_EQ(0, second);
    EXPECT_EQ("find_file2.txt", DTA.filename);
    EXPECT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, noMoreTextFiles)
{
    fr_findfirst((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());
    fr_findnext();

    const int third = fr_findnext();

    EXPECT_EQ(-1, third);
}

TEST(TestFindFile, noMatchingFiles)
{
    const int result = fr_findfirst((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.goink").string().c_str());

    EXPECT_EQ(1, result);
}

TEST(TestFindFile, findSubDir)
{
    const fs::path path{(fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*")};
    int result = fr_findfirst(path.string().c_str());
    const std::string subdir{fs::path{ID_TEST_DATA_SUBDIR}.filename().string()};

    while (result == 0 && DTA.filename != subdir)
    {
        result = fr_findnext();
    }
    
    EXPECT_EQ(0, result);
    EXPECT_EQ(subdir, DTA.filename);
    EXPECT_EQ(SUBDIR, DTA.attribute);
}
