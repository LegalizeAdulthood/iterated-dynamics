// SPDX-License-Identifier: GPL-3.0-only
//
#include <expand_dirname.h>

#include <port.h>

#include <id.h>

#include "current_path_saver.h"
#include "test_data.h"

#include <gtest/gtest.h>

#include <cstring>

namespace fs = std::filesystem;

namespace
{

class TestExpandDirName : public ::testing::Test
{
public:
    ~TestExpandDirName() override = default;

protected:
    void SetUp() override;

    fs::path m_test_data_dir{ID_TEST_DATA_DIR};
    fs::path m_test_data_subdir{ID_TEST_DATA_SUBDIR};
    char m_dir_name[FILE_MAX_PATH]{};
    char m_drive[FILE_MAX_PATH]{};
    char m_sep{fs::path::preferred_separator};
};

void TestExpandDirName::SetUp()
{
    Test::SetUp();
    m_test_data_dir.make_preferred();
    m_test_data_subdir.make_preferred();
}

} // namespace

TEST_F(TestExpandDirName, resolvesRelativeDir)
{
    current_path_saver saver(ID_TEST_DATA_DIR);
    std::strcpy(m_dir_name, ID_TEST_DATA_SUBDIR_NAME);
    std::strcpy(m_drive, m_test_data_dir.root_name().string().c_str());

    expand_dirname(m_dir_name, m_drive);

    EXPECT_EQ(m_dir_name, m_test_data_subdir.string().substr(std::strlen(m_drive)) + m_sep);
}

TEST_F(TestExpandDirName, setsDriveToCurrentDrive)
{
    current_path_saver saver(ID_TEST_DATA_DIR);
    std::strcpy(m_dir_name, ID_TEST_DATA_SUBDIR_NAME);

    expand_dirname(m_dir_name, m_drive);

    EXPECT_EQ(m_drive, m_test_data_subdir.root_name().string());
}

TEST_F(TestExpandDirName, absolutePathUnchanged)
{
    std::strcpy(m_dir_name, m_test_data_dir.string().c_str());

    expand_dirname(m_dir_name, m_drive);

    EXPECT_EQ(m_test_data_dir.root_name().string(), m_drive);
    EXPECT_EQ(m_test_data_dir.string().substr(std::strlen(m_drive)) + m_sep, m_dir_name);
}
