#include <split_path.h>

#include <port.h>
#include <id.h>

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
    fs::path m_file_template;
    char m_drive[FILE_MAX_PATH]{};
    char m_dir[FILE_MAX_PATH]{};
    char m_filename[FILE_MAX_PATH]{};
    char m_ext[FILE_MAX_PATH]{};
};

}

TEST_F(TestSplitPath, absolutePath)
{
    m_file_template = (fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred();

    splitpath(m_file_template.string(), m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(m_file_template.root_name().string(), m_drive);
    EXPECT_EQ(m_file_template.parent_path().string().substr(m_file_template.root_name().string().length()) +
            static_cast<char>(fs::path::preferred_separator),
        m_dir);
    EXPECT_EQ(m_file_template.stem().string(), m_filename);
    EXPECT_EQ(m_file_template.extension().string(), m_ext);
}

TEST_F(TestSplitPath, relativePath)
{
    current_path_saver saver{ID_TEST_DATA_DIR};
    m_file_template = ID_TEST_DATA_SUBDIR_NAME;

    splitpath(m_file_template.string(), m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(m_file_template.root_name().string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(m_file_template.filename().string(), m_filename);
    EXPECT_EQ(std::string(), m_ext);
}

TEST_F(TestSplitPath, drive)
{
    m_file_template = ID_TEST_DATA_DIR;

    splitpath(m_file_template.root_name().string(), m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(m_file_template.root_name().string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(std::string(), m_filename);
    EXPECT_EQ(std::string(), m_ext);
}

TEST_F(TestSplitPath, directory)
{
    m_file_template = fs::path{ID_TEST_DATA_DIR}.make_preferred();

    splitpath(m_file_template.string(), m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(m_file_template.root_name().string(), m_drive);
    EXPECT_EQ(m_file_template.parent_path().string().substr(m_file_template.root_name().string().length()) +
            static_cast<char>(fs::path::preferred_separator),
        m_dir);
    EXPECT_EQ(m_file_template.filename().string(), m_filename);
    EXPECT_EQ(std::string(), m_ext);
}

TEST_F(TestSplitPath, fileName)
{
    m_file_template = fs::path{ID_TEST_IFS_FILE}.make_preferred();

    splitpath(m_file_template.string(), m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(std::string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(m_file_template.stem().string(), m_filename);
    EXPECT_EQ(m_file_template.extension().string(), m_ext);
}

TEST_F(TestSplitPath, fileNameNoExtension)
{
    m_file_template = fs::path{ID_TEST_IFS_FILE}.replace_extension().make_preferred();

    splitpath(m_file_template.string(), m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(std::string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(m_file_template.stem().string(), m_filename);
    EXPECT_EQ(std::string(), m_ext);
}

TEST_F(TestSplitPath, extensionNoFilename)
{
    m_file_template = fs::path{ID_TEST_IFS_FILE}.replace_filename("").make_preferred();

    splitpath(m_file_template.string(), m_drive, m_dir, m_filename, m_ext);

    EXPECT_EQ(std::string(), m_drive);
    EXPECT_EQ(std::string(), m_dir);
    EXPECT_EQ(std::string(), m_filename);
    EXPECT_EQ(m_file_template.extension().string(), m_ext);
}

TEST_F(TestSplitPath, filenameExtensionOnly)
{
    m_file_template = (fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred();

    split_fname_ext(m_file_template.string(), m_filename, m_ext);

    EXPECT_EQ(m_file_template.stem().string(), m_filename);
    EXPECT_EQ(m_file_template.extension().string(), m_ext);
}

TEST_F(TestSplitPath, driveDirectoryOnly)
{
    m_file_template = (fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred();

    split_drive_dir(m_file_template.string(), m_drive, m_dir);

    EXPECT_EQ(m_file_template.root_name().string(), m_drive);
    EXPECT_EQ(m_file_template.parent_path().string().substr(m_file_template.root_name().string().length()) +
            static_cast<char>(fs::path::preferred_separator),
        m_dir);
}
