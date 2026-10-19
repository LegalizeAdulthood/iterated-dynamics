// SPDX-License-Identifier: GPL-3.0-only
//
#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/sound.h>
#include <engine/VideoInfo.h>
#include <fractals/lorenz.h>
#include <io/gifview.h>
#include <io/RestoredImageOutput.h>
#include <misc/ValueSaver.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

std::vector<std::array<int, 3>> s_pixels;

void capture_restored_pixel(const int col, const int row, const int color)
{
    s_pixels.push_back({col, row, color});
}

} // namespace

class TestRestoredImageOutput : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void drain(RestoredImageOutput &output);

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, capture_restored_pixel};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{4, 4, 0, 0, 3.0, 3.0}};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_row_count{g_row_count, 0};
    ValueSaver<int> m_saved_orbit_flags{g_orbit_save_flags, 0};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<int> m_saved_base_hertz{g_base_hertz, 220};
};

void TestRestoredImageOutput::SetUp()
{
    s_pixels.clear();
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, sound_on(_)).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);
    EXPECT_CALL(m_driver, delay(_)).Times(0);
}

void TestRestoredImageOutput::TearDown()
{
    s_pixels.clear();
}

void TestRestoredImageOutput::drain(RestoredImageOutput &output)
{
    for (int steps = 0; steps < 100; ++steps)
    {
        if (output.done())
        {
            return;
        }
        output.iterate();
    }
    FAIL() << "Restored-image output did not complete";
}

TEST_F(TestRestoredImageOutput, iterationCommitsExactlyOnePixel)
{
    RestoredImageOutput output{4};
    const Byte pixels[]{3, 5, 7};
    output.set_line(pixels, 3);

    EXPECT_FALSE(output.done());
    EXPECT_EQ(3, output.pending_color());
    EXPECT_EQ(3, output.pending_color());
    EXPECT_TRUE(s_pixels.empty());
    output.iterate();

    const std::vector<std::array<int, 3>> expected{{0, 0, 3}};
    EXPECT_EQ(expected, s_pixels);
    EXPECT_FALSE(output.done());
    EXPECT_EQ(0, output.row());
    EXPECT_EQ(1, output.column());
    EXPECT_EQ(5, output.pending_color());
    drain(output);
    EXPECT_TRUE(output.done());
    EXPECT_EQ(3, output.column());
    output.iterate();
    EXPECT_EQ(3U, s_pixels.size());
    EXPECT_THROW(output.pending_color(), std::logic_error);
}

TEST_F(TestRestoredImageOutput, ownsPixelsAfterCallerReusesAndReleasesBuffer)
{
    RestoredImageOutput output{4};
    {
        std::vector<Byte> pixels{9, 8, 7};
        output.set_line(pixels.data(), 3);
        pixels.assign(100, 0);
    }

    drain(output);

    const std::vector<std::array<int, 3>> expected{{0, 0, 9}, {1, 0, 8}, {2, 0, 7}};
    EXPECT_EQ(expected, s_pixels);
}

TEST_F(TestRestoredImageOutput, wrapsWideLinesFromPartialRow)
{
    RestoredImageOutput output{3, 4, 2};
    const Byte pixels[]{1, 2, 3, 4, 5};
    output.set_line(pixels, 5);
    output.iterate();

    EXPECT_EQ(5, output.row());
    EXPECT_EQ(0, output.column());
    EXPECT_FALSE(output.done());
    drain(output);

    const std::vector<std::array<int, 3>> expected{{2, 4, 1}, {0, 5, 2}, {1, 5, 3}, {2, 5, 4}, {0, 6, 5}};
    EXPECT_EQ(expected, s_pixels);
    EXPECT_EQ(6, output.row());
    EXPECT_EQ(1, output.column());
}

TEST_F(TestRestoredImageOutput, nextLineContinuesAtCurrentColumn)
{
    RestoredImageOutput output{3};
    const Byte first[]{1, 2};
    output.set_line(first, 2);
    drain(output);
    const Byte second[]{3, 4, 5, 6};
    output.set_line(second, 4);

    drain(output);

    const std::vector<std::array<int, 3>> expected{{0, 0, 1}, {1, 0, 2}, {2, 0, 3}, {0, 1, 4}, {1, 1, 5}, {2, 1, 6}};
    EXPECT_EQ(expected, s_pixels);
    EXPECT_EQ(2, output.row());
    EXPECT_EQ(0, output.column());
}

TEST_F(TestRestoredImageOutput, emptyLineDoesNotMovePosition)
{
    RestoredImageOutput output{4, 2, 1};

    output.set_line(nullptr, 0);
    output.iterate();

    EXPECT_TRUE(output.done());
    EXPECT_TRUE(s_pixels.empty());
    EXPECT_EQ(2, output.row());
    EXPECT_EQ(1, output.column());
    const Byte pixels[]{6};
    output.set_line(pixels, 1);
    drain(output);
    const std::vector<std::array<int, 3>> expected{{1, 2, 6}};
    EXPECT_EQ(expected, s_pixels);
}

TEST_F(TestRestoredImageOutput, replacingPendingLineThrowsWithoutLosingPixels)
{
    RestoredImageOutput output{4};
    const Byte first[]{1, 2};
    const Byte second[]{8, 9};
    output.set_line(first, 2);
    output.iterate();

    EXPECT_THROW(output.set_line(second, 2), std::logic_error);
    EXPECT_THROW(output.set_line(nullptr, 0), std::logic_error);
    drain(output);

    const std::vector<std::array<int, 3>> expected{{0, 0, 1}, {1, 0, 2}};
    EXPECT_EQ(expected, s_pixels);
}

TEST_F(TestRestoredImageOutput, rejectsInvalidPositionsAndLineArguments)
{
    EXPECT_THROW(RestoredImageOutput{0}, std::logic_error);
    EXPECT_THROW(RestoredImageOutput{-1}, std::logic_error);
    EXPECT_THROW((RestoredImageOutput{4, -1, 0}), std::logic_error);
    EXPECT_THROW((RestoredImageOutput{4, 0, -1}), std::logic_error);
    EXPECT_THROW((RestoredImageOutput{4, 0, 4}), std::logic_error);
    RestoredImageOutput output{4};
    const Byte pixels[]{1};

    EXPECT_THROW(output.set_line(pixels, -1), std::logic_error);
    EXPECT_THROW(output.set_line(nullptr, 1), std::logic_error);

    EXPECT_TRUE(output.done());
    EXPECT_TRUE(s_pixels.empty());
}

TEST_F(TestRestoredImageOutput, instancesKeepIndependentBuffersAndPositions)
{
    RestoredImageOutput first{2};
    RestoredImageOutput second{3, 5, 1};
    const Byte first_pixels[]{1, 2};
    const Byte second_pixels[]{3, 4};
    first.set_line(first_pixels, 2);
    second.set_line(second_pixels, 2);

    first.iterate();
    second.iterate();
    first.iterate();
    second.iterate();

    const std::vector<std::array<int, 3>> expected{{0, 0, 1}, {1, 5, 3}, {1, 0, 2}, {2, 5, 4}};
    EXPECT_EQ(expected, s_pixels);
    EXPECT_TRUE(first.done());
    EXPECT_TRUE(second.done());
    EXPECT_EQ(1, first.row());
    EXPECT_EQ(6, second.row());
}

TEST_F(TestRestoredImageOutput, legacySoundCallbackPreservesWrappingMappingAndOrder)
{
    g_row_count = 5;
    std::vector<int> tones;
    EXPECT_CALL(m_driver, sound_on(_))
        .WillRepeatedly(Invoke(
            [&tones](int tone)
            {
                EXPECT_EQ(tones.size() + 1, s_pixels.size());
                EXPECT_EQ(s_pixels.back()[2] * 3000 / g_colors + g_base_hertz, tone);
                tones.push_back(tone);
                return false;
            }));
    EXPECT_CALL(m_driver, mute()).Times(2);
    Byte first[]{1, 2, 3, 4, 5, 6};
    Byte second[]{7, 8, 9, 10, 11, 12};

    EXPECT_EQ(0, sound_line(first, 6));
    EXPECT_EQ(6, g_row_count);
    EXPECT_EQ(0, sound_line(second, 6));

    std::vector<std::array<int, 3>> expected;
    for (int i = 0; i < 12; ++i)
    {
        expected.push_back({i % 4, 5 + i / 4, i + 1});
    }
    EXPECT_EQ(expected, s_pixels);
    EXPECT_EQ(12U, tones.size());
    EXPECT_EQ(8, g_row_count);
}

TEST_F(TestRestoredImageOutput, legacyEmptyLineDoesNotPlotOrAdvance)
{
    g_row_count = 3;
    EXPECT_CALL(m_driver, mute()).Times(1);

    EXPECT_EQ(0, sound_line(nullptr, 0));

    EXPECT_TRUE(s_pixels.empty());
    EXPECT_EQ(3, g_row_count);
}

} // namespace id::test
