// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/Potential.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <fractals/lorenz.h>
#include <io/decoder.h>
#include <io/disk_storage.h>
#include <io/gifview.h>
#include <io/GifWriter.h>
#include <io/library.h>
#include <io/loadfile.h>
#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/memory.h>
#include <misc/ValueSaver.h>
#include <ui/diskvid.h>
#include <ui/id_keys.h>
#include <ui/restore_image.h>
#include <ui/slideshw.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <stdexcept>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestPotentialRestore : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void prepare_storage();
    void expect_potential_bytes();

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{3, 2, 1, 1, 2.0, 1.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 5};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 4};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<OutLine> m_saved_out_line{g_out_line, pot_line};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{{}, true, true}};
    ValueSaver<int> m_saved_row{g_row_count, 0};
    ValueSaver<bool> m_saved_read_color{g_read_color, true};
    ValueSaver<ColorState> m_saved_color_state{g_color_state};
    ValueSaver<Display3DMode> m_saved_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver<bool> m_saved_map_set{g_map_set, false};
    ValueSaver<bool> m_saved_dither{g_dither_flag, false};
    ValueSaver<int> m_saved_skip_x{g_skip_x_dots, 0};
    ValueSaver<int> m_saved_skip_y{g_skip_y_dots, 0};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<bool> m_saved_busy{g_busy, false};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<bool> m_saved_good_mode{g_good_mode, true};
    ValueSaver<std::filesystem::path> m_saved_temp{g_temp_dir};
    ValueSaver<std::filesystem::path> m_saved_read_filename{g_read_filename};
    Byte m_palette[256][3]{};
    std::array<int, 20> m_frame{};
    std::filesystem::path m_directory;
};

void TestPotentialRestore::SetUp()
{
    end_disk();
    init_memory();
    set_normal_dot();
    std::memcpy(m_palette, g_dac_box, sizeof(m_palette));
    m_directory = std::filesystem::temp_directory_path() /
        ("id-potential-restore-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(m_directory);
    set_save_library(m_directory);
    g_temp_dir = m_directory / "missing";
    g_read_filename = m_directory / "image.pot";
    GifWriter::Palette palette(256);
    palette[0] = {255, 0, 0};
    GifWriter writer{g_read_filename, {6, 2}, palette};
    ASSERT_TRUE(writer.write_pixels({12, 34, 56, 78, 90, 123}));
    ASSERT_TRUE(writer.write_pixels({45, 67, 89, 101, 112, 234}));
    ASSERT_TRUE(writer.finish());

    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, wait_key_pressed(_)).Times(0);
    EXPECT_CALL(m_driver, check_memory()).Times(AnyNumber());
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(Invoke(
            [this](const int x, const int y) { return x >= 0 && x < 5 && y >= 0 && y < 4 ? m_frame[y * 5 + x] : 0; }));
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](const int x, const int y, const int color)
            {
                if (x >= 0 && x < 5 && y >= 0 && y < 4)
                {
                    m_frame[y * 5 + x] = color;
                }
            }));
}

void TestPotentialRestore::TearDown()
{
    end_disk();
    EXPECT_TRUE(exit_check().empty());
    set_null_video();
    clear_save_library();
    std::memcpy(g_dac_box, m_palette, sizeof(m_palette));
    std::error_code ignored;
    std::filesystem::remove_all(m_directory, ignored);
}

void TestPotentialRestore::prepare_storage()
{
    start_disk_storage(5, 8, 256, memory_alloc(DISK_STORAGE_BLOCK_SIZE, 1, MemoryLocation::MEMORY));
    g_disk_16_bit = true;
}

void TestPotentialRestore::expect_potential_bytes()
{
    constexpr std::array<Byte, 6> high{12, 34, 56, 45, 67, 89};
    constexpr std::array<Byte, 6> low{78, 90, 123, 101, 112, 234};
    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            EXPECT_EQ(high[row * 3 + col], read_disk_pixel(col + 1, row + 1));
            EXPECT_EQ(low[row * 3 + col], read_disk_pixel(col + 1, row + 3));
        }
    }
}

TEST_F(TestPotentialRestore, scanlinesUsePreparedStorageWithoutUiOrDriverQueries)
{
    prepare_storage();
    write_disk_pixel(4, 7, 171);
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    std::array<Byte, 3> high{12, 34, 56};
    std::array<Byte, 3> low{78, 90, 123};

    EXPECT_EQ(0, pot_line(high.data(), static_cast<int>(high.size())));
    EXPECT_EQ(1, g_row_count);
    EXPECT_EQ(0, pot_line(low.data(), static_cast<int>(low.size())));

    EXPECT_EQ(2, g_row_count);
    EXPECT_EQ(12, m_frame[6]);
    EXPECT_EQ(34, m_frame[7]);
    EXPECT_EQ(56, m_frame[8]);
    EXPECT_EQ(12, read_disk_pixel(1, 1));
    EXPECT_EQ(78, read_disk_pixel(1, 3));
    EXPECT_EQ(171, read_disk_pixel(4, 7));
}

TEST_F(TestPotentialRestore, diskPixelOutputNeedsNoDriver)
{
    prepare_storage();
    set_disk_dot();
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};
    std::array<Byte, 3> high{12, 34, 56};
    std::array<Byte, 3> low{78, 90, 123};

    EXPECT_EQ(0, pot_line(high.data(), static_cast<int>(high.size())));
    EXPECT_EQ(0, pot_line(low.data(), static_cast<int>(low.size())));

    EXPECT_EQ(2, g_row_count);
    EXPECT_EQ(12, read_disk_pixel(1, 1));
    EXPECT_EQ(78, read_disk_pixel(1, 3));
    EXPECT_THAT(m_frame, Each(0));
}

TEST_F(TestPotentialRestore, missingStorageIsAPreconditionFailureWithoutUi)
{
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    std::array<Byte, 3> pixels{12, 34, 56};

    EXPECT_THROW(pot_line(pixels.data(), static_cast<int>(pixels.size())), std::logic_error);

    EXPECT_EQ(0, g_row_count);
    EXPECT_FALSE(g_disk_flag);
    EXPECT_THAT(m_frame, Each(0));
}

TEST_F(TestPotentialRestore, ordinaryDiskStorageIsNotPotentialStorage)
{
    start_disk_storage(5, 4, 256, memory_alloc(DISK_STORAGE_BLOCK_SIZE, 1, MemoryLocation::MEMORY));
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    std::array<Byte, 3> pixels{12, 34, 56};

    EXPECT_THROW(pot_line(pixels.data(), static_cast<int>(pixels.size())), std::logic_error);

    EXPECT_EQ(0, g_row_count);
    EXPECT_EQ(0, read_disk_pixel(1, 1));
}

TEST_F(TestPotentialRestore, uiPreparesStorageOnceBeforeRestoringAllScanlines)
{
    EXPECT_CALL(m_driver, display_string(_, _, _, _, StrEq("clearing 16bit pot work area")));
    EXPECT_CALL(m_driver, flush());
    ASSERT_TRUE(prepare_potential_storage());
    write_disk_pixel(4, 7, 171);

    EXPECT_EQ(0, restore_image());

    expect_potential_bytes();
    EXPECT_EQ(171, read_disk_pixel(4, 7));
    EXPECT_EQ(4, g_row_count);
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(g_disk_16_bit);
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

TEST_F(TestPotentialRestore, failedUiSetupCannotBeRetriedInsideScanlineOutput)
{
    g_debug_flag = DebugFlags::FORCE_MEMORY_FROM_DISK;
    EXPECT_CALL(m_driver, display_string(_, _, _, _, StrEq("clearing 16bit pot work area")));
    EXPECT_CALL(m_driver, flush());
    EXPECT_CALL(m_driver, stack_screen());
    EXPECT_CALL(m_driver, unstack_screen());
    EXPECT_CALL(m_driver, move_cursor(_, _));
    EXPECT_CALL(m_driver, hide_text_cursor());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _));
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));
    ASSERT_FALSE(prepare_potential_storage());
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    std::array<Byte, 3> pixels{12, 34, 56};

    EXPECT_THROW(pot_line(pixels.data(), static_cast<int>(pixels.size())), std::logic_error);

    EXPECT_FALSE(g_disk_flag);
    EXPECT_FALSE(g_good_mode);
    EXPECT_EQ(0, g_row_count);
    EXPECT_THAT(m_frame, Each(0));
    EXPECT_TRUE(exit_check().empty());
}

TEST_F(TestPotentialRestore, failedDecodeClosesTheFileAndLeavesCleanupWithUi)
{
    prepare_storage();
    std::filesystem::resize_file(g_read_filename, std::filesystem::file_size(g_read_filename) - 1);

    EXPECT_EQ(-1, restore_image());

    expect_potential_bytes();
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
    end_disk();
    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(exit_check().empty());
}

} // namespace id::test
