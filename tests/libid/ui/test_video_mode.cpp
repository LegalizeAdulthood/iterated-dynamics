// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/video_mode.h>

#include <engine/pixel_limits.h>
#include <engine/VideoInfo.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>

#include <gtest/gtest.h>

#include <string>
#include <string_view>

using namespace id::engine;
using namespace id::misc;
using namespace id::ui;

namespace id::test
{

TEST(TestVideoMode, checkKeyNameFnKey)
{
    EXPECT_EQ(ID_KEY_F1, check_vid_mode_key_name("F1"));
    EXPECT_EQ(ID_KEY_F2, check_vid_mode_key_name("f2"));
    EXPECT_EQ(ID_KEY_F6, check_vid_mode_key_name("F6   "));
    EXPECT_EQ(ID_KEY_F10, check_vid_mode_key_name("F10"));
    EXPECT_EQ(0, check_vid_mode_key_name("b"));
    EXPECT_EQ(0, check_vid_mode_key_name("f"));
    EXPECT_EQ(0, check_vid_mode_key_name("f0"));
    EXPECT_EQ(0, check_vid_mode_key_name("ff"));
    EXPECT_EQ(0, check_vid_mode_key_name("f11"));
}

TEST(TestVideoMode, checkKeyNameShiftFnKey)
{
    EXPECT_EQ(ID_KEY_SHF_F1, check_vid_mode_key_name("SF1"));
    EXPECT_EQ(ID_KEY_SHF_F2, check_vid_mode_key_name("sf2"));
    EXPECT_EQ(ID_KEY_SHF_F6, check_vid_mode_key_name("SF6   "));
    EXPECT_EQ(ID_KEY_SHF_F10, check_vid_mode_key_name("SF10"));
    EXPECT_EQ(0, check_vid_mode_key_name("s"));
    EXPECT_EQ(0, check_vid_mode_key_name("sf"));
    EXPECT_EQ(0, check_vid_mode_key_name("sf0"));
    EXPECT_EQ(0, check_vid_mode_key_name("sff"));
    EXPECT_EQ(0, check_vid_mode_key_name("sf11"));
}

TEST(TestVideoMode, checkKeyNameControlFnKey)
{
    EXPECT_EQ(ID_KEY_CTL_F1, check_vid_mode_key_name("CF1"));
    EXPECT_EQ(ID_KEY_CTL_F2, check_vid_mode_key_name("cf2"));
    EXPECT_EQ(ID_KEY_CTL_F6, check_vid_mode_key_name("CF6   "));
    EXPECT_EQ(ID_KEY_CTL_F10, check_vid_mode_key_name("CF10"));
    EXPECT_EQ(0, check_vid_mode_key_name("c"));
    EXPECT_EQ(0, check_vid_mode_key_name("cf"));
    EXPECT_EQ(0, check_vid_mode_key_name("cf0"));
    EXPECT_EQ(0, check_vid_mode_key_name("cff"));
    EXPECT_EQ(0, check_vid_mode_key_name("cf11"));
}

TEST(TestVideoMode, checkKeyNameAltFnKey)
{
    EXPECT_EQ(ID_KEY_ALT_F1, check_vid_mode_key_name("AF1"));
    EXPECT_EQ(ID_KEY_ALT_F2, check_vid_mode_key_name("af2"));
    EXPECT_EQ(ID_KEY_ALT_F6, check_vid_mode_key_name("AF6   "));
    EXPECT_EQ(ID_KEY_ALT_F10, check_vid_mode_key_name("AF10"));
    EXPECT_EQ(0, check_vid_mode_key_name("a"));
    EXPECT_EQ(0, check_vid_mode_key_name("af"));
    EXPECT_EQ(0, check_vid_mode_key_name("af0"));
    EXPECT_EQ(0, check_vid_mode_key_name("aff"));
    EXPECT_EQ(0, check_vid_mode_key_name("af11"));
}

TEST(TestVideoMode, checkKeyNameHonorsStringViewLength)
{
    const std::string storage{"F6-suffix"};
    const std::string_view key_name{storage.data(), 2};

    EXPECT_EQ(ID_KEY_F6, check_vid_mode_key_name(key_name));
}

TEST(TestVideoMode, checkKeyFnKeyNoModes)
{
    ValueSaver saved_video_table_len{g_video_table_len, 0};

    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_F1));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_F10));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_SHF_F1));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_SHF_F10));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_CTL_F1));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_CTL_F10));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_ALT_F1));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_ALT_F10));
}

static void push_modes(const std::initializer_list<int> keys)
{
    for (const int key : keys)
    {
        g_video_table[g_video_table_len].key = key;
        ++g_video_table_len;
    }
}

TEST(TestVideoMode, checkKeyFnKeyNoMatchingModes)
{
    ValueSaver saved_video_table_len{g_video_table_len, 0};
    push_modes({ID_KEY_F2, ID_KEY_SHF_F2, ID_KEY_CTL_F2, ID_KEY_ALT_F2});

    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_F1));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_F10));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_SHF_F1));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_SHF_F10));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_CTL_F1));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_CTL_F10));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_ALT_F1));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_ALT_F10));
}

TEST(TestVideoMode, checkKeyFnKeyMatchingModes)
{
    ValueSaver saved_video_table_len{g_video_table_len, 0};
    push_modes({ID_KEY_F1, ID_KEY_F10, //
        ID_KEY_SHF_F1, ID_KEY_SHF_F10, //
        ID_KEY_CTL_F1, ID_KEY_CTL_F10, //
        ID_KEY_ALT_F1, ID_KEY_ALT_F10});

    EXPECT_EQ(0, check_vid_mode_key(ID_KEY_F1));
    EXPECT_EQ(1, check_vid_mode_key(ID_KEY_F10));
    EXPECT_EQ(2, check_vid_mode_key(ID_KEY_SHF_F1));
    EXPECT_EQ(3, check_vid_mode_key(ID_KEY_SHF_F10));
    EXPECT_EQ(4, check_vid_mode_key(ID_KEY_CTL_F1));
    EXPECT_EQ(5, check_vid_mode_key(ID_KEY_CTL_F10));
    EXPECT_EQ(6, check_vid_mode_key(ID_KEY_ALT_F1));
    EXPECT_EQ(7, check_vid_mode_key(ID_KEY_ALT_F10));
}

TEST(TestVideoMode, keyName)
{
    ValueSaver saved_video_table_len{g_video_table_len, 0};
    push_modes({ID_KEY_F1, ID_KEY_F10, //
        ID_KEY_SHF_F1, ID_KEY_SHF_F10, //
        ID_KEY_CTL_F1, ID_KEY_CTL_F10, //
        ID_KEY_ALT_F1, ID_KEY_ALT_F10});

    EXPECT_EQ("F1", vid_mode_key_name(ID_KEY_F1));
    EXPECT_EQ("F10", vid_mode_key_name(ID_KEY_F10));
    EXPECT_EQ("SF1", vid_mode_key_name(ID_KEY_SHF_F1));
    EXPECT_EQ("SF10", vid_mode_key_name(ID_KEY_SHF_F10));
    EXPECT_EQ("CF1", vid_mode_key_name(ID_KEY_CTL_F1));
    EXPECT_EQ("CF10", vid_mode_key_name(ID_KEY_CTL_F10));
    EXPECT_EQ("AF1", vid_mode_key_name(ID_KEY_ALT_F1));
    EXPECT_EQ("AF10", vid_mode_key_name(ID_KEY_ALT_F10));
    EXPECT_EQ("", vid_mode_key_name(-1));
    EXPECT_EQ("", vid_mode_key_name(ID_KEY_CTL_A));
}

TEST(TestVideoMode, diskVideoModeAcceptsGifSizedImages)
{
    EXPECT_TRUE(is_valid_disk_video_mode({0, 32768, 32768, 256, nullptr, ""}));
    EXPECT_TRUE(is_valid_disk_video_mode({0, GIF_MAX_PIXELS, GIF_MAX_PIXELS, 256, nullptr, ""}));
}

TEST(TestVideoMode, diskVideoModeRejectsNonGifSizedImages)
{
    EXPECT_FALSE(is_valid_disk_video_mode({0, 0, 480, 256, nullptr, ""}));
    EXPECT_FALSE(is_valid_disk_video_mode({0, 640, 0, 256, nullptr, ""}));
    EXPECT_FALSE(is_valid_disk_video_mode({0, GIF_MAX_PIXELS + 1, 480, 256, nullptr, ""}));
    EXPECT_FALSE(is_valid_disk_video_mode({0, 640, GIF_MAX_PIXELS + 1, 256, nullptr, ""}));
    EXPECT_FALSE(is_valid_disk_video_mode({0, 640, 480, 16, nullptr, ""}));
}

TEST(TestVideoMode, displayVideoModeAcceptsLargePhysicalScreens)
{
    EXPECT_TRUE(is_valid_display_video_mode({0, 32768, 32768, 256, nullptr, ""}, 32768, 32768));
    EXPECT_TRUE(is_valid_display_video_mode(
        {0, GIF_MAX_PIXELS, GIF_MAX_PIXELS, 256, nullptr, ""}, GIF_MAX_PIXELS, GIF_MAX_PIXELS));
}

TEST(TestVideoMode, displayVideoModeRejectsInvalidMetadata)
{
    EXPECT_FALSE(is_valid_display_video_mode({0, 0, 480, 256, nullptr, ""}, 640, 480));
    EXPECT_FALSE(is_valid_display_video_mode({0, 640, 0, 256, nullptr, ""}, 640, 480));
    EXPECT_FALSE(is_valid_display_video_mode({0, 640, 480, 16, nullptr, ""}, 640, 480));
    EXPECT_FALSE(is_valid_display_video_mode({0, 640, 480, 256, nullptr, ""}, 0, 480));
    EXPECT_FALSE(is_valid_display_video_mode({0, 640, 480, 256, nullptr, ""}, 640, 0));
}

TEST(TestVideoMode, displayVideoModeRejectsModesLargerThanDisplay)
{
    EXPECT_FALSE(is_valid_display_video_mode({0, 1025, 768, 256, nullptr, ""}, 1024, 768));
    EXPECT_FALSE(is_valid_display_video_mode({0, 1024, 769, 256, nullptr, ""}, 1024, 768));
    EXPECT_TRUE(is_valid_display_video_mode({0, 1024, 768, 256, nullptr, ""}, 1024, 768));
}

} // namespace id::test
