// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <filesystem>
#include <string>
#include <vector>

namespace id::ui
{
struct VideoInfo;
}
namespace id::help
{
enum class HelpLabels;
}

namespace id::engine
{

// -1 no fractal
//  0 params changed, recalculation required
//  1 actively calculating
//  2 interrupted, resumable
//  3 interrupted, not resumable
//  4 completed
enum class CalcStatus
{
    NO_FRACTAL = -1,
    PARAMS_CHANGED = 0,
    IN_PROGRESS = 1,
    RESUMABLE = 2,
    NON_RESUMABLE = 3,
    COMPLETED = 4
};

enum class SaveDAC
{
    NO = 0,
    YES = 1,
    NEXT_TIME = 2,
};

extern int                   g_adapter;             // index into g_video_table[]
extern long                  g_calc_time;
extern CalcStatus            g_calc_status;
extern int                   g_colors;
extern bool                  g_compare_gif;
extern double                g_delta_min;
extern LDouble               g_delta_x2;
extern LDouble               g_delta_x;
extern LDouble               g_delta_y2;
extern LDouble               g_delta_y;
extern std::filesystem::path g_fractal_search_dir1;
extern std::filesystem::path g_fractal_search_dir2;
extern bool                  g_has_inverse;
extern help::HelpLabels      g_help_mode;
extern long                  g_max_iterations;
extern double                g_plot_mx1;
extern double                g_plot_mx2;
extern double                g_plot_my1;
extern double                g_plot_my2;
extern double                g_params[];
extern SaveDAC               g_save_dac;
extern int                   g_save_system;
extern int                   g_scale_map[];
extern int                   g_screen_x_dots;
extern int                   g_screen_y_dots;
extern ui::VideoInfo         g_video_entry;

} // namespace id::engine
