// SPDX-License-Identifier: GPL-3.0-only
//
#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/sound.h>
#include <engine/SoundOutput.h>
#include <engine/VideoInfo.h>
#include <fractals/lorenz.h>
#include <io/gifview.h>
#include <io/RestoredImageOutput.h>
#include <misc/ValueSaver.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <memory>
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

class MockRestoredSound : public SoundOutput
{
public:
    MOCK_METHOD(bool, play, (const SoundSample &), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, ready, (), (const, override));
    MOCK_METHOD(void, begin_orbit, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, finish, (), (override));
};

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
    std::shared_ptr<StrictMock<MockRestoredSound>> m_sound{std::make_shared<StrictMock<MockRestoredSound>>()};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound, m_sound};
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
    EXPECT_CALL(*m_sound, play(_)).WillRepeatedly(Return(true));
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
    EXPECT_TRUE(s_pixels.empty());
    output.iterate();

    const std::vector<std::array<int, 3>> expected{{0, 0, 3}};
    EXPECT_EQ(expected, s_pixels);
    EXPECT_FALSE(output.done());
    EXPECT_EQ(0, output.row());
    EXPECT_EQ(1, output.column());
    drain(output);
    EXPECT_TRUE(output.done());
    EXPECT_EQ(3, output.column());
    output.iterate();
    EXPECT_EQ(3U, s_pixels.size());
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

TEST_F(TestRestoredImageOutput, refusalRetainsThePixelAndItsPositionUntilAccepted)
{
    RestoredImageOutput output{4, 2, 1};
    const Byte pixels[]{3, 5};
    output.set_line(pixels, 2);
    EXPECT_CALL(*m_sound, play(FieldsAre(3.0, 0.0, 0.0)))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_sound, play(FieldsAre(5.0, 0.0, 0.0))).WillOnce(Return(true));

    output.iterate();
    output.iterate();

    EXPECT_TRUE(s_pixels.empty());
    EXPECT_EQ(2, output.row());
    EXPECT_EQ(1, output.column());
    EXPECT_FALSE(output.done());
    drain(output);
    EXPECT_THAT(s_pixels, ElementsAre(std::array{1, 2, 3}, std::array{2, 2, 5}));
}

TEST_F(TestRestoredImageOutput, acceptsEachSampleBeforePlottingWithoutServicingPlayback)
{
    RestoredImageOutput output{4};
    const Byte pixels[]{3, 5};
    output.set_line(pixels, 2);
    int samples{};
    EXPECT_CALL(*m_sound, play(_))
        .WillRepeatedly(
            [&samples](const SoundSample &sample)
            {
                EXPECT_EQ(samples, s_pixels.size());
                EXPECT_EQ(samples == 0 ? 3.0 : 5.0, sample.x);
                ++samples;
                return true;
            });

    drain(output);

    EXPECT_EQ(2, samples);
    EXPECT_EQ(2U, s_pixels.size());
}

TEST_F(TestRestoredImageOutput, soundFailureDoesNotAdvanceOrPlotThePendingPixel)
{
    RestoredImageOutput output{4};
    const Byte pixels[]{3};
    output.set_line(pixels, 1);
    EXPECT_CALL(*m_sound, play(_)).WillOnce(Throw(std::runtime_error("Sound output failed")));

    EXPECT_THROW(output.iterate(), std::runtime_error);

    EXPECT_TRUE(s_pixels.empty());
    EXPECT_EQ(0, output.column());
    EXPECT_FALSE(output.done());
}

} // namespace id::test
