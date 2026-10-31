// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/find_file.h>

#include <test_data.h>

#include <algos/string_algorithms.h>
#include <io/CurrentPathSaver.h>

#include <gmock/gmock.h>

#include <algorithm>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace id::algos;
using namespace id::io;
using namespace id::test::data;
using namespace testing;

namespace id::test
{

static void expect_case_variant_match(const bool result)
{
#if defined(_WIN32)
    EXPECT_TRUE(result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, g_dta.filename);
    EXPECT_EQ(0, g_dta.attribute);
#else
    EXPECT_FALSE(result);
#endif
}

TEST(TestFindFile, firstTextFile)
{
    const bool result = fr_find_first((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());

    EXPECT_TRUE(result);
    EXPECT_THAT(g_dta.filename, AnyOf(StrEq(ID_TEST_FIND_FILE1), StrEq(ID_TEST_FIND_FILE2)));
    EXPECT_EQ(0, g_dta.attribute);
}

class TestFindFileTextSearch : public Test
{
protected:
    void SetUp() override;

    std::string m_first_filename;
};

void TestFindFileTextSearch::SetUp()
{
    Test::SetUp();
    fr_find_first((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());
    m_first_filename = g_dta.filename;
}

TEST_F(TestFindFileTextSearch, secondTextFile)
{
    std::vector<std::string> files{m_first_filename};

    const bool second = fr_find_next();
    files.push_back(g_dta.filename);

    EXPECT_TRUE(second);
    EXPECT_THAT(files, AllOf(Contains(ID_TEST_FIND_FILE1), Contains(ID_TEST_FIND_FILE2)));
}

TEST_F(TestFindFileTextSearch, noMoreTextFiles)
{
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

struct FileCasePattern
{
    const char *name;
    std::string pattern;
};

void PrintTo(const FileCasePattern &value, std::ostream *str)
{
    *str << value.name;
}

class TestFindFileCaseMatch : public TestWithParam<FileCasePattern>
{
private:
    CurrentPathSaver m_current_directory{fs::path{ID_TEST_DATA_FIND_FILE_DIR} / ID_TEST_FIND_FILE_CASEDIR};
};

TEST_P(TestFindFileCaseMatch, matchesCaseVariantPattern)
{
    const bool result{fr_find_first(GetParam().pattern.c_str())};

    expect_case_variant_match(result);
}

INSTANTIATE_TEST_SUITE_P(CasePatterns, TestFindFileCaseMatch,
    Values(FileCasePattern{"Extension", "*.txt"}, FileCasePattern{"ExtensionWildcard", "*.?xt"},
        FileCasePattern{"ExtensionWildcardRegex", "*.?x*"},
        FileCasePattern{"Filename", ascii_to_lower_copy(std::string{ID_TEST_FIND_FILE_CASE_FILENAME} + ".*")},
        FileCasePattern{
            "FilenameWildcard", '?' + ascii_to_lower_copy(std::string{ID_TEST_FIND_FILE_CASE_FILENAME}).substr(1)},
        FileCasePattern{"FilenameWildcardRegex",
            "?*" + ascii_to_lower_copy(std::string{ID_TEST_FIND_FILE_CASE_FILENAME}).substr(2)}),
    [](const TestParamInfo<FileCasePattern> &info) { return info.param.name; });

} // namespace id::test
