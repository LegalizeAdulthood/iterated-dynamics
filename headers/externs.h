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
enum class fractal_type;
struct GENEBASE;
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
struct VIDEOINFO;
struct WORKLIST;

// keep var names in column 30 for sorting via sort /+30 <in >out
extern int                   g_adapter;             // index into g_video_table[]
extern AlternateMath         g_alternate_math[];    // alternate math function pointers
extern int                   g_ambient;             // Ambient= parameter value
extern int                   g_and_color;           // AND mask for iteration to get color index
extern int                   g_halley_a_plus_one_times_degree;
extern int                   g_halley_a_plus_one;
extern bool                  g_ask_video;
extern float                 g_aspect_drift;
extern int                   g_attractors;
extern int                   g_attractor_period[];
extern DComplex              g_attractor[];
extern bool                  g_auto_browse;
extern std::string           g_auto_name;
extern char                  g_auto_show_dot;
extern int                   g_auto_stereo_depth;
extern double                g_auto_stereo_width;
extern BYTE                  g_background_color[];
extern int                   g_bad_config;
extern bool                  g_bad_outside;
extern int const             g_bad_value;
extern long                  g_bail_out;
extern bailouts              g_bail_out_test;
extern int                   g_base_hertz;
extern int                   g_basin;
extern int                   g_bf_save_len;
extern int                   g_bf_digits;
extern int                   g_biomorph;
extern unsigned int          g_diffusion_bits;
extern int                   bitshift;
extern int                   bitshiftless1;
extern BYTE                  g_block[];
extern int                   g_blue_bright;
extern int                   g_blue_crop_left;
extern int                   g_blue_crop_right;
extern int                   g_box_color;
extern int                   g_box_count;
extern int                   g_box_values[];
extern int                   g_box_x[];
extern int                   g_box_y[];
extern bool                  g_brief;
extern std::string           g_browse_mask;
extern std::string           g_browse_name;
extern bool                  g_browsing;
extern bool                  g_browse_check_fractal_params;
extern bool                  g_browse_check_fractal_type;
extern bool                  g_busy;
extern long                  g_calc_time;
extern int                 (*calctype)();
extern calc_status_value     g_calc_status;
extern char                  g_calibrate;
extern bool                  g_check_cur_dir;
extern double                g_close_enough;
extern double                g_close_proximity;
extern DComplex              g_marks_coefficient;
extern int                   col;
extern int                   g_color;
extern std::string           g_color_file;
extern long                  g_color_iter;
extern bool                  g_colors_preloaded;
extern int                   g_colors;
extern int                   g_color_state;
extern int                   g_color_bright;    // brightest color in palette
extern int                   g_color_dark;      // darkest color in palette
extern int                   g_color_medium;    // nearest to medbright grey in palette
extern std::string           g_command_comment[4];
extern std::string           g_command_file;
extern std::string           g_command_name;
extern bool                  g_compare_gif;
extern long                  con;
extern double                cosx;
extern int                   g_current_column;
extern int                   g_current_pass;
extern int                   g_current_row;
extern int                   g_cycle_limit;
extern int                   g_c_exponent;
extern double                g_degree_minus_1_over_degree;
extern BYTE                  g_dac_box[256][3];
extern int                   g_dac_count;
extern bool                  g_dac_learn;
extern double                ddelmin;
extern int                   g_debug_flag;
extern int                   g_decimals;
extern int                   g_decomp[];
extern int                   degree;
extern long                  delmin;
extern long                  delx2;
extern long                  delx;
extern LDBL                  delxx2;
extern LDBL                  delxx;
extern long                  dely2;
extern long                  dely;
extern LDBL                  delyy2;
extern LDBL                  delyy;
extern float                 g_julibrot_depth_fp;
extern unsigned long         g_diffusion_counter;
extern unsigned long         g_diffusion_limit;
extern bool                  g_disk_16_bit;
extern bool                  g_disk_flag;       // disk video active flag
extern bool                  g_disk_targa;
extern display_3d_modes      g_display_3d;
extern long                  g_distance_estimator;
extern int                   g_distance_estimator_width_factor;
extern float                 g_julibrot_dist_fp;
extern int                   g_distribution;
extern bool                  g_dither_flag;
extern bool                  g_read_color;
extern int                   g_dot_mode;
extern bool                  g_confirm_file_deletes;
extern double                g_evolve_dist_per_x;
extern double                g_evolve_dist_per_y;
extern char                  g_draw_mode;
extern std::vector<double>   dx0;
extern std::vector<double>   dx1;
extern double              (*dxpixel)();
extern double                x_size_d;
extern std::vector<double>   dy0;
extern std::vector<double>   dy1;
extern double              (*dypixel)();
extern double                y_size_d;
extern bool                  g_escape_exit;
extern BYTE                  g_exit_video_mode;
extern int                   g_evolving;
extern bool                  g_have_evolve_info;
extern EVOLUTION_INFO        g_evolve_info;
extern int                   g_eye_separation;
extern float                 g_eyes_fp;
extern bool                  g_fast_restore;
extern long                  g_fudge_half;
extern double                g_fudge_limit;
extern long                  g_fudge_one;
extern long                  g_fudge_two;
extern int                   g_evolve_image_grid_size;
extern double                g_evolve_max_random_mutation;
extern double                g_evolve_mutation_reduction_factor;
extern float                 g_file_aspect_ratio;
extern int                   g_file_colors;
extern int                   g_file_x_dots;
extern int                   g_file_y_dots;
extern std::string           g_file_name_stack[16];
extern int                   g_fill_color;
extern float                 g_final_aspect_ratio;
extern bool                  g_finite_attractor;
extern int                   g_finish_row;
extern bool                  g_first_init;
extern bool                  g_float_flag;
extern DComplex *            g_float_param;
extern int                   g_fm_attack;
extern int                   g_fm_decay;
extern int                   g_fm_release;
extern int                   g_fm_sustain;
extern int                   g_fm_wavetype;
extern int                   g_fm_volume;            // volume of OPL-3 soundcard output
extern symmetry_type         g_force_symmetry;
extern std::string           g_formula_filename;
extern std::string           g_formula_name;
extern char const *          g_fractal_search_dir1;
extern char const *          g_fractal_search_dir2;
extern long                  g_fudge_factor;
extern bool                  g_new_bifurcation_functions_loaded;
extern double                g_f_at_rad;
extern double                g_f_radius;
extern double                g_f_x_center;
extern double                g_f_y_center;
extern GENEBASE              g_gene_bank[NUMGENES];
extern bool                  g_gif87a_flag;
extern std::string           g_gif_filename_mask;
extern std::string const     g_glasses1_map;
extern int                   g_glasses_type;
extern bool                  g_good_mode;       // video mode ok?
extern bool                  g_got_real_dac;    // loaddac worked, really got a dac
extern int                   g_got_status;
extern bool                  g_gray_flag;
extern std::string const     g_gray_map_file;
extern bool                  g_has_inverse;
extern int                   g_haze;
extern unsigned int          g_height;
extern float                 g_julibrot_height_fp;
extern int                   g_help_mode;
extern int                   g_hi_attenuation;
extern std::string           g_ifs_filename;
extern std::string           g_ifs_name;
extern std::vector<float>    g_ifs_definition;
extern bool                  g_ifs_type;
extern int                   g_image_box_count;
extern bool                  g_image_map;
extern int                   g_init_3d[20];
extern DComplex              g_init;
extern batch_modes           g_init_batch;
extern int                   g_init_cycle_limit;
extern int                   g_init_mode;
extern DComplex              g_init_orbit;
extern int                   g_init_save_time;
extern int                   g_inside_color;
extern int                   g_integer_fractal;
extern double                g_inversion[];
extern int                   g_invert;
extern bool                  g_is_true_color;
extern bool                  g_is_mandelbrot;
extern int                   g_i_x_start;
extern int                   g_i_x_stop;
extern int                   g_i_y_start;
extern int                   g_i_y_stop;
extern std::string const     g_jiim_left_right[];
extern std::string const     g_jiim_method[];
extern int                   g_julibrot_3d_mode;
extern std::string const     g_julibrot_3d_options[];
extern bool                  g_julibrot;
extern int                   g_keyboard_check_interval;
extern bool                  g_keep_screen_coords;
extern int                   g_last_init_op;
extern unsigned              g_last_op;
extern LComplex              g_l_attractor[];
extern long                  g_l_close_enough;
extern LComplex              g_l_coefficient;
extern bool                  g_ld_check;
extern std::string           g_l_system_filename;
extern std::string           g_light_name;
extern std::vector<BYTE>     g_line_buff;
extern LComplex              g_l_init;
extern LComplex              g_l_init_orbit;
extern long                  g_l_init_x;
extern long                  g_l_init_y;
extern long                  g_l_limit2;
extern long                  g_l_limit;
extern long                  g_l_magnitude;
extern std::string           g_l_system_name;
extern LComplex              g_l_new_z;
extern bool                  g_loaded_3d;
extern int                   g_load_index;
extern bool                  g_log_map_auto_calculate;
extern bool                  g_log_map_calculate;
extern int                   g_log_map_fly_calculate;
extern long                  g_log_map_flag;
extern std::vector<BYTE>     g_log_map_table;
extern long                  g_log_map_table_max_size;
extern LComplex              g_l_old_z;
extern LComplex *            g_long_param;
extern int                   g_look_at_mouse;
extern LComplex              g_l_param2;
extern LComplex              g_l_param;
extern long                  g_l_temp_sqr_x;
extern long                  g_l_temp_sqr_y;
extern LComplex              g_l_temp;
extern std::vector<long>     g_l_x0;
extern std::vector<long>     g_l_x1;
extern long                (*g_l_x_pixel)();
extern std::vector<long>     g_l_y0;
extern std::vector<long>     g_l_y1;
extern long                (*g_l_y_pixel)();
extern long                  g_l_at_rad;
extern MATRIX                g_m;
extern double                g_magnitude;
extern Major                 g_major_method;
extern bool                  g_map_specified;
extern BYTE                  g_map_clut[256][3];
extern bool                  g_map_set;
extern std::string           g_map_name;
extern double                g_math_tol[2];
extern int                   g_max_color;
extern long                  g_max_count;
extern char                  g_max_function;
extern long                  g_max_iterations;
extern int                   g_max_line_length;
extern unsigned              g_max_function_args;
extern unsigned              g_max_function_ops;
extern long                  g_bignum_max_stack_addr;
extern int                   g_max_keyboard_check_interval;
extern int                   g_max_image_history;
extern int                   g_max_rhombus_depth;
extern int                   g_smallest_box_size_shown;
extern Minor                 g_inverse_julia_minor_method;
extern int                   g_soi_min_stack;
extern int                   g_soi_min_stack_available;
extern MOREPARAMS            g_more_fractal_params[];
extern MP                    g_halley_mp_a_plus_one_times_degree;
extern MP                    g_halley_mp_a_plus_one;
extern MPC                   g_mpc_one;
extern std::vector<MPC>      g_mpc_roots;
extern MPC                   g_mpc_temp_param;
extern MP                    g_mp_degree_minus_1_over_degree;
extern MP                    g_mp_one;
extern int                   g_mp_overflow;
extern MP                    g_newton_mp_r_over_d;
extern MP                    g_mp_temp2;
extern MP                    g_mp_threshold;
extern MP                    g_mp_temp_param2_x;
extern double                g_julibrot_x_max;
extern double                g_julibrot_x_min;
extern double                g_julibrot_y_max;
extern double                g_julibrot_y_min;
extern int                   g_filename_stack_index;
extern DComplex              g_new_z;
extern char                  g_evolve_new_discrete_x_parameter_offset;
extern char                  g_evolve_new_discrete_y_parameter_offset;
extern double                g_evolve_new_x_parameter_offset;
extern double                g_evolve_new_y_parameter_offset;
extern fractal_type          g_new_orbit_type;
extern int                   g_periodicity_next_saved_incr;
extern bool                  g_browse_sub_images;
extern bool                  g_magnitude_calc;
extern bool                  g_bof_match_book_images;
extern int                   g_num_affine_transforms;
extern unsigned              g_num_colors;
extern const int             g_num_trig_functions;
extern int                   g_num_fractal_types;
extern int                   g_num_work_list;
extern bool                  g_cellular_next_screen;
extern DComplex              g_old_z;
extern long                  g_old_color_iter;
extern BYTE                  g_old_dac_box[256][3];
extern bool                  g_old_demm_colors;
extern char                  g_old_std_calc_mode;
extern char                  g_evolve_discrete_x_parameter_offset;
extern char                  g_evolve_discrete_y_parameter_offset;
extern double                g_evolve_x_parameter_offset;
extern double                g_evolve_y_parameter_offset;
extern int                   g_orbit_save_flags;
extern int                   g_orbit_color;
extern int                   g_orbit_delay;
extern long                  g_orbit_interval;
extern int                   g_orbit_save_index;
extern std::string           g_organize_formulas_dir;
extern bool                  g_organize_formulas_search;
extern float                 g_julibrot_origin_fp;
extern int                 (*g_out_line)(BYTE *, int);
extern void                (*g_out_line_cleanup)();
extern int                   g_outside_color;
extern bool                  g_overflow;
extern bool                  g_overlay_3d;
extern bool                  g_overwrite_file;
extern double                g_orbit_corner_3_x;
extern double                g_orbit_corner_max_x;
extern double                g_orbit_corner_min_x;
extern double                g_orbit_corner_3_y;
extern double                g_orbit_corner_max_y;
extern double                g_orbit_corner_min_y;
extern double                g_params[];
extern double                g_evolve_x_parameter_range;
extern double                g_evolve_y_parameter_range;
extern double                g_evolve_param_zoom;
extern DComplex              g_param_z2;
extern DComplex              g_param_z1;
extern int                   g_patch_level;
extern int                   g_periodicity_check;
struct fn_operand;
extern std::vector<fn_operand> g_function_operands;
extern int                   g_pi_in_pixels;
extern void                (*g_plot)(int, int, int);
extern double                g_plot_mx1;
extern double                g_plot_mx2;
extern double                g_plot_my1;
extern double                g_plot_my2;
extern int                   g_polyphony;
extern unsigned              g_operation_index;
extern bool                  g_potential_16bit;
extern bool                  g_potential_flag;
extern double                g_potential_params[];
extern bool                  g_preview;
extern int                   g_preview_factor;
extern int                   g_evolve_param_grid_x;
extern int                   g_evolve_param_grid_y;
extern int                   g_evolve_param_box_count;
extern int                   g_distance_estimator_x_dots;
extern int                   g_distance_estimator_y_dots;
extern void                (*g_put_color)(int, int, int);
extern DComplex              g_power_z;
extern double                g_quaternion_c;
extern double                g_quaternion_ci;
extern double                g_quaternion_cj;
extern double                g_quaternino_ck;
extern bool                  g_quick_calc;
extern int                   g_randomize_3d;
extern std::vector<int>      g_iteration_ranges;
extern int                   g_iteration_ranges_len;
extern raytrace_formats      g_raytrace_format;
extern std::string           g_raytrace_filename;
extern std::string           g_read_filename;
extern long                  g_real_color_iter;
extern record_colors_mode    g_record_colors;
extern int                   g_red_bright;
extern int                   g_red_crop_left;
extern int                   g_red_crop_right;
extern int                   g_release;
extern int                   g_resave_flag;
extern bool                  g_reset_periodicity;
extern std::vector<BYTE>     g_resume_data;
extern int                   g_resume_len;
extern bool                  g_resuming;
extern bool                  g_random_seed_flag;
extern int                   g_rhombus_stack[];
extern std::vector<DComplex> g_roots;
extern int                   g_color_cycle_range_hi;
extern int                   g_color_cycle_range_lo;
extern double                g_newton_r_over_d;
extern int                   row;
extern int                   g_row_count;       // row-counter for decoder and out_line
extern double                rqlim2;
extern double                rqlim;
extern int                   g_random_seed;
extern long                  g_save_base;
extern DComplex              SaveC;
extern int                   g_save_dac;
extern std::string           g_save_filename;
extern long                  g_save_ticks;
extern int                   g_save_release;
extern int                   g_save_system;
extern int                   scale_map[];
extern float                 screenaspect;
extern id::SearchPath        searchfor;
extern bool                  set_orbit_corners;
extern bool                  showbox;
extern int show_dot;
extern int show_file;
extern bool                  show_orbit;
extern double                sinx;
extern int                   sizedot;
extern short                 skipxdots;
extern short                 skipydots;
extern slides_mode           g_slides;
extern int                   Slope;
extern int                   soundflag;
extern std::string const     speed_prompt;
extern void                (*standardplot)(int, int, int);
extern bool start_show_orbit;
extern bool                  started_resaves;
extern char                  stdcalcmode;
extern std::string           stereomapname;
extern int                   StoPtr;
extern int                   stoppass;
extern double                sx3rd;
extern int                   sxdots;
extern double                sxmax;
extern double                sxmin;
extern int                   sxoffs;
extern double                sy3rd;
extern int                   sydots;
extern double                symax;
extern double                symin;
extern symmetry_type         symmetry;
extern int                   syoffs;
extern bool make_parameter_file;
extern bool make_parameter_file_map;
extern bool tab_mode;
extern bool                  taborhelp;
extern bool                  Targa_Out;
extern bool                  Targa_Overlay;
extern double                tempsqrx;
extern double                tempsqry;
extern int                   g_text_cbase;      // g_text_col is relative to this
extern int                   g_text_col;        // current column in text mode
extern int                   g_text_rbase;      // g_text_row is relative to this
extern int                   g_text_row;        // current row in text mode
extern unsigned int evolve_this_generation_random_seed;
extern unsigned *            tga16;
extern long *                tga32;
extern bool                  three_pass;
extern double                threshold;
extern int                   timedsave;
extern bool                  timerflag;
extern long                  timer_interval;
extern long                  timer_start;
extern DComplex              tmp;
extern std::string           tempdir;
extern double smallest_window_display_size;
extern int                   totpasses;
extern int                   transparent[];
extern bool                  truecolor;
extern int                   truemode;
extern double                twopi;
extern char                  useinitorbit;
extern bool                  use_grid;
extern bool                  usemag;
extern bool                  uses_ismand;
extern bool                  uses_p1;
extern bool                  uses_p2;
extern bool                  uses_p3;
extern bool                  uses_p4;
extern bool                  uses_p5;
extern bool                  use_old_distest;
extern bool                  use_old_period;
extern bool                  using_jiim;
extern int                   usr_biomorph;
extern long                  usr_distest;
extern bool                  usr_floatflag;
extern int                   usr_periodicitycheck;
extern char                  usr_stdcalcmode;
extern int                   g_vesa_x_res;
extern int                   g_vesa_y_res;
extern VIDEOINFO             g_video_entry;
extern VIDEOINFO             g_video_table[];
extern int                   g_video_table_len;
extern bool                  video_cutboth;
extern bool                  g_video_scroll;
extern int                   g_video_start_x;
extern int                   g_video_start_y;
extern int                   g_video_type;      // video adapter type
extern VECTOR                view;
extern bool                  viewcrop;
extern float                 viewreduction;
extern bool                  viewwindow;
extern int                   viewxdots;
extern int                   viewydots;
extern bool                  g_virtual_screens;
extern unsigned              vsp;
extern int                   g_vxdots;
extern stereo_images         g_which_image;
extern float                 widthfp;
extern std::string           workdir;
extern WORKLIST              worklist[MAXCALCWORK];
extern int                   workpass;
extern int                   worksym;
extern long                  x3rd;
extern int                   xadjust;
extern double                xcjul;
extern int                   xdots;
extern long                  xmax;
extern long                  xmin;
extern int                   xshift1;
extern int                   xshift;
extern int                   xtrans;
extern double                xx3rd;
extern int                   xxadjust1;
extern int                   xxadjust;
extern double                xxmax;
extern double                xxmin;
extern long                  XXOne;
extern int                   xxstart;
extern int                   xxstop;
extern long                  y3rd;
extern int                   yadjust;
extern double                ycjul;
extern int                   ydots;
extern long                  ymax;
extern long                  ymin;
extern int                   yshift1;
extern int                   yshift;
extern int                   ytrans;
extern double                yy3rd;
extern int                   yyadjust1;
extern int                   yyadjust;
extern int                   yyadjust;
extern double                yymax;
extern double                yymin;
extern int                   yystart;
extern int                   yystop;
extern double                zbx;
extern double                zby;
extern double                zoom_box_height;
extern int                   zdots;
extern bool                  zoomoff;
extern int                   zoom_box_rotation;
extern bool                  zscroll;
extern double                zoom_box_skew;
extern double                zoom_box_width;
#if defined(XFRACT)
extern bool                  fake_lut;
extern bool                  XZoomWaiting;
#endif
#endif
