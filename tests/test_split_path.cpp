#include <split_path.h>

#include <port.h>
#include <fractint.h>

#include "current_path_saver.h"
#include "test_data.h"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace {

class TestSplitPath : public ::testing::Test
{
protected:
    fs::path m_file;
    std::string m_file_template;
    char m_drive[FILE_MAX_PATH]{};
    char m_dir[FILE_MAX_PATH]{};
    char m_filename[FILE_MAX_PATH]{};
    char m_ext[FILE_MAX_PATH]{};
};

}

TEST_F(TestSplitPath, absolutePath)
{
    m_file = fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE;
    m_file.make_preferred();
    m_file_template = m_file.string();

    splitpath(m_file_template, m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(m_file.root_name().string(), m_drive);
    EXPECT_EQ(m_file.parent_path().string().substr(m_file.root_name().string().length()) +
            static_cast<char>(fs::path::preferred_separator),
        m_dir);
    EXPECT_EQ(m_file.stem().string(), m_filename);
    EXPECT_EQ(m_file.extension().string(), m_ext);
}

TEST_F(TestSplitPath, relativePath)
{
    current_path_saver saver{ID_TEST_DATA_DIR};
    m_file = ID_TEST_DATA_SUBDIR_NAME;
    m_file_template = m_file.string();

    splitpath(m_file_template, m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(m_file.root_name().string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(m_file.filename().string(), m_filename);
    EXPECT_EQ(std::string(), m_ext);
}

TEST_F(TestSplitPath, drive)
{
    m_file = ID_TEST_DATA_DIR;
    m_file_template = m_file.root_name().string();

    splitpath(m_file_template, m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(m_file.root_name().string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(std::string(), m_filename);
    EXPECT_EQ(std::string(), m_ext);
}

TEST_F(TestSplitPath, directory)
{
    m_file = ID_TEST_DATA_DIR;
    m_file.make_preferred();
    m_file_template = m_file.string();

    splitpath(m_file_template, m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(m_file.root_name().string(), m_drive);
    EXPECT_EQ(m_file.parent_path().string().substr(m_file.root_name().string().length()) +
            static_cast<char>(fs::path::preferred_separator),
        m_dir);
    EXPECT_EQ(m_file.filename().string(), m_filename);
    EXPECT_EQ(std::string(), m_ext);
}

TEST_F(TestSplitPath, fileName)
{
    m_file = ID_TEST_IFS_FILE;
    m_file.make_preferred();
    m_file_template = m_file.string();

    splitpath(m_file_template, m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(std::string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(m_file.stem().string(), m_filename);
    EXPECT_EQ(m_file.extension().string(), m_ext);
}

TEST_F(TestSplitPath, fileNameNoExtension)
{
    m_file = ID_TEST_IFS_FILE;
    m_file.replace_extension();
    m_file.make_preferred();
    m_file_template = m_file.string();

    splitpath(m_file_template, m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(std::string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(m_file.stem().string(), m_filename);
    EXPECT_EQ(std::string(), m_ext);
}

TEST_F(TestSplitPath, extensionNoFilename)
{
    m_file = ID_TEST_IFS_FILE;
    m_file.replace_filename("");
    m_file.make_preferred();
    m_file_template = m_file.string();

    splitpath(m_file_template, m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(std::string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(std::string(), m_filename);
    EXPECT_EQ(m_file.extension().string(), m_ext);
}
