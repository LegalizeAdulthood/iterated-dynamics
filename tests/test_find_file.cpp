#include <find_file.h>

#include "current_path_saver.h"
#include "test_data.h"

#include <gmock/gmock.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace algo = boost::algorithm;

using namespace testing;

TEST(TestFindFile, firstTextFile)
{
    const int result = fr_findfirst((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());

    EXPECT_EQ(0, result);
    EXPECT_THAT(DTA.filename, AnyOf(StrEq(ID_TEST_FIND_FILE1), StrEq(ID_TEST_FIND_FILE2)));
    EXPECT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, secondTextFile)
{
    fr_findfirst((fs::path(ID_TEST_DATA_FIND_FILE_DIR) / "*.txt").string().c_str());
    std::vector<std::string> files;
    files.push_back(DTA.filename);

    const int second = fr_findnext();
    files.push_back(DTA.filename);

    EXPECT_EQ(0, second);
    EXPECT_THAT(files, AllOf(Contains(ID_TEST_FIND_FILE1), Contains(ID_TEST_FIND_FILE2)));
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

TEST(TestFindFile, findFileCurrentDirectory)
{
    current_path_saver saver{ID_TEST_DATA_FIND_FILE_DIR};

    const int result = fr_findfirst(ID_TEST_FIND_FILE1);

    EXPECT_EQ(0, result);
    EXPECT_EQ(ID_TEST_FIND_FILE1, DTA.filename);
    EXPECT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, caseInsensitiveExtension)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    current_path_saver saver{search_dir};

    const int result = fr_findfirst("*.txt");

    EXPECT_EQ(0, result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, DTA.filename);
    EXPECT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, caseInsensitiveExtensionWildcard)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    current_path_saver saver{search_dir};

    const int result = fr_findfirst("*.?xt");

    EXPECT_EQ(0, result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, DTA.filename);
    EXPECT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, caseInsensitiveExtensionWildcardRegex)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    current_path_saver saver{search_dir};

    const int result = fr_findfirst("*.?x*");

    EXPECT_EQ(0, result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, DTA.filename);
    EXPECT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, caseInsensitiveFilename)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    current_path_saver saver{search_dir};
    const std::string filename{algo::to_lower_copy(std::string{ID_TEST_FIND_FILE_CASE_FILENAME} + ".*")};

    const int result = fr_findfirst(filename.c_str());

    EXPECT_EQ(0, result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, DTA.filename);
    EXPECT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, caseInsensitiveFilenameWildcard)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    current_path_saver saver{search_dir};
    const std::string filename{
        '?' + algo::to_lower_copy(std::string{ID_TEST_FIND_FILE_CASE_FILENAME}).substr(1)};

    const int result = fr_findfirst(filename.c_str());

    EXPECT_EQ(0, result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, DTA.filename);
    EXPECT_EQ(0, DTA.attribute);
}

TEST(TestFindFile, caseInsensitiveFilenameWildcardRegex)
{
    fs::path search_dir{ID_TEST_DATA_FIND_FILE_DIR};
    search_dir /= ID_TEST_FIND_FILE_CASEDIR;
    current_path_saver saver{search_dir};
    const std::string filename{
        "?*" + algo::to_lower_copy(std::string{ID_TEST_FIND_FILE_CASE_FILENAME}).substr(2)};

    const int result = fr_findfirst(filename.c_str());

    EXPECT_EQ(0, result);
    EXPECT_EQ(ID_TEST_FIND_FILE_CASE, DTA.filename);
    EXPECT_EQ(0, DTA.attribute);
}
