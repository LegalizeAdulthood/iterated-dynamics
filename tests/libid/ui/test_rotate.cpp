// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/rotate.h>

#include "ColorMapSaver.h"
#include "MockDriver.h"
#include "test_data.h"

#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/VideoInfo.h>
#include <io/loadmap.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <optional>
#include <ostream>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestPaletteRotation : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void expect_color(int index, int source);

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_count{g_dac_count, 250};
    ValueSaver<int> m_saved_lo{g_color_cycle_range_lo, 1};
    ValueSaver<int> m_saved_hi{g_color_cycle_range_hi, 4};
    ValueSaver<bool> m_saved_real_dac{g_got_real_dac, true};
    ValueSaver<bool> m_saved_true_color{g_is_true_color, false};
    ValueSaver<TrueColorMode> m_saved_true_mode{g_true_mode, TrueColorMode::DEFAULT_COLOR};
    std::optional<ColorMapSaver> m_saved_palette;
    std::array<Byte, sizeof(g_old_dac_box)> m_saved_old_palette{};
};

void TestPaletteRotation::SetUp()
{
    m_saved_palette.emplace();
    std::memcpy(m_saved_old_palette.data(), g_old_dac_box, sizeof(g_old_dac_box));
    for (int color = 0; color < 256; ++color)
    {
        for (int channel = 0; channel < 3; ++channel)
        {
            g_dac_box[color][channel] = static_cast<Byte>((color + channel) % 64);
        }
    }
    std::memcpy(g_old_dac_box, g_dac_box, sizeof(g_dac_box));
}

void TestPaletteRotation::TearDown()
{
    m_saved_palette.reset();
    std::memcpy(g_old_dac_box, m_saved_old_palette.data(), sizeof(g_old_dac_box));
}

void TestPaletteRotation::expect_color(const int index, const int source)
{
    for (int channel = 0; channel < 3; ++channel)
    {
        EXPECT_EQ((source + channel) % 64, g_dac_box[index][channel]) << index << ":" << channel;
    }
}

struct PaletteRotationInput
{
    const char *name;
    SpinDirection direction;
    int steps;
    std::array<int, 4> sources;
};

void PrintTo(const PaletteRotationInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPaletteRotationSteps : public TestPaletteRotation, public WithParamInterface<PaletteRotationInput>
{
};

TEST_P(TestPaletteRotationSteps, rotatesPaletteWithoutDelaying)
{
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(_)).Times(0);

    spin_dac(GetParam().direction, GetParam().steps);

    expect_color(0, 0);
    for (int color = 1; color <= 4; ++color)
    {
        expect_color(color, GetParam().sources[color - 1]);
    }
    for (int color = 5; color < 256; ++color)
    {
        expect_color(color, color);
    }
}

INSTANTIATE_TEST_SUITE_P(Direction, TestPaletteRotationSteps,
    Values(PaletteRotationInput{"Forward", SpinDirection::FORWARD, 1, {2, 3, 4, 1}},
        PaletteRotationInput{"Backward", SpinDirection::BACKWARD, 2, {3, 4, 1, 2}}),
    [](const TestParamInfo<PaletteRotationInput> &info) { return info.param.name; });

TEST_F(TestPaletteRotation, plainRefreshWritesUnchangedPaletteWithoutDelaying)
{
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(_)).Times(0);

    refresh_dac();

    EXPECT_EQ(0, std::memcmp(g_old_dac_box, g_dac_box, sizeof(g_dac_box)));
}

TEST_F(TestPaletteRotation, engineClampsCyclingRangeToAvailableColors)
{
    g_colors = 16;
    g_color_cycle_range_lo = 14;
    g_color_cycle_range_hi = 255;
    EXPECT_CALL(m_driver, write_palette());

    spin_dac(SpinDirection::FORWARD, 1);

    expect_color(13, 13);
    expect_color(14, 15);
    expect_color(15, 14);
    expect_color(16, 16);
}

TEST_F(TestPaletteRotation, disabledRangeStillRefreshesWithoutRotation)
{
    g_color_cycle_range_hi = g_color_cycle_range_lo;
    EXPECT_CALL(m_driver, write_palette());

    spin_dac(SpinDirection::FORWARD, 1);

    EXPECT_EQ(0, std::memcmp(g_old_dac_box, g_dac_box, sizeof(g_dac_box)));
}

struct UnsupportedPaletteInput
{
    const char *name;
    int colors;
    bool true_color;
    TrueColorMode mode;
};

void PrintTo(const UnsupportedPaletteInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPaletteRotationUnsupported : public TestPaletteRotation, public WithParamInterface<UnsupportedPaletteInput>
{
};

TEST_P(TestPaletteRotationUnsupported, doesNotWritePalette)
{
    EXPECT_CALL(m_driver, write_palette()).Times(0);
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    g_colors = GetParam().colors;
    g_is_true_color = GetParam().true_color;
    g_true_mode = GetParam().mode;

    refresh_dac();

    EXPECT_EQ(0, std::memcmp(g_old_dac_box, g_dac_box, sizeof(g_dac_box)));
}

INSTANTIATE_TEST_SUITE_P(Configuration, TestPaletteRotationUnsupported,
    Values(UnsupportedPaletteInput{"TwoColors", 2, false, TrueColorMode::DEFAULT_COLOR},
        UnsupportedPaletteInput{"TrueColorIterate", 256, true, TrueColorMode::ITERATE}),
    [](const TestParamInfo<UnsupportedPaletteInput> &info) { return info.param.name; });

TEST_F(TestPaletteRotation, cyclingControllerAppliesSpeedKeysBeforeTheNextDelay)
{
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    InSequence sequence;
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(ID_KEY_DOWN_ARROW));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_DOWN_ARROW));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(6));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(ID_KEY_UP_ARROW));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_UP_ARROW));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(5));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(ID_KEY_ESC));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));

    rotate(SpinDirection::FORWARD);

    EXPECT_EQ(250, g_dac_count);
    expect_color(1, 3);
}

TEST_F(TestPaletteRotation, pauseAndResumeRefreshTheBorderWithoutChangingSpeedOrDelaying)
{
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    InSequence sequence;
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(' '));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(' '));
    EXPECT_CALL(m_driver, write_palette())
        .WillOnce(Invoke(
            []
            {
                EXPECT_EQ(250, g_dac_count);
                for (const Byte channel : g_dac_box[0])
                {
                    EXPECT_EQ(48, channel);
                }
            }));
    EXPECT_CALL(m_driver, wait_key_pressed(false)).WillOnce(Return(' '));
    EXPECT_CALL(m_driver, write_palette()).WillOnce(Invoke([this] { expect_color(0, 0); }));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(' '));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(' '));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(5));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(ID_KEY_ESC));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));

    rotate(SpinDirection::FORWARD);

    EXPECT_EQ(250, g_dac_count);
    expect_color(0, 0);
    expect_color(1, 2);
}

TEST_F(TestPaletteRotation, singleStepDelaysOnceThenPausesWithoutDelayingBorderRefresh)
{
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    InSequence sequence;
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return('<'));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return('<'));
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(5));
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, wait_key_pressed(false)).WillOnce(Return(ID_KEY_ESC));
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(ID_KEY_ESC));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));

    rotate(SpinDirection::FORWARD);

    EXPECT_EQ(250, g_dac_count);
    expect_color(1, 2);
}

TEST_F(TestPaletteRotation, cyclingDoesNotDelayWhenTrueColorPreventsPaletteUpdates)
{
    g_is_true_color = true;
    g_true_mode = TrueColorMode::ITERATE;
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, write_palette()).Times(0);
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    InSequence sequence;
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(ID_KEY_ESC));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));

    rotate(SpinDirection::FORWARD);
}

TEST_F(TestPaletteRotation, missingPaletteIsReportedAndSelectionRetriesWithoutChangingPriorPalette)
{
    ValueSaver saved_first_init{g_first_init, false};
    ValueSaver saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver saved_map_name{g_map_name, std::string{"previous.map"}};
    ValueSaver saved_last_map_name{g_last_map_name};
    ValueSaver saved_color_state{g_color_state};
    const std::string filename{std::string{data::ID_TEST_MAP_DIR} + "/" + data::ID_TEST_MAP_FILE};
    EXPECT_CALL(m_driver, stack_screen()).Times(3);
    EXPECT_CALL(m_driver, unstack_screen()).Times(3);
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(2);
    EXPECT_CALL(m_driver, move_cursor(_, _));
    EXPECT_CALL(m_driver, set_attr(_, _, _, _));
    EXPECT_CALL(m_driver, hide_text_cursor());
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    InSequence sequence;
    EXPECT_CALL(m_driver, get_filename(_, _, _, _))
        .WillOnce(Invoke(
            [](const char *, const char *, const char *, std::string &name)
            {
                name = "missing-palette-selection.map";
                return false;
            }));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
    EXPECT_CALL(m_driver, get_filename(_, _, _, _))
        .WillOnce(Invoke(
            [this, &filename](const char *, const char *, const char *, std::string &name)
            {
                EXPECT_EQ("previous.map", g_map_name);
                expect_color(0, 0);
                expect_color(255, 255);
                name = filename;
                return false;
            }));

    EXPECT_FALSE(load_palette());

    EXPECT_EQ(filename, g_map_name);
    EXPECT_EQ(filename, g_last_map_name);
    EXPECT_EQ(0, std::memcmp(g_dac_box, g_old_dac_box, sizeof(g_dac_box)));
}

} // namespace id::test
