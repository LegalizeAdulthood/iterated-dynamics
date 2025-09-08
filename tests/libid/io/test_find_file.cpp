// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/find_file.h>

#include "test_data.h"

#include <io/CurrentPathSaver.h>

#include <gmock/gmock.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace algo = boost::algorithm;

using namespace id::io;
using namespace id::test::data;
using namespace testing;

namespace id::test
{

TEST(TestFindFile, firstTextFile)
{
    const bool result = fr_find_first((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());

    EXPECT_TRUE(result);
    EXPECT_THAT(g_dta.filename, AnyOf(StrEq(ID_TEST_FIND_FILE1), StrEq(ID_TEST_FIND_FILE2)));
    EXPECT_EQ(0, g_dta.attribute);
}

TEST(TestFindFile, secondTextFile)
{
    fr_find_first((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());
    std::vector<std::string> files;
    files.push_back(g_dta.filename);

    const bool second = fr_find_next();
    files.push_back(g_dta.filename);

    EXPECT_TRUE(second);
    EXPECT_THAT(files, AllOf(Contains(ID_TEST_FIND_FILE1), Contains(ID_TEST_FIND_FILE2)));
}

TEST(TestFindFile, noMoreTextFiles)
{
    fr_find_first((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());
    fr_find_next();

    const bool third = fr_find_next();

    EXPECT_FALSE(third);
}

TEST(TestFindFile, noMatchingFiles)
{
    const bool result = fr_find_first((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.goink").string().c_str());

    EXPECT_FALSE(result);
}

TEST(TestFindFile, findSubDir)
{
    const fs::path path{(fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*")};
    bool result = fr_find_first(path.string().c_str());
    const std::string subdir{fs::path{ID_TEST_DATA_SUBDIR}.filename().string()};

    while (result && g_dta.filename != subdir)
    {
        result = fr_find_next();
    }

    EXPECT_TRUE(result);
    EXPECT_EQ(subdir, g_dta.filename);
    EXPECT_EQ(SUB_DIR, g_dta.attribute);
}

TEST(TestFindFile, findFileCurrentDirectory)
{
    CurrentPathSaver saver{ID_TEST_DATA_FIND_FILE_DIR};

    const bool result = fr_find_first(ID_TEST_FIND_FILE1);

    EXPECT_TRUE(result);
    EXPECT_EQ(ID_TEST_FIND_FILE1, g_dta.filename);
    EXPECT_EQ(0, g_dta.attribute);
}

TEST(TestFindFile, caseInsensitiveExtension)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    CurrentPathSaver saver{search_dir};

    const bool result = fr_find_first("*.txt");

    EXPECT_TRUE(result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, g_dta.filename);
    EXPECT_EQ(0, g_dta.attribute);
}

TEST(TestFindFile, caseInsensitiveExtensionWildcard)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    CurrentPathSaver saver{search_dir};

    const bool result = fr_find_first("*.?xt");

    EXPECT_TRUE(result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, g_dta.filename);
    EXPECT_EQ(0, g_dta.attribute);
}

TEST(TestFindFile, caseInsensitiveExtensionWildcardRegex)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    CurrentPathSaver saver{search_dir};

    const bool result = fr_find_first("*.?x*");

    EXPECT_TRUE(result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, g_dta.filename);
    EXPECT_EQ(0, g_dta.attribute);
}

TEST(TestFindFile, caseInsensitiveFilename)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    CurrentPathSaver saver{search_dir};
    const std::string filename{algo::to_lower_copy(std::string{ID_TEST_FIND_FILE_CASE_FILENAME} + ".*")};

    const bool result = fr_find_first(filename.c_str());

    EXPECT_TRUE(result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, g_dta.filename);
    EXPECT_EQ(0, g_dta.attribute);
}

TEST(TestFindFile, caseInsensitiveFilenameWildcard)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    CurrentPathSaver saver{search_dir};
    const std::string filename{
        '?' + algo::to_lower_copy(std::string{ID_TEST_FIND_FILE_CASE_FILENAME}).substr(1)};

    const bool result = fr_find_first(filename.c_str());

    EXPECT_TRUE(result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, g_dta.filename);
    EXPECT_EQ(0, g_dta.attribute);
}

TEST(TestFindFile, caseInsensitiveFilenameWildcardRegex)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    CurrentPathSaver saver{search_dir};
    const std::string filename{
        "?*" + algo::to_lower_copy(std::string{ID_TEST_FIND_FILE_CASE_FILENAME}).substr(2)};

    const bool result = fr_find_first(filename.c_str());

    EXPECT_TRUE(result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, g_dta.filename);
    EXPECT_EQ(0, g_dta.attribute);
}

} // namespace id::test
