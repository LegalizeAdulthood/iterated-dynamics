// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/check_write_file.h>

#include "test_check_write_file_data.h"
#include "test_library.h"

#include <io/CurrentPathSaver.h>
#include <io/library.h>
#include <io/special_dirs.h>
#include <misc/ValueSaver.h>

#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <gtest/gtest.h>

using namespace id::io;
using namespace id::misc;
using namespace id::test::check_write_file;
using namespace id::test::library;

namespace fs = std::filesystem;

namespace id::test
{

static void create_file(const fs::path &path)
{
    fs::create_directories(path.parent_path());
    std::ofstream out{path};
}

struct FilenameAdvancementInput
{
    const char *name;
    const char *requested;
    const char *extension;
    const char *expected;
};

void PrintTo(const FilenameAdvancementInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestCheckWriteFile : public testing::TestWithParam<FilenameAdvancementInput>
{
};

TEST_P(TestCheckWriteFile, advancesPastExistingFilenames)
{
    std::string name{GetParam().requested};

    io::check_write_file(name, GetParam().extension);

    EXPECT_EQ(GetParam().expected, name);
}

INSTANTIATE_TEST_SUITE_P(FilenameInputs, TestCheckWriteFile,
    testing::Values(FilenameAdvancementInput{"NewFile", ID_TEST_CHECK_WRITE_FILE_NEW, ID_TEST_CHECK_WRITE_FILE_EXT,
                        ID_TEST_CHECK_WRITE_FILE_NEW},
        FilenameAdvancementInput{"ExistingSecondFile", ID_TEST_CHECK_WRITE_FILE_EXISTS2, ID_TEST_CHECK_WRITE_FILE_EXT,
            ID_TEST_CHECK_WRITE_FILE3},
        FilenameAdvancementInput{"ExistingFirstFile", ID_TEST_CHECK_WRITE_FILE_EXISTS1, ID_TEST_CHECK_WRITE_FILE_EXT,
            ID_TEST_CHECK_WRITE_FILE3},
        FilenameAdvancementInput{"OmittedExtension", ID_TEST_CHECK_WRITE_FILE_BASE1, ID_TEST_CHECK_WRITE_FILE_EXT,
            ID_TEST_CHECK_WRITE_FILE3}),
    [](const testing::TestParamInfo<FilenameAdvancementInput> &info) { return info.param.name; });

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

TEST(TestGetCheckedSavePath, explicitExtensionIsPreserved)
{
    ValueSaver saved_overwrite{g_overwrite_file, false};
    set_save_library(ID_TEST_SAVE_DIR);

    const fs::path path{get_checked_save_path(WriteFile::IMAGE, "newimage.tga")};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image/newimage.tga", path);
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

TEST(TestGetCheckedSavePath, mapCollisionAdvancesWhenOverwriteIsOff)
{
    ValueSaver saved_overwrite{g_overwrite_file, false};
    set_save_library(ID_TEST_SAVE_DIR);
    const fs::path existing{fs::path{ID_TEST_SAVE_DIR} / "map/colors.map"};
    create_file(existing);

    const fs::path path{get_checked_save_path(WriteFile::MAP, "colors")};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "map/colors2.map", path);
    clear_save_library();
}

TEST(TestGetCheckedSavePath, migGifCollisionAdvancesWhenOverwriteIsOff)
{
    ValueSaver saved_overwrite{g_overwrite_file, false};
    set_save_library(ID_TEST_SAVE_DIR);
    const fs::path existing{fs::path{ID_TEST_SAVE_DIR} / "image/fractmig.gif"};
    create_file(existing);

    const fs::path path{get_checked_save_path(WriteFile::IMAGE, "fractmig.gif")};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image/fractmig2.gif", path);
    clear_save_library();
}

TEST(TestGetCheckedSavePath, makeMigBatchCollisionAdvancesWhenOverwriteIsOff)
{
    ValueSaver saved_overwrite{g_overwrite_file, false};
    set_save_library(ID_TEST_SAVE_DIR);
    const fs::path existing{fs::path{ID_TEST_SAVE_DIR} / "makemig.bat"};
    create_file(existing);

    const fs::path path{get_checked_save_path(WriteFile::ROOT, "makemig.bat")};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "makemig2.bat", path);
    clear_save_library();
}

TEST(TestGetCheckedSavePath, debugCollisionAdvancesWhenOverwriteIsOff)
{
    ValueSaver saved_overwrite{g_overwrite_file, false};
    set_save_library(ID_TEST_SAVE_DIR);
    const fs::path existing{fs::path{ID_TEST_SAVE_DIR} / "debug/debugfrm.txt"};
    create_file(existing);

    const fs::path path{get_checked_save_path(WriteFile::DEBUG, "debugfrm")};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "debug/debugfrm2.txt", path);
    clear_save_library();
}

TEST(TestGetCheckedSavePath, debugCollisionIsReusedWhenOverwriteIsOn)
{
    ValueSaver saved_overwrite{g_overwrite_file, true};
    set_save_library(ID_TEST_SAVE_DIR);
    const fs::path existing{fs::path{ID_TEST_SAVE_DIR} / "debug/stopmsg.txt"};
    create_file(existing);

    const fs::path path{get_checked_save_path(WriteFile::DEBUG, "stopmsg")};

    EXPECT_EQ(existing, path);
    clear_save_library();
}

TEST(TestGetCheckedSavePath, appendPathIgnoresOverwritePolicy)
{
    ValueSaver saved_overwrite{g_overwrite_file, false};
    set_save_library(ID_TEST_SAVE_DIR);
    const fs::path existing{fs::path{ID_TEST_SAVE_DIR} / "debug/frmtokens.txt"};
    create_file(existing);

    const fs::path path{get_append_save_path(WriteFile::DEBUG, "frmtokens")};

    EXPECT_EQ(existing, path);
    clear_save_library();
}

} // namespace id::test
