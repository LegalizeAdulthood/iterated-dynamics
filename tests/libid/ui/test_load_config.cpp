// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/load_config.h>

#include <MockDriver.h>
#include <test_config_data.h>
#include <ui/test_config_keys.h>

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
    std::filesystem::path write_config_text(const std::string &text) const;

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
    return write_config_text("F1," + std::to_string(width) + ',' + std::to_string(height) + ",256,disk,test mode\n");
}

std::filesystem::path TestLoadConfig::write_config_text(const std::string &text) const
{
    const std::filesystem::path path{config_path()};
    std::ofstream cfg{path};
    if (!cfg)
    {
        ADD_FAILURE() << "Unable to write " << path;
        return path;
    }
    cfg << text;
    return path;
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

TEST_F(TestLoadConfig, missingFileMarksConfigurationUnavailable)
{
    load_config(config_path().string());

    EXPECT_EQ(ConfigStatus::BAD_NO_MESSAGE, g_bad_config);
    EXPECT_EQ(0, g_video_table_len);
}

TEST_F(TestLoadConfig, emptyFilenameMarksConfigurationUnavailable)
{
    load_config("");

    EXPECT_EQ(ConfigStatus::BAD_NO_MESSAGE, g_bad_config);
    EXPECT_EQ(0, g_video_table_len);
}

TEST_F(TestLoadConfig, unknownDriverDoesNotInstallAMode)
{
    load_config(write_config(320, 200).string());

    EXPECT_EQ(ConfigStatus::OK, g_bad_config);
    EXPECT_EQ(0, g_video_table_len);
}

TEST_F(TestLoadConfig, driverRejectedModeIsSkipped)
{
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(_)).WillOnce(Return(false));

    load_config(write_config(320, 200).string());

    EXPECT_EQ(ConfigStatus::OK, g_bad_config);
    EXPECT_EQ(0, g_video_table_len);
}

TEST_F(TestLoadConfig, malformedLineRetainsPreviouslyInstalledModes)
{
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(_)).WillOnce(Return(true));
    const auto path{write_config_text("F1,320,200,256,disk,first\ninvalid\nF2,640,480,256,disk,last\n")};

    load_config(path.string());

    EXPECT_EQ(ConfigStatus::BAD_NO_MESSAGE, g_bad_config);
    ASSERT_EQ(1, g_video_table_len);
    EXPECT_EQ(ID_KEY_F1, g_video_mode_keys[0]);
    EXPECT_STREQ("first", g_video_table[0].comment);
    EXPECT_EQ(1, g_cfg_line_nums[0]);
}

TEST_F(TestLoadConfig, fullCatalogDoesNotParseTheRemainingLines)
{
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(_)).WillOnce(Return(true));
    std::fill_n(g_video_table, MAX_VIDEO_MODES - 1, VideoInfo{});
    g_video_table_len = MAX_VIDEO_MODES - 1;
    const auto path{write_config_text("F1,320,200,256,disk,last mode\ninvalid\n")};

    load_config(path.string());

    EXPECT_EQ(ConfigStatus::OK, g_bad_config);
    ASSERT_EQ(MAX_VIDEO_MODES, g_video_table_len);
    EXPECT_EQ(ID_KEY_F1, g_video_mode_keys[MAX_VIDEO_MODES - 1]);
    EXPECT_EQ(1, g_cfg_line_nums[MAX_VIDEO_MODES - 1]);
    EXPECT_EQ(-1, g_cfg_line_nums[MAX_VIDEO_MODES - 2]);
}

TEST_F(TestLoadConfig, retainsSourceLinesForConfigurationRewriting)
{
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(_)).WillRepeatedly(Return(true));
    const auto path{write_config_text("; first\nF1,320,200,256,disk,one\n; second\nF2,640,480,256,disk,two\n")};

    load_config(path.string());

    ASSERT_EQ(2, g_video_table_len);
    EXPECT_EQ(2, g_cfg_line_nums[0]);
    EXPECT_EQ(4, g_cfg_line_nums[1]);
}

TEST_F(TestLoadConfig, catalogCommentsRemainBoundedAndTerminated)
{
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(_)).WillOnce(Return(true));
    const auto path{write_config_text("F1,320,200,256,disk," + std::string(100, 'x') + '\n')};

    load_config(path.string());

    ASSERT_EQ(1, g_video_table_len);
    EXPECT_STREQ(std::string(25, 'x').c_str(), g_video_table[0].comment);
}

struct ConfigKeyInput
{
    const char *name;
    int key;
};

class TestLoadConfigKey : public TestLoadConfig, public WithParamInterface<ConfigKeyInput>
{
};

TEST_P(TestLoadConfigKey, installsTranslatedKeyOrAnUnboundMode)
{
    register_driver(m_disk, m_disk_name);
    EXPECT_CALL(m_disk, validate_mode(_)).WillOnce(Return(true));
    const auto path{write_config_text(std::string{GetParam().name} + ",320,200,256,disk,test\n")};

    load_config(path.string());

    EXPECT_EQ(ConfigStatus::OK, g_bad_config);
    ASSERT_EQ(1, g_video_table_len);
    EXPECT_EQ(GetParam().key, g_video_mode_keys[0]);
}

INSTANTIATE_TEST_SUITE_P(Keys, TestLoadConfigKey,
    Values(ConfigKeyInput{"F6", ID_KEY_F6}, ConfigKeyInput{"sf8", ID_KEY_SHF_F8},
        ConfigKeyInput{"CF10", ID_KEY_CTL_F10}, ConfigKeyInput{"AF1", ID_KEY_ALT_F1},
        ConfigKeyInput{"F1   ", ID_KEY_F1}, ConfigKeyInput{"invalid", 0}, ConfigKeyInput{"F11", 0}));

} // namespace id::test
