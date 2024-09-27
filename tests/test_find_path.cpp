// SPDX-License-Identifier: GPL-3.0-only
//
#include <find_path.h>

#include <port.h>
#include <prototyp.h>

#include <cmdfiles.h>
#include <id_data.h>
#include <search_path.h>

#include "current_path_saver.h"
#include "test_data.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace
{

using GetEnv = std::function<const char *(const char *)>;

class TestFindPath : public ::testing::Test
{
public:
    ~TestFindPath() override = default;

protected:
    void SetUp() override;

    const char *m_non_existent{"goink.goink"};
    GetEnv m_empty_env{[](const char *) -> const char * { return nullptr; }};
    std::string m_path{std::string{ID_TEST_DATA_DIR} + PATH_SEPARATOR + ID_TEST_DATA_SUBDIR};
    GetEnv m_env_path{[&](const char *var) { return (std::string{var} == "PATH") ? m_path.c_str() : nullptr; }};
};

void TestFindPath::SetUp()
{
    Test::SetUp();
    g_check_cur_dir = false;
    g_fractal_search_dir1.clear();
    g_fractal_search_dir2.clear();
}

} // namespace

TEST_F(TestFindPath, fileFoundInCurrentDirectory)
{
    current_path_saver cd(ID_TEST_DATA_DIR);
    g_check_cur_dir = true;

    const std::string result = find_path(ID_TEST_IFS_FILE, m_empty_env);

    ASSERT_EQ((fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred(), fs::path{result});
}

TEST_F(TestFindPath, filePathFoundInCurrentDirectory)
{
    current_path_saver cd(ID_TEST_DATA_DIR);
    g_check_cur_dir = true;
    const fs::path relativeFile{fs::path{ID_TEST_DATA_SUBDIR}.filename() / ID_TEST_IFS_FILE};

    const std::string result = find_path(relativeFile.string().c_str(), m_empty_env);

    ASSERT_EQ((fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred(), fs::path{result});
}

TEST_F(TestFindPath, notFoundInCurrentDirectory)
{
    current_path_saver cd(ID_TEST_DATA_DIR);
    g_check_cur_dir = true;

    const std::string result = find_path(m_non_existent, m_empty_env);

    ASSERT_TRUE(result.empty());
}

TEST_F(TestFindPath, foundInSearchDir1)
{
    g_fractal_search_dir1 = ID_TEST_DATA_DIR;

    const std::string result = find_path(ID_TEST_IFS_FILE, m_empty_env);

    ASSERT_EQ((fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred(), fs::path{result});
}

TEST_F(TestFindPath, notFoundInSearchDir1)
{
    g_fractal_search_dir1 = ID_TEST_DATA_DIR;

    const std::string result = find_path(m_non_existent, m_empty_env);

    ASSERT_TRUE(result.empty());
}

TEST_F(TestFindPath, foundInSearchDir2)
{
    g_fractal_search_dir2 = ID_TEST_DATA_DIR;

    const std::string result = find_path(ID_TEST_IFS_FILE, m_empty_env);

    ASSERT_EQ((fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred(), fs::path{result});
}

TEST_F(TestFindPath, notFoundInSearchDir2)
{
    g_fractal_search_dir2 = ID_TEST_DATA_DIR;

    const std::string result = find_path(m_non_existent, m_empty_env);

    ASSERT_TRUE(result.empty());
}

TEST_F(TestFindPath, foundInPath)
{
    const std::string result = find_path(ID_TEST_IFS_FILE, m_env_path);

    ASSERT_EQ((fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred(), fs::path{result});
}

TEST_F(TestFindPath, foundInPathSubDir)
{
    const std::string result = find_path(ID_TEST_IFS_FILE2, m_env_path);

    ASSERT_EQ((fs::path{ID_TEST_DATA_SUBDIR} / ID_TEST_IFS_FILE2).make_preferred(), fs::path{result});
}

TEST_F(TestFindPath, notFoundInPath)
{
    const std::string result = find_path(m_non_existent, m_env_path);

    ASSERT_TRUE(result.empty());
}

TEST_F(TestFindPath, absolutePath)
{
    const fs::path absoluteLocation{(fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred()};
    ASSERT_TRUE(exists(absoluteLocation));

    const std::string result = find_path(absoluteLocation.string().c_str(), m_empty_env);

    ASSERT_EQ(absoluteLocation.string(), result);
}
