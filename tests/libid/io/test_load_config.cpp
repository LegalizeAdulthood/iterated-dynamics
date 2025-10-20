// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/load_config.h>

#include "MockDriver.h"
#include "test_config_data.h"

#include <misc/Driver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::test::config;
using namespace id::ui;
using namespace testing;

namespace id::test
{

TEST(TestLoadConfig, gdiDisk)
{
    MockDriver gdi;
    const std::string gdi_name{"gdi"};
    MockDriver disk;
    const std::string disk_name{"disk"};
    ExpectationSet init_gdi = EXPECT_CALL(gdi, init(nullptr, nullptr)).WillOnce(Return(true));
    ExpectationSet init_disk = EXPECT_CALL(disk, init(nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(gdi, get_name()).WillRepeatedly(ReturnRef(gdi_name));
    EXPECT_CALL(gdi, validate_mode(_)).After(init_gdi).WillRepeatedly(Return(true));
    EXPECT_CALL(disk, get_name()).WillRepeatedly(ReturnRef(disk_name));
    EXPECT_CALL(disk, validate_mode(_)).After(init_disk).WillRepeatedly(Return(true));
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

} // namespace id::test
