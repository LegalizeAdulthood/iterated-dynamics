// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/load_config.h>

#include "MockDriver.h"
#include "test_config_data.h"

#include <engine/pixel_limits.h>
#include <misc/Driver.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::test::config;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestLoadConfig : public Test
{
protected:
    void SetUp() override
    {
        g_video_table_len = 0;
        g_bad_config = ConfigStatus::OK;
    }

    void TearDown() override
    {
        if (g_driver != nullptr)
        {
            close_drivers();
        }
        g_video_table_len = 0;
        g_bad_config = ConfigStatus::OK;
    }

    std::filesystem::path config_path() const
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        std::string name{info->test_suite_name()};
        name += '_';
        name += info->name();
        std::replace_if(name.begin(), name.end(), [](const unsigned char ch) { return std::isalnum(ch) == 0; }, '_');
        return std::filesystem::path{ID_TEST_HOME_DIR} / (name + ".cfg");
    }

    std::filesystem::path write_config(const int width, const int height) const
    {
        const std::filesystem::path path{config_path()};
        std::ofstream cfg{path};
        if (!cfg)
        {
            ADD_FAILURE() << "Unable to write " << path;
            return path;
        }
        cfg << "F1," << width << ',' << height << ",256,disk,test mode\n";
        return path;
    }
};

class TestLoadConfigGifDimension : public TestLoadConfig, public WithParamInterface<int>
{
};

TEST_F(TestLoadConfig, pixelLimits)
{
    EXPECT_EQ(65535, GIF_MAX_PIXELS);
    EXPECT_EQ(32767, MAX_PIXELS);
    EXPECT_EQ(2048, OLD_MAX_PIXELS);
    EXPECT_EQ(10, MIN_PIXELS);
}

TEST_F(TestLoadConfig, gdiDisk)
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

TEST_P(TestLoadConfigGifDimension, acceptsModeUpToGifLimit)
{
    const int size{GetParam()};
    MockDriver disk;
    const std::string disk_name{"disk"};
    EXPECT_CALL(disk, init(nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(disk, get_name()).WillRepeatedly(ReturnRef(disk_name));
    EXPECT_CALL(disk, validate_mode(AllOf(Field(&VideoInfo::x_dots, size), Field(&VideoInfo::y_dots, size))))
        .WillOnce(Return(true));
    EXPECT_CALL(disk, terminate());
    load_driver(&disk, nullptr, nullptr);

    load_config(write_config(size, size).string());

    const int len{g_video_table_len};
    VideoInfo mode{};
    if (len > 0)
    {
        mode = g_video_table[0];
    }
    close_drivers();

    EXPECT_EQ(ConfigStatus::OK, g_bad_config);
    EXPECT_EQ(1, len);
    EXPECT_EQ(ID_KEY_F1, mode.key);
    EXPECT_EQ(size, mode.x_dots);
    EXPECT_EQ(size, mode.y_dots);
    EXPECT_EQ(256, mode.colors);
    EXPECT_EQ(&disk, mode.driver);
}

INSTANTIATE_TEST_SUITE_P(GifDimension, TestLoadConfigGifDimension, Values(MAX_PIXELS, MAX_PIXELS + 1, GIF_MAX_PIXELS));

TEST_F(TestLoadConfig, rejectsModeAboveGifLimit)
{
    MockDriver disk;
    EXPECT_CALL(disk, init(nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(disk, terminate());
    load_driver(&disk, nullptr, nullptr);

    load_config(write_config(GIF_MAX_PIXELS + 1, GIF_MAX_PIXELS + 1).string());

    const int len{g_video_table_len};
    close_drivers();

    EXPECT_EQ(ConfigStatus::BAD_NO_MESSAGE, g_bad_config);
    EXPECT_EQ(0, len);
}

} // namespace id::test
