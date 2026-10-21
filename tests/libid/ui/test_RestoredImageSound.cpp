// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#include <ui/restore_image.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/Potential.h>
#include <engine/sound.h>
#include <engine/spindac.h>
#include <engine/SoundOutput.h>
#include <engine/VideoInfo.h>
#include <fractals/lorenz.h>
#include <io/check_write_file.h>
#include <io/decoder.h>
#include <io/gifview.h>
#include <io/GifWriter.h>
#include <io/library.h>
#include <io/loadfile.h>
#include <io/special_dirs.h>
#include <misc/ValueSaver.h>
#include <ui/evolve.h>
#include <ui/slideshw.h>
#include <ui/SoundSession.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
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

class MockRestoreOutput : public SoundOutput
{
public:
    MOCK_METHOD(bool, play, (const SoundSample &), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, ready, (), (const, override));
    MOCK_METHOD(void, begin_orbit, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, finish, (), (override));
};

std::vector<std::array<int, 3>> s_restored_pixels;

void record_pixel(const int col, const int row, const int color)
{
    s_restored_pixels.push_back({col, row, color});
}

void fail_pixel(int, int, int)
{
    throw std::runtime_error("Pixel output failed");
}

} // namespace

class TestRestoredImageSound : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    std::shared_ptr<StrictMock<MockRestoreOutput>> m_output{std::make_shared<StrictMock<MockRestoreOutput>>()};
    std::filesystem::path m_directory;
    Byte m_palette[256][3]{};
    std::vector<int> m_samples;
    int m_delays{};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, record_pixel};
    ValueSaver<decltype(g_out_line)> m_saved_out_line{g_out_line, out_line};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{2, 2, 0, 0, 1.0, 1.0}};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_row{g_row_count, 0};
    ValueSaver<int> m_saved_sound_flag{g_sound_flag, SOUNDFLAG_X};
    ValueSaver<int> m_saved_orbit_flags{g_orbit_save_flags, 0};
    ValueSaver<int> m_saved_base_hertz{g_base_hertz, 220};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<std::filesystem::path> m_saved_read_filename{g_read_filename};
    ValueSaver<std::filesystem::path> m_saved_save_dir{g_save_dir};
    ValueSaver<bool> m_saved_read_color{g_read_color, true};
    ValueSaver<ColorState> m_saved_color_state{g_color_state};
    ValueSaver<Display3DMode> m_saved_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver<bool> m_saved_map_set{g_map_set, false};
    ValueSaver<bool> m_saved_dither{g_dither_flag, false};
    ValueSaver<bool> m_saved_potential{g_potential.store_16bit, false};
    ValueSaver<int> m_saved_skip_x{g_skip_x_dots, 0};
    ValueSaver<int> m_saved_skip_y{g_skip_y_dots, 0};
    ValueSaver<CalcStatus> m_saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<bool> m_saved_busy{g_busy, false};
    ValueSaver<EvolutionModeFlags> m_saved_evolving{g_evolving, EvolutionModeFlags::NONE};
    ValueSaver<bool> m_saved_overwrite{g_overwrite_file, false};
};

void TestRestoredImageSound::SetUp()
{
    const std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    m_directory = std::filesystem::temp_directory_path() / ("id-restored-sound-" + name);
    ASSERT_TRUE(std::filesystem::create_directory(m_directory));
    g_read_filename = m_directory / "image.gif";
    g_save_dir = m_directory;
    set_save_library(m_directory);
    std::memcpy(m_palette, g_dac_box, sizeof(m_palette));
    s_restored_pixels.clear();
    GifWriter writer{g_read_filename, {2, 2}, {{255, 0, 0}, {0, 0, 0}}};
    ASSERT_TRUE(writer.write_pixels({0, 1}));
    ASSERT_TRUE(writer.write_pixels({1, 0}));
    ASSERT_TRUE(writer.finish());
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    ON_CALL(m_driver, key_pressed()).WillByDefault(Return('x'));
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, wait_key_pressed(_)).Times(0);
    EXPECT_CALL(m_driver, delay(1)).WillRepeatedly([this] { ++m_delays; });
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(
            [this](const SoundSample &sample)
            {
                EXPECT_TRUE(g_busy);
                EXPECT_EQ(m_samples.size(), s_restored_pixels.size());
                m_samples.push_back(static_cast<int>(sample.x));
                return true;
            });
    EXPECT_CALL(*m_output, update()).Times(AnyNumber());
    EXPECT_CALL(*m_output, ready()).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_output, finish()).WillRepeatedly(Return(true));
}

void TestRestoredImageSound::TearDown()
{
    s_restored_pixels.clear();
    clear_save_library();
    std::memcpy(g_dac_box, m_palette, sizeof(m_palette));
    std::filesystem::remove_all(m_directory);
}

TEST_F(TestRestoredImageSound, coordinatesScanlinesAndReleaseTailsWithoutConsumingPendingInput)
{
    int pending{};
    bool rejected{};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(
            [this, &pending, &rejected](const SoundSample &sample)
            {
                EXPECT_TRUE(g_busy);
                EXPECT_EQ(0, pending);
                EXPECT_EQ(m_samples.size(), s_restored_pixels.size());
                EXPECT_EQ(m_samples.size() < 2 ? 0 : 1, g_row_count);
                if (!rejected)
                {
                    rejected = true;
                    return false;
                }
                m_samples.push_back(static_cast<int>(sample.x));
                pending = 2;
                return true;
            });
    EXPECT_CALL(*m_output, update())
        .WillRepeatedly(
            [&pending]
            {
                if (pending > 0)
                {
                    --pending;
                }
            });
    EXPECT_CALL(*m_output, ready()).WillRepeatedly([&pending] { return pending == 0; });
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(false)).WillOnce(Return(false)).WillOnce(Return(true));
    EXPECT_CALL(*m_output, mute()).Times(0);
    SoundSession sound{m_output};

    ASSERT_EQ(0, restore_image(sound));

    EXPECT_THAT(m_samples, ElementsAre(0, 1, 1, 0));
    EXPECT_THAT(s_restored_pixels,
        ElementsAre(std::array{0, 0, 0}, std::array{1, 0, 1}, std::array{0, 1, 1}, std::array{1, 1, 0}));
    EXPECT_EQ(6, m_delays);
    EXPECT_EQ(2, g_row_count);
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

TEST_F(TestRestoredImageSound, readFailureClosesTheGifAndFinishesAcceptedSoundNormally)
{
    std::filesystem::resize_file(g_read_filename, std::filesystem::file_size(g_read_filename) - 1);
    EXPECT_CALL(*m_output, mute()).Times(0);
    SoundSession sound{m_output};

    EXPECT_EQ(-1, restore_image(sound));

    EXPECT_THAT(m_samples, ElementsAre(0, 1, 1, 0));
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

TEST_F(TestRestoredImageSound, pixelFailureClosesTheGifAndRestoresBusyState)
{
    ValueSaver plot{g_put_color, fail_pixel};
    EXPECT_CALL(*m_output, mute()).Times(1);
    {
        SoundSession sound{m_output};

        EXPECT_THROW(restore_image(sound), std::runtime_error);
    }

    EXPECT_THAT(m_samples, ElementsAre(0));
    EXPECT_TRUE(s_restored_pixels.empty());
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

TEST_F(TestRestoredImageSound, soundFailureDoesNotPlotAndClosesTheGif)
{
    EXPECT_CALL(*m_output, play(_)).WillOnce(Throw(std::runtime_error("Sound output failed")));
    EXPECT_CALL(*m_output, mute()).Times(1);
    {
        SoundSession sound{m_output};

        EXPECT_THROW(restore_image(sound), std::runtime_error);
    }

    EXPECT_TRUE(s_restored_pixels.empty());
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

TEST_F(TestRestoredImageSound, missingFileFinishesWithoutMuting)
{
    std::filesystem::remove(g_read_filename);
    EXPECT_CALL(*m_output, mute()).Times(0);
    SoundSession sound{m_output};

    EXPECT_EQ(-1, restore_image(sound));

    EXPECT_TRUE(s_restored_pixels.empty());
    EXPECT_FALSE(g_busy);
}

TEST_F(TestRestoredImageSound, finalizationFailureClosesTheGifWithoutMutingAClosedPort)
{
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("Sound close failed")));
    EXPECT_CALL(*m_output, mute()).Times(0);
    {
        SoundSession sound{m_output};

        EXPECT_THROW(restore_image(sound), std::runtime_error);
    }

    EXPECT_EQ(4U, s_restored_pixels.size());
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

TEST_F(TestRestoredImageSound, uiEntrySelectsRestoredMappingAndWritesEachLegacyRecordOnce)
{
    g_orbit_save_flags = OSF_MIDI;
    EXPECT_CALL(m_driver, sound_on(220)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, sound_on(407)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, sound_off()).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);
    const SoundOutputPtr previous{g_sound};

    ASSERT_EQ(0, restore_image());

    EXPECT_EQ(previous, g_sound);
    std::ifstream file{m_directory / "sound/sound001.txt"};
    ASSERT_TRUE(file.is_open());
    const std::string records{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    EXPECT_EQ("220\n407\n407\n220\n", records);
    EXPECT_EQ(4U, s_restored_pixels.size());
    EXPECT_EQ(0, m_delays);
    file.close();
    EXPECT_TRUE(std::filesystem::remove(m_directory / "sound/sound001.txt"));
}

} // namespace id::test
