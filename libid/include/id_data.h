#pragma once

#include "port.h"

#include <string>

struct VIDEOINFO;

// -1 no fractal
//  0 parms changed, recalc reqd
//  1 actively calculating
//  2 interrupted, resumable
//  3 interrupted, not resumable
//  4 completed
enum class calc_status_value
{
    NO_FRACTAL = -1,
    PARAMS_CHANGED = 0,
    IN_PROGRESS = 1,
    RESUMABLE = 2,
    NON_RESUMABLE = 3,
    COMPLETED = 4
};

enum class config_status
{
    OK = 0,
    BAD_WITH_MESSAGE = 1,
    BAD_NO_MESSAGE = -1
};

enum class help_labels;

extern int                   g_adapter;             // index into g_video_table[]
extern bool                  g_auto_browse;
extern config_status         g_bad_config;
extern int                   g_box_count;
extern std::string           g_browse_mask;
extern bool                  g_browsing;
extern bool                  g_browse_check_fractal_params;
extern bool                  g_browse_check_fractal_type;
extern bool                  g_browse_sub_images;
extern long                  g_calc_time;
extern calc_status_value     g_calc_status;
extern int                   g_colors;
extern bool                  g_compare_gif;
extern bool                  g_confirm_file_deletes;
extern double                g_delta_min;
extern LDBL                  g_delta_x2;
extern LDBL                  g_delta_x;
extern LDBL                  g_delta_y2;
extern LDBL                  g_delta_y;
extern int                   g_dot_mode;
extern std::string           g_file_name_stack[16];
extern int                   g_filename_stack_index;
extern float                 g_final_aspect_ratio;
extern std::string           g_fractal_search_dir1;
extern std::string           g_fractal_search_dir2;
extern bool                  g_has_inverse;
extern help_labels           g_help_mode;
extern int                   g_integer_fractal;
extern bool                  g_keep_aspect_ratio;
extern long                  g_l_delta_min;
extern long                  g_l_delta_x2;
extern long                  g_l_delta_x;
extern long                  g_l_delta_y2;
extern long                  g_l_delta_y;
extern long                  g_l_x_3rd;
extern long                  g_l_x_max;
extern long                  g_l_x_min;
extern long                  g_l_y_3rd;
extern long                  g_l_y_max;
extern long                  g_l_y_min;
extern int                   g_logical_screen_x_dots;
extern int                   g_logical_screen_x_offset;
extern double                g_logical_screen_x_size_dots;
extern int                   g_logical_screen_y_dots;
extern int                   g_logical_screen_y_offset;
extern double                g_logical_screen_y_size_dots;
extern int                   g_look_at_mouse;
extern long                  g_max_iterations;
extern double                g_plot_mx1;
extern double                g_plot_mx2;
extern double                g_plot_my1;
extern double                g_plot_my2;
extern double                g_params[];
extern double                g_potential_params[];
extern int                   g_resave_flag;
extern int                   g_save_dac;
extern int                   g_save_system;
extern double                g_save_x_3rd;
extern double                g_save_x_max;
extern double                g_save_x_min;
extern double                g_save_y_3rd;
extern double                g_save_y_max;
extern double                g_save_y_min;
extern int                   g_scale_map[];
extern int                   g_screen_x_dots;
extern int                   g_screen_y_dots;
extern int                   g_smallest_box_size_shown;
extern bool                  g_started_resaves;
extern char                  g_std_calc_mode;
extern bool                  g_tab_mode;
extern int                   g_timed_save;
extern double                g_smallest_window_display_size;
extern long                  g_user_distance_estimator_value;
extern bool                  g_user_float_flag;
extern int                   g_user_periodicity_value;
extern char                  g_user_std_calc_mode;
extern VIDEOINFO             g_video_entry;
extern bool                  g_view_crop;
extern float                 g_view_reduction;
extern bool                  g_view_window;
extern int                   g_view_x_dots;
extern int                   g_view_y_dots;
extern double                g_x_3rd;
extern double                g_x_max;
extern double                g_x_min;
extern double                g_y_3rd;
extern double                g_y_max;
extern double                g_y_min;
extern bool                  g_z_scroll;
extern double                g_zoom_box_height;
extern int                   g_zoom_box_rotation;
extern double                g_zoom_box_skew;
extern double                g_zoom_box_width;
extern double                g_zoom_box_x;
extern double                g_zoom_box_y;
extern bool                  g_zoom_off;
