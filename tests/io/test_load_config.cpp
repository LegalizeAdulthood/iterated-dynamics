// SPDX-License-Identifier: GPL-3.0-only
//

#include "CurrentPathSaver.h"

#include <io/load_config.h>

#include "MockDriver.h"
#include "test_config_data.h"

#include <engine/id_data.h>
#include <io/special_dirs.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>
#include <ui/video_mode.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

using namespace testing;

TEST(TestLoadConfig, gdiDisk)
{
    MockDriver gdi;
    const std::string gdi_name{"gdi"};
    MockDriver disk;
    const std::string disk_name{"disk"};
    ExpectationSet init_gdi = EXPECT_CALL(gdi, init(nullptr, nullptr)).WillOnce(Return(true));
    ExpectationSet init_disk = EXPECT_CALL(disk, init(nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(gdi, get_name()).WillRepeatedly(ReturnRef(gdi_name));
    EXPECT_CALL(gdi, validate_mode(NotNull())).After(init_gdi).WillRepeatedly(Return(true));
    EXPECT_CALL(disk, get_name()).WillRepeatedly(ReturnRef(disk_name));
    EXPECT_CALL(disk, validate_mode(NotNull())).After(init_disk).WillRepeatedly(Return(true));
    EXPECT_CALL(gdi, terminate());
    EXPECT_CALL(disk, terminate());
    load_driver(&gdi, nullptr, nullptr);
    load_driver(&disk, nullptr, nullptr);

    load_config(ID_TEST_HOME_CONFIG_FILE);

    ASSERT_EQ(2, g_video_table_len);
    const VideoInfo &gdi_mode{g_video_table[0]};
    EXPECT_STREQ(ID_TEST_GDI_COMMENT, gdi_mode.comment);
    EXPECT_EQ(ID_TEST_GDI_FN_KEY, gdi_mode.key);
    EXPECT_EQ(ID_TEST_GDI_WIDTH, gdi_mode.x_dots);
    EXPECT_EQ(ID_TEST_GDI_HEIGHT, gdi_mode.y_dots);
    EXPECT_EQ(ID_TEST_GDI_COLORS, gdi_mode.colors);
    EXPECT_EQ(&gdi, gdi_mode.driver);
    const VideoInfo &disk_mode{g_video_table[1]};
    EXPECT_STREQ(ID_TEST_DISK_COMMENT, disk_mode.comment);
    EXPECT_EQ(ID_TEST_DISK_FN_KEY, disk_mode.key);
    EXPECT_EQ(ID_TEST_DISK_WIDTH, disk_mode.x_dots);
    EXPECT_EQ(ID_TEST_DISK_HEIGHT, disk_mode.y_dots);
    EXPECT_EQ(ID_TEST_DISK_COLORS, disk_mode.colors);
    EXPECT_EQ(&disk, disk_mode.driver);

    close_drivers();
}

TEST(TestLocateConfigFile, preferConfigFileFromSaveDir)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_HOME_DIR};

    const std::filesystem::path result{locate_config_file(ID_TEST_CFG)};

    EXPECT_EQ(std::filesystem::path{ID_TEST_SAVE_CONFIG_FILE}, result);
}

TEST(TestLocateConfigFile, loadToolsFileFallbackToSearchDir1)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_HOME_DIR};

    const std::filesystem::path result{locate_config_file(ID_TEST_INI)};

    EXPECT_EQ(std::filesystem::path{ID_TEST_HOME_TOOLS_INI}, result);
}

TEST(TestLocateConfigFile, loadToolsFileFallbackToSearchDir2)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir2{g_fractal_search_dir2, ID_TEST_HOME_DIR};

    const std::filesystem::path result{locate_config_file(ID_TEST_INI)};

    EXPECT_EQ(std::filesystem::path{ID_TEST_HOME_TOOLS_INI}, result);
}

TEST(TestLocateConfigFile, loadToolsFileFallbackToCurrentDir)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir2{g_fractal_search_dir2, ID_TEST_SAVE_DIR};
    CurrentPathSaver saved_current_path{ID_TEST_HOME_DIR};

    const std::filesystem::path result{locate_config_file(ID_TEST_INI)};

    EXPECT_EQ(std::filesystem::path{ID_TEST_HOME_TOOLS_INI}, result);
}
