// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/load_config.h>

#include <MockDriver.h>
#include <test_config_data.h>

#include <engine/pixel_limits.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>
#include <ui/video_mode.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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
    void SetUp() override;
    void TearDown() override;
    ExpectationSet register_driver(MockDriver &driver, const std::string &name);
    std::filesystem::path config_path() const;
    std::filesystem::path write_config(int width, int height) const;

    std::vector<std::string> m_args{"id"};
    MockDriver m_gdi;
    MockDriver m_disk;
    const std::string m_gdi_name{"gdi"};
    const std::string m_disk_name{"disk"};
    ValueSaver<std::array<int, MAX_VIDEO_MODES>> m_saved_keys{g_video_mode_keys};
};

void TestLoadConfig::SetUp()
{
    Test::SetUp();
    g_video_table_len = 0;
    g_video_mode_keys.fill(0);
    g_bad_config = ConfigStatus::OK;
}

void TestLoadConfig::TearDown()
{
    if (g_driver != nullptr)
    {
        close_drivers();
    }
    std::error_code error;
    std::filesystem::remove(config_path(), error);
    g_video_table_len = 0;
    g_bad_config = ConfigStatus::OK;
    Test::TearDown();
}

ExpectationSet TestLoadConfig::register_driver(MockDriver &driver, const std::string &name)
{
    const ExpectationSet initialized = EXPECT_CALL(driver, init(Ref(m_args))).WillOnce(Return(true));
    EXPECT_CALL(driver, get_name()).WillRepeatedly(ReturnRef(name));
    EXPECT_CALL(driver, terminate());
    load_driver(&driver, m_args);
    return initialized;
}

std::filesystem::path TestLoadConfig::config_path() const
{
    const TestInfo *info = UnitTest::GetInstance()->current_test_info();
    std::string name{info->test_suite_name()};
    name += '_';
    name += info->name();
    std::replace_if(name.begin(), name.end(), [](const unsigned char ch) { return std::isalnum(ch) == 0; }, '_');
    return std::filesystem::path{ID_TEST_HOME_DIR} / (name + ".cfg");
}

std::filesystem::path TestLoadConfig::write_config(const int width, const int height) const
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
    const ExpectationSet init_gdi{register_driver(m_gdi, m_gdi_name)};
    const ExpectationSet init_disk{register_driver(m_disk, m_disk_name)};
    EXPECT_CALL(m_gdi, validate_mode(_)).After(init_gdi).WillRepeatedly(Return(true));
    EXPECT_CALL(m_disk, validate_mode(_)).After(init_disk).WillRepeatedly(Return(true));

    load_config(ID_TEST_HOME_CONFIG_FILE);

    ASSERT_EQ(2, g_video_table_len);
    const VideoInfo &gdi_mode{g_video_table[0]};
    EXPECT_STREQ(ID_TEST_GDI_COMMENT, gdi_mode.comment);
    EXPECT_EQ(ID_TEST_GDI_FN_KEY, g_video_mode_keys[0]);
    EXPECT_EQ(ID_TEST_GDI_WIDTH, gdi_mode.x_dots);
    EXPECT_EQ(ID_TEST_GDI_HEIGHT, gdi_mode.y_dots);
    EXPECT_EQ(ID_TEST_GDI_COLORS, gdi_mode.colors);
    EXPECT_EQ(&m_gdi, gdi_mode.driver);
    const VideoInfo &disk_mode{g_video_table[1]};
    EXPECT_STREQ(ID_TEST_DISK_COMMENT, disk_mode.comment);
    EXPECT_EQ(ID_TEST_DISK_FN_KEY, g_video_mode_keys[1]);
    EXPECT_EQ(ID_TEST_DISK_WIDTH, disk_mode.x_dots);
    EXPECT_EQ(ID_TEST_DISK_HEIGHT, disk_mode.y_dots);
    EXPECT_EQ(ID_TEST_DISK_COLORS, disk_mode.colors);
    EXPECT_EQ(&m_disk, disk_mode.driver);
}

TEST_F(TestLoadConfig, bindsExistingUnassignedModeWithoutChangingDescription)
{
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(_)).WillOnce(Return(true));
    VideoInfo mode{320, 200, 256, nullptr, "built in"};
    add_video_mode(&m_disk, &mode);

    load_config(write_config(320, 200).string());

    EXPECT_EQ(ConfigStatus::OK, g_bad_config);
    ASSERT_EQ(1, g_video_table_len);
    EXPECT_EQ(ID_KEY_F1, g_video_mode_keys[0]);
    EXPECT_EQ(mode, g_video_table[0]);
}

TEST_F(TestLoadConfig, identicalModesKeepDistinctBindings)
{
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(_)).WillRepeatedly(Return(true));
    const std::filesystem::path path{config_path()};
    std::ofstream cfg{path};
    ASSERT_TRUE(cfg) << "Unable to write " << path;
    cfg << "F6,320,200,256,disk,same mode\n"
           "F7,320,200,256,disk,same mode\n";
    cfg.close();

    load_config(path.string());

    EXPECT_EQ(ConfigStatus::OK, g_bad_config);
    ASSERT_EQ(2, g_video_table_len);
    EXPECT_EQ(g_video_table[0], g_video_table[1]);
    EXPECT_EQ(0, check_vid_mode_key(ID_KEY_F6));
    EXPECT_EQ(1, check_vid_mode_key(ID_KEY_F7));
}

TEST_F(TestLoadConfig, parsesFinalLineWithoutTrailingNewline)
{
    const std::string comment{"last char"};
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(_)).WillOnce(Return(true));
    const std::filesystem::path path{config_path()};
    std::ofstream cfg{path};
    ASSERT_TRUE(cfg) << "Unable to write " << path;
    cfg << "F1,320,200,256,disk," << comment;
    cfg.close();

    load_config(path.string());

    ASSERT_EQ(1, g_video_table_len);
    EXPECT_STREQ(comment.c_str(), g_video_table[0].comment);
}

TEST_P(TestLoadConfigGifDimension, acceptsModeUpToGifLimit)
{
    const int size{GetParam()};
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(AllOf(Field(&VideoInfo::x_dots, size), Field(&VideoInfo::y_dots, size))))
        .WillOnce(Return(true));

    load_config(write_config(size, size).string());

    const int len{g_video_table_len};
    VideoInfo mode{};
    if (len > 0)
    {
        mode = g_video_table[0];
    }

    EXPECT_EQ(ConfigStatus::OK, g_bad_config);
    EXPECT_EQ(1, len);
    EXPECT_EQ(ID_KEY_F1, g_video_mode_keys[0]);
    EXPECT_EQ(size, mode.x_dots);
    EXPECT_EQ(size, mode.y_dots);
    EXPECT_EQ(256, mode.colors);
    EXPECT_EQ(&m_disk, mode.driver);
}

INSTANTIATE_TEST_SUITE_P(GifDimension, TestLoadConfigGifDimension, Values(MAX_PIXELS, MAX_PIXELS + 1, GIF_MAX_PIXELS));

TEST_F(TestLoadConfig, rejectsModeAboveGifLimit)
{
    register_driver(m_disk, m_disk_name);

    load_config(write_config(GIF_MAX_PIXELS + 1, GIF_MAX_PIXELS + 1).string());

    const int len{g_video_table_len};

    EXPECT_EQ(ConfigStatus::BAD_NO_MESSAGE, g_bad_config);
    EXPECT_EQ(0, len);
}

} // namespace id::test
