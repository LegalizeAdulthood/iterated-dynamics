/*
        loadfile.c - load an existing fractal image, control level
*/
#include "port.h"
#include "prototyp.h"

#include "loadfile.h"

#include "bailout_formula.h"
#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "encoder.h"
#include "evolve.h"
#include "find_file.h"
#include "find_special_colors.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "fractype.h"
#include "framain2.h"
#include "get_3d_params.h"
#include "get_browse_params.h"
#include "id_data.h"
#include "jb.h"
#include "line3d.h"
#include "loadfdos.h"
#include "lorenz.h"
#include "make_batch_file.h"
#include "make_path.h"
#include "miscres.h"
#include "parser.h"
#include "plot3d.h"
#include "prompts2.h"
#include "realdos.h"
#include "split_path.h"
#include "rotate.h"
#include "stop_msg.h"
#include "zoom.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace
{

#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct ext_blk_2
{
    bool got_data;
    int length;
    std::vector<BYTE> resume_data;
};

struct ext_blk_4
{
    bool got_data;
    int length;
    std::vector<int> range_data;
};

struct ext_blk_5
{
    bool got_data;
    int length;
    std::vector<char> apm_data;
};

// parameter evolution stuff
struct ext_blk_6
{
    bool got_data;
    int length;
    short evolving;
    short image_grid_size;
    unsigned short this_generation_random_seed;
    double max_random_mutation;
    double x_parameter_range;
    double y_parameter_range;
    double x_parameter_offset;
    double y_parameter_offset;
    short discrete_x_parameter_offset;
    short discrete_y_parameter_offset;
    short  px;
    short  py;
    short  sxoffs;
    short  syoffs;
    short  xdots;
    short  ydots;
    short  ecount;
    short  mutate[NUM_GENES];
};

struct ext_blk_7
{
    bool got_data;
    int length;
    double oxmin;
    double oxmax;
    double oymin;
    double oymax;
    double ox3rd;
    double oy3rd;
    short keep_scrn_coords;
    char drawmode;
};
#if defined(_WIN32)
#pragma pack(pop)
#endif

struct dblcoords
{
    double x;
    double y;
};

} // namespace

static int  find_fractal_info(char const *gif_file, FRACTAL_INFO *info,
                             ext_blk_2 *blk_2_info,
                             ext_blk_3 *blk_3_info,
                             ext_blk_4 *blk_4_info,
                             ext_blk_5 *blk_5_info,
                             ext_blk_6 *blk_6_info,
                             ext_blk_7 *blk_7_info);
static void load_ext_blk(char *loadptr, int loadlen);
static void skip_ext_blk(int *, int *);
static void backwardscompat(FRACTAL_INFO *info);
static bool fix_bof();
static bool fix_period_bof();

bool g_loaded_3d = false;
static std::FILE *fp;
int g_file_y_dots;
int g_file_x_dots;
int g_file_colors;
float g_file_aspect_ratio;
short g_skip_x_dots;
short g_skip_y_dots;      // for decoder, when reducing image
bool g_bad_outside = false;
bool g_ld_check = false;

int read_overlay()      // read overlay/3D files, if reqr'd
{
    FRACTAL_INFO read_info;
    char msg[110];
    ext_blk_2 blk_2_info;
    ext_blk_3 blk_3_info;
    ext_blk_4 blk_4_info;
    ext_blk_5 blk_5_info;
    ext_blk_6 blk_6_info;
    ext_blk_7 blk_7_info;

    g_show_file = 1;                // for any abort exit, pretend done
    g_init_mode = -1;               // no viewing mode set yet
    const bool old_float_flag = g_user_float_flag;
    g_loaded_3d = false;
    if (g_fast_restore)
    {
        g_view_window = false;
    }
    if (has_ext(g_read_filename.c_str()) == nullptr)
    {
        g_read_filename += ".gif";
    }

    if (find_fractal_info(
        g_read_filename.c_str(),
        &read_info, &blk_2_info, &blk_3_info,
        &blk_4_info, &blk_5_info, &blk_6_info, &blk_7_info))
    {
        // didn't find a useable file
        std::snprintf(msg, NUM_OF(msg), "Sorry, %s isn't a file I can decode.", g_read_filename.c_str());
        stopmsg(STOPMSG_NONE, msg);
        return -1;
    }

    g_max_iterations        = read_info.iterationsold;
    int const read_fractype = read_info.fractal_type;
    if (read_fractype < 0 || read_fractype >= g_num_fractal_types)
    {
        std::snprintf(msg, NUM_OF(msg), "Warning: %s has a bad fractal type; using 0", g_read_filename.c_str());
        g_fractal_type = fractal_type::MANDEL;
    }
    g_fractal_type = static_cast<fractal_type>(read_fractype);
    g_cur_fractal_specific = &g_fractal_specific[read_fractype];
    g_x_min        = read_info.xmin;
    g_x_max        = read_info.xmax;
    g_y_min        = read_info.ymin;
    g_y_max        = read_info.ymax;
    g_params[0]     = read_info.creal;
    g_params[1]     = read_info.cimag;

    g_invert = 0;
    if (read_info.version > 0)
    {
        g_params[2]      = read_info.parm3;
        roundfloatd(&g_params[2]);
        g_params[3]      = read_info.parm4;
        roundfloatd(&g_params[3]);
        g_potential_params[0]   = read_info.potential[0];
        g_potential_params[1]   = read_info.potential[1];
        g_potential_params[2]   = read_info.potential[2];
        if (g_make_parameter_file)
        {
            g_colors = read_info.colors;
        }
        g_potential_flag       = (g_potential_params[0] != 0.0);
        g_random_seed_flag         = read_info.rflag != 0;
        g_random_seed         = read_info.rseed;
        g_inside_color        = read_info.inside;
        g_log_map_flag       = read_info.logmapold;
        g_inversion[0]  = read_info.invert[0];
        g_inversion[1]  = read_info.invert[1];
        g_inversion[2]  = read_info.invert[2];
        if (g_inversion[0] != 0.0)
        {
            g_invert = 3;
        }
        g_decomp[0]     = read_info.decomp[0];
        g_decomp[1]     = read_info.decomp[1];
        g_user_biomorph_value  = read_info.biomorph;
        g_force_symmetry = static_cast<symmetry_type>(read_info.symmetry);
    }

    if (read_info.version > 1)
    {
        if ((g_display_3d == display_3d_modes::NONE)
            && (read_info.version <= 4
                || read_info.display_3d > 0
                || (g_cur_fractal_specific->flags & PARMS3D) != 0))
        {
            for (int i = 0; i < 16; i++)
            {
                g_init_3d[i] = read_info.init3d[i];
            }
            g_preview_factor   = read_info.previewfactor;
            g_adjust_3d_x          = read_info.xtrans;
            g_adjust_3d_y          = read_info.ytrans;
            g_red_crop_left   = read_info.red_crop_left;
            g_red_crop_right  = read_info.red_crop_right;
            g_blue_crop_left  = read_info.blue_crop_left;
            g_blue_crop_right = read_info.blue_crop_right;
            g_red_bright      = read_info.red_bright;
            g_blue_bright     = read_info.blue_bright;
            g_converge_x_adjust         = read_info.xadjust;
            g_eye_separation   = read_info.eyeseparation;
            g_glasses_type     = read_info.glassestype;
        }
    }

    if (read_info.version > 2)
    {
        g_outside_color      = read_info.outside;
    }

    g_calc_status = calc_status_value::PARAMS_CHANGED;       // defaults if version < 4
    g_x_3rd = g_x_min;
    g_y_3rd = g_y_min;
    g_user_distance_estimator_value = 0;
    g_calc_time = 0;
    if (read_info.version > 3)
    {
        g_x_3rd       = read_info.x3rd;
        g_y_3rd       = read_info.y3rd;
        g_calc_status = static_cast<calc_status_value>(read_info.calc_status);
        g_user_std_calc_mode = read_info.stdcalcmode;
        g_three_pass = false;
        if (g_user_std_calc_mode == 127)
        {
            g_three_pass = true;
            g_user_std_calc_mode = '3';
        }
        g_user_distance_estimator_value     = read_info.distestold;
        g_user_float_flag   = read_info.floatflag != 0;
        g_bail_out     = read_info.bailoutold;
        g_calc_time    = read_info.calctime;
        g_trig_index[0]  = static_cast<trig_fn>(read_info.trigndx[0]);
        g_trig_index[1]  = static_cast<trig_fn>(read_info.trigndx[1]);
        g_trig_index[2]  = static_cast<trig_fn>(read_info.trigndx[2]);
        g_trig_index[3]  = static_cast<trig_fn>(read_info.trigndx[3]);
        g_finite_attractor  = read_info.finattract != 0;
        g_init_orbit.x = read_info.initorbit[0];
        g_init_orbit.y = read_info.initorbit[1];
        g_use_init_orbit = static_cast<init_orbit_mode>(read_info.useinitorbit);
        g_user_periodicity_value = read_info.periodicity;
    }

    g_potential_16bit = false;
    g_save_system = 0;
    if (read_info.version > 4)
    {
        g_potential_16bit = read_info.pot16bit != 0;
        if (g_potential_16bit)
        {
            g_file_x_dots >>= 1;
        }
        g_file_aspect_ratio = read_info.faspectratio;
        if (g_file_aspect_ratio < 0.01)       // fix files produced in early v14.1
        {
            g_file_aspect_ratio = g_screen_aspect;
        }
        g_save_system = 0;
        if (g_display_3d == display_3d_modes::NONE && read_info.display_3d > 0)
        {
            g_loaded_3d       = true;
            g_ambient        = read_info.ambient;
            g_randomize_3d      = read_info.randomize;
            g_haze           = read_info.haze;
            g_transparent_color_3d[0] = read_info.transparent[0];
            g_transparent_color_3d[1] = read_info.transparent[1];
        }
    }

    g_color_cycle_range_lo = 1;
    g_color_cycle_range_hi = 255;
    g_distance_estimator_width_factor = 71;
    if (read_info.version > 5)
    {
        g_color_cycle_range_lo         = read_info.rotate_lo;
        g_color_cycle_range_hi         = read_info.rotate_hi;
        g_distance_estimator_width_factor      = read_info.distestwidth;
    }

    if (read_info.version > 6)
    {
        g_params[2]          = read_info.dparm3;
        g_params[3]          = read_info.dparm4;
    }

    if (read_info.version > 7)
    {
        g_fill_color         = read_info.fillcolor;
    }

    if (read_info.version > 8)
    {
        g_julibrot_x_max   =  read_info.mxmaxfp        ;
        g_julibrot_x_min   =  read_info.mxminfp        ;
        g_julibrot_y_max   =  read_info.mymaxfp        ;
        g_julibrot_y_min   =  read_info.myminfp        ;
        g_julibrot_z_dots     =  read_info.zdots          ;
        g_julibrot_origin_fp  =  read_info.originfp       ;
        g_julibrot_depth_fp   =  read_info.depthfp        ;
        g_julibrot_height_fp  =  read_info.heightfp       ;
        g_julibrot_width_fp   =  read_info.widthfp        ;
        g_julibrot_dist_fp    =  read_info.distfp         ;
        g_eyes_fp    =  read_info.eyesfp         ;
        g_new_orbit_type = static_cast<fractal_type>(read_info.orbittype);
        g_julibrot_3d_mode   = read_info.juli3Dmode   ;
        g_max_function    = (char)read_info.maxfn          ;
        g_major_method = static_cast<Major>(read_info.inversejulia >> 8);
        g_inverse_julia_minor_method = static_cast<Minor>(read_info.inversejulia & 255);
        g_params[4] = read_info.dparm5;
        g_params[5] = read_info.dparm6;
        g_params[6] = read_info.dparm7;
        g_params[7] = read_info.dparm8;
        g_params[8] = read_info.dparm9;
        g_params[9] = read_info.dparm10;
    }

    if (read_info.version < 4 && read_info.version != 0) // pre-version 14.0?
    {
        backwardscompat(&read_info); // translate obsolete types
        if (g_log_map_flag)
        {
            g_log_map_flag = 2;
        }
        g_user_float_flag = g_cur_fractal_specific->isinteger == 0;
    }

    if (read_info.version < 5 && read_info.version != 0) // pre-version 15.0?
    {
        if (g_log_map_flag == 2) // logmap=old changed again in format 5!
        {
            g_log_map_flag = -1;
        }
        if (g_decomp[0] > 0 && g_decomp[1] > 0)
        {
            g_bail_out = g_decomp[1];
        }
    }
    if (g_potential_flag) // in version 15.x and 16.x logmap didn't work with pot
    {
        if (read_info.version == 6 || read_info.version == 7)
        {
            g_log_map_flag = 0;
        }
    }
    set_trig_pointers(-1);

    if (read_info.version < 9 && read_info.version != 0) // pre-version 18.0?
    {
        /* forcesymmetry==1000 means we want to force symmetry but don't
            know which symmetry yet, will find out in setsymmetry() */
        if (g_outside_color == REAL
            || g_outside_color == IMAG
            || g_outside_color == MULT
            || g_outside_color == SUM
            || g_outside_color == ATAN)
        {
            if (g_force_symmetry == symmetry_type::NOT_FORCED)
            {
                g_force_symmetry = static_cast<symmetry_type>(1000);
            }
        }
    }

    if (read_info.version > 9)
    {
        // post-version 18.22
        g_bail_out     = read_info.bailout; // use long bailout
        g_bail_out_test = static_cast<bailouts>(read_info.bailoutest);
    }
    else
    {
        g_bail_out_test = bailouts::Mod;
    }
    set_bailout_formula(g_bail_out_test);

    if (read_info.version > 9)
    {
        // post-version 18.23
        g_max_iterations = read_info.iterations; // use long maxit
        // post-version 18.27
        g_old_demm_colors = read_info.old_demm_colors != 0;
    }

    if (read_info.version > 10) // post-version 19.20
    {
        g_log_map_flag = read_info.logmap;
        g_user_distance_estimator_value = read_info.distest;
    }

    if (read_info.version > 11) // post-version 19.20, inversion fix
    {
        g_inversion[0] = read_info.dinvert[0];
        g_inversion[1] = read_info.dinvert[1];
        g_inversion[2] = read_info.dinvert[2];
        g_log_map_fly_calculate = read_info.logcalc;
        g_stop_pass     = read_info.stoppass;
    }

    if (read_info.version > 12) // post-version 19.60
    {
        g_quick_calc   = read_info.quick_calc != 0;
        g_close_proximity    = read_info.closeprox;
        if (g_fractal_type == fractal_type::FPPOPCORN || g_fractal_type == fractal_type::LPOPCORN ||
                g_fractal_type == fractal_type::FPPOPCORNJUL || g_fractal_type == fractal_type::LPOPCORNJUL ||
                g_fractal_type == fractal_type::LATOO)
        {
            g_new_bifurcation_functions_loaded = true;
        }
    }

    g_bof_match_book_images = true;
    if (read_info.version > 13) // post-version 20.1.2
    {
        g_bof_match_book_images = read_info.nobof == 0;
    }

    // if (read_info.version > 14)  post-version 20.1.12
    g_log_map_auto_calculate = false;              // make sure it's turned off

    g_orbit_interval = 1;
    if (read_info.version > 15) // post-version 20.3.2
    {
        g_orbit_interval = read_info.orbit_interval;
    }

    g_orbit_delay = 0;
    g_math_tol[0] = 0.05;
    g_math_tol[1] = 0.05;
    if (read_info.version > 16) // post-version 20.4.0
    {
        g_orbit_delay = read_info.orbit_delay;
        g_math_tol[0] = read_info.math_tol[0];
        g_math_tol[1] = read_info.math_tol[1];
    }

    backwards_v18();
    backwards_v19();
    backwards_v20();

    if (g_display_3d != display_3d_modes::NONE)
    {
        g_user_float_flag = old_float_flag;
    }

    if (g_overlay_3d)
    {
        g_init_mode = g_adapter;          // use previous adapter mode for overlays
        if (g_file_x_dots > g_logical_screen_x_dots || g_file_y_dots > g_logical_screen_y_dots)
        {
            stopmsg(STOPMSG_NONE, "Can't overlay with a larger image");
            g_init_mode = -1;
            return -1;
        }
    }
    else
    {
        display_3d_modes const old_display_ed = g_display_3d;
        bool const old_float_flag2 = g_float_flag;
        g_display_3d = g_loaded_3d ? display_3d_modes::YES : display_3d_modes::NONE;   // for <tab> display during next
        g_float_flag = g_user_float_flag; // ditto
        int i = get_video_mode(&read_info, &blk_3_info);
#if defined(_WIN32)
        _ASSERTE(_CrtCheckMemory());
#endif
        g_display_3d = old_display_ed;
        g_float_flag = old_float_flag2;
        if (i)
        {
            if (blk_2_info.got_data)
            {
                blk_2_info.resume_data.clear();
                blk_2_info.length = 0;
            }
            g_init_mode = -1;
            return -1;
        }
    }

    if (g_display_3d != display_3d_modes::NONE)
    {
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        g_fractal_type = fractal_type::PLASMA;
        g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(fractal_type::PLASMA)];
        g_params[0] = 0;
        if (g_init_batch == batch_modes::NONE)
        {
            if (get_3d_params() < 0)
            {
                g_init_mode = -1;
                return -1;
            }
        }
    }

    g_resume_data.clear();

    if (blk_2_info.got_data)
    {
        g_resume_data = blk_2_info.resume_data;
        g_resume_len = blk_2_info.length;
    }

    if (blk_3_info.got_data)
    {
        blk_3_info.form_name[ITEM_NAME_LEN] = 0;
        switch (static_cast<fractal_type>(read_info.fractal_type))
        {
        case fractal_type::LSYSTEM:
            g_l_system_name = blk_3_info.form_name;
            break;

        case fractal_type::IFS:
        case fractal_type::IFS3D:
            g_ifs_name = blk_3_info.form_name;
            break;

        default:
            g_formula_name = blk_3_info.form_name;
            g_frm_uses_p1 = blk_3_info.uses_p1 != 0;
            g_frm_uses_p2 = blk_3_info.uses_p2 != 0;
            g_frm_uses_p3 = blk_3_info.uses_p3 != 0;
            g_frm_uses_ismand = blk_3_info.uses_ismand != 0;
            g_is_mandelbrot = blk_3_info.ismand != 0;
            g_frm_uses_p4 = blk_3_info.uses_p4 != 0;
            g_frm_uses_p5 = blk_3_info.uses_p5 != 0;
            break;
        }
        // perhaps in future add more here, check block_len for backward compatibility
    }

    if (g_iteration_ranges_len) // free prior ranges
    {
        g_iteration_ranges.clear();
        g_iteration_ranges_len = 0;
    }

    if (blk_4_info.got_data)
    {
        g_iteration_ranges_len = blk_4_info.length;
        g_iteration_ranges = blk_4_info.range_data;
    }

    if (blk_5_info.got_data)
    {
        bf_math = bf_math_type::BIGNUM;
        init_bf_length(read_info.bflength);
        std::memcpy((char *) g_bf_x_min, &blk_5_info.apm_data[0], blk_5_info.length);
    }
    else
    {
        bf_math = bf_math_type::NONE;
    }

    if (blk_6_info.got_data)
    {
        GENEBASE gene[NUM_GENES];
        copy_genes_from_bank(gene);
        if (read_info.version < 15)
        {
            // Increasing NUM_GENES moves ecount in the data structure
            // We added 4 to NUM_GENES, so ecount is at NUM_GENES-4
            blk_6_info.ecount = blk_6_info.mutate[NUM_GENES - 4];
        }
        if (blk_6_info.ecount != blk_6_info.image_grid_size *blk_6_info.image_grid_size
            && g_calc_status != calc_status_value::COMPLETED)
        {
            g_calc_status = calc_status_value::RESUMABLE;
            g_evolve_info.x_parameter_range = blk_6_info.x_parameter_range;
            g_evolve_info.y_parameter_range = blk_6_info.y_parameter_range;
            g_evolve_info.x_parameter_offset = blk_6_info.x_parameter_offset;
            g_evolve_info.y_parameter_offset = blk_6_info.y_parameter_offset;
            g_evolve_info.discrete_x_parameter_offset = blk_6_info.discrete_x_parameter_offset;
            g_evolve_info.discrete_y_paramter_offset = blk_6_info.discrete_y_parameter_offset;
            g_evolve_info.px           = blk_6_info.px;
            g_evolve_info.py           = blk_6_info.py;
            g_evolve_info.sxoffs       = blk_6_info.sxoffs;
            g_evolve_info.syoffs       = blk_6_info.syoffs;
            g_evolve_info.xdots        = blk_6_info.xdots;
            g_evolve_info.ydots        = blk_6_info.ydots;
            g_evolve_info.image_grid_size = blk_6_info.image_grid_size;
            g_evolve_info.evolving     = blk_6_info.evolving;
            g_evolve_info.this_generation_random_seed = blk_6_info.this_generation_random_seed;
            g_evolve_info.max_random_mutation = blk_6_info.max_random_mutation;
            g_evolve_info.ecount       = blk_6_info.ecount;
            g_have_evolve_info = true;
        }
        else
        {
            g_have_evolve_info = false;
            g_calc_status = calc_status_value::COMPLETED;
        }
        g_evolve_x_parameter_range = blk_6_info.x_parameter_range;
        g_evolve_y_parameter_range = blk_6_info.y_parameter_range;
        g_evolve_new_x_parameter_offset = blk_6_info.x_parameter_offset;
        g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
        g_evolve_new_y_parameter_offset = blk_6_info.y_parameter_offset;
        g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
        g_evolve_new_discrete_x_parameter_offset = (char) blk_6_info.discrete_x_parameter_offset;
        g_evolve_discrete_x_parameter_offset = g_evolve_new_discrete_x_parameter_offset;
        g_evolve_new_discrete_y_parameter_offset = (char) blk_6_info.discrete_y_parameter_offset;
        g_evolve_discrete_y_parameter_offset = g_evolve_new_discrete_y_parameter_offset;
        g_evolve_param_grid_x           = blk_6_info.px;
        g_evolve_param_grid_y           = blk_6_info.py;
        g_logical_screen_x_offset       = blk_6_info.sxoffs;
        g_logical_screen_y_offset       = blk_6_info.syoffs;
        g_logical_screen_x_dots        = blk_6_info.xdots;
        g_logical_screen_y_dots        = blk_6_info.ydots;
        g_evolve_image_grid_size = blk_6_info.image_grid_size;
        g_evolve_this_generation_random_seed = blk_6_info.this_generation_random_seed;
        g_evolve_max_random_mutation = blk_6_info.max_random_mutation;
        g_evolving = (int) blk_6_info.evolving;
        g_view_window = g_evolving != 0;
        g_evolve_dist_per_x = g_evolve_x_parameter_range /(g_evolve_image_grid_size - 1);
        g_evolve_dist_per_y = g_evolve_y_parameter_range /(g_evolve_image_grid_size - 1);
        if (read_info.version > 14)
        {
            for (int i = 0; i < NUM_GENES; i++)
            {
                gene[i].mutate = static_cast<variations>(blk_6_info.mutate[i]);
            }
        }
        else
        {
            for (int i = 0; i < 6; i++)
            {
                gene[i].mutate = static_cast<variations>(blk_6_info.mutate[i]);
            }
            for (int i = 6; i < 10; i++)
            {
                gene[i].mutate = variations::NONE;
            }
            for (int i = 10; i < NUM_GENES; i++)
            {
                gene[i].mutate = static_cast<variations>(blk_6_info.mutate[i-4]);
            }
        }
        copy_genes_to_bank(gene);
        param_history(0); // store history
    }
    else
    {
        g_evolving = 0;
    }

    if (blk_7_info.got_data)
    {
        g_orbit_corner_min_x       = blk_7_info.oxmin;
        g_orbit_corner_max_x       = blk_7_info.oxmax;
        g_orbit_corner_min_y       = blk_7_info.oymin;
        g_orbit_corner_max_y       = blk_7_info.oymax;
        g_orbit_corner_3_x       = blk_7_info.ox3rd;
        g_orbit_corner_3_y       = blk_7_info.oy3rd;
        g_keep_screen_coords = blk_7_info.keep_scrn_coords != 0;
        g_draw_mode    = blk_7_info.drawmode;
        if (g_keep_screen_coords)
        {
            g_set_orbit_corners = true;
        }
    }

    g_show_file = 0;                   // trigger the file load
    return 0;
}

inline void freader(void *ptr, size_t size, size_t nmemb, std::FILE *stream)
{
    if (std::fread(ptr, size, nmemb, stream) != nmemb)
    {
        throw std::system_error(errno, std::system_category(), "failed fread");
    }
}


static int find_fractal_info(char const *gif_file, FRACTAL_INFO *info,
                             ext_blk_2 *blk_2_info,
                             ext_blk_3 *blk_3_info,
                             ext_blk_4 *blk_4_info,
                             ext_blk_5 *blk_5_info,
                             ext_blk_6 *blk_6_info,
                             ext_blk_7 *blk_7_info)
{
    BYTE gifstart[18];
    char temp1[81];
    int block_len;
    int data_len;
    int fractinf_len;
    int hdr_offset;
    formula_info fload_info;
    EVOLUTION_INFO eload_info;
    ORBITS_INFO oload_info;

    blk_2_info->got_data = false;
    blk_3_info->got_data = false;
    blk_4_info->got_data = false;
    blk_5_info->got_data = false;
    blk_6_info->got_data = false;
    blk_7_info->got_data = false;

    fp = std::fopen(gif_file, "rb");
    if (fp == nullptr)
    {
        return -1;
    }
    freader(gifstart, 13, 1, fp);
    if (std::strncmp((char *)gifstart, "GIF", 3) != 0)
    {
        // not GIF, maybe old .tga?
        std::fclose(fp);
        return -1;
    }

    GET16(gifstart[6], g_file_x_dots);
    GET16(gifstart[8], g_file_y_dots);
    g_file_colors = 2 << (gifstart[10] & 7);
    g_file_aspect_ratio = 0; // unknown
    if (gifstart[12])
    {
        // calc reasonably close value from gif header
        g_file_aspect_ratio = (float)((64.0 / ((double)(gifstart[12]) + 15.0))
                                  * (double)g_file_y_dots / (double)g_file_x_dots);
        if (g_file_aspect_ratio > g_screen_aspect-0.03
            && g_file_aspect_ratio < g_screen_aspect+0.03)
        {
            g_file_aspect_ratio = g_screen_aspect;
        }
    }
    else if (g_file_y_dots * 4 == g_file_x_dots * 3)   // assume the common square pixels
    {
        g_file_aspect_ratio = g_screen_aspect;
    }

    if (g_make_parameter_file && (gifstart[10] & 0x80) != 0)
    {
        for (int i = 0; i < g_file_colors; i++)
        {
            int k = 0;
            for (int j = 0; j < 3; j++)
            {
                k = getc(fp);
                if (k < 0)
                {
                    break;
                }
                g_dac_box[i][j] = (BYTE)(k >> 2);
            }
            if (k < 0)
            {
                break;
            }
        }
    }

    /* Format of .gif extension blocks is:
           1 byte    '!', extension block identifier
           1 byte    extension block number, 255
           1 byte    length of id, 11
          11 bytes   alpha id, "fractintnnn" with fractint, nnn is secondary id
        n * {
           1 byte    length of block info in bytes
           x bytes   block info
            }
           1 byte    0, extension terminator
       To scan extension blocks, we first look in file at length of FRACTAL_INFO
       (the main extension block) from end of file, looking for a literal known
       to be at start of our block info.  Then we scan forward a bit, in case
       the file is from an earlier fractint vsn with shorter FRACTAL_INFO.
       If FRACTAL_INFO is found and is from vsn>=14, it includes the total length
       of all extension blocks; we then scan them all first to last to load
       any optional ones which are present.
       Defined extension blocks:
         fractint001     header, always present
         fractint002     resume info for interrupted resumable image
         fractint003     additional formula type info
         fractint004     ranges info
         fractint005     extended precision parameters
         fractint006     evolver params
    */

    std::memset(info, 0, FRACTAL_INFO_SIZE);
    fractinf_len = FRACTAL_INFO_SIZE + (FRACTAL_INFO_SIZE+254)/255;
    std::fseek(fp, (long)(-1-fractinf_len), SEEK_END);
    /* TODO: revise this to read members one at a time so we get natural alignment
       of fields within the FRACTAL_INFO structure for the platform */
    freader(info, 1, FRACTAL_INFO_SIZE, fp);
    if (std::strcmp(INFO_ID, info->info_id) == 0)
    {
#ifdef XFRACT
        decode_fractal_info(info, 1);
#endif
        hdr_offset = -1-fractinf_len;
    }
    else
    {
        // didn't work 1st try, maybe an older vsn, maybe junk at eof, scan:
        int offset;
        char tmpbuf[110];
        hdr_offset = 0;
        offset = 80; // don't even check last 80 bytes of file for id
        while (offset < fractinf_len+513)
        {
            // allow 512 garbage at eof
            offset += 100; // go back 100 bytes at a time
            std::fseek(fp, (long)(0-offset), SEEK_END);
            freader(tmpbuf, 1, 110, fp); // read 10 extra for string compare
            for (int i = 0; i < 100; ++i)
            {
                if (!std::strcmp(INFO_ID, &tmpbuf[i]))
                {
                    // found header?
                    std::strcpy(info->info_id, INFO_ID);
                    std::fseek(fp, (long)(hdr_offset = i-offset), SEEK_END);
                    /* TODO: revise this to read members one at a time so we get natural alignment
                        of fields within the FRACTAL_INFO structure for the platform */
                    freader(info, 1, FRACTAL_INFO_SIZE, fp);
#ifdef XFRACT
                    decode_fractal_info(info, 1);
#endif
                    offset = 10000; // force exit from outer loop
                    break;
                }
            }
        }
    }

    if (hdr_offset)
    {
        // we found INFO_ID

        if (info->version >= 4)
        {
            /* first reload main extension block, reasons:
                 might be over 255 chars, and thus earlier load might be bad
                 find exact endpoint, so scan back to start of ext blks works
               */
            fseek(fp, (long)(hdr_offset-15), SEEK_END);
            int scan_extend = 1;
            while (scan_extend)
            {
                if (fgetc(fp) != '!' // if not what we expect just give up
                    || std::fread(temp1, 1, 13, fp) != 13
                    || std::strncmp(&temp1[2], "fractint", 8))
                {
                    break;
                }
                temp1[13] = 0;
                switch (atoi(&temp1[10]))   // e.g. "fractint002"
                {
                case 1: // "fractint001", the main extension block
                    if (scan_extend == 2)
                    {
                        // we've been here before, done now
                        scan_extend = 0;
                        break;
                    }
                    load_ext_blk((char *)info, FRACTAL_INFO_SIZE);
#ifdef XFRACT
                    decode_fractal_info(info, 1);
#endif
                    scan_extend = 2;
                    // now we know total extension len, back up to first block
                    fseek(fp, 0L-info->tot_extend_len, SEEK_CUR);
                    break;
                case 2: // resume info
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    blk_2_info->resume_data.resize(data_len);
                    fseek(fp, (long)(0-block_len), SEEK_CUR);
                    load_ext_blk((char *)g_block, data_len);
                    std::copy(&g_block[0], &g_block[data_len], &blk_2_info->resume_data[0]);
                    blk_2_info->length = data_len;
                    blk_2_info->got_data = true;
                    break;
                case 3: // formula info
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    // check data_len for backward compatibility
                    fseek(fp, (long)(0-block_len), SEEK_CUR);
                    load_ext_blk((char *)&fload_info, data_len);
                    std::strcpy(blk_3_info->form_name, fload_info.form_name);
                    blk_3_info->length = data_len;
                    blk_3_info->got_data = true;
                    if (data_len < sizeof(fload_info))
                    {
                        // must be old GIF
                        blk_3_info->uses_p1 = 1;
                        blk_3_info->uses_p2 = 1;
                        blk_3_info->uses_p3 = 1;
                        blk_3_info->uses_ismand = 0;
                        blk_3_info->ismand = 1;
                        blk_3_info->uses_p4 = 0;
                        blk_3_info->uses_p5 = 0;
                    }
                    else
                    {
                        blk_3_info->uses_p1 = fload_info.uses_p1;
                        blk_3_info->uses_p2 = fload_info.uses_p2;
                        blk_3_info->uses_p3 = fload_info.uses_p3;
                        blk_3_info->uses_ismand = fload_info.uses_ismand;
                        blk_3_info->ismand = fload_info.ismand;
                        blk_3_info->uses_p4 = fload_info.uses_p4;
                        blk_3_info->uses_p5 = fload_info.uses_p5;
                    }
                    break;
                case 4: // ranges info
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    assert(data_len % 2 == 0);  // should specify an integral number of 16-bit ints
                    blk_4_info->length = data_len/2;
                    blk_4_info->range_data.resize(blk_4_info->length);
                    fseek(fp, (long) -block_len, SEEK_CUR);
                    {
                        std::vector<char> buffer(data_len, 0);
                        load_ext_blk(&buffer[0], data_len);
                        for (int i = 0; i < blk_4_info->length; ++i)
                        {
                            // int16 stored in little-endian byte order
                            blk_4_info->range_data[i] = buffer[i*2 + 0] | (buffer[i*2 + 1] << 8);
                        }
                    }
                    blk_4_info->got_data = true;
                    break;
                case 5: // extended precision parameters
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    blk_5_info->apm_data.resize(data_len);
                    fseek(fp, (long)(0-block_len), SEEK_CUR);
                    load_ext_blk(&blk_5_info->apm_data[0], data_len);
                    blk_5_info->length = data_len;
                    blk_5_info->got_data = true;
                    break;
                case 6: // evolver params
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    fseek(fp, (long)(0-block_len), SEEK_CUR);
                    load_ext_blk((char *)&eload_info, data_len);
                    // XFRACT processing of doubles here
#ifdef XFRACT
                    decode_evolver_info(&eload_info, 1);
#endif
                    blk_6_info->length = data_len;
                    blk_6_info->got_data = true;

                    blk_6_info->x_parameter_range = eload_info.x_parameter_range;
                    blk_6_info->y_parameter_range = eload_info.y_parameter_range;
                    blk_6_info->x_parameter_offset = eload_info.x_parameter_offset;
                    blk_6_info->y_parameter_offset = eload_info.y_parameter_offset;
                    blk_6_info->discrete_x_parameter_offset = (char)eload_info.discrete_x_parameter_offset;
                    blk_6_info->discrete_y_parameter_offset = (char)eload_info.discrete_y_paramter_offset;
                    blk_6_info->px              = eload_info.px;
                    blk_6_info->py              = eload_info.py;
                    blk_6_info->sxoffs          = eload_info.sxoffs;
                    blk_6_info->syoffs          = eload_info.syoffs;
                    blk_6_info->xdots           = eload_info.xdots;
                    blk_6_info->ydots           = eload_info.ydots;
                    blk_6_info->image_grid_size = eload_info.image_grid_size;
                    blk_6_info->evolving        = eload_info.evolving;
                    blk_6_info->this_generation_random_seed = eload_info.this_generation_random_seed;
                    blk_6_info->max_random_mutation = eload_info.max_random_mutation;
                    blk_6_info->ecount          = eload_info.ecount;
                    for (int i = 0; i < NUM_GENES; i++)
                    {
                        blk_6_info->mutate[i]    = eload_info.mutate[i];
                    }
                    break;
                case 7: // orbits parameters
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    fseek(fp, (long)(0-block_len), SEEK_CUR);
                    load_ext_blk((char *)&oload_info, data_len);
                    // XFRACT processing of doubles here
#ifdef XFRACT
                    decode_orbits_info(&oload_info, 1);
#endif
                    blk_7_info->length = data_len;
                    blk_7_info->got_data = true;
                    blk_7_info->oxmin           = oload_info.oxmin;
                    blk_7_info->oxmax           = oload_info.oxmax;
                    blk_7_info->oymin           = oload_info.oymin;
                    blk_7_info->oymax           = oload_info.oymax;
                    blk_7_info->ox3rd           = oload_info.ox3rd;
                    blk_7_info->oy3rd           = oload_info.oy3rd;
                    blk_7_info->keep_scrn_coords= oload_info.keep_scrn_coords;
                    blk_7_info->drawmode        = oload_info.drawmode;
                    break;
                default:
                    skip_ext_blk(&block_len, &data_len);
                }
            }
        }

        std::fclose(fp);
        g_file_aspect_ratio = g_screen_aspect; // if not >= v15, this is correct
        return 0;
    }

    std::strcpy(info->info_id, "GIFFILE");
    info->iterations = 150;
    info->iterationsold = 150;
    info->fractal_type = static_cast<short>(fractal_type::PLASMA);
    info->xmin = -1;
    info->xmax = 1;
    info->ymin = -1;
    info->ymax = 1;
    info->x3rd = -1;
    info->y3rd = -1;
    info->creal = 0;
    info->cimag = 0;
    info->videomodeax = 255;
    info->videomodebx = 255;
    info->videomodecx = 255;
    info->videomodedx = 255;
    info->dotmode = 0;
    info->xdots = (short)g_file_x_dots;
    info->ydots = (short)g_file_y_dots;
    info->colors = (short)g_file_colors;
    info->version = 0; // this forces lots more init at calling end too

    // zero means we won
    std::fclose(fp);
    return 0;
}

static void load_ext_blk(char *loadptr, int loadlen)
{
    int len;
    while ((len = fgetc(fp)) > 0)
    {
        while (--len >= 0)
        {
            if (--loadlen >= 0)
            {
                *(loadptr++) = (char)fgetc(fp);
            }
            else
            {
                fgetc(fp); // discard excess characters
            }
        }
    }
}

static void skip_ext_blk(int *block_len, int *data_len)
{
    int len;
    *data_len = 0;
    *block_len = 1;
    while ((len = fgetc(fp)) > 0)
    {
        fseek(fp, (long)len, SEEK_CUR);
        *data_len += len;
        *block_len += len + 1;
    }
}


// switch obsolete fractal types to new generalizations
static void backwardscompat(FRACTAL_INFO *info)
{
    switch (g_fractal_type)
    {
    case fractal_type::LAMBDASINE:
        g_fractal_type = fractal_type::LAMBDATRIGFP;
        g_trig_index[0] = trig_fn::SIN;
        break;
    case fractal_type::LAMBDACOS:
        g_fractal_type = fractal_type::LAMBDATRIGFP;
        g_trig_index[0] = trig_fn::COSXX;
        break;
    case fractal_type::LAMBDAEXP:
        g_fractal_type = fractal_type::LAMBDATRIGFP;
        g_trig_index[0] = trig_fn::EXP;
        break;
    case fractal_type::MANDELSINE:
        g_fractal_type = fractal_type::MANDELTRIGFP;
        g_trig_index[0] = trig_fn::SIN;
        break;
    case fractal_type::MANDELCOS:
        g_fractal_type = fractal_type::MANDELTRIGFP;
        g_trig_index[0] = trig_fn::COSXX;
        break;
    case fractal_type::MANDELEXP:
        g_fractal_type = fractal_type::MANDELTRIGFP;
        g_trig_index[0] = trig_fn::EXP;
        break;
    case fractal_type::MANDELSINH:
        g_fractal_type = fractal_type::MANDELTRIGFP;
        g_trig_index[0] = trig_fn::SINH;
        break;
    case fractal_type::LAMBDASINH:
        g_fractal_type = fractal_type::LAMBDATRIGFP;
        g_trig_index[0] = trig_fn::SINH;
        break;
    case fractal_type::MANDELCOSH:
        g_fractal_type = fractal_type::MANDELTRIGFP;
        g_trig_index[0] = trig_fn::COSH;
        break;
    case fractal_type::LAMBDACOSH:
        g_fractal_type = fractal_type::LAMBDATRIGFP;
        g_trig_index[0] = trig_fn::COSH;
        break;
    case fractal_type::LMANDELSINE:
        g_fractal_type = fractal_type::MANDELTRIG;
        g_trig_index[0] = trig_fn::SIN;
        break;
    case fractal_type::LLAMBDASINE:
        g_fractal_type = fractal_type::LAMBDATRIG;
        g_trig_index[0] = trig_fn::SIN;
        break;
    case fractal_type::LMANDELCOS:
        g_fractal_type = fractal_type::MANDELTRIG;
        g_trig_index[0] = trig_fn::COSXX;
        break;
    case fractal_type::LLAMBDACOS:
        g_fractal_type = fractal_type::LAMBDATRIG;
        g_trig_index[0] = trig_fn::COSXX;
        break;
    case fractal_type::LMANDELSINH:
        g_fractal_type = fractal_type::MANDELTRIG;
        g_trig_index[0] = trig_fn::SINH;
        break;
    case fractal_type::LLAMBDASINH:
        g_fractal_type = fractal_type::LAMBDATRIG;
        g_trig_index[0] = trig_fn::SINH;
        break;
    case fractal_type::LMANDELCOSH:
        g_fractal_type = fractal_type::MANDELTRIG;
        g_trig_index[0] = trig_fn::COSH;
        break;
    case fractal_type::LLAMBDACOSH:
        g_fractal_type = fractal_type::LAMBDATRIG;
        g_trig_index[0] = trig_fn::COSH;
        break;
    case fractal_type::LMANDELEXP:
        g_fractal_type = fractal_type::MANDELTRIG;
        g_trig_index[0] = trig_fn::EXP;
        break;
    case fractal_type::LLAMBDAEXP:
        g_fractal_type = fractal_type::LAMBDATRIG;
        g_trig_index[0] = trig_fn::EXP;
        break;
    case fractal_type::DEMM:
        g_fractal_type = fractal_type::MANDELFP;
        g_user_distance_estimator_value = (info->ydots - 1) * 2;
        break;
    case fractal_type::DEMJ:
        g_fractal_type = fractal_type::JULIAFP;
        g_user_distance_estimator_value = (info->ydots - 1) * 2;
        break;
    case fractal_type::MANDELLAMBDA:
        g_use_init_orbit = init_orbit_mode::pixel;
        break;
    default:
        break;
    }
    g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
}

// switch old bifurcation fractal types to new generalizations
void set_if_old_bif()
{
    /* set functions if not set already, may need to check 'new_bifurcation_functions_loaded'
       before calling this routine. */

    switch (g_fractal_type)
    {
    case fractal_type::BIFURCATION:
    case fractal_type::LBIFURCATION:
    case fractal_type::BIFSTEWART:
    case fractal_type::LBIFSTEWART:
    case fractal_type::BIFLAMBDA:
    case fractal_type::LBIFLAMBDA:
        set_trig_array(0, "ident");
        break;

    case fractal_type::BIFEQSINPI:
    case fractal_type::LBIFEQSINPI:
    case fractal_type::BIFADSINPI:
    case fractal_type::LBIFADSINPI:
        set_trig_array(0, "sin");
        break;

    default:
        break;
    }
}

// miscellaneous function variable defaults
void set_function_parm_defaults()
{
    switch (g_fractal_type)
    {
    case fractal_type::FPPOPCORN:
    case fractal_type::LPOPCORN:
    case fractal_type::FPPOPCORNJUL:
    case fractal_type::LPOPCORNJUL:
        set_trig_array(0, "sin");
        set_trig_array(1, "tan");
        set_trig_array(2, "sin");
        set_trig_array(3, "tan");
        break;

    case fractal_type::LATOO:
        set_trig_array(0, "sin");
        set_trig_array(1, "sin");
        set_trig_array(2, "sin");
        set_trig_array(3, "sin");
        break;

    default:
        break;
    }
}

void backwards_v18()
{
    if (!g_new_bifurcation_functions_loaded)
    {
        set_if_old_bif(); // old bifs need function set
    }
}

void backwards_v19()
{
    // fractal might have old bof60/61 problem with magnitude
    g_magnitude_calc = !fix_bof();
    // fractal might use old periodicity method
    g_use_old_periodicity = fix_period_bof();
    g_use_old_distance_estimator = false;
}

void backwards_v20()
{
    // Fractype == FP type is not seen from PAR file ?????
    g_bad_outside = false;
    g_ld_check = (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
        && (g_debug_flag == debug_flags::force_ld_check);
    if (!g_new_bifurcation_functions_loaded)
    {
        set_function_parm_defaults();
    }
}

static bool fix_bof()
{
    return false;
}

static bool fix_period_bof()
{
    return false;
}

// browse code RB

#define MAX_WINDOWS_OPEN 450

namespace
{

struct window
{
    // for fgetwindow on screen browser
    coords      itl;      // screen coordinates
    coords      ibl;      //
    coords      itr;      //
    coords      ibr;      //
    double      win_size; // box size for drawindow()
    std::string name;     // for filename
    int         boxcount; // bytes of saved screen info
};

} // namespace

// prototypes
static void drawindow(int colour, window const *info);
static bool is_visible_window(window *list, FRACTAL_INFO const *info, ext_blk_5 const *blk_5_info);
static void transform(dblcoords *);
static bool paramsOK(FRACTAL_INFO const *info);
static bool typeOK(FRACTAL_INFO const *info, ext_blk_3 const *blk_3_info);
static bool functionOK(FRACTAL_INFO const *info, int numfn);
static void check_history(char const *oldname, char const *newname);
static void bfsetup_convert_to_screen();
static void bftransform(bf_t, bf_t, dblcoords *);

std::string g_browse_name; // name for browse file

static std::vector<window> browse_windows;
static std::vector<int> browse_box_x;
static std::vector<int> browse_box_y;
static std::vector<int> browse_box_values;

inline void save_box(int num_dots, int which)
{
    std::copy(&g_box_x[0], &g_box_x[num_dots], &browse_box_x[num_dots*which]);
    std::copy(&g_box_y[0], &g_box_y[num_dots], &browse_box_y[num_dots*which]);
    std::copy(&g_box_values[0], &g_box_values[num_dots], &browse_box_values[num_dots*which]);
}

inline void restore_box(int num_dots, int which)
{
    std::copy(&browse_box_x[num_dots*which], &browse_box_x[num_dots*(which + 1)], &g_box_x[0]);
    std::copy(&browse_box_y[num_dots*which], &browse_box_y[num_dots*(which + 1)], &g_box_y[0]);
    std::copy(&browse_box_values[num_dots*which], &browse_box_values[num_dots*(which + 1)], &g_box_values[0]);
}

// here because must be visible inside several routines
static affine *cvt;
static bf_t bt_a;
static bf_t bt_b;
static bf_t bt_c;
static bf_t bt_d;
static bf_t bt_e;
static bf_t bt_f;
static bf_t n_a;
static bf_t n_b;
static bf_t n_c;
static bf_t n_d;
static bf_t n_e;
static bf_t n_f;
static bf_math_type oldbf_math;

// fgetwindow reads all .GIF files and draws window outlines on the screen
int fgetwindow()
{
    affine stack_cvt;
    std::time_t thistime;
    std::time_t lastime;
    char mesg[40];
    char newname[60];
    char oldname[60];
    int c;
    int done;
    int wincount;
    int toggle;
    int color_of_box;
    window winlist;
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    char tmpmask[FILE_MAX_PATH];
    int vid_too_big = 0;
    bool no_memory = false;
    int saved;

    oldbf_math = bf_math;
    bf_math = bf_math_type::BIGFLT;
    if (oldbf_math == bf_math_type::NONE)
    {
        calc_status_value oldcalc_status = g_calc_status; // kludge because next sets it = 0
        fractal_floattobf();
        g_calc_status = oldcalc_status;
    }
    saved = save_stack();
    bt_a = alloc_stack(rbflength+2);
    bt_b = alloc_stack(rbflength+2);
    bt_c = alloc_stack(rbflength+2);
    bt_d = alloc_stack(rbflength+2);
    bt_e = alloc_stack(rbflength+2);
    bt_f = alloc_stack(rbflength+2);

    int const num_dots = g_screen_x_dots + g_screen_y_dots;
    browse_windows.resize(MAX_WINDOWS_OPEN);
    browse_box_x.resize(num_dots*MAX_WINDOWS_OPEN);
    browse_box_y.resize(num_dots*MAX_WINDOWS_OPEN);
    browse_box_values.resize(num_dots*MAX_WINDOWS_OPEN);

    // set up complex-plane-to-screen transformation
    if (oldbf_math != bf_math_type::NONE)
    {
        bfsetup_convert_to_screen();
    }
    else
    {
        cvt = &stack_cvt; // use stack
        setup_convert_to_screen(cvt);
        // put in bf variables
        floattobf(bt_a, cvt->a);
        floattobf(bt_b, cvt->b);
        floattobf(bt_c, cvt->c);
        floattobf(bt_d, cvt->d);
        floattobf(bt_e, cvt->e);
        floattobf(bt_f, cvt->f);
    }
    find_special_colors();
    color_of_box = g_color_medium;
rescan:  // entry for changed browse parms
    std::time(&lastime);
    toggle = 0;
    wincount = 0;
    g_browse_sub_images = true;
    split_drive_dir(g_read_filename, drive, dir);
    split_fname_ext(g_browse_mask, fname, ext);
    make_path(tmpmask, drive, dir, fname, ext);
    done = (vid_too_big == 2) || no_memory || fr_findfirst(tmpmask);
    // draw all visible windows
    while (!done)
    {
        if (driver_key_pressed())
        {
            driver_get_key();
            break;
        }
        split_fname_ext(DTA.filename, fname, ext);
        make_path(tmpmask, drive, dir, fname, ext);
        FRACTAL_INFO read_info;
        ext_blk_2 blk_2_info;
        ext_blk_3 blk_3_info;
        ext_blk_4 blk_4_info;
        ext_blk_5 blk_5_info;
        ext_blk_6 blk_6_info;
        ext_blk_7 blk_7_info;
        if (!find_fractal_info(tmpmask, &read_info, &blk_2_info, &blk_3_info,
                &blk_4_info, &blk_5_info, &blk_6_info, &blk_7_info)
            && (typeOK(&read_info, &blk_3_info) || !g_browse_check_fractal_type)
            && (paramsOK(&read_info) || !g_browse_check_fractal_params)
            && stricmp(g_browse_name.c_str(), DTA.filename.c_str()) != 0
            && !blk_6_info.got_data
            && is_visible_window(&winlist, &read_info, &blk_5_info))
        {
            winlist.name = DTA.filename;
            drawindow(color_of_box, &winlist);
            winlist.boxcount = g_box_count;
            browse_windows[wincount] = winlist;
            save_box(num_dots, wincount);
            wincount++;
        }
        done = (fr_findnext() || wincount >= MAX_WINDOWS_OPEN);
    }

    if (no_memory)
    {
        texttempmsg("Sorry...not enough memory to browse.");// doesn't work if NO memory available, go figure
    }
    if (wincount >= MAX_WINDOWS_OPEN)
    {
        // hard code message at MAX_WINDOWS_OPEN = 450
        texttempmsg("Sorry...no more space, 450 displayed.");
    }
    if (vid_too_big == 2)
    {
        texttempmsg("Xdots + Ydots > 4096.");
    }
    c = 0;
    if (wincount)
    {
        driver_buzzer(buzzer_codes::COMPLETE); //let user know we've finished
        int index = 0;
        done = 0;
        winlist = browse_windows[index];
        restore_box(num_dots, index);
        showtempmsg(winlist.name);
        while (!done)  /* on exit done = 1 for quick exit,
                                 done = 2 for erase boxes and  exit
                                 done = 3 for rescan
                                 done = 4 for set boxes and exit to save image */
        {
#ifdef XFRACT
            U32 blinks = 1;
#endif
            while (!driver_key_pressed())
            {
                std::time(&thistime);
                if (static_cast<double>(thistime - lastime) > 0.2)
                {
                    lastime = thistime;
                    toggle = 1- toggle;
                }
                if (toggle)
                {
                    drawindow(g_color_bright, &winlist);   // flash current window
                }
                else
                {
                    drawindow(g_color_dark, &winlist);
                }
#ifdef XFRACT
                blinks++;
#endif
            }
#ifdef XFRACT
            if ((blinks & 1) == 1)     // Need an odd # of blinks, so next one leaves box turned off
            {
                drawindow(g_color_bright, &winlist);
            }
#endif

            c = driver_get_key();
            switch (c)
            {
            case FIK_RIGHT_ARROW:
            case FIK_LEFT_ARROW:
            case FIK_DOWN_ARROW:
            case FIK_UP_ARROW:
                cleartempmsg();
                drawindow(color_of_box, &winlist);// dim last window
                if (c == FIK_RIGHT_ARROW || c == FIK_UP_ARROW)
                {
                    index++;                     // shift attention to next window
                    if (index >= wincount)
                    {
                        index = 0;
                    }
                }
                else
                {
                    index -- ;
                    if (index < 0)
                    {
                        index = wincount -1 ;
                    }
                }
                winlist = browse_windows[index];
                restore_box(num_dots, index);
                showtempmsg(winlist.name);
                break;
            case FIK_CTL_INSERT:
                color_of_box += key_count(FIK_CTL_INSERT);
                for (int i = 0; i < wincount ; i++)
                {
                    drawindow(color_of_box, &browse_windows[i]);
                }
                winlist = browse_windows[index];
                drawindow(color_of_box, &winlist);
                break;

            case FIK_CTL_DEL:
                color_of_box -= key_count(FIK_CTL_DEL);
                for (int i = 0; i < wincount ; i++)
                {
                    drawindow(color_of_box, &browse_windows[i]);
                }
                winlist = browse_windows[index];
                drawindow(color_of_box, &winlist);
                break;
            case FIK_ENTER:
            case FIK_ENTER_2:   // this file please
                g_browse_name = winlist.name;
                done = 1;
                break;

            case FIK_ESC:
            case 'l':
            case 'L':
#ifdef XFRACT
                // Need all boxes turned on, turn last one back on.
                drawindow(g_color_bright, &winlist);
#endif
                g_auto_browse = false;
                done = 2;
                break;

            case 'D': // delete file
                cleartempmsg();
                std::snprintf(mesg, NUM_OF(mesg), "Delete %s? (Y/N)", winlist.name.c_str());
                showtempmsg(mesg);
                driver_wait_key_pressed(0);
                cleartempmsg();
                c = driver_get_key();
                if (c == 'Y' && g_confirm_file_deletes)
                {
                    texttempmsg("ARE YOU SURE???? (Y/N)");
                    if (driver_get_key() != 'Y')
                    {
                        c = 'N';

                    }
                }
                if (c == 'Y')
                {
                    split_drive_dir(g_read_filename, drive, dir);
                    const std::filesystem::path name_path(winlist.name);
                    const std::string fname2 = name_path.stem().string();
                    const std::string ext2 = name_path.extension().string();
                    make_path(tmpmask, drive, dir, fname2.c_str(), ext2.c_str());
                    if (!std::remove(tmpmask))
                    {
                        // do a rescan
                        done = 3;
                        std::strcpy(oldname, winlist.name.c_str());
                        tmpmask[0] = '\0';
                        check_history(oldname, tmpmask);
                        break;
                    }
                    else if (errno == EACCES)
                    {
                        texttempmsg("Sorry...it's a read only file, can't del");
                        showtempmsg(winlist.name);
                        break;
                    }
                }
                {
                    texttempmsg("file not deleted (phew!)");
                }
                showtempmsg(winlist.name);
                break;

            case 'R':
                cleartempmsg();
                driver_stack_screen();
                newname[0] = 0;
                std::strcpy(mesg, "Enter the new filename for ");
                split_drive_dir(g_read_filename, drive, dir);
                {
                    const std::filesystem::path name_path{winlist.name};
                    const std::string           fname2{name_path.stem().string()};
                    const std::string           ext2{name_path.extension().string()};
                    make_path(tmpmask, drive, dir, fname2.c_str(), ext2.c_str());
                }
                std::strcpy(newname, tmpmask);
                std::strcat(mesg, tmpmask);
                {
                    int i = field_prompt(mesg, nullptr, newname, 60, nullptr);
                    driver_unstack_screen();
                    if (i != -1)
                    {
                        if (!std::rename(tmpmask, newname))
                        {
                            if (errno == EACCES)
                            {
                                texttempmsg("Sorry....can't rename");
                            }
                            else
                            {
                                split_fname_ext(newname, fname, ext);
                                make_fname_ext(tmpmask, fname, ext);
                                std::strcpy(oldname, winlist.name.c_str());
                                check_history(oldname, tmpmask);
                                winlist.name = tmpmask;
                            }
                        }
                    }
                    browse_windows[index] = winlist;
                    showtempmsg(winlist.name);
                }
                break;

            case FIK_CTL_B:
                cleartempmsg();
                driver_stack_screen();
                done = std::abs(get_browse_params());
                driver_unstack_screen();
                showtempmsg(winlist.name);
                break;

            case 's': // save image with boxes
                g_auto_browse = false;
                drawindow(color_of_box, &winlist); // current window white
                done = 4;
                break;

            case '\\': //back out to last image
                done = 2;
                break;

            default:
                break;
            } //switch
        } //while

        // now clean up memory (and the screen if necessary)
        cleartempmsg();
        if (done >= 1 && done < 4)
        {
            for (int i = wincount-1; i >= 0; i--)
            {
                winlist = browse_windows[i];
                g_box_count = winlist.boxcount;
                restore_box(num_dots, i);
                if (g_box_count > 0)
                {
                    clearbox();
                }
            }
        }
        if (done == 3)
        {
            goto rescan; // hey everybody I just used the g word!
        }
    }//if
    else
    {
        driver_buzzer(buzzer_codes::INTERRUPT); //no suitable files in directory!
        texttempmsg("Sorry.. I can't find anything");
        g_browse_sub_images = false;
    }

    browse_windows.clear();
    browse_box_x.clear();
    browse_box_y.clear();
    browse_box_values.clear();
    restore_stack(saved);
    if (oldbf_math == bf_math_type::NONE)
    {
        free_bf_vars();
    }
    bf_math = oldbf_math;
    g_float_flag = g_user_float_flag;

    return c;
}


static void drawindow(int colour, window const *info)
{
    coords ibl, itr;

    g_box_color = colour;
    g_box_count = 0;
    if (info->win_size >= g_smallest_box_size_shown)
    {
        // big enough on screen to show up as a box so draw it
        // corner pixels
        addbox(info->itl);
        addbox(info->itr);
        addbox(info->ibl);
        addbox(info->ibr);
        drawlines(info->itl, info->itr, info->ibl.x-info->itl.x, info->ibl.y-info->itl.y); // top & bottom lines
        drawlines(info->itl, info->ibl, info->itr.x-info->itl.x, info->itr.y-info->itl.y); // left & right lines
        dispbox();
    }
    else
    {
        // draw crosshairs
        int cross_size = g_logical_screen_y_dots / 45;
        if (cross_size < 2)
        {
            cross_size = 2;
        }
        itr.x = info->itl.x - cross_size;
        itr.y = info->itl.y;
        ibl.y = info->itl.y - cross_size;
        ibl.x = info->itl.x;
        drawlines(info->itl, itr, ibl.x-itr.x, 0); // top & bottom lines
        drawlines(info->itl, ibl, 0, itr.y-ibl.y); // left & right lines
        dispbox();
    }
}

// maps points onto view screen
static void transform(dblcoords *point)
{
    double tmp_pt_x;
    tmp_pt_x = cvt->a * point->x + cvt->b * point->y + cvt->e;
    point->y = cvt->c * point->x + cvt->d * point->y + cvt->f;
    point->x = tmp_pt_x;
}

static bool is_visible_window(
    window *list,
    FRACTAL_INFO const *info,
    ext_blk_5 const *blk_5_info)
{
    dblcoords tl, tr, bl, br;
    bf_t bt_x, bt_y;
    bf_t bt_xmin, bt_xmax, bt_ymin, bt_ymax, bt_x3rd, bt_y3rd;
    int saved;
    int two_len;
    int cornercount;
    bool cant_see;
    int  orig_bflength,
         orig_bnlength,
         orig_padding,
         orig_rlength,
         orig_shiftfactor,
         orig_rbflength;
    double toobig, tmp_sqrt;
    toobig = std::sqrt(sqr((double)g_screen_x_dots)+sqr((double)g_screen_y_dots)) * 1.5;
    // arbitrary value... stops browser zooming out too far
    cornercount = 0;
    cant_see = false;

    saved = save_stack();
    // Save original values.
    orig_bflength      = bflength;
    orig_bnlength      = bnlength;
    orig_padding       = padding;
    orig_rlength       = rlength;
    orig_shiftfactor   = shiftfactor;
    orig_rbflength     = rbflength;
    /*
       if (oldbf_math && info->bf_math && (bnlength+4 < info->bflength)) {
          bnlength = info->bflength;
          calc_lengths();
       }
    */
    two_len = bflength + 2;
    bt_x = alloc_stack(two_len);
    bt_y = alloc_stack(two_len);
    bt_xmin = alloc_stack(two_len);
    bt_xmax = alloc_stack(two_len);
    bt_ymin = alloc_stack(two_len);
    bt_ymax = alloc_stack(two_len);
    bt_x3rd = alloc_stack(two_len);
    bt_y3rd = alloc_stack(two_len);

    if (info->bf_math)
    {
        bf_t   bt_t1, bt_t2, bt_t3, bt_t4, bt_t5, bt_t6;
        int di_bflength, two_di_len, two_rbf;

        di_bflength = info->bflength + bnstep;
        two_di_len = di_bflength + 2;
        two_rbf = rbflength + 2;

        n_a     = alloc_stack(two_rbf);
        n_b     = alloc_stack(two_rbf);
        n_c     = alloc_stack(two_rbf);
        n_d     = alloc_stack(two_rbf);
        n_e     = alloc_stack(two_rbf);
        n_f     = alloc_stack(two_rbf);

        convert_bf(n_a, bt_a, rbflength, orig_rbflength);
        convert_bf(n_b, bt_b, rbflength, orig_rbflength);
        convert_bf(n_c, bt_c, rbflength, orig_rbflength);
        convert_bf(n_d, bt_d, rbflength, orig_rbflength);
        convert_bf(n_e, bt_e, rbflength, orig_rbflength);
        convert_bf(n_f, bt_f, rbflength, orig_rbflength);

        bt_t1   = alloc_stack(two_di_len);
        bt_t2   = alloc_stack(two_di_len);
        bt_t3   = alloc_stack(two_di_len);
        bt_t4   = alloc_stack(two_di_len);
        bt_t5   = alloc_stack(two_di_len);
        bt_t6   = alloc_stack(two_di_len);

        std::memcpy((char *)bt_t1, &blk_5_info->apm_data[0], (two_di_len));
        std::memcpy((char *)bt_t2, &blk_5_info->apm_data[two_di_len], (two_di_len));
        std::memcpy((char *)bt_t3, &blk_5_info->apm_data[2*two_di_len], (two_di_len));
        std::memcpy((char *)bt_t4, &blk_5_info->apm_data[3*two_di_len], (two_di_len));
        std::memcpy((char *)bt_t5, &blk_5_info->apm_data[4*two_di_len], (two_di_len));
        std::memcpy((char *)bt_t6, &blk_5_info->apm_data[5*two_di_len], (two_di_len));

        convert_bf(bt_xmin, bt_t1, two_len, two_di_len);
        convert_bf(bt_xmax, bt_t2, two_len, two_di_len);
        convert_bf(bt_ymin, bt_t3, two_len, two_di_len);
        convert_bf(bt_ymax, bt_t4, two_len, two_di_len);
        convert_bf(bt_x3rd, bt_t5, two_len, two_di_len);
        convert_bf(bt_y3rd, bt_t6, two_len, two_di_len);
    }

    /* tranform maps real plane co-ords onto the current screen view
      see above */
    if (oldbf_math != bf_math_type::NONE || info->bf_math != 0)
    {
        if (!info->bf_math)
        {
            floattobf(bt_x, info->xmin);
            floattobf(bt_y, info->ymax);
        }
        else
        {
            copy_bf(bt_x, bt_xmin);
            copy_bf(bt_y, bt_ymax);
        }
        bftransform(bt_x, bt_y, &tl);
    }
    else
    {
        tl.x = info->xmin;
        tl.y = info->ymax;
        transform(&tl);
    }
    list->itl.x = (int)(tl.x + 0.5);
    list->itl.y = (int)(tl.y + 0.5);
    if (oldbf_math != bf_math_type::NONE || info->bf_math)
    {
        if (!info->bf_math)
        {
            floattobf(bt_x, (info->xmax)-(info->x3rd-info->xmin));
            floattobf(bt_y, (info->ymax)+(info->ymin-info->y3rd));
        }
        else
        {
            neg_a_bf(sub_bf(bt_x, bt_x3rd, bt_xmin));
            add_a_bf(bt_x, bt_xmax);
            sub_bf(bt_y, bt_ymin, bt_y3rd);
            add_a_bf(bt_y, bt_ymax);
        }
        bftransform(bt_x, bt_y, &tr);
    }
    else
    {
        tr.x = (info->xmax)-(info->x3rd-info->xmin);
        tr.y = (info->ymax)+(info->ymin-info->y3rd);
        transform(&tr);
    }
    list->itr.x = (int)(tr.x + 0.5);
    list->itr.y = (int)(tr.y + 0.5);
    if (oldbf_math != bf_math_type::NONE || info->bf_math)
    {
        if (!info->bf_math)
        {
            floattobf(bt_x, info->x3rd);
            floattobf(bt_y, info->y3rd);
        }
        else
        {
            copy_bf(bt_x, bt_x3rd);
            copy_bf(bt_y, bt_y3rd);
        }
        bftransform(bt_x, bt_y, &bl);
    }
    else
    {
        bl.x = info->x3rd;
        bl.y = info->y3rd;
        transform(&bl);
    }
    list->ibl.x = (int)(bl.x + 0.5);
    list->ibl.y = (int)(bl.y + 0.5);
    if (oldbf_math != bf_math_type::NONE || info->bf_math)
    {
        if (!info->bf_math)
        {
            floattobf(bt_x, info->xmax);
            floattobf(bt_y, info->ymin);
        }
        else
        {
            copy_bf(bt_x, bt_xmax);
            copy_bf(bt_y, bt_ymin);
        }
        bftransform(bt_x, bt_y, &br);
    }
    else
    {
        br.x = info->xmax;
        br.y = info->ymin;
        transform(&br);
    }
    list->ibr.x = (int)(br.x + 0.5);
    list->ibr.y = (int)(br.y + 0.5);

    tmp_sqrt = std::sqrt(sqr(tr.x-bl.x) + sqr(tr.y-bl.y));
    list->win_size = tmp_sqrt; // used for box vs crosshair in drawindow()
    // reject anything too small or too big on screen
    if ((tmp_sqrt < g_smallest_window_display_size) || (tmp_sqrt > toobig))
    {
        cant_see = true;
    }

    // restore original values
    bflength      = orig_bflength;
    bnlength      = orig_bnlength;
    padding       = orig_padding;
    rlength       = orig_rlength;
    shiftfactor   = orig_shiftfactor;
    rbflength     = orig_rbflength;

    restore_stack(saved);
    if (cant_see)   // do it this way so bignum stack is released
    {
        return false;
    }

    // now see how many corners are on the screen, accept if one or more
    if (tl.x >= (0-g_logical_screen_x_offset) && tl.x <= (g_screen_x_dots-g_logical_screen_x_offset)
        && tl.y >= (0-g_logical_screen_y_offset) && tl.y <= (g_screen_y_dots-g_logical_screen_y_offset))
    {
        cornercount++;
    }
    if (bl.x >= (0-g_logical_screen_x_offset) && bl.x <= (g_screen_x_dots-g_logical_screen_x_offset)
        && bl.y >= (0-g_logical_screen_y_offset) && bl.y <= (g_screen_y_dots-g_logical_screen_y_offset))
    {
        cornercount++;
    }
    if (tr.x >= (0-g_logical_screen_x_offset) && tr.x <= (g_screen_x_dots-g_logical_screen_x_offset)
        && tr.y >= (0-g_logical_screen_y_offset) && tr.y <= (g_screen_y_dots-g_logical_screen_y_offset))
    {
        cornercount++;
    }
    if (br.x >= (0-g_logical_screen_x_offset) && br.x <= (g_screen_x_dots-g_logical_screen_x_offset)
        && br.y >= (0-g_logical_screen_y_offset) && br.y <= (g_screen_y_dots-g_logical_screen_y_offset))
    {
        cornercount++;
    }

    return cornercount >= 1;
}

static bool paramsOK(FRACTAL_INFO const *info)
{
    double tmpparm3, tmpparm4;
    double tmpparm5, tmpparm6;
    double tmpparm7, tmpparm8;
    double tmpparm9, tmpparm10;
#define MINDIF 0.001

    if (info->version > 6)
    {
        tmpparm3 = info->dparm3;
        tmpparm4 = info->dparm4;
    }
    else
    {
        tmpparm3 = info->parm3;
        roundfloatd(&tmpparm3);
        tmpparm4 = info->parm4;
        roundfloatd(&tmpparm4);
    }
    if (info->version > 8)
    {
        tmpparm5 = info->dparm5;
        tmpparm6 = info->dparm6;
        tmpparm7 = info->dparm7;
        tmpparm8 = info->dparm8;
        tmpparm9 = info->dparm9;
        tmpparm10 = info->dparm10;
    }
    else
    {
        tmpparm5 = 0.0;
        tmpparm6 = 0.0;
        tmpparm7 = 0.0;
        tmpparm8 = 0.0;
        tmpparm9 = 0.0;
        tmpparm10 = 0.0;
    }
    if (std::fabs(info->creal - g_params[0]) < MINDIF
        && std::fabs(info->cimag - g_params[1]) < MINDIF
        && std::fabs(tmpparm3 - g_params[2]) < MINDIF
        && std::fabs(tmpparm4 - g_params[3]) < MINDIF
        && std::fabs(tmpparm5 - g_params[4]) < MINDIF
        && std::fabs(tmpparm6 - g_params[5]) < MINDIF
        && std::fabs(tmpparm7 - g_params[6]) < MINDIF
        && std::fabs(tmpparm8 - g_params[7]) < MINDIF
        && std::fabs(tmpparm9 - g_params[8]) < MINDIF
        && std::fabs(tmpparm10 - g_params[9]) < MINDIF
        && info->invert[0] - g_inversion[0] < MINDIF)
    {
        return true; // parameters are in range
    }
    else
    {
        return false;
    }
}

static bool functionOK(FRACTAL_INFO const *info, int numfn)
{
    int mzmatch = 0;
    for (int i = 0; i < numfn; i++)
    {
        if (static_cast<trig_fn>(info->trigndx[i]) != g_trig_index[i])
        {
            mzmatch++;
        }
    }
    return mzmatch <= 0; // they all match
}

static bool typeOK(FRACTAL_INFO const *info, ext_blk_3 const *blk_3_info)
{
    int numfn;
    if ((g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
        && (info->fractal_type == static_cast<int>(fractal_type::FORMULA)
            || info->fractal_type == static_cast<int>(fractal_type::FFORMULA)))
    {
        if (!stricmp(blk_3_info->form_name, g_formula_name.c_str()))
        {
            numfn = g_max_function;
            if (numfn > 0)
            {
                return functionOK(info, numfn);
            }
            else
            {
                return true; // match up formula names with no functions
            }
        }
        else
        {
            return false; // two formulas but names don't match
        }
    }
    else if (info->fractal_type == static_cast<int>(g_fractal_type)
        || info->fractal_type == static_cast<int>(g_cur_fractal_specific->tofloat))
    {
        numfn = (g_cur_fractal_specific->flags >> 6) & 7;
        if (numfn > 0)
        {
            return functionOK(info, numfn);
        }
        else
        {
            return true; // match types with no functions
        }
    }
    else
    {
        return false; // no match
    }
}

static void check_history(char const *oldname, char const *newname)
{
    // file_name_stack[] is maintained in framain2.c.  It is the history
    //  file for the browser and holds a maximum of 16 images.  The history
    //  file needs to be adjusted if the rename or delete functions of the
    //  browser are used.
    // name_stack_ptr is also maintained in framain2.c.  It is the index into
    //  file_name_stack[].
    for (int i = 0; i < g_filename_stack_index; i++)
    {
        if (stricmp(g_file_name_stack[i].c_str(), oldname) == 0)   // we have a match
        {
            g_file_name_stack[i] = newname;    // insert the new name
        }
    }
}

static void bfsetup_convert_to_screen()
{
    // setup_convert_to_screen() in LORENZ.C, converted to bf_math
    // Call only from within fgetwindow()
    bf_t   bt_det, bt_xd, bt_yd, bt_tmp1, bt_tmp2;
    bf_t   bt_inter1, bt_inter2;
    int saved;

    saved = save_stack();
    bt_inter1 = alloc_stack(rbflength+2);
    bt_inter2 = alloc_stack(rbflength+2);
    bt_det = alloc_stack(rbflength+2);
    bt_xd  = alloc_stack(rbflength+2);
    bt_yd  = alloc_stack(rbflength+2);
    bt_tmp1 = alloc_stack(rbflength+2);
    bt_tmp2 = alloc_stack(rbflength+2);

    // xx3rd-xxmin
    sub_bf(bt_inter1, g_bf_x_3rd, g_bf_x_min);
    // yymin-yymax
    sub_bf(bt_inter2, g_bf_y_min, g_bf_y_max);
    // (xx3rd-xxmin)*(yymin-yymax)
    mult_bf(bt_tmp1, bt_inter1, bt_inter2);

    // yymax-yy3rd
    sub_bf(bt_inter1, g_bf_y_max, g_bf_y_3rd);
    // xxmax-xxmin
    sub_bf(bt_inter2, g_bf_x_max, g_bf_x_min);
    // (yymax-yy3rd)*(xxmax-xxmin)
    mult_bf(bt_tmp2, bt_inter1, bt_inter2);

    // det = (xx3rd-xxmin)*(yymin-yymax) + (yymax-yy3rd)*(xxmax-xxmin)
    add_bf(bt_det, bt_tmp1, bt_tmp2);

    // xd = x_size_d/det
    floattobf(bt_tmp1, g_logical_screen_x_size_dots);
    div_bf(bt_xd, bt_tmp1, bt_det);

    // a =  xd*(yymax-yy3rd)
    sub_bf(bt_inter1, g_bf_y_max, g_bf_y_3rd);
    mult_bf(bt_a, bt_xd, bt_inter1);

    // b =  xd*(xx3rd-xxmin)
    sub_bf(bt_inter1, g_bf_x_3rd, g_bf_x_min);
    mult_bf(bt_b, bt_xd, bt_inter1);

    // e = -(a*xxmin + b*yymax)
    mult_bf(bt_tmp1, bt_a, g_bf_x_min);
    mult_bf(bt_tmp2, bt_b, g_bf_y_max);
    neg_a_bf(add_bf(bt_e, bt_tmp1, bt_tmp2));

    // xx3rd-xxmax
    sub_bf(bt_inter1, g_bf_x_3rd, g_bf_x_max);
    // yymin-yymax
    sub_bf(bt_inter2, g_bf_y_min, g_bf_y_max);
    // (xx3rd-xxmax)*(yymin-yymax)
    mult_bf(bt_tmp1, bt_inter1, bt_inter2);

    // yymin-yy3rd
    sub_bf(bt_inter1, g_bf_y_min, g_bf_y_3rd);
    // xxmax-xxmin
    sub_bf(bt_inter2, g_bf_x_max, g_bf_x_min);
    // (yymin-yy3rd)*(xxmax-xxmin)
    mult_bf(bt_tmp2, bt_inter1, bt_inter2);

    // det = (xx3rd-xxmax)*(yymin-yymax) + (yymin-yy3rd)*(xxmax-xxmin)
    add_bf(bt_det, bt_tmp1, bt_tmp2);

    // yd = y_size_d/det
    floattobf(bt_tmp2, g_logical_screen_y_size_dots);
    div_bf(bt_yd, bt_tmp2, bt_det);

    // c =  yd*(yymin-yy3rd)
    sub_bf(bt_inter1, g_bf_y_min, g_bf_y_3rd);
    mult_bf(bt_c, bt_yd, bt_inter1);

    // d =  yd*(xx3rd-xxmax)
    sub_bf(bt_inter1, g_bf_x_3rd, g_bf_x_max);
    mult_bf(bt_d, bt_yd, bt_inter1);

    // f = -(c*xxmin + d*yymax)
    mult_bf(bt_tmp1, bt_c, g_bf_x_min);
    mult_bf(bt_tmp2, bt_d, g_bf_y_max);
    neg_a_bf(add_bf(bt_f, bt_tmp1, bt_tmp2));

    restore_stack(saved);
}

// maps points onto view screen
static void bftransform(bf_t bt_x, bf_t bt_y, dblcoords *point)
{
    bf_t   bt_tmp1, bt_tmp2;
    int saved;

    saved = save_stack();
    bt_tmp1 = alloc_stack(rbflength+2);
    bt_tmp2 = alloc_stack(rbflength+2);

    //  point->x = cvt->a * point->x + cvt->b * point->y + cvt->e;
    mult_bf(bt_tmp1, n_a, bt_x);
    mult_bf(bt_tmp2, n_b, bt_y);
    add_a_bf(bt_tmp1, bt_tmp2);
    add_a_bf(bt_tmp1, n_e);
    point->x = (double)bftofloat(bt_tmp1);

    //  point->y = cvt->c * point->x + cvt->d * point->y + cvt->f;
    mult_bf(bt_tmp1, n_c, bt_x);
    mult_bf(bt_tmp2, n_d, bt_y);
    add_a_bf(bt_tmp1, bt_tmp2);
    add_a_bf(bt_tmp1, n_f);
    point->y = (double)bftofloat(bt_tmp1);

    restore_stack(saved);
}
