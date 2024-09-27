// SPDX-License-Identifier: GPL-3.0-only
//
#include <merge_path_names.h>

#include "current_path_saver.h"
#include "test_data.h"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{

class TestMergePathNames : public ::testing::Test
{
public:
    ~TestMergePathNames() override = default;

protected:
    void SetUp() override;

    std::string m_path;
    cmd_file m_mode{cmd_file::AT_CMD_LINE};
    fs::path m_existing_dir{ID_TEST_DATA_DIR};
    fs::path m_non_existing_dir{m_existing_dir / "goink"};
};

void TestMergePathNames::SetUp()
{
    m_existing_dir.make_preferred();
    m_non_existing_dir.make_preferred();
}

} // namespace

TEST_F(TestMergePathNames, basicGetPath)
{
    const int result = merge_pathnames(m_path, "new_filename.par", m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ("new_filename.par", m_path);
}

TEST_F(TestMergePathNames, replaceExistingFilenameGetPath)
{
    m_path = "old_filename.par";

    const int result = merge_pathnames(m_path, "new_filename.par", m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ("new_filename.par", m_path);
}

TEST_F(TestMergePathNames, setParentDirNewFilenameForExistingDirGetPath)
{
    m_path = m_existing_dir.string();

    const int result = merge_pathnames(m_path, "new_filename.par", m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ((m_existing_dir.parent_path() / "new_filename.par").make_preferred(), m_path);
}

TEST_F(TestMergePathNames, setParentDirNewFilenameForNonExistingDirGetPath)
{
    m_path = m_non_existing_dir.string();

    const int result = merge_pathnames(m_path, "new_filename.par", m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ((m_non_existing_dir.parent_path() / "new_filename.par").make_preferred(), m_path);
}

TEST_F(TestMergePathNames, newPathIsDirectoryGetPath)
{
    m_path = fs::path{ID_TEST_IFS_FILE}.string();

    const int result = merge_pathnames(m_path, fs::path{ID_TEST_DATA_SUBDIR}.make_preferred().string().c_str(), m_mode);

    EXPECT_EQ(1, result);
    EXPECT_EQ((fs::path{ID_TEST_DATA_SUBDIR} / ID_TEST_IFS_FILE).make_preferred().string(), m_path);
}

TEST_F(TestMergePathNames, expandsDotSlashGetPath)
{
    current_path_saver saver{ID_TEST_DATA_DIR};
    const std::string new_filename{fs::path{"./"}.make_preferred().string()};

    const int result = merge_pathnames(m_path, new_filename.c_str(), m_mode);

    EXPECT_EQ(1, result);
    EXPECT_EQ(fs::path{ID_TEST_DATA_DIR}.make_preferred().string() + SLASHC, m_path);
}

TEST_F(TestMergePathNames, expandsDotSlashSubDirGetPath)
{
    current_path_saver saver{ID_TEST_DATA_DIR};
    const std::string new_filename{fs::path{"./" ID_TEST_DATA_SUBDIR_NAME}.make_preferred().string()};

    const int result = merge_pathnames(m_path, new_filename.c_str(), m_mode);

    EXPECT_EQ(1, result);
    EXPECT_EQ(fs::path{ID_TEST_DATA_SUBDIR}.make_preferred().string() + SLASHC, m_path);
}

TEST_F(TestMergePathNames, fileInCurrentDirectoryGetPath)
{
    current_path_saver saver{ID_TEST_DATA_DIR};

    const int result = merge_pathnames(m_path, ID_TEST_IFS_FILE, m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ(ID_TEST_IFS_FILE, m_path);
}

TEST_F(TestMergePathNames, directoryGetPath)
{
    const std::string new_filename{fs::path{ID_TEST_DATA_DIR}.make_preferred().string()};

    const int result = merge_pathnames(m_path, new_filename.c_str(), m_mode);

    EXPECT_EQ(1, result);
    EXPECT_EQ(new_filename + SLASHC, m_path);
}

TEST_F(TestMergePathNames, replaceDirectoryGetPath)
{
    m_path = fs::path{ID_TEST_DATA_SUBDIR}.make_preferred().string();
    const std::string new_filename{fs::path{ID_TEST_DATA_DIR}.make_preferred().string()};

    const int result = merge_pathnames(m_path, new_filename.c_str(), m_mode);

    EXPECT_EQ(1, result);
    EXPECT_EQ(fs::path{ID_TEST_DATA_SUBDIR}.make_preferred().string(), m_path);
}

TEST_F(TestMergePathNames, notFileNotDirectoryReturnParentDirGetPath)
{
    m_path = fs::path{ID_TEST_DATA_SUBDIR}.make_preferred().string();
    const std::string new_filename{"foo.foo"};

    const int result = merge_pathnames(m_path, new_filename.c_str(), m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ((fs::path{ID_TEST_DATA_DIR} / "foo.foo").make_preferred().string(), m_path);
}
