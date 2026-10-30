// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ColorMapSaver.h"
#include "MockDriver.h"
#include "RestoreDecodeTestState.h"
#include "StopMessageTestSupport.h"
#include "TemporaryDirectory.h"

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
#include <filesystem>
#include <memory>
#include <optional>
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
    RestoreDecodeTestState m_saved_decode;
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<OutLine> m_saved_out_line{g_out_line, pot_line};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{{}, true, true}};
    ValueSaver<Display3DMode> m_saved_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver<bool> m_saved_good_mode{g_good_mode, true};
    ValueSaver<std::filesystem::path> m_saved_temp{g_temp_dir};
    std::optional<ColorMapSaver> m_saved_palette;
    std::array<int, 20> m_frame{};
    TemporaryDirectory m_directory;
};

void TestPotentialRestore::SetUp()
{
    end_disk();
    init_memory();
    set_normal_dot();
    m_saved_palette.emplace();
    m_directory.create("id-potential-restore-");
    set_save_library(m_directory.path());
    g_temp_dir = m_directory.path() / "missing";
    g_read_filename = m_directory.path() / "image.pot";
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
    m_saved_palette.reset();
    std::error_code ignored;
    m_directory.remove(ignored);
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

class TestPotentialScanlineTarget : public TestPotentialRestore, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<ValueSaver<Driver *>> m_driver_guard;
};

void TestPotentialScanlineTarget::SetUp()
{
    TestPotentialRestore::SetUp();
    prepare_storage();
    if (GetParam())
    {
        set_disk_dot();
        m_driver_guard = std::make_unique<ValueSaver<Driver *>>(g_driver, nullptr);
    }
    else
    {
        write_disk_pixel(4, 7, 171);
        EXPECT_CALL(m_driver, is_disk()).Times(0);
    }
}

void TestPotentialScanlineTarget::TearDown()
{
    m_driver_guard.reset();
    TestPotentialRestore::TearDown();
}

TEST_P(TestPotentialScanlineTarget, scanlinesUsePreparedStorageWithoutDriverQueries)
{
    std::array<Byte, 3> high{12, 34, 56};
    std::array<Byte, 3> low{78, 90, 123};

    EXPECT_EQ(0, pot_line(high.data(), static_cast<int>(high.size())));
    EXPECT_EQ(1, g_row_count);
    EXPECT_EQ(0, pot_line(low.data(), static_cast<int>(low.size())));

    EXPECT_EQ(2, g_row_count);
    EXPECT_EQ(12, read_disk_pixel(1, 1));
    EXPECT_EQ(78, read_disk_pixel(1, 3));
    if (GetParam())
    {
        EXPECT_THAT(m_frame, Each(0));
    }
    else
    {
        EXPECT_EQ(12, m_frame[6]);
        EXPECT_EQ(34, m_frame[7]);
        EXPECT_EQ(56, m_frame[8]);
        EXPECT_EQ(171, read_disk_pixel(4, 7));
    }
}

INSTANTIATE_TEST_SUITE_P(Output, TestPotentialScanlineTarget, Values(false, true),
    [](const TestParamInfo<bool> &info) { return info.param ? "DiskWithoutDriver" : "ScreenWithDriver"; });

class TestUnavailablePotentialStorage : public TestPotentialRestore, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
};

void TestUnavailablePotentialStorage::SetUp()
{
    TestPotentialRestore::SetUp();
    if (GetParam())
    {
        start_disk_storage(5, 4, 256, memory_alloc(DISK_STORAGE_BLOCK_SIZE, 1, MemoryLocation::MEMORY));
    }
    EXPECT_CALL(m_driver, is_disk()).Times(0);
}

TEST_P(TestUnavailablePotentialStorage, scanlineOutputRejectsUnavailablePotentialStorage)
{
    std::array<Byte, 3> pixels{12, 34, 56};

    EXPECT_THROW(pot_line(pixels.data(), static_cast<int>(pixels.size())), std::logic_error);

    EXPECT_EQ(0, g_row_count);
    EXPECT_THAT(m_frame, Each(0));
    if (GetParam())
    {
        EXPECT_EQ(0, read_disk_pixel(1, 1));
    }
    else
    {
        EXPECT_FALSE(g_disk_flag);
    }
}

INSTANTIATE_TEST_SUITE_P(Storage, TestUnavailablePotentialStorage, Values(false, true),
    [](const TestParamInfo<bool> &info) { return info.param ? "Ordinary" : "Absent"; });

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
    expect_stop_messages(m_driver);
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
