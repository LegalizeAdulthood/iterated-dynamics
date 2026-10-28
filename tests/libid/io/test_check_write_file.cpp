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
#include <vector>
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

class TestGetCheckedSavePath : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void create_owned_file(const fs::path &path);

private:
    ValueSaver<bool> m_overwrite{g_overwrite_file, false};
    ValueSaver<fs::path> m_save_dir{g_save_dir};
    std::vector<fs::path> m_files;
};

void TestGetCheckedSavePath::SetUp()
{
    Test::SetUp();
    set_save_library(ID_TEST_SAVE_DIR);
}

void TestGetCheckedSavePath::TearDown()
{
    for (const fs::path &path : m_files)
    {
        std::error_code error;
        fs::remove(path, error);
    }
    clear_save_library();
    Test::TearDown();
}

void TestGetCheckedSavePath::create_owned_file(const fs::path &path)
{
    m_files.emplace_back(path);
    create_file(path);
}

struct SaveCollisionInput
{
    const char *name;
    WriteFile category;
    const char *requested;
    bool overwrite;
    fs::path existing;
    fs::path expected;
};

void PrintTo(const SaveCollisionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestGetCheckedSavePathCollision : public TestGetCheckedSavePath,
                                        public testing::WithParamInterface<SaveCollisionInput>
{
protected:
    void SetUp() override;
};

void TestGetCheckedSavePathCollision::SetUp()
{
    TestGetCheckedSavePath::SetUp();
    g_overwrite_file = GetParam().overwrite;
    if (!GetParam().existing.empty())
    {
        create_owned_file(fs::path{ID_TEST_SAVE_DIR} / GetParam().existing);
    }
}

TEST_P(TestGetCheckedSavePathCollision, resolvesCollisionUsingOverwritePolicy)
{
    const fs::path path{get_checked_save_path(GetParam().category, GetParam().requested)};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / GetParam().expected, path);
}

INSTANTIATE_TEST_SUITE_P(CollisionInputs, TestGetCheckedSavePathCollision,
    testing::Values(
        SaveCollisionInput{"ImageAdvances", WriteFile::IMAGE, ID_TEST_SAVE_IMAGE_FILE, false, {}, "image/save2.gif"},
        SaveCollisionInput{"ImageOverwrites", WriteFile::IMAGE, ID_TEST_SAVE_IMAGE_FILE, true, {}, "image/save.gif"},
        SaveCollisionInput{"MapAdvances", WriteFile::MAP, "colors", false, "map/colors.map", "map/colors2.map"},
        SaveCollisionInput{
            "MigGifAdvances", WriteFile::IMAGE, "fractmig.gif", false, "image/fractmig.gif", "image/fractmig2.gif"},
        SaveCollisionInput{"MigBatchAdvances", WriteFile::ROOT, "makemig.bat", false, "makemig.bat", "makemig2.bat"},
        SaveCollisionInput{
            "DebugAdvances", WriteFile::DEBUG, "debugfrm", false, "debug/debugfrm.txt", "debug/debugfrm2.txt"},
        SaveCollisionInput{
            "DebugOverwrites", WriteFile::DEBUG, "stopmsg", true, "debug/stopmsg.txt", "debug/stopmsg.txt"}),
    [](const testing::TestParamInfo<SaveCollisionInput> &info) { return info.param.name; });

struct SaveExtensionInput
{
    const char *name;
    const char *requested;
    const char *expected;
};

void PrintTo(const SaveExtensionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestGetCheckedSavePathExtension : public TestGetCheckedSavePath,
                                        public testing::WithParamInterface<SaveExtensionInput>
{
};

TEST_P(TestGetCheckedSavePathExtension, resolvesFilenameExtension)
{
    const fs::path path{get_checked_save_path(WriteFile::IMAGE, GetParam().requested)};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image" / GetParam().expected, path);
}

INSTANTIATE_TEST_SUITE_P(ExtensionInputs, TestGetCheckedSavePathExtension,
    testing::Values(SaveExtensionInput{"Default", "newimage", "newimage.gif"},
        SaveExtensionInput{"Explicit", "newimage.tga", "newimage.tga"}),
    [](const testing::TestParamInfo<SaveExtensionInput> &info) { return info.param.name; });

TEST_F(TestGetCheckedSavePath, saveDirIsUsedWithoutSaveLibrary)
{
    clear_save_library();
    g_save_dir = ID_TEST_SAVE_DIR;

    const fs::path path{get_checked_save_path(WriteFile::IMAGE, ID_TEST_SAVE_IMAGE_FILE)};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image/save2.gif", path);
}

TEST_F(TestGetCheckedSavePath, currentDirectoryCollisionDoesNotAdvanceFinalPath)
{
    CurrentPathSaver saved_cur_dir{ID_TEST_CHECK_WRITE_FILE_DATA_DIR};

    const fs::path path{get_checked_save_path(WriteFile::IMAGE, "fract001")};

    EXPECT_EQ(fs::path{ID_TEST_SAVE_DIR} / "image/fract001.gif", path);
}

TEST_F(TestGetCheckedSavePath, appendPathIgnoresOverwritePolicy)
{
    const fs::path existing{fs::path{ID_TEST_SAVE_DIR} / "debug/frmtokens.txt"};
    create_owned_file(existing);

    const fs::path path{get_append_save_path(WriteFile::DEBUG, "frmtokens")};

    EXPECT_EQ(existing, path);
}

} // namespace id::test
