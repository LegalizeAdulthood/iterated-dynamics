// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/fractals.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/Potential.h>
#include <engine/random_seed.h>
#include <engine/VideoInfo.h>
#include <fractals/Plasma.h>
#include <io/disk_storage.h>
#include <io/loadmap.h>
#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/memory.h>
#include <misc/ValueSaver.h>
#include <ui/diskvid.h>
#include <ui/id_keys.h>
#include <ui/plasma.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <stdexcept>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestPlasma : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void prepare_storage();

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{5, 5, 1, 1, 4.0, 4.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 7};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 7};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{{}, true, true}};
    ValueSaver<double> m_saved_param0{g_params[0], 1.0};
    ValueSaver<double> m_saved_param1{g_params[1], 0.0};
    ValueSaver<double> m_saved_param2{g_params[2], 0.0};
    ValueSaver<double> m_saved_param3{g_params[3], 1.0};
    ValueSaver<DComplex> m_saved_param_z1{g_param_z1, DComplex{1.0, 0.0}};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<int> m_saved_outside_color{g_outside_color, 1234};
    ValueSaver<int> m_saved_seed{g_random_seed, 4567};
    ValueSaver<bool> m_saved_seed_flag{g_random_seed_flag, true};
    ValueSaver<bool> m_saved_palette{g_colors_preloaded, true};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::PREVENT_PLASMA_RANDOM};
    std::array<int, 49> m_pixels{};
};

void TestPlasma::SetUp()
{
    end_disk();
    init_memory();
    set_normal_dot();
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(Invoke(
            [this](const int x, const int y) { return x >= 0 && x < 7 && y >= 0 && y < 7 ? m_pixels[y * 7 + x] : 0; }));
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](const int x, const int y, const int color)
            {
                if (x >= 0 && x < 7 && y >= 0 && y < 7)
                {
                    m_pixels[y * 7 + x] = color;
                }
            }));
}

void TestPlasma::TearDown()
{
    end_disk();
    EXPECT_TRUE(exit_check().empty());
    set_null_video();
}

void TestPlasma::prepare_storage()
{
    start_disk_storage(7, 14, 256, memory_alloc(DISK_STORAGE_BLOCK_SIZE, 1, MemoryLocation::MEMORY));
}

TEST_F(TestPlasma, constructionNormalizesConfigurationWithoutStorageOrPixels)
{
    g_params[0] = 1.2;
    g_params[1] = 2.0;
    g_params[2] = 2.0;
    g_params[3] = 2.0;

    Plasma plasma;

    EXPECT_TRUE(plasma.potential_storage_required());
    EXPECT_FALSE(plasma.done());
    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(put_color_a, g_plot);
    EXPECT_EQ(1.125, g_params[0]);
    EXPECT_EQ(1.0, g_params[1]);
    EXPECT_EQ(1.0, g_params[2]);
    EXPECT_EQ(1.0, g_params[3]);
    EXPECT_TRUE(std::all_of(m_pixels.begin(), m_pixels.end(), [](int color) { return color == 0; }));
    EXPECT_THROW(plasma.iterate(), std::logic_error);
}

TEST_F(TestPlasma, preparedStorageReceivesBothBytesWithLogicalOffsets)
{
    Plasma plasma;
    prepare_storage();

    plasma.init(true);

    EXPECT_FALSE(plasma.potential_storage_required());
    EXPECT_EQ(1.0, g_params[3]);
    EXPECT_EQ(1, m_pixels[8]);
    EXPECT_EQ(0, read_disk_pixel(1, 1));
    EXPECT_EQ(1, read_disk_pixel(1, 8));
    g_plot(2, 2, 0x1234);
    EXPECT_EQ(0x12, m_pixels[24]);
    EXPECT_EQ(0x12, read_disk_pixel(3, 3));
    EXPECT_EQ(0x34, read_disk_pixel(3, 10));
    while (!plasma.done())
    {
        plasma.iterate();
    }
}

TEST_F(TestPlasma, diskPixelOutputDoesNotOverwriteThePlottedHighByte)
{
    Plasma plasma;
    prepare_storage();
    set_disk_dot();

    plasma.init(true);

    // The visible color is clamped to one even when the potential high byte is zero.
    EXPECT_EQ(1, read_disk_pixel(1, 1));
    EXPECT_EQ(1, read_disk_pixel(1, 8));
    g_plot(2, 2, 0x1234);
    EXPECT_EQ(0x12, read_disk_pixel(3, 3));
    EXPECT_EQ(0x34, read_disk_pixel(3, 10));
}

TEST_F(TestPlasma, unavailableStorageFallsBackToNormalColorAndBorder)
{
    g_outside_method = ColorMethod::COLOR;
    g_outside_color = 9;
    Plasma plasma;

    plasma.init(false);

    EXPECT_FALSE(plasma.potential_storage_required());
    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(0.0, g_params[3]);
    EXPECT_EQ(9, m_pixels[8]);
    g_plot(2, 2, 7);
    EXPECT_EQ(7, m_pixels[24]);
    while (!plasma.done())
    {
        plasma.iterate();
    }
}

TEST_F(TestPlasma, potentialBorderAndSavedStateSurviveInitialization)
{
    g_outside_method = ColorMethod::COLOR;
    prepare_storage();
    {
        Plasma plasma;
        g_potential.flag = false;
        g_potential.store_16bit = false;

        plasma.init(true);

        EXPECT_EQ(0x04, read_disk_pixel(1, 1));
        EXPECT_EQ(0xD2, read_disk_pixel(1, 8));
        EXPECT_THROW(plasma.init(true), std::logic_error);
    }

    EXPECT_TRUE(g_potential.flag);
    EXPECT_TRUE(g_potential.store_16bit);
    EXPECT_EQ(g_put_color, g_plot);
}

class TestPlasmaStorage : public TestPlasma
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<bool> m_saved_good_mode{g_good_mode, true};
    ValueSaver<std::filesystem::path> m_saved_temp{g_temp_dir};
    std::filesystem::path m_directory;
};

void TestPlasmaStorage::SetUp()
{
    TestPlasma::SetUp();
    m_directory = std::filesystem::temp_directory_path() /
        ("id-plasma-storage-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(m_directory);
    g_temp_dir = m_directory / "missing";
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
}

void TestPlasmaStorage::TearDown()
{
    TestPlasma::TearDown();
    std::error_code ignored;
    std::filesystem::remove_all(m_directory, ignored);
}

TEST_F(TestPlasmaStorage, controllerPreparesPotentialStorageBeforePlotting)
{
    g_params[1] = 1.0;
    EXPECT_CALL(m_driver, display_string(_, _, _, _, StrEq("clearing 16bit pot work area")))
        .WillOnce(Invoke(
            [this](int, int, int, int, const char *)
            {
                EXPECT_FALSE(g_disk_flag);
                EXPECT_EQ(0, m_pixels[8]);
            }));
    EXPECT_CALL(m_driver, flush());

    EXPECT_EQ(0, plasma_type());

    EXPECT_TRUE(g_disk_flag);
    EXPECT_TRUE(g_disk_16_bit);
    EXPECT_EQ(1.0, g_params[3]);
    EXPECT_TRUE(g_potential.flag);
    EXPECT_TRUE(g_potential.store_16bit);
    EXPECT_EQ(g_put_color, g_plot);
}

TEST_F(TestPlasmaStorage, cancelledStorageFallsBackBeforeCalculation)
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
    EXPECT_CALL(m_driver, get_key())
        .WillOnce(Invoke(
            [this]
            {
                EXPECT_EQ(0, m_pixels[8]);
                EXPECT_FALSE(g_disk_flag);
                EXPECT_TRUE(exit_check().empty());
                return ID_KEY_ESC;
            }));

    EXPECT_EQ(0, plasma_type());

    EXPECT_FALSE(g_disk_flag);
    EXPECT_FALSE(g_disk_16_bit);
    EXPECT_EQ(0.0, g_params[3]);
    EXPECT_GT(m_pixels[8], 0);
    EXPECT_TRUE(g_potential.flag);
    EXPECT_TRUE(g_potential.store_16bit);
    EXPECT_EQ(g_put_color, g_plot);
}

TEST_F(TestPlasmaStorage, normalColorSkipsPotentialStorage)
{
    g_params[3] = 0.5;

    EXPECT_EQ(0, plasma_type());

    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(0.5, g_params[3]);
    EXPECT_GT(m_pixels[8], 0);
}

} // namespace id::test
