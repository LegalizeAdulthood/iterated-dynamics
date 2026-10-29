// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/video_mode.h>

#include "MockDriver.h"

#include <engine/pixel_limits.h>
#include <engine/video_mode.h>
#include <engine/VideoInfo.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>
#include <ui/select_video_mode.h>
#include <ui/text_screen.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

using namespace id::engine;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

struct VideoModeKeyTokenInput
{
    const char *name;
    const char *token;
    int expected;
};

void PrintTo(const VideoModeKeyTokenInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestVideoModeKeyToken : public TestWithParam<VideoModeKeyTokenInput>
{
};

TEST_P(TestVideoModeKeyToken, decodesFunctionKeyName)
{
    EXPECT_EQ(GetParam().expected, check_vid_mode_key_name(GetParam().token));
}

INSTANTIATE_TEST_SUITE_P(Spelling, TestVideoModeKeyToken,
    Values(VideoModeKeyTokenInput{"PlainFirst", "F1", ID_KEY_F1},
        VideoModeKeyTokenInput{"PlainLowerSecond", "f2", ID_KEY_F2},
        VideoModeKeyTokenInput{"PlainTrailingSixth", "F6   ", ID_KEY_F6},
        VideoModeKeyTokenInput{"PlainTenth", "F10", ID_KEY_F10}, VideoModeKeyTokenInput{"PlainInvalidLetter", "b", 0},
        VideoModeKeyTokenInput{"PlainIncomplete", "f", 0}, VideoModeKeyTokenInput{"PlainZero", "f0", 0},
        VideoModeKeyTokenInput{"PlainRepeatedF", "ff", 0}, VideoModeKeyTokenInput{"PlainEleven", "f11", 0},
        VideoModeKeyTokenInput{"ShiftFirst", "SF1", ID_KEY_SHF_F1},
        VideoModeKeyTokenInput{"ShiftLowerSecond", "sf2", ID_KEY_SHF_F2},
        VideoModeKeyTokenInput{"ShiftTrailingSixth", "SF6   ", ID_KEY_SHF_F6},
        VideoModeKeyTokenInput{"ShiftTenth", "SF10", ID_KEY_SHF_F10}, VideoModeKeyTokenInput{"ShiftBarePrefix", "s", 0},
        VideoModeKeyTokenInput{"ShiftIncomplete", "sf", 0}, VideoModeKeyTokenInput{"ShiftZero", "sf0", 0},
        VideoModeKeyTokenInput{"ShiftRepeatedF", "sff", 0}, VideoModeKeyTokenInput{"ShiftEleven", "sf11", 0},
        VideoModeKeyTokenInput{"ControlFirst", "CF1", ID_KEY_CTL_F1},
        VideoModeKeyTokenInput{"ControlLowerSecond", "cf2", ID_KEY_CTL_F2},
        VideoModeKeyTokenInput{"ControlTrailingSixth", "CF6   ", ID_KEY_CTL_F6},
        VideoModeKeyTokenInput{"ControlTenth", "CF10", ID_KEY_CTL_F10},
        VideoModeKeyTokenInput{"ControlBarePrefix", "c", 0}, VideoModeKeyTokenInput{"ControlIncomplete", "cf", 0},
        VideoModeKeyTokenInput{"ControlZero", "cf0", 0}, VideoModeKeyTokenInput{"ControlRepeatedF", "cff", 0},
        VideoModeKeyTokenInput{"ControlEleven", "cf11", 0}, VideoModeKeyTokenInput{"AltFirst", "AF1", ID_KEY_ALT_F1},
        VideoModeKeyTokenInput{"AltLowerSecond", "af2", ID_KEY_ALT_F2},
        VideoModeKeyTokenInput{"AltTrailingSixth", "AF6   ", ID_KEY_ALT_F6},
        VideoModeKeyTokenInput{"AltTenth", "AF10", ID_KEY_ALT_F10}, VideoModeKeyTokenInput{"AltBarePrefix", "a", 0},
        VideoModeKeyTokenInput{"AltIncomplete", "af", 0}, VideoModeKeyTokenInput{"AltZero", "af0", 0},
        VideoModeKeyTokenInput{"AltRepeatedF", "aff", 0}, VideoModeKeyTokenInput{"AltEleven", "af11", 0}),
    [](const TestParamInfo<VideoModeKeyTokenInput> &info) { return info.param.name; });

TEST(TestVideoMode, checkKeyNameHonorsStringViewLength)
{
    const std::string storage{"F6-suffix"};
    const std::string_view key_name{storage.data(), 2};

    EXPECT_EQ(ID_KEY_F6, check_vid_mode_key_name(key_name));
}

static void push_modes(const std::initializer_list<int> keys)
{
    for (const int key : keys)
    {
        g_video_mode_keys[g_video_table_len] = key;
        ++g_video_table_len;
    }
}

struct VideoModeBindingInput
{
    std::string name;
    std::vector<int> bindings;
    int query;
    int expected;
};

void PrintTo(const VideoModeBindingInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<VideoModeBindingInput> video_mode_binding_inputs()
{
    const std::vector<int> queries{ID_KEY_F1, ID_KEY_F10, ID_KEY_SHF_F1, ID_KEY_SHF_F10, ID_KEY_CTL_F1, ID_KEY_CTL_F10,
        ID_KEY_ALT_F1, ID_KEY_ALT_F10};
    const std::array<const char *, 8> names{
        "F1", "F10", "ShiftF1", "ShiftF10", "ControlF1", "ControlF10", "AltF1", "AltF10"};
    const std::vector<int> nonmatching{ID_KEY_F2, ID_KEY_SHF_F2, ID_KEY_CTL_F2, ID_KEY_ALT_F2};
    std::vector<VideoModeBindingInput> inputs;
    for (int i = 0; i < 8; ++i)
    {
        inputs.push_back({std::string{"Empty"} + names[i], {}, queries[i], -1});
        inputs.push_back({std::string{"Nonmatching"} + names[i], nonmatching, queries[i], -1});
        inputs.push_back({std::string{"Matching"} + names[i], queries, queries[i], i});
    }
    inputs.push_back({"UnboundZero", {0}, 0, -1});
    inputs.push_back({"UnboundF6", {0}, ID_KEY_F6, -1});
    inputs.push_back({"ReservedEntry", {0}, 1400, MAX_VIDEO_MODES - 1});
    return inputs;
}

class TestVideoModeBindingLookup : public TestWithParam<VideoModeBindingInput>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_length{g_video_table_len, 0};
    ValueSaver<std::array<int, MAX_VIDEO_MODES>> m_saved_bindings{g_video_mode_keys};
};

void TestVideoModeBindingLookup::SetUp()
{
    for (const int key : GetParam().bindings)
    {
        g_video_mode_keys[g_video_table_len] = key;
        ++g_video_table_len;
    }
}

TEST_P(TestVideoModeBindingLookup, findsAssignedMode)
{
    EXPECT_EQ(GetParam().expected, check_vid_mode_key(GetParam().query));
}

INSTANTIATE_TEST_SUITE_P(Layout, TestVideoModeBindingLookup, ValuesIn(video_mode_binding_inputs()),
    [](const TestParamInfo<VideoModeBindingInput> &info) { return info.param.name; });

TEST(TestVideoMode, bindingChangesLeaveVideoInfoUnchanged)
{
    ValueSaver saved_video_table_len{g_video_table_len, 2};
    ValueSaver saved_keys{g_video_mode_keys};
    ValueSaver saved_first{g_video_table[0]};
    ValueSaver saved_second{g_video_table[1]};
    ValueSaver saved_active{g_video_entry};
    const VideoInfo mode{800, 600, 256, nullptr, "same mode"};
    g_video_table[0] = mode;
    g_video_table[1] = mode;
    g_video_entry = mode;
    g_video_mode_keys[0] = ID_KEY_F6;
    g_video_mode_keys[1] = ID_KEY_F7;

    EXPECT_EQ(0, check_vid_mode_key(ID_KEY_F6));
    EXPECT_EQ(1, check_vid_mode_key(ID_KEY_F7));

    g_video_mode_keys[0] = ID_KEY_F8;
    g_video_mode_keys[1] = ID_KEY_F6;

    EXPECT_EQ(1, check_vid_mode_key(ID_KEY_F6));
    EXPECT_EQ(-1, check_vid_mode_key(ID_KEY_F7));
    EXPECT_EQ(0, check_vid_mode_key(ID_KEY_F8));
    EXPECT_EQ(mode, g_video_table[0]);
    EXPECT_EQ(mode, g_video_table[1]);
    EXPECT_EQ(mode, g_video_entry);
}

class TestVideoModeSelection : public Test
{
protected:
    void SetUp() override;
    void expect_key(int key);

    MockDriver m_driver;
    MockDriver m_other_driver;
    const std::string m_name{"display"};
    const std::string m_other_name{"alternate"};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<int> m_saved_len{g_video_table_len, 3};
    ValueSaver<std::array<int, MAX_VIDEO_MODES>> m_saved_keys{g_video_mode_keys};
    ValueSaver<VideoInfo> m_saved_first{g_video_table[0]};
    ValueSaver<VideoInfo> m_saved_second{g_video_table[1]};
    ValueSaver<VideoInfo> m_saved_third{g_video_table[2]};
    ValueSaver<VideoInfo> m_saved_reserved{g_video_table[MAX_VIDEO_MODES - 1]};
    ValueSaver<VideoInfo> m_saved_active{g_video_entry};
    ValueSaver<int> m_saved_active_key{g_video_entry_key};
    ValueSaver<int> m_saved_adapter{g_adapter};
    ValueSaver<int> m_saved_col_base{g_text_col_base, 0};
    ValueSaver<int> m_saved_row_base{g_text_row_base, 0};
};

void TestVideoModeSelection::SetUp()
{
    g_video_table[0] = VideoInfo{640, 480, 256, &m_driver, "first"};
    g_video_table[1] = VideoInfo{800, 600, 256, &m_driver, "unbound"};
    g_video_table[2] = VideoInfo{800, 600, 256, &m_other_driver, "unbound"};
    g_video_mode_keys[0] = ID_KEY_F7;
    g_video_mode_keys[1] = 0;
    g_video_mode_keys[2] = ID_KEY_F6;
    EXPECT_CALL(m_driver, get_name()).WillRepeatedly(ReturnRef(m_name));
    EXPECT_CALL(m_driver, get_description()).WillRepeatedly(ReturnRef(m_name));
    EXPECT_CALL(m_other_driver, get_name()).WillRepeatedly(ReturnRef(m_other_name));
    EXPECT_CALL(m_other_driver, get_description()).WillRepeatedly(ReturnRef(m_other_name));
    EXPECT_CALL(m_driver, set_clear()).Times(AnyNumber());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, hide_text_cursor()).Times(AnyNumber());
}

void TestVideoModeSelection::expect_key(const int key)
{
    const InSequence sequence;
    EXPECT_CALL(m_driver, wait_key_pressed(false)).WillOnce(Return(key));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(key));
}

TEST_F(TestVideoModeSelection, reopeningUnboundModeRetainsItsDimensionsAndDriver)
{
    expect_key(ID_KEY_ENTER);
    const int key{select_video_mode(1)};
    ASSERT_EQ(1400, key);
    g_adapter = check_vid_mode_key(key);
    ASSERT_EQ(MAX_VIDEO_MODES - 1, g_adapter);
    ASSERT_EQ(g_video_table[1], g_video_table[g_adapter]);
    expect_key(ID_KEY_ENTER);

    EXPECT_EQ(1400, select_video_mode(g_adapter));

    EXPECT_EQ(g_video_table[1], g_video_entry);
    EXPECT_EQ(800, g_video_entry.x_dots);
    EXPECT_EQ(600, g_video_entry.y_dots);
    EXPECT_EQ(&m_driver, g_video_entry.driver);
    EXPECT_EQ(g_video_entry, g_video_table[MAX_VIDEO_MODES - 1]);
    EXPECT_EQ(0, g_video_entry_key);
}

TEST_F(TestVideoModeSelection, equalCatalogModesRetainTheRequestedIndex)
{
    g_video_table[1] = g_video_table[0];
    g_video_mode_keys[1] = ID_KEY_F8;
    expect_key(ID_KEY_ENTER);

    EXPECT_EQ(ID_KEY_F8, select_video_mode(1));

    EXPECT_EQ(ID_KEY_F8, g_video_entry_key);
    EXPECT_EQ(g_video_table[1], g_video_entry);
}

TEST_F(TestVideoModeSelection, unmatchedReservedModeDefaultsToFirstSortedEntry)
{
    g_video_table[MAX_VIDEO_MODES - 1] = VideoInfo{1024, 768, 256, &m_driver, "unlisted"};
    expect_key(ID_KEY_ENTER);

    EXPECT_EQ(ID_KEY_F6, select_video_mode(MAX_VIDEO_MODES - 1));

    EXPECT_EQ(g_video_table[2], g_video_entry);
}

TEST_F(TestVideoModeSelection, withoutCurrentModeSelectsFirst256ColorEntry)
{
    g_video_table[0].colors = 16;
    g_video_table[2].colors = 16;
    expect_key(ID_KEY_ENTER);

    EXPECT_EQ(1400, select_video_mode(-1));

    EXPECT_EQ(g_video_table[1], g_video_entry);
}

TEST_F(TestVideoModeSelection, cancelPreservesTheActiveUnboundMode)
{
    g_video_entry = g_video_table[1];
    g_video_entry_key = 0;
    g_video_table[MAX_VIDEO_MODES - 1] = g_video_entry;
    expect_key(ID_KEY_ESC);

    EXPECT_EQ(-1, select_video_mode(MAX_VIDEO_MODES - 1));

    EXPECT_EQ(g_video_table[1], g_video_entry);
    EXPECT_EQ(g_video_entry, g_video_table[MAX_VIDEO_MODES - 1]);
    EXPECT_EQ(0, g_video_entry_key);
}

struct VideoModeKeyNameInput
{
    const char *name;
    int key;
    const char *expected;
};

void PrintTo(const VideoModeKeyNameInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestVideoModeKeyName : public TestWithParam<VideoModeKeyNameInput>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_length{g_video_table_len, 0};
    ValueSaver<std::array<int, MAX_VIDEO_MODES>> m_saved_bindings{g_video_mode_keys};
};

void TestVideoModeKeyName::SetUp()
{
    push_modes({ID_KEY_F1, ID_KEY_F10, ID_KEY_SHF_F1, ID_KEY_SHF_F10, ID_KEY_CTL_F1, ID_KEY_CTL_F10, ID_KEY_ALT_F1,
        ID_KEY_ALT_F10});
}

TEST_P(TestVideoModeKeyName, formatsAssignedKey)
{
    EXPECT_EQ(GetParam().expected, vid_mode_key_name(GetParam().key));
}

INSTANTIATE_TEST_SUITE_P(Key, TestVideoModeKeyName,
    Values(VideoModeKeyNameInput{"F1", ID_KEY_F1, "F1"}, VideoModeKeyNameInput{"F10", ID_KEY_F10, "F10"},
        VideoModeKeyNameInput{"ShiftF1", ID_KEY_SHF_F1, "SF1"},
        VideoModeKeyNameInput{"ShiftF10", ID_KEY_SHF_F10, "SF10"},
        VideoModeKeyNameInput{"ControlF1", ID_KEY_CTL_F1, "CF1"},
        VideoModeKeyNameInput{"ControlF10", ID_KEY_CTL_F10, "CF10"},
        VideoModeKeyNameInput{"AltF1", ID_KEY_ALT_F1, "AF1"}, VideoModeKeyNameInput{"AltF10", ID_KEY_ALT_F10, "AF10"},
        VideoModeKeyNameInput{"Negative", -1, ""}, VideoModeKeyNameInput{"ControlA", ID_KEY_CTL_A, ""}),
    [](const TestParamInfo<VideoModeKeyNameInput> &info) { return info.param.name; });

struct DiskVideoModeInput
{
    const char *name;
    VideoInfo mode;
    bool valid;
};

void PrintTo(const DiskVideoModeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDiskVideoModeValidity : public TestWithParam<DiskVideoModeInput>
{
};

TEST_P(TestDiskVideoModeValidity, validatesImageDimensionsAndColors)
{
    EXPECT_EQ(GetParam().valid, is_valid_disk_video_mode(GetParam().mode));
}

INSTANTIATE_TEST_SUITE_P(Mode, TestDiskVideoModeValidity,
    Values(DiskVideoModeInput{"Large", VideoInfo{32768, 32768, 256, nullptr, ""}, true},
        DiskVideoModeInput{"Maximum", VideoInfo{GIF_MAX_PIXELS, GIF_MAX_PIXELS, 256, nullptr, ""}, true},
        DiskVideoModeInput{"ZeroWidth", VideoInfo{0, 480, 256, nullptr, ""}, false},
        DiskVideoModeInput{"ZeroHeight", VideoInfo{640, 0, 256, nullptr, ""}, false},
        DiskVideoModeInput{"ExcessWidth", VideoInfo{GIF_MAX_PIXELS + 1, 480, 256, nullptr, ""}, false},
        DiskVideoModeInput{"ExcessHeight", VideoInfo{640, GIF_MAX_PIXELS + 1, 256, nullptr, ""}, false},
        DiskVideoModeInput{"UnsupportedColors", VideoInfo{640, 480, 16, nullptr, ""}, false}),
    [](const TestParamInfo<DiskVideoModeInput> &info) { return info.param.name; });

struct DisplayVideoModeInput
{
    const char *name;
    VideoInfo mode;
    int width;
    int height;
    bool valid;
};

void PrintTo(const DisplayVideoModeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDisplayVideoModeValidity : public TestWithParam<DisplayVideoModeInput>
{
};

TEST_P(TestDisplayVideoModeValidity, validatesModeAgainstDisplay)
{
    EXPECT_EQ(GetParam().valid, is_valid_display_video_mode(GetParam().mode, GetParam().width, GetParam().height));
}

INSTANTIATE_TEST_SUITE_P(Mode, TestDisplayVideoModeValidity,
    Values(DisplayVideoModeInput{"Large", VideoInfo{32768, 32768, 256, nullptr, ""}, 32768, 32768, true},
        DisplayVideoModeInput{"Maximum", VideoInfo{GIF_MAX_PIXELS, GIF_MAX_PIXELS, 256, nullptr, ""}, GIF_MAX_PIXELS,
            GIF_MAX_PIXELS, true},
        DisplayVideoModeInput{"ZeroWidth", VideoInfo{0, 480, 256, nullptr, ""}, 640, 480, false},
        DisplayVideoModeInput{"ZeroHeight", VideoInfo{640, 0, 256, nullptr, ""}, 640, 480, false},
        DisplayVideoModeInput{"UnsupportedColors", VideoInfo{640, 480, 16, nullptr, ""}, 640, 480, false},
        DisplayVideoModeInput{"ZeroDisplayWidth", VideoInfo{640, 480, 256, nullptr, ""}, 0, 480, false},
        DisplayVideoModeInput{"ZeroDisplayHeight", VideoInfo{640, 480, 256, nullptr, ""}, 640, 0, false},
        DisplayVideoModeInput{"ExcessWidth", VideoInfo{1025, 768, 256, nullptr, ""}, 1024, 768, false},
        DisplayVideoModeInput{"ExcessHeight", VideoInfo{1024, 769, 256, nullptr, ""}, 1024, 768, false},
        DisplayVideoModeInput{"ExactFit", VideoInfo{1024, 768, 256, nullptr, ""}, 1024, 768, true}),
    [](const TestParamInfo<DisplayVideoModeInput> &info) { return info.param.name; });

} // namespace id::test
