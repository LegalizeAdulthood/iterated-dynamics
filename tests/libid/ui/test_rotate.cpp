// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/rotate.h>

#include "MockDriver.h"

#include <engine/VideoInfo.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>

using namespace id::engine;
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
    std::array<Byte, sizeof(g_dac_box)> m_saved_palette{};
    std::array<Byte, sizeof(g_old_dac_box)> m_saved_old_palette{};
};

void TestPaletteRotation::SetUp()
{
    std::memcpy(m_saved_palette.data(), g_dac_box, sizeof(g_dac_box));
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
    std::memcpy(g_dac_box, m_saved_palette.data(), sizeof(g_dac_box));
    std::memcpy(g_old_dac_box, m_saved_old_palette.data(), sizeof(g_old_dac_box));
}

void TestPaletteRotation::expect_color(const int index, const int source)
{
    for (int channel = 0; channel < 3; ++channel)
    {
        EXPECT_EQ((source + channel) % 64, g_dac_box[index][channel]) << index << ":" << channel;
    }
}

TEST_F(TestPaletteRotation, engineRotatesForwardWithoutDelaying)
{
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(_)).Times(0);

    spin_dac(SpinDirection::FORWARD, 1);

    expect_color(0, 0);
    expect_color(1, 2);
    expect_color(2, 3);
    expect_color(3, 4);
    expect_color(4, 1);
    for (int color = 5; color < 256; ++color)
    {
        expect_color(color, color);
    }
}

TEST_F(TestPaletteRotation, engineRotatesBackwardMultipleStepsWithoutDelaying)
{
    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(_)).Times(0);

    spin_dac(SpinDirection::BACKWARD, 2);

    expect_color(0, 0);
    expect_color(1, 3);
    expect_color(2, 4);
    expect_color(3, 1);
    expect_color(4, 2);
    for (int color = 5; color < 256; ++color)
    {
        expect_color(color, color);
    }
}

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

TEST_F(TestPaletteRotation, engineDoesNotWriteUnsupportedPalettes)
{
    EXPECT_CALL(m_driver, write_palette()).Times(0);
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    g_colors = 2;
    refresh_dac();

    g_colors = 256;
    g_is_true_color = true;
    g_true_mode = TrueColorMode::ITERATE;
    refresh_dac();

    EXPECT_EQ(0, std::memcmp(g_old_dac_box, g_dac_box, sizeof(g_dac_box)));
}

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

} // namespace id::test
