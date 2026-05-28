// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/check_write_file.h>

#include "test_check_write_file_data.h"
#include "test_library.h"

#include <io/CurrentPathSaver.h>
#include <io/library.h>
#include <io/special_dirs.h>
#include <misc/ValueSaver.h>

#include <filesystem>
#include <gtest/gtest.h>

using namespace id::io;
using namespace id::misc;
using namespace id::test::check_write_file;
using namespace id::test::library;

namespace fs = std::filesystem;

namespace id::test
{

TEST(TestCheckWriteFile, newFile)
{
    std::string name{ID_TEST_CHECK_WRITE_FILE_NEW};

    io::check_write_file(name, ID_TEST_CHECK_WRITE_FILE_EXT);

    EXPECT_EQ(id::test::check_write_file::ID_TEST_CHECK_WRITE_FILE_NEW, name);
}

TEST(TestCheckWriteFile, existingFile2)
{
    std::string name{ID_TEST_CHECK_WRITE_FILE_EXISTS2};

    io::check_write_file(name, ID_TEST_CHECK_WRITE_FILE_EXT);

    EXPECT_EQ(id::test::check_write_file::ID_TEST_CHECK_WRITE_FILE3, name);
}

TEST(TestCheckWriteFile, existingFile1)
{
    std::string name{ID_TEST_CHECK_WRITE_FILE_EXISTS1};

    io::check_write_file(name, ID_TEST_CHECK_WRITE_FILE_EXT);

    EXPECT_EQ(id::test::check_write_file::ID_TEST_CHECK_WRITE_FILE3, name);
}

TEST(TestCheckWriteFile, existingFile1NoExtension)
{
    std::string name{ID_TEST_CHECK_WRITE_FILE_BASE1};

    io::check_write_file(name, ID_TEST_CHECK_WRITE_FILE_EXT);

    EXPECT_EQ(ID_TEST_CHECK_WRITE_FILE3, name);
}

TEST(TestGetCheckedSavePath, existingFileAdvancesWhenOverwriteIsOff)
{
    ValueSaver saved_overwrite{g_overwrite_file, false};
    set_save_library(ID_TEST_SAVE_DIR);

    const fs::path path{get_checked_save_path(WriteFile::IMAGE, ID_TEST_SAVE_IMAGE_FILE)};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image/save2.gif", path);
    clear_save_library();
}

TEST(TestGetCheckedSavePath, existingFileIsReusedWhenOverwriteIsOn)
{
    ValueSaver saved_overwrite{g_overwrite_file, true};
    set_save_library(ID_TEST_SAVE_DIR);

    const fs::path path{get_checked_save_path(WriteFile::IMAGE, ID_TEST_SAVE_IMAGE_FILE)};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image/save.gif", path);
    clear_save_library();
}

TEST(TestGetCheckedSavePath, defaultExtensionIsAdded)
{
    ValueSaver saved_overwrite{g_overwrite_file, false};
    set_save_library(ID_TEST_SAVE_DIR);

    const fs::path path{get_checked_save_path(WriteFile::IMAGE, "newimage")};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image/newimage.gif", path);
    clear_save_library();
}

TEST(TestGetCheckedSavePath, saveDirIsUsedWithoutSaveLibrary)
{
    ValueSaver saved_overwrite{g_overwrite_file, false};
    ValueSaver saved_save_dir{g_save_dir, fs::path{ID_TEST_SAVE_DIR}};

    const fs::path path{get_checked_save_path(WriteFile::IMAGE, ID_TEST_SAVE_IMAGE_FILE)};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image/save2.gif", path);
}

TEST(TestGetCheckedSavePath, currentDirectoryCollisionDoesNotAdvanceFinalPath)
{
    CurrentPathSaver saved_cur_dir{ID_TEST_CHECK_WRITE_FILE_DATA_DIR};
    ValueSaver saved_overwrite{g_overwrite_file, false};
    set_save_library(ID_TEST_SAVE_DIR);

    const fs::path path{get_checked_save_path(WriteFile::IMAGE, "fract001")};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image/fract001.gif", path);
    clear_save_library();
}

} // namespace id::test
