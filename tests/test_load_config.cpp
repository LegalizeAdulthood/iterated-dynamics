// SPDX-License-Identifier: GPL-3.0-only
//
#include <load_config.h>
#include <drivers.h>

#include "MockDriver.h"
#include "test_config_data.h"

#include <video_mode.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

TEST(TestDriver, loadClose)
{
    MockDriver gdi;
    EXPECT_CALL(gdi, init(nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(gdi, terminate());

    load_driver(&gdi, nullptr, nullptr);

    EXPECT_EQ(&gdi, g_driver);

    close_drivers();
    EXPECT_EQ(nullptr, g_driver);
}

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

    load_config(ID_TEST_CONFIG_FILE);

    ASSERT_EQ(2, g_video_table_len);
    const VideoInfo &gdi_mode{g_video_table[0]};
    EXPECT_STREQ(ID_TEST_GDI_COMMENT, gdi_mode.comment);
    EXPECT_EQ(ID_TEST_GDI_FN_KEY, gdi_mode.keynum);
    EXPECT_EQ(ID_TEST_GDI_WIDTH, gdi_mode.xdots);
    EXPECT_EQ(ID_TEST_GDI_HEIGHT, gdi_mode.ydots);
    EXPECT_EQ(ID_TEST_GDI_COLORS, gdi_mode.colors);
    EXPECT_EQ(&gdi, gdi_mode.driver);
    const VideoInfo &disk_mode{g_video_table[1]};
    EXPECT_STREQ(ID_TEST_DISK_COMMENT, disk_mode.comment);
    EXPECT_EQ(ID_TEST_DISK_FN_KEY, disk_mode.keynum);
    EXPECT_EQ(ID_TEST_DISK_WIDTH, disk_mode.xdots);
    EXPECT_EQ(ID_TEST_DISK_HEIGHT, disk_mode.ydots);
    EXPECT_EQ(ID_TEST_DISK_COLORS, disk_mode.colors);
    EXPECT_EQ(&disk, disk_mode.driver);

    close_drivers();
}
