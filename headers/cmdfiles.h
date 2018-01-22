#pragma once
#if !defined(CMDFILES_H)
#define CMDFILES_H

#include <string>
#include <vector>

namespace id
{

struct SearchPath
{
    char par[FILE_MAX_PATH];
    char frm[FILE_MAX_PATH];
    char ifs[FILE_MAX_PATH];
    char lsys[FILE_MAX_PATH];
};

} // namespace id

extern bool                  g_ask_video;
extern float                 g_aspect_drift;
extern std::string           g_auto_name;
extern char                  g_auto_show_dot;
extern long                  g_bail_out;
extern bailouts              g_bail_out_test;
extern int                   g_base_hertz;
extern int                   g_biomorph;
extern bool                  g_bof_match_book_images;
extern bool                  g_check_cur_dir;
extern int                   g_color_cycle_range_hi;
extern int                   g_color_cycle_range_lo;
extern std::string           g_color_file;
extern int                   g_color_state;
extern bool                  g_colors_preloaded;
extern std::string           g_command_comment[4];
extern std::string           g_command_file;
extern std::string           g_command_name;
extern fractalspecificstuff *g_cur_fractal_specific;
extern int                   g_cycle_limit;
extern int                   g_debug_flag;
extern int                   g_decomp[];
extern int                   g_bf_digits;
extern display_3d_modes      g_display_3d;
extern long                  g_distance_estimator;
extern int                   g_distance_estimator_width_factor;
extern int                   g_distance_estimator_x_dots;
extern int                   g_distance_estimator_y_dots;
extern bool                  g_dither_flag;
extern bool                  g_escape_exit;
extern BYTE                  g_exit_video_mode;
extern bool                  g_fast_restore;
extern int                   g_fill_color;
extern bool                  g_finite_attractor;
extern bool                  g_first_init;
extern bool                  g_float_flag;
extern symmetry_type         g_force_symmetry;
extern std::string           g_formula_filename;
extern std::string           g_formula_name;
extern bool                  g_gif87a_flag;
extern std::string           g_gif_filename_mask;
extern std::vector<float>    g_ifs_definition;
extern std::string           g_ifs_filename;
extern std::string           g_ifs_name;
extern bool                  g_ifs_type;
extern int                   g_init_3d[20];
extern batch_modes           g_init_batch;
extern int                   g_init_cycle_limit;
extern int                   g_init_mode;
extern DComplex              g_init_orbit;
extern int                   g_init_save_time;
extern int                   g_inside_color;
extern double                g_inversion[];
extern std::vector<int>      g_iteration_ranges;
extern int                   g_iteration_ranges_len;
extern std::string           g_l_system_filename;
extern std::string           g_l_system_name;
extern bool                  g_log_map_auto_calculate;
extern long                  g_log_map_flag;
extern int                   g_log_map_fly_calculate;
extern BYTE                  g_map_clut[256][3];
extern bool                  g_map_specified;
extern double                g_math_tol[2];
extern bool                  g_new_bifurcation_functions_loaded;
extern int                   g_orbit_delay;
extern int                   g_orbit_save_flags;
extern std::string           g_organize_formulas_dir;
extern bool                  g_organize_formulas_search;
extern int                   g_outside_color;
extern bool                  g_overlay_3d;
extern bool                  g_overwrite_file;
extern bool                  g_potential_16bit;
extern bool                  g_potential_flag;
extern int                   g_random_seed;
extern bool                  g_random_seed_flag;
extern bool                  g_read_color;
extern std::string           g_read_filename;
extern record_colors_mode    g_record_colors;
extern std::string           g_save_filename;
extern float                 g_screen_aspect;
extern id::SearchPath        g_search_for;
extern int                   g_show_dot;
extern int                   g_show_file;
extern int                   g_size_dot;
extern slides_mode           g_slides;
extern int                   g_sound_flag;
extern bool                  g_start_show_orbit;
extern int                   g_stop_pass;
extern bool                  g_targa_out;
extern std::string           g_temp_dir;
extern BYTE                  g_text_color[];
extern bool                  g_timer_flag;
extern int                   g_transparent_color_3d[];
extern true_color_mode       g_true_mode;
extern bool                  g_truecolor;
extern bool                  g_use_center_mag;
extern init_orbit_mode       g_use_init_orbit;
extern int                   g_user_biomorph_value;
extern std::string           g_working_dir;

extern int cmdfiles(int argc, char const *const *argv);
extern int load_commands(FILE *);
extern void set_3d_defaults();
extern int get_curarg_len(char const *curarg);
extern int get_max_curarg_len(char const *floatvalstr[], int totparm);
extern int init_msg(char const *cmdstr, char const *badfilename, cmd_file mode);
extern int cmdarg(char *curarg, cmd_file mode);
extern int getpower10(LDBL x);
extern void dopause(int);

#endif
