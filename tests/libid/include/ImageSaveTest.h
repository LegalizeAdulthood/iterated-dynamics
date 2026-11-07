// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <ColorMapSaver.h>
#include <MockDriver.h>
#include <TemporaryDirectory.h>

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractalp.h>
#include <io/check_write_file.h>
#include <io/disk_storage.h>
#include <io/save_timer.h>
#include <io/special_dirs.h>
#include <math/biginit.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <filesystem>
#include <optional>
#include <vector>

namespace id::test::image_save_test
{

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

class ImageSaveTest : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void configure_image(int width, int height, int colors, int x_offset = 0, int y_offset = 0);
    int save();
    std::vector<Byte> image_pixels() const;

    MockDriver m_driver;
    TemporaryDirectory m_directory;
    std::filesystem::path m_path;
    int m_frame_width{};
    std::vector<Byte> m_frame;
    std::optional<ColorMapSaver> m_saved_palette;
    bool m_memory_initialized{};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen};
    ValueSaver<int> m_saved_x_dots{g_screen_x_dots};
    ValueSaver<int> m_saved_y_dots{g_screen_y_dots};
    ValueSaver<int> m_saved_colors{g_colors};
    ValueSaver<int> m_saved_and_color{g_and_color};
    ValueSaver<Viewport> m_saved_viewport{g_viewport, Viewport{}};
    ValueSaver<float> m_saved_screen_aspect{g_screen_aspect, 0.75f};
    ValueSaver<bool> m_saved_real_dac{g_got_real_dac, false};
    ValueSaver<bool> m_saved_disk_16_bit{g_disk_16_bit, false};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NORMAL};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<TimedSave> m_saved_timed_save{g_timed_save, TimedSave::STARTED};
    ValueSaver<TimedSave> m_saved_resave{g_resave_flag, TimedSave::NONE};
    ValueSaver<bool> m_saved_started_resaves{g_started_resaves, false};
    ValueSaver<int> m_saved_save_time{g_save_time_interval, 0};
    ValueSaver<bool> m_saved_overwrite{g_overwrite_file, true};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<BFMathType> m_saved_bf_math{g_bf_math, BFMathType::NONE};
    ValueSaver<std::filesystem::path> m_saved_save_dir{g_save_dir};
    ValueSaver<std::filesystem::path> m_saved_temp_dir{g_temp_dir};
};

} // namespace id::test::image_save_test
