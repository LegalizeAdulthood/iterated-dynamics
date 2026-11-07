// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <io/decoder.h>
#include <io/gifview.h>
#include <io/loadfile.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <filesystem>

namespace id::test
{

class RestoreDecodeTestState
{
    misc::ValueSaver<int> m_saved_colors{engine::g_colors, 256};
    misc::ValueSaver<int> m_saved_row{io::g_row_count, 0};
    misc::ValueSaver<bool> m_saved_read_color{engine::g_read_color, true};
    misc::ValueSaver<engine::ColorState> m_saved_color_state{engine::g_color_state};
    misc::ValueSaver<bool> m_saved_map{engine::g_map_set, false};
    misc::ValueSaver<bool> m_saved_dither{io::g_dither_flag, false};
    misc::ValueSaver<int> m_saved_skip_x{io::g_skip_x_dots, 0};
    misc::ValueSaver<int> m_saved_skip_y{io::g_skip_y_dots, 0};
    misc::ValueSaver<engine::CalcStatus> m_saved_status{engine::g_calc_status, engine::CalcStatus::COMPLETED};
    misc::ValueSaver<bool> m_saved_first_init{engine::g_first_init, false};
    misc::ValueSaver<engine::BatchMode> m_saved_batch{engine::g_init_batch, engine::BatchMode::NONE};
    misc::ValueSaver<misc::DebugFlags> m_saved_debug{misc::g_debug_flag, misc::DebugFlags::NONE};
    misc::ValueSaver<std::filesystem::path> m_saved_filename{io::g_read_filename};
};

} // namespace id::test
