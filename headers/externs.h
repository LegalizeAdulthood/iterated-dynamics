#ifndef EXTERNS_H
#define EXTERNS_H
#include <string>
#include <vector>

struct AlternateMath;
enum class bailouts;
enum class batch_modes;
enum class calc_status_value;
struct DComplex;
enum display_3d_modes;
struct EVOLUTION_INFO;
struct fn_operand;
enum class fractal_type;
struct GENEBASE;
enum class init_orbit_mode;
struct LComplex;
enum class Major;
enum class Minor;
struct MOREPARAMS;
struct MP;
struct MPC;
enum class raytrace_formats;
enum class record_colors_mode;
namespace id
{
struct SearchPath;
}
enum class slides_mode;
enum class stereo_images;
enum class symmetry_type;
enum class true_color_mode;
struct VIDEOINFO;
struct WORKLIST;

// keep var names in column 30 for sorting via sort /+30 <in >out
extern int                   g_adapter;             // index into g_video_table[]
extern bool                  g_auto_browse;
extern int                   g_bad_config;
extern int                   g_bit_shift;
extern int                   g_box_count;
extern std::string           g_browse_mask;
extern bool                  g_browsing;
extern bool                  g_browse_check_fractal_params;
extern bool                  g_browse_check_fractal_type;
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
extern float                 g_final_aspect_ratio;
extern char const *          g_fractal_search_dir1;
extern char const *          g_fractal_search_dir2;
extern long                  g_fudge_factor;
extern std::vector<double>   g_grid_x0;
extern std::vector<double>   g_grid_x1;
extern std::vector<double>   g_grid_y0;
extern std::vector<double>   g_grid_y1;
extern bool                  g_has_inverse;
extern int                   g_help_mode;
extern int                   g_integer_fractal;
extern long                  g_l_delta_min;
extern long                  g_l_delta_x2;
extern long                  g_l_delta_x;
extern long                  g_l_delta_y2;
extern long                  g_l_delta_y;
extern std::vector<long>     g_l_x0;
extern std::vector<long>     g_l_x1;
extern std::vector<long>     g_l_y0;
extern std::vector<long>     g_l_y1;
extern double                g_logical_screen_x_size_dots;
extern double                g_logical_screen_y_size_dots;
extern int                   g_look_at_mouse;
extern long                  g_max_iterations;
extern int                   g_smallest_box_size_shown;

extern int                   g_color_bright;    // brightest color in palette
extern int                   g_color_dark;      // darkest color in palette
extern int                   g_color_medium;    // nearest to medbright grey in palette
extern int                   g_dac_count;
extern int                   g_fm_attack;
extern int                   g_fm_decay;
extern int                   g_fm_release;
extern int                   g_fm_sustain;
extern int                   g_fm_wavetype;
extern int                   g_fm_volume;            // volume of OPL-3 soundcard output
extern int                   g_hi_attenuation;
extern long                  g_l_init_x;
extern long                  g_l_init_y;

extern int                   g_filename_stack_index;
extern bool                  g_browse_sub_images;
extern int                   g_num_affine_transforms;
extern int                   g_num_fractal_types;
extern std::string           g_organize_formulas_dir;
extern int                 (*g_out_line)(BYTE *, int);
extern double                g_params[];
extern double                g_plot_mx1;
extern double                g_plot_mx2;
extern double                g_plot_my1;
extern double                g_plot_my2;
extern int                   g_polyphony;
extern bool                  g_potential_16bit;
extern bool                  g_potential_flag;
extern double                g_potential_params[];
extern std::string           g_read_filename;
extern record_colors_mode    g_record_colors;
extern int                   g_resave_flag;
extern int                   g_resume_len;
extern int                   g_row_count;       // row-counter for decoder and out_line
extern long                  g_save_base;
extern DComplex              g_save_c;
extern int                   g_save_dac;
extern std::string           g_save_filename;
extern long                  g_save_ticks;
extern int                   g_save_system;
extern int                   g_scale_map[];
extern int                   g_size_dot;
extern bool                  g_started_resaves;
extern char                  g_std_calc_mode;
extern double                g_save_x_3rd;
extern int                   g_screen_x_dots;
extern double                g_save_x_max;
extern double                g_save_x_min;
extern int                   g_logical_screen_x_offset;
extern double                g_save_y_3rd;
extern int                   g_screen_y_dots;
extern double                g_save_y_max;
extern double                g_save_y_min;
extern int                   g_logical_screen_y_offset;
extern bool                  g_tab_mode;
extern bool                  g_tab_or_help;
extern int                   g_text_cbase;      // g_text_col is relative to this
extern int                   g_text_col;        // current column in text mode
extern int                   g_text_rbase;      // g_text_row is relative to this
extern int                   g_text_row;        // current row in text mode
extern int                   g_timed_save;
extern long                  g_timer_interval;
extern long                  g_timer_start;
extern std::string           g_temp_dir;
extern double                g_smallest_window_display_size;
extern bool                  g_use_grid;
extern int                   g_user_biomorph_value;
extern long                  g_user_distance_estimator_value;
extern bool                  g_user_float_flag;
extern int                   g_user_periodicity_value;
extern char                  g_user_std_calc_mode;
extern int                   g_vesa_x_res;
extern int                   g_vesa_y_res;
extern VIDEOINFO             g_video_entry;
extern VIDEOINFO             g_video_table[];
extern bool                  g_keep_aspect_ratio;
extern int                   g_video_start_x;
extern int                   g_video_start_y;
extern int                   g_video_type;      // video adapter type
extern bool                  g_view_crop;
extern float                 g_view_reduction;
extern bool                  g_view_window;
extern int                   g_view_x_dots;
extern int                   g_view_y_dots;
extern std::string           g_working_dir;
extern long                  g_l_x_3rd;
extern double                g_julia_c_x;
extern int                   g_logical_screen_x_dots;
extern long                  g_l_x_max;
extern long                  g_l_x_min;
extern double                g_x_3rd;
extern double                g_x_max;
extern double                g_x_min;
extern long                  g_l_y_3rd;
extern double                g_julia_c_y;
extern int                   g_logical_screen_y_dots;
extern long                  g_l_y_max;
extern long                  g_l_y_min;
extern double                g_y_3rd;
extern double                g_y_max;
extern double                g_y_min;
extern double                g_zoom_box_x;
extern double                g_zoom_box_y;
extern double                g_zoom_box_height;
extern bool                  g_zoom_off;
extern int                   g_zoom_box_rotation;
extern bool                  g_z_scroll;
extern double                g_zoom_box_skew;
extern double                g_zoom_box_width;
#if defined(XFRACT)
extern bool                  g_fake_lut;
extern bool                  g_x_zoom_waiting;
#endif
#endif
