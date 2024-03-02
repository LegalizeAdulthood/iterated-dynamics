#include <merge_path_names.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

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

TEST_F(TestMergePathNames, basicAtCommandLine)
{
    const int result = merge_pathnames(m_path, "new_filename.par", m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ("new_filename.par", m_path);
}

TEST_F(TestMergePathNames, replaceExistingFilenameAtCommandLine)
{
    m_path = "old_filename.par";

    const int result = merge_pathnames(m_path, "new_filename.par", m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ("new_filename.par", m_path);
}

TEST_F(TestMergePathNames, setParentDirNewFilenameForExistingDirAtCommandLine)
{
    m_path = m_existing_dir.string();

    const int result = merge_pathnames(m_path, "new_filename.par", m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ((m_existing_dir.parent_path() / "new_filename.par").make_preferred(), m_path);
}

TEST_F(TestMergePathNames, setParentDirNewFilenameForNonExistingDirAtCommandLine)
{
    m_path = m_non_existing_dir.string();

    const int result = merge_pathnames(m_path, "new_filename.par", m_mode);

    EXPECT_EQ(0, result);
    EXPECT_EQ((m_non_existing_dir.parent_path() / "new_filename.par").make_preferred(), m_path);
}
