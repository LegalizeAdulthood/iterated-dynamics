// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include "math/Point.h"

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

enum class ConfigStatus
{
    OK = 0,
    BAD_WITH_MESSAGE = 1,
    BAD_NO_MESSAGE = -1
};

enum class SaveDAC
{
    NO = 0,
    YES = 1,
    NEXT_TIME = 2,
};

using FilenameStack = std::vector<std::string>;

extern int                   g_adapter;             // index into g_video_table[]
extern bool                  g_auto_browse;
extern ConfigStatus          g_bad_config;
extern std::filesystem::path g_browse_mask;
extern bool                  g_browsing;
extern bool                  g_browse_check_fractal_params;
extern bool                  g_browse_check_fractal_type;
extern bool                  g_browse_sub_images;
extern long                  g_calc_time;
extern CalcStatus            g_calc_status;
extern int                   g_colors;
extern bool                  g_compare_gif;
extern bool                  g_confirm_file_deletes;
extern double                g_delta_min;
extern LDouble               g_delta_x2;
extern LDouble               g_delta_x;
extern LDouble               g_delta_y2;
extern LDouble               g_delta_y;
extern FilenameStack         g_filename_stack;
extern float                 g_final_aspect_ratio;
extern std::filesystem::path g_fractal_search_dir1;
extern std::filesystem::path g_fractal_search_dir2;
extern bool                  g_has_inverse;
extern help::HelpLabels      g_help_mode;
extern bool                  g_keep_aspect_ratio;
extern long                  g_max_iterations;
extern double                g_plot_mx1;
extern double                g_plot_mx2;
extern double                g_plot_my1;
extern double                g_plot_my2;
extern double                g_params[];
extern double                g_potential_params[];
extern SaveDAC               g_save_dac;
extern int                   g_save_system;
extern int                   g_scale_map[];
extern int                   g_screen_x_dots;
extern int                   g_screen_y_dots;
extern int                   g_smallest_box_size_shown;
extern bool                  g_tab_mode;
extern double                g_smallest_window_display_size;
extern long                  g_user_distance_estimator_value;
extern int                   g_user_periodicity_value;
extern ui::VideoInfo         g_video_entry;
extern bool                  g_view_crop;
extern float                 g_view_reduction;
extern bool                  g_view_window;
extern int                   g_view_x_dots;
extern int                   g_view_y_dots;
extern bool g_z_scroll;

} // namespace id::engine
