#include <find_file.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <vector>

TEST(TestFindFile, firstTextFile)
{
    const int result = fr_findfirst((std::filesystem::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());

    ASSERT_EQ(0, result);
    ASSERT_EQ("find_file1.txt", DTA.filename);
    ASSERT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, secondTextFile)
{
    fr_findfirst((std::filesystem::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());

    const int second = fr_findnext();

    ASSERT_EQ(0, second);
    ASSERT_EQ("find_file2.txt", DTA.filename);
    ASSERT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, noMoreTextFiles)
{
    fr_findfirst((std::filesystem::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());
    fr_findnext();

    const int third = fr_findnext();

    ASSERT_EQ(-1, third);
}

TEST(TestFindFile, allFiles)
{
    const std::filesystem::path path{(std::filesystem::path(ID_TEST_DATA_FIND_FILE_DIR) / "*")};
    std::vector<int> results;
    results.push_back(fr_findfirst(path.string().c_str())); // "."
    results.push_back(fr_findnext());                       // ".."
    results.push_back(fr_findnext());                       // "find_file1.txt"
    results.push_back(fr_findnext());                       // "find_file2.txt"
                                                            //
    results.push_back(fr_findnext());                       // "subdir"

    ASSERT_EQ(results.end(), std::find_if(results.begin(), results.end(), [](int value) { return value != 0; }));
    ASSERT_EQ("subdir", DTA.filename);
    ASSERT_EQ(SUBDIR, DTA.attribute);
}
