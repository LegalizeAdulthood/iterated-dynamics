// SPDX-License-Identifier: GPL-3.0-only
//
/*
        load an existing fractal image, control level
*/
#include "io/loadfile.h"

#include "engine/bailout_formula.h"
#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/fractals.h"
#include "engine/ImageRegion.h"
#include "engine/Inversion.h"
#include "engine/log_map.h"
#include "engine/LogicalScreen.h"
#include "engine/orbit.h"
#include "engine/Potential.h"
#include "engine/random_seed.h"
#include "engine/resume.h"
#include "engine/solid_guess.h"
#include "engine/spindac.h"
#include "engine/sticky_orbits.h"
#include "engine/trig_fns.h"
#include "engine/UserData.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "fractals/fractalp.h"
#include "fractals/ifs.h"
#include "fractals/julibrot.h"
#include "fractals/lorenz.h"
#include "fractals/lsystem.h"
#include "fractals/parser.h"
#include "geometry/3d.h"
#include "geometry/line3d.h"
#include "geometry/plot3d.h"
#include "io/decode_info.h"
#include "io/encoder.h"
#include "io/gif_extensions.h"
#include "io/library.h"
#include "math/biginit.h"
#include "math/round_float_double.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "ui/big_while_loop.h"
#include "ui/get_3d_params.h"
#include "ui/get_video_mode.h"
#include "ui/make_batch_file.h"
#include "ui/stop_msg.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#ifdef LOW_BYTE_FIRST
#define GET16(c, i)              (i) = *((U16*)(&(c)))
#else
#define GET16(c, i)              (i) = (*(unsigned char *)&(c))+\
                                ((*((unsigned char*)&(c)+1)) << 8)
#endif

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::math;
using namespace id::misc;
using namespace id::ui;

namespace id::io
{

namespace
{

struct DeprecatedFractalType
{
    enum
    {
        LAMBDA_SINE = 8,    // obsolete
        LAMBDA_COS = 9,     // obsolete
        LAMBDA_EXP = 10,    // obsolete
        MANDEL_SINE = 17,   // obsolete
        MANDEL_COS = 18,    // obsolete
        MANDEL_EXP = 19,    // obsolete
        DEM_M = 30,         // obsolete
        DEM_J = 31,         // obsolete
        MANDEL_SINH = 33,   // obsolete
        LAMBDA_SINH = 34,   // obsolete
        MANDEL_COSH = 35,   // obsolete
        LAMBDA_COSH = 36,   // obsolete
        MANDEL_SINE_L = 37, // obsolete
        LAMBDA_SINE_L = 38, // obsolete
        MANDEL_COS_L = 39,  // obsolete
        LAMBDA_COS_L = 40,  // obsolete
        MANDEL_SINH_L = 41, // obsolete
        LAMBDA_SINH_L = 42, // obsolete
        MANDEL_COSH_L = 43, // obsolete
        LAMBDA_COSH_L = 44, // obsolete
        MANDEL_EXP_L = 49,  // obsolete
        LAMBDA_EXP_L = 50,  // obsolete
    };
};

struct GifExtensionId
{
    enum
    {
        FRACTAL_INFO = 1,
        RESUME_INFO = 2,
        FORMULA_INFO = 3,
        RANGES_INFO = 4,
        EXTENDED_PRECISION = 5,
        EVOLVER_INFO = 6,
        ORBITS_INFO = 7,
    };
};

} // namespace

// prototypes
static void load_ext_blk(char *load_ptr, int load_len);
static void skip_ext_blk(int *block_len, int *data_len);
static void backwards_compat(const FractalInfo &info);
static bool fix_bof();
static bool fix_period_bof();

static std::FILE *s_fp{};

bool g_fast_restore{}; // true - reset viewwindows prior to a restore and
                       // do not display warnings when video mode changes during restore
bool g_loaded_3d{};
bool g_overlay_3d{};   // 3D overlay flag
int g_file_y_dots{};
int g_file_x_dots{};
int g_file_colors{};
float g_file_aspect_ratio{};
short g_skip_x_dots{};
short g_skip_y_dots{}; // for decoder, when reducing image
bool g_bad_outside{};
Version g_file_version{};
std::filesystem::path g_read_filename;
bool g_new_bifurcation_functions_loaded{};
ShowFile g_show_file{};                                   // LOAD_IMAGE if file display pending

static bool within_eps(const float lhs, const float rhs)
{
    return std::abs(lhs - rhs) < 1.0e-6f;
}

static bool within_eps(const double lhs, const double rhs)
{
    return std::abs(lhs - rhs) < 1.0e-6f;
}

template <size_t N, typename T>
static bool equal(const T (&lhs)[N], const T (&rhs)[N])
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

template <size_t N>
static bool equal(const float (&lhs)[N], const float (&rhs)[N])
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs), //
        [](const float lhs, const float rhs) { return within_eps(lhs, rhs); });
}

template <size_t N>
static bool equal(const double (&lhs)[N], const double (&rhs)[N])
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs), //
        [](const double lhs, const double rhs) { return within_eps(lhs, rhs); });
}

bool operator==(const FractalInfo &lhs, const FractalInfo &rhs)
{
    return equal(lhs.info_id, rhs.info_id)                            //
        && lhs.iterations_old == rhs.iterations_old                   //
        && lhs.fractal_type == rhs.fractal_type                       //
        && within_eps(lhs.x_min, rhs.x_min)                           //
        && within_eps(lhs.x_max, rhs.x_max)                           //
        && within_eps(lhs.y_min, rhs.y_min)                           //
        && within_eps(lhs.y_max, rhs.y_max)                           //
        && within_eps(lhs.c_real, rhs.c_real)                         //
        && within_eps(lhs.c_imag, rhs.c_imag)                         //
        && lhs.ax == rhs.ax                                           //
        && lhs.bx == rhs.bx                                           //
        && lhs.cx == rhs.cx                                           //
        && lhs.dx == rhs.dx                                           //
        && lhs.dot_mode == rhs.dot_mode                               //
        && lhs.x_dots == rhs.x_dots                                   //
        && lhs.y_dots == rhs.y_dots                                   //
        && lhs.colors == rhs.colors                                   //
        && lhs.info_version == rhs.info_version                       //
        && within_eps(lhs.param3, rhs.param3)                         //
        && within_eps(lhs.param4, rhs.param4)                         //
        && equal(lhs.potential, rhs.potential)                        //
        && lhs.random_seed == rhs.random_seed                         //
        && lhs.random_seed_flag == rhs.random_seed_flag               //
        && lhs.biomorph == rhs.biomorph                               //
        && lhs.inside == rhs.inside                                   //
        && lhs.log_map_old == rhs.log_map_old                         //
        && equal(lhs.invert, rhs.invert)                              //
        && equal(lhs.decomp, rhs.decomp)                              //
        && lhs.symmetry == rhs.symmetry                               //
        && equal(lhs.init3d, rhs.init3d)                              //
        && lhs.preview_factor == rhs.preview_factor                   //
        && lhs.x_trans == rhs.x_trans                                 //
        && lhs.y_trans == rhs.y_trans                                 //
        && lhs.red_crop_left == rhs.red_crop_left                     //
        && lhs.red_crop_right == rhs.red_crop_right                   //
        && lhs.blue_crop_left == rhs.blue_crop_left                   //
        && lhs.blue_crop_right == rhs.blue_crop_right                 //
        && lhs.red_bright == rhs.red_bright                           //
        && lhs.blue_bright == rhs.blue_bright                         //
        && lhs.x_adjust == rhs.x_adjust                               //
        && lhs.eye_separation == rhs.eye_separation                   //
        && lhs.glasses_type == rhs.glasses_type                       //
        && lhs.outside == rhs.outside                                 //
        && within_eps(lhs.x3rd, rhs.x3rd)                             //
        && within_eps(lhs.y3rd, rhs.y3rd)                             //
        && lhs.std_calc_mode == rhs.std_calc_mode                     //
        && lhs.use_init_orbit == rhs.use_init_orbit                   //
        && lhs.calc_status == rhs.calc_status                         //
        && lhs.tot_extend_len == rhs.tot_extend_len                   //
        && lhs.dist_est_old == rhs.dist_est_old                       //
        && lhs.float_flag == rhs.float_flag                           //
        && lhs.bailout_old == rhs.bailout_old                         //
        && lhs.calc_time == rhs.calc_time                             //
        && equal(lhs.trig_index, rhs.trig_index)                      //
        && lhs.finite_attractor == rhs.finite_attractor               //
        && equal(lhs.init_orbit, rhs.init_orbit)                      //
        && lhs.periodicity == rhs.periodicity                         //
        && lhs.pot16bit == rhs.pot16bit                               //
        && within_eps(lhs.final_aspect_ratio, rhs.final_aspect_ratio) //
        && lhs.system == rhs.system                                   //
        && lhs.release == rhs.release                                 //
        && lhs.display_3d == rhs.display_3d                           //
        && equal(lhs.transparent, rhs.transparent)                    //
        && lhs.ambient == rhs.ambient                                 //
        && lhs.haze == rhs.haze                                       //
        && lhs.randomize == rhs.randomize                             //
        && lhs.rotate_lo == rhs.rotate_lo                             //
        && lhs.rotate_hi == rhs.rotate_hi                             //
        && lhs.dist_est_width == rhs.dist_est_width                   //
        && within_eps(lhs.d_param3, rhs.d_param3)                     //
        && within_eps(lhs.d_param4, rhs.d_param4)                     //
        && lhs.fill_color == rhs.fill_color                           //
        && within_eps(lhs.julibrot_x_max, rhs.julibrot_x_max)         //
        && within_eps(lhs.julibrot_x_min, rhs.julibrot_x_min)         //
        && within_eps(lhs.julibrot_y_max, rhs.julibrot_y_max)         //
        && within_eps(lhs.julibrot_y_min, rhs.julibrot_y_min)         //
        && lhs.julibrot_z_dots == rhs.julibrot_z_dots                 //
        && within_eps(lhs.julibrot_origin_fp, rhs.julibrot_origin_fp) //
        && within_eps(lhs.julibrot_depth_fp, rhs.julibrot_depth_fp)   //
        && within_eps(lhs.julibrot_height_fp, rhs.julibrot_height_fp) //
        && within_eps(lhs.julibrot_width_fp, rhs.julibrot_width_fp)   //
        && within_eps(lhs.julibrot_dist_fp, rhs.julibrot_dist_fp)     //
        && within_eps(lhs.eyes_fp, rhs.eyes_fp)                       //
        && lhs.orbit_type == rhs.orbit_type                           //
        && lhs.juli3d_mode == rhs.juli3d_mode                         //
        && lhs.max_fn == rhs.max_fn                                   //
        && lhs.inverse_julia == rhs.inverse_julia                     //
        && within_eps(lhs.d_param5, rhs.d_param5)                     //
        && within_eps(lhs.d_param6, rhs.d_param6)                     //
        && within_eps(lhs.d_param7, rhs.d_param7)                     //
        && within_eps(lhs.d_param8, rhs.d_param8)                     //
        && within_eps(lhs.d_param9, rhs.d_param9)                     //
        && within_eps(lhs.d_param10, rhs.d_param10)                   //
        && lhs.bailout == rhs.bailout                                 //
        && lhs.bailout_test == rhs.bailout_test                       //
        && lhs.iterations == rhs.iterations                           //
        && lhs.bf_math == rhs.bf_math                                 //
        && lhs.bf_length == rhs.bf_length                             //
        && lhs.y_adjust == rhs.y_adjust                               //
        && lhs.old_demm_colors == rhs.old_demm_colors                 //
        && lhs.log_map == rhs.log_map                                 //
        && lhs.dist_est == rhs.dist_est                               //
        && equal(lhs.d_invert, rhs.d_invert)                          //
        && lhs.log_calc == rhs.log_calc                               //
        && lhs.stop_pass == rhs.stop_pass                             //
        && lhs.quick_calc == rhs.quick_calc                           //
        && within_eps(lhs.close_prox, rhs.close_prox)                 //
        && lhs.no_bof == rhs.no_bof                                   //
        && lhs.orbit_interval == rhs.orbit_interval                   //
        && lhs.orbit_delay == rhs.orbit_delay                         //
        && equal(lhs.math_tol, rhs.math_tol)                          //
        && lhs.version_major == rhs.version_major                     //
        && lhs.version_minor == rhs.version_minor                     //
        && lhs.version_patch == rhs.version_patch                     //
        && lhs.version_tweak == rhs.version_tweak;                    //
}

bool operator==(const FormulaInfo &lhs, const FormulaInfo &rhs)
{
    return equal(lhs.form_name, rhs.form_name) //
        && lhs.uses_p1 == rhs.uses_p1          //
        && lhs.uses_p2 == rhs.uses_p2          //
        && lhs.uses_p3 == rhs.uses_p3          //
        && lhs.uses_ismand == rhs.uses_ismand  //
        && lhs.ismand == rhs.ismand            //
        && lhs.uses_p4 == rhs.uses_p4          //
        && lhs.uses_p5 == rhs.uses_p5;         //
}

bool operator==(const OrbitsInfo &lhs, const OrbitsInfo &rhs)
{
    return lhs.orbit_corner_min_x == rhs.orbit_corner_min_x //
        && lhs.orbit_corner_max_x == rhs.orbit_corner_max_x //
        && lhs.orbit_corner_min_y == rhs.orbit_corner_min_y //
        && lhs.orbit_corner_max_y == rhs.orbit_corner_max_y //
        && lhs.orbit_corner_3rd_x == rhs.orbit_corner_3rd_x //
        && lhs.orbit_corner_3rd_y == rhs.orbit_corner_3rd_y //
        && lhs.keep_screen_coords == rhs.keep_screen_coords //
        && lhs.draw_mode == rhs.draw_mode;                  //
}

static void backwards_info1(const FractalInfo &read_info)
{
    if (read_info.info_version > 0)
    {
        g_params[2] = read_info.param3;
        round_float_double(&g_params[2]);
        g_params[3] = read_info.param4;
        round_float_double(&g_params[3]);
        g_potential.params[0] = read_info.potential[0];
        g_potential.params[1] = read_info.potential[1];
        g_potential.params[2] = read_info.potential[2];
        if (g_make_parameter_file)
        {
            g_colors = read_info.colors;
        }
        g_potential.flag = g_potential.params[0] != 0.0;
        g_random_seed_flag = read_info.random_seed_flag != 0;
        g_random_seed = read_info.random_seed;
        g_inside_color = read_info.inside;
        g_log_map_flag = read_info.log_map_old;
        g_inversion.params[0] = read_info.invert[0];
        g_inversion.params[1] = read_info.invert[1];
        g_inversion.params[2] = read_info.invert[2];
        if (g_inversion.params[0] != 0.0)
        {
            g_inversion.invert = 3;
        }
        g_decomp[0] = read_info.decomp[0];
        g_decomp[1] = read_info.decomp[1];
        g_user.biomorph_value = read_info.biomorph;
        g_force_symmetry = static_cast<SymmetryType>(read_info.symmetry);
    }
}

static void backwards_info2(const FractalInfo &read_info)
{
    if (read_info.info_version > 1)
    {
        g_file_version = Version{12, 0, 0, 0, true};
        if (g_display_3d == Display3DMode::NONE                       //
            && (read_info.info_version <= 4                           //
                   || read_info.display_3d > 0                        //
                   || bit_set(g_cur_fractal_specific->flags, FractalFlags::PARAMS_3D)))
        {
            g_sphere = read_info.init3d[0] != 0;                      // sphere? 1 = yes, 0 = no
            g_x_rot = read_info.init3d[1];                            // rotate x-axis 60 degrees
            g_y_rot = read_info.init3d[2];                            // rotate y-axis 90 degrees
            g_z_rot = read_info.init3d[3];                            // rotate x-axis  0 degrees
            g_x_scale = read_info.init3d[4];                          // scale x-axis, 90 percent
            g_y_scale = read_info.init3d[5];                          // scale y-axis, 90 percent
            g_sphere_phi_min = read_info.init3d[1];                   // longitude start, 180
            g_sphere_phi_max = read_info.init3d[2];                   // longitude end ,   0
            g_sphere_theta_min = read_info.init3d[3];                 // latitude start,-90 degrees
            g_sphere_theta_max = read_info.init3d[4];                 // latitude stop,  90 degrees
            g_sphere_radius = read_info.init3d[5];                    // should be user input
            g_rough = read_info.init3d[6];                            // scale z-axis, 30 percent
            g_water_line = read_info.init3d[7];                       // water level
            g_fill_type = static_cast<FillType>(read_info.init3d[8]); // fill type
            g_viewer_z = read_info.init3d[9];                         // perspective view point
            g_shift_x = read_info.init3d[10];                         // x shift
            g_shift_y = read_info.init3d[11];                         // y shift
            g_light_x = read_info.init3d[12];                         // x light vector coordinate
            g_light_y = read_info.init3d[13];                         // y light vector coordinate
            g_light_z = read_info.init3d[14];                         // z light vector coordinate
            g_light_avg = read_info.init3d[15];                       // number of points to average
            g_preview_factor = read_info.preview_factor;
            g_adjust_3d.x = read_info.x_trans;
            g_adjust_3d.y = read_info.y_trans;
            g_red_crop_left = read_info.red_crop_left;
            g_red_crop_right = read_info.red_crop_right;
            g_blue_crop_left = read_info.blue_crop_left;
            g_blue_crop_right = read_info.blue_crop_right;
            g_red_bright = read_info.red_bright;
            g_blue_bright = read_info.blue_bright;
            g_converge_x_adjust = read_info.x_adjust;
            g_eye_separation = read_info.eye_separation;
            g_glasses_type = static_cast<GlassesType>(read_info.glasses_type);
        }
    }
}

static void backwards_info3(const FractalInfo &read_info)
{
    if (read_info.info_version > 2)
    {
        g_file_version = Version{13, 0, 0, 0, true};
        g_outside_color = read_info.outside;
    }

    g_calc_status = CalcStatus::PARAMS_CHANGED; // defaults if version < 4
    g_image_region.m_3rd = g_image_region.m_min;
    g_user.distance_estimator_value = 0;
    g_calc_time = 0;
}

static void backwards_info4(const FractalInfo &read_info)
{
    if (read_info.info_version > 3)
    {
        g_file_version = Version{14, 0, 0, 0, true};
        g_image_region.m_3rd.x = read_info.x3rd;
        g_image_region.m_3rd.y = read_info.y3rd;
        g_calc_status = static_cast<CalcStatus>(read_info.calc_status);
        g_user.std_calc_mode = static_cast<CalcMode>(read_info.std_calc_mode);
        g_three_pass = false;
        if (g_user.std_calc_mode == static_cast<CalcMode>(127))
        {
            g_three_pass = true;
            g_user.std_calc_mode = CalcMode::THREE_PASS;
        }
        g_user.distance_estimator_value = read_info.dist_est_old;
        g_user.bailout_value = read_info.bailout_old;
        g_calc_time = read_info.calc_time;
        g_trig_index[0] = static_cast<TrigFn>(read_info.trig_index[0]);
        g_trig_index[1] = static_cast<TrigFn>(read_info.trig_index[1]);
        g_trig_index[2] = static_cast<TrigFn>(read_info.trig_index[2]);
        g_trig_index[3] = static_cast<TrigFn>(read_info.trig_index[3]);
        g_attractor.enabled = read_info.finite_attractor != 0;
        g_init_orbit.x = read_info.init_orbit[0];
        g_init_orbit.y = read_info.init_orbit[1];
        g_use_init_orbit = static_cast<InitOrbitMode>(read_info.use_init_orbit);
        g_user.periodicity_value = read_info.periodicity;
    }

    g_potential.store_16bit = false;
}

static void backwards_info5(const FractalInfo &read_info)
{
    if (read_info.info_version > 4)
    {
        g_potential.store_16bit = read_info.pot16bit != 0;
        if (g_potential.store_16bit)
        {
            g_file_x_dots >>= 1;
        }
        g_file_aspect_ratio = read_info.final_aspect_ratio;
        if (g_file_aspect_ratio < 0.01) // fix files produced in early v14.1
        {
            g_file_aspect_ratio = g_screen_aspect;
        }
        g_file_version = parse_legacy_version(read_info.release);
        if (read_info.info_version == 5 /* except a few early fmt 5 cases: */
            && (read_info.release <= 0 || read_info.release >= 4000))
        {
            g_file_version = parse_legacy_version(1410);
        }
        if (g_display_3d == Display3DMode::NONE && read_info.display_3d > 0)
        {
            g_loaded_3d = true;
            g_ambient = read_info.ambient;
            g_randomize_3d = read_info.randomize;
            g_haze = read_info.haze;
            g_transparent_color_3d[0] = read_info.transparent[0];
            g_transparent_color_3d[1] = read_info.transparent[1];
        }
    }

    g_color_cycle_range_lo = 1;
    g_color_cycle_range_hi = 255;
    g_distance_estimator_width_factor = 71;
}

static void backwards_info6(const FractalInfo &read_info)
{
    if (read_info.info_version > 5)
    {
        g_color_cycle_range_lo = read_info.rotate_lo;
        g_color_cycle_range_hi = read_info.rotate_hi;
        g_distance_estimator_width_factor = read_info.dist_est_width;
    }
}

static void backwards_info7(const FractalInfo &read_info)
{
    if (read_info.info_version > 6)
    {
        g_params[2] = read_info.d_param3;
        g_params[3] = read_info.d_param4;
    }
}

static void backwards_info8(const FractalInfo &read_info)
{
    if (read_info.info_version > 7)
    {
        g_fill_color = read_info.fill_color;
    }
}

static void backwards_info9(const FractalInfo &read_info)
{
    if (read_info.info_version > 8)
    {
        g_julibrot_x_max = read_info.julibrot_x_max;
        g_julibrot_x_min = read_info.julibrot_x_min;
        g_julibrot_y_max = read_info.julibrot_y_max;
        g_julibrot_y_min = read_info.julibrot_y_min;
        g_julibrot_z_dots = read_info.julibrot_z_dots;
        g_julibrot_origin = read_info.julibrot_origin_fp;
        g_julibrot_depth = read_info.julibrot_depth_fp;
        g_julibrot_height = read_info.julibrot_height_fp;
        g_julibrot_width = read_info.julibrot_width_fp;
        g_julibrot_dist = read_info.julibrot_dist_fp;
        g_eyes = read_info.eyes_fp;
        g_new_orbit_type = static_cast<FractalType>(read_info.orbit_type);
        g_julibrot_3d_mode = static_cast<Julibrot3DMode>(read_info.juli3d_mode);
        g_max_function = static_cast<char>(read_info.max_fn);
        g_major_method = static_cast<Major>(read_info.inverse_julia >> 8);
        g_inverse_julia_minor_method = static_cast<Minor>(read_info.inverse_julia & 255);
        g_params[4] = read_info.d_param5;
        g_params[5] = read_info.d_param6;
        g_params[6] = read_info.d_param7;
        g_params[7] = read_info.d_param8;
        g_params[8] = read_info.d_param9;
        g_params[9] = read_info.d_param10;
    }
}

static void backwards_info_pre4(const FractalInfo &read_info)
{
    if (read_info.info_version < 4 && read_info.info_version != 0) // pre-version 14.0?
    {
        backwards_compat(read_info);                              // translate obsolete types
        if (g_log_map_flag)
        {
            g_log_map_flag = 2;
        }
    }
}

static void backwards_info_pre5(const FractalInfo &read_info)
{
    if (read_info.info_version < 5 && read_info.info_version != 0) // pre-version 15.0?
    {
        if (g_log_map_flag == 2)                                   // logmap=old changed again in format 5!
        {
            g_log_map_flag = -1;
        }
        if (g_decomp[0] > 0 && g_decomp[1] > 0)
        {
            g_user.bailout_value = g_decomp[1];
        }
    }
}

static void backwards_info_pre8(const FractalInfo &read_info)
{
    if (g_potential.flag) // in version 15.x and 16.x logmap didn't work with pot
    {
        if (read_info.info_version == 6 || read_info.info_version == 7)
        {
            g_log_map_flag = 0;
        }
    }
    set_trig_pointers(-1);
}

static void backwards_info_pre9(const FractalInfo &read_info)
{
    if (read_info.info_version < 9 && read_info.info_version != 0) // pre-version 18.0?
    {
        /* forcesymmetry==1000 means we want to force symmetry but don't
            know which symmetry yet, will find out in setsymmetry() */
        if (g_outside_color == REAL || g_outside_color == IMAG || g_outside_color == MULT ||
            g_outside_color == SUM || g_outside_color == ATAN)
        {
            if (g_force_symmetry == SymmetryType::NOT_FORCED)
            {
                g_force_symmetry = static_cast<SymmetryType>(1000);
            }
        }
    }
}

static void backwards_legacy_v17_25(const FractalInfo &read_info)
{
    // pre-version 17.25
    if (g_file_version < 1725 && read_info.info_version != 0)
    {
        set_if_old_bif(); /* translate bifurcation types */
        g_new_bifurcation_functions_loaded = true;
    }
}

static void backwards_info10(FractalInfo read_info)
{
    if (read_info.info_version > 9)
    {
        // post-version 18.22
        g_user.bailout_value = read_info.bailout; // use long bailout
        g_bailout_test = static_cast<Bailout>(read_info.bailout_test);
    }
    else
    {
        g_bailout_test = Bailout::MOD;
    }
    set_bailout_formula(g_bailout_test);

    if (read_info.info_version > 9)
    {
        // post-version 18.23
        g_max_iterations = read_info.iterations; // use long maxit
        // post-version 18.27
        g_old_demm_colors = read_info.old_demm_colors != 0;
    }
}

static void backwards_info11(const FractalInfo &read_info)
{
    if (read_info.info_version > 10) // post-version 19.20
    {
        g_log_map_flag = read_info.log_map;
        g_user.distance_estimator_value = read_info.dist_est;
    }
}

static void backwards_info12(FractalInfo read_info)
{
    if (read_info.info_version > 11) // post-version 19.20, inversion fix
    {
        g_inversion.params[0] = read_info.d_invert[0];
        g_inversion.params[1] = read_info.d_invert[1];
        g_inversion.params[2] = read_info.d_invert[2];
        g_log_map_fly_calculate = static_cast<LogMapCalculate>(read_info.log_calc);
        g_stop_pass = read_info.stop_pass;
    }
}

static void backwards_info13(const FractalInfo &read_info)
{
    if (read_info.info_version > 12) // post-version 19.60
    {
        g_quick_calc = read_info.quick_calc != 0;
        g_close_proximity = read_info.close_prox;
        if (g_fractal_type == FractalType::POPCORN ||
            g_fractal_type == FractalType::POPCORN_JUL ||
            g_fractal_type == FractalType::LATOO)
        {
            g_new_bifurcation_functions_loaded = true;
        }
    }
}

static void backwards_info14(const FractalInfo &read_info)
{
    g_bof_match_book_images = true;
    if (read_info.info_version > 13) // post-version 20.1.2
    {
        g_bof_match_book_images = read_info.no_bof == 0;
    }
}

static void backwards_info15()
{
    // if (read_info.version > 14)  post-version 20.1.12
    g_log_map_auto_calculate = false; // make sure it's turned off
}

static void backwards_info16(const FractalInfo &read_info)
{
    g_orbit_interval = 1;
    if (read_info.info_version > 15) // post-version 20.3.2
    {
        g_orbit_interval = read_info.orbit_interval;
    }
}

static void backwards_info17(const FractalInfo &read_info)
{
    g_orbit_delay = 0;
    g_math_tol[0] = 0.05;
    g_math_tol[1] = 0.05;
    if (read_info.info_version > 16) // post-version 20.4.0
    {
        g_orbit_delay = read_info.orbit_delay;
        // read_info.math_tol[0] value for integer -> float transition is ignored.
        g_math_tol[1] = read_info.math_tol[1];
    }
}

static void backwards_id1_1(const FractalInfo &read_info)
{
    // Id 1.0 and 1.1 used a legacy info version but wrote new version into release field
    if (read_info.info_version == FRACTAL_INFO_VERSION_LEGACY_20_4)
    {
        if (read_info.release == 100 || read_info.release == 101)
        {
            g_file_version = Version{read_info.release / 100, read_info.release % 100, 0, 0, false};
        }
    }
}

static void backwards_id1_2(const FractalInfo &read_info)
{
    // Id 1.2
    if (read_info.info_version > FRACTAL_INFO_VERSION_LEGACY_20_4)
    {
        g_file_version = Version{read_info.version_major, read_info.version_minor, //
            read_info.version_patch, read_info.version_tweak, false};
    }
}

namespace
{

// Integer fractal types that could be encountered from loaded GIF files
enum class DeprecatedIntegerType
{
    MANDEL                      = 0,
    JULIA                       = 1,
    LAMBDA                      = 3,
    MAN_O_WAR                   = 10,
    SIERPINSKI                  = 12,
    BARNSLEY_M1                 = 13,
    BARNSLEY_J1                 = 14,
    BARNSLEY_M2                 = 15,
    BARNSLEY_J2                 = 16,
    SQR_TRIG                    = 17,
    TRIG_PLUS_TRIG              = 19,
    MANDEL_LAMBDA               = 20,
    MARKS_MANDEL                = 21,
    MARKS_JULIA                 = 22,
    UNITY                       = 23,
    MANDEL4                     = 24,
    JULIA4                      = 25,
    BARNSLEY_M3                 = 28,
    BARNSLEY_J3                 = 29,
    TRIG_SQR                    = 30,
    TRIG_X_TRIG                 = 34,
    SQR_1_OVER_TRIG             = 36,
    Z_X_TRIG_PLUS_Z             = 38,
    KAM                         = 41,
    KAM_3D                      = 43,
    LAMBDA_TRIG                 = 44,
    MAN_TRIG_PLUS_Z_SQRD_L      = 45,
    JUL_TRIG_PLUS_Z_SQRD_L      = 46,
    MANDEL_TRIG                 = 50,
    MANDEL_Z_POWER_L            = 51,
    JULIA_Z_POWER_L             = 52,
    MAN_TRIG_PLUS_EXP_L         = 57,
    JUL_TRIG_PLUS_EXP_L         = 58,
    POPCORN_L                   = 62,
    LORENZ_L                    = 64,
    LORENZ_3D_L                 = 65,
    FORMULA                     = 72,
    JULIBROT                    = 83,
    ROSSLER_L                   = 85,
    HENON_L                     = 87,
    SPIDER                      = 94,
    BIFURCATION_L               = 100,
    BIF_LAMBDA_L                = 101,
    POPCORN_JUL_L               = 106,
    MAN_O_WAR_J                 = 109,
    FN_PLUS_FN_PIX_LONG         = 111,
    MARKS_MANDEL_PWR            = 113,
    TIMS_ERROR                  = 115,
    BIF_EQ_SIN_PI_L             = 116,
    BIF_PLUS_SIN_PI_L           = 117,
    BIF_STEWART_L               = 119,
    LAMBDA_FN_FN_L              = 127,
    JUL_FN_FN_L                 = 129,
    MAN_LAM_FN_FN_L             = 131,
    MAN_FN_FN_L                 = 133,
    BIF_MAY_L                   = 135,
    HALLEY_MP                   = 137,
    INVERSE_JULIA               = 144,
    PHOENIX                     = 147,
    MAND_PHOENIX                = 149,
    FROTH                       = 153,
    PHOENIX_CPLX                = 161,
    MAND_PHOENIX_CPLX           = 163,
};

int operator+(DeprecatedIntegerType value)
{
    return static_cast<int>(value);
}

struct MigrateReadType
{
    DeprecatedIntegerType deprecated;
    FractalType migrated;
};

} // namespace

static constexpr MigrateReadType MIGRATED_TYPES[]{
    {DeprecatedIntegerType::MANDEL, FractalType::MANDEL},                                //
    {DeprecatedIntegerType::JULIA, FractalType::JULIA},                                  //
    {DeprecatedIntegerType::LAMBDA, FractalType::LAMBDA},                                //
    {DeprecatedIntegerType::MAN_O_WAR, FractalType::MAN_O_WAR},                          //
    {DeprecatedIntegerType::SIERPINSKI, FractalType::SIERPINSKI},                        //
    {DeprecatedIntegerType::BARNSLEY_M1, FractalType::BARNSLEY_M1},                      //
    {DeprecatedIntegerType::BARNSLEY_J1, FractalType::BARNSLEY_J1},                      //
    {DeprecatedIntegerType::BARNSLEY_M2, FractalType::BARNSLEY_M2},                      //
    {DeprecatedIntegerType::BARNSLEY_J2, FractalType::BARNSLEY_J2},                      //
    {DeprecatedIntegerType::SQR_TRIG, FractalType::SQR_FN},                              //
    {DeprecatedIntegerType::TRIG_PLUS_TRIG, FractalType::FN_PLUS_FN},                    //
    {DeprecatedIntegerType::MANDEL_LAMBDA, FractalType::MANDEL_LAMBDA},                  //
    {DeprecatedIntegerType::MARKS_MANDEL, FractalType::MARKS_MANDEL},                    //
    {DeprecatedIntegerType::MARKS_JULIA, FractalType::MARKS_JULIA},                      //
    {DeprecatedIntegerType::UNITY, FractalType::UNITY},                                  //
    {DeprecatedIntegerType::MANDEL4, FractalType::MANDEL4},                              //
    {DeprecatedIntegerType::JULIA4, FractalType::JULIA4},                                //
    {DeprecatedIntegerType::BARNSLEY_M3, FractalType::BARNSLEY_M3},                      //
    {DeprecatedIntegerType::BARNSLEY_J3, FractalType::BARNSLEY_J3},                      //
    {DeprecatedIntegerType::TRIG_SQR, FractalType::FN_Z_SQR},                            //
    {DeprecatedIntegerType::TRIG_X_TRIG, FractalType::FN_TIMES_FN},                      //
    {DeprecatedIntegerType::SQR_1_OVER_TRIG, FractalType::SQR_1_OVER_FN},                //
    {DeprecatedIntegerType::Z_X_TRIG_PLUS_Z, FractalType::FN_MUL_Z_PLUS_Z},              //
    {DeprecatedIntegerType::KAM, FractalType::KAM},                                      //
    {DeprecatedIntegerType::KAM_3D, FractalType::KAM_3D},                                //
    {DeprecatedIntegerType::LAMBDA_TRIG, FractalType::LAMBDA_FN},                        //
    {DeprecatedIntegerType::MAN_TRIG_PLUS_Z_SQRD_L, FractalType::MANDEL_FN_PLUS_Z_SQRD}, //
    {DeprecatedIntegerType::JUL_TRIG_PLUS_Z_SQRD_L, FractalType::JULIA_FN_PLUS_Z_SQRD},  //
    {DeprecatedIntegerType::MANDEL_TRIG, FractalType::MANDEL_FN},                        //
    {DeprecatedIntegerType::MANDEL_Z_POWER_L, FractalType::MANDEL_Z_POWER},              //
    {DeprecatedIntegerType::JULIA_Z_POWER_L, FractalType::JULIA_Z_POWER},                //
    {DeprecatedIntegerType::MAN_TRIG_PLUS_EXP_L, FractalType::MANDEL_FN_PLUS_EXP},       //
    {DeprecatedIntegerType::JUL_TRIG_PLUS_EXP_L, FractalType::JULIA_FN_PLUS_EXP},        //
    {DeprecatedIntegerType::POPCORN_L, FractalType::POPCORN},                            //
    {DeprecatedIntegerType::LORENZ_L, FractalType::LORENZ},                              //
    {DeprecatedIntegerType::LORENZ_3D_L, FractalType::LORENZ_3D},                        //
    {DeprecatedIntegerType::FORMULA, FractalType::FORMULA},                              //
    {DeprecatedIntegerType::JULIBROT, FractalType::JULIBROT},                            //
    {DeprecatedIntegerType::ROSSLER_L, FractalType::ROSSLER},                            //
    {DeprecatedIntegerType::HENON_L, FractalType::HENON},                                //
    {DeprecatedIntegerType::SPIDER, FractalType::SPIDER},                                //
    {DeprecatedIntegerType::BIFURCATION_L, FractalType::BIFURCATION},                    //
    {DeprecatedIntegerType::BIF_LAMBDA_L, FractalType::BIF_LAMBDA},                      //
    {DeprecatedIntegerType::POPCORN_JUL_L, FractalType::POPCORN_JUL},                    //
    {DeprecatedIntegerType::MAN_O_WAR_J, FractalType::MAN_O_WAR_J},                      //
    {DeprecatedIntegerType::FN_PLUS_FN_PIX_LONG, FractalType::FN_PLUS_FN_PIX},           //
    {DeprecatedIntegerType::MARKS_MANDEL_PWR, FractalType::MARKS_MANDEL_PWR},            //
    {DeprecatedIntegerType::TIMS_ERROR, FractalType::TIMS_ERROR},                        //
    {DeprecatedIntegerType::BIF_EQ_SIN_PI_L, FractalType::BIF_EQ_SIN_PI},                //
    {DeprecatedIntegerType::BIF_PLUS_SIN_PI_L, FractalType::BIF_PLUS_SIN_PI},            //
    {DeprecatedIntegerType::BIF_STEWART_L, FractalType::BIF_STEWART},                    //
    {DeprecatedIntegerType::LAMBDA_FN_FN_L, FractalType::LAMBDA_FN_FN},                  //
    {DeprecatedIntegerType::JUL_FN_FN_L, FractalType::JUL_FN_FN},                        //
    {DeprecatedIntegerType::MAN_LAM_FN_FN_L, FractalType::MAN_LAM_FN_FN},                //
    {DeprecatedIntegerType::MAN_FN_FN_L, FractalType::MAN_FN_FN},                        //
    {DeprecatedIntegerType::BIF_MAY_L, FractalType::BIF_MAY},                            //
    {DeprecatedIntegerType::HALLEY_MP, FractalType::HALLEY},                             //
    {DeprecatedIntegerType::INVERSE_JULIA, FractalType::INVERSE_JULIA},                  //
    {DeprecatedIntegerType::PHOENIX, FractalType::PHOENIX},                              //
    {DeprecatedIntegerType::MAND_PHOENIX, FractalType::MAND_PHOENIX},                    //
    {DeprecatedIntegerType::FROTH, FractalType::FROTHY_BASIN},                           //
    {DeprecatedIntegerType::PHOENIX_CPLX, FractalType::PHOENIX_CPLX},                    //
    {DeprecatedIntegerType::MAND_PHOENIX_CPLX, FractalType::MAND_PHOENIX_CPLX}           //
};

FractalType migrate_integer_types(int read_type)
{
    if (const auto it = std::lower_bound(std::begin(MIGRATED_TYPES), std::end(MIGRATED_TYPES), read_type,
            [](const MigrateReadType &migrate, const int read_type)
            { return +migrate.deprecated < read_type; });
        it != std::end(MIGRATED_TYPES) && +it->deprecated == read_type)
    {
        return it->migrated;
    }
    return static_cast<FractalType>(read_type);
}

int read_overlay()      // read overlay/3D files, if required
{
    FractalInfo read_info;
    ExtBlock2 blk_2_info;
    ExtBlock3 blk_3_info;
    ExtBlock4 blk_4_info;
    ExtBlock5 blk_5_info;
    ExtBlock6 blk_6_info;
    ExtBlock7 blk_7_info;

    g_show_file = ShowFile::IMAGE_LOADED; // for any abort exit, pretend done
    g_init_mode = -1;                     // no viewing mode set yet
    g_loaded_3d = false;
    if (g_fast_restore)
    {
        g_viewport.enabled = false;
    }
    if (!g_read_filename.has_extension())
    {
        g_read_filename.replace_extension(".gif");
    }

    if (find_fractal_info(find_file(ReadFile::IMAGE, g_read_filename).string(), //
            &read_info, &blk_2_info, &blk_3_info, &blk_4_info, &blk_5_info, &blk_6_info, &blk_7_info))
    {
        // didn't find a usable file
        stop_msg(fmt::format("Sorry, {:s} isn't a file I can decode.", g_read_filename.string()));
        return -1;
    }

    g_max_iterations = read_info.iterations_old;
    const int read_fractal_type = read_info.fractal_type;
    if (read_fractal_type < 0 || read_fractal_type >= +FractalType::MAX)
    {
        set_fractal_type(FractalType::MANDEL);
        stop_msg(fmt::format("Warning: {:s} has a bad fractal type {:d}; using mandel",
            g_read_filename.string(), read_fractal_type));
        return -1;
    }
    set_fractal_type(migrate_integer_types(read_fractal_type));
    g_image_region.m_min.x = read_info.x_min;
    g_image_region.m_max.x = read_info.x_max;
    g_image_region.m_min.y = read_info.y_min;
    g_image_region.m_max.y = read_info.y_max;
    g_params[0] = read_info.c_real;
    g_params[1] = read_info.c_imag;
    g_file_version = Version{11, 0, 0, 0, true};

    g_inversion.invert = 0;
    backwards_info1(read_info);
    backwards_info2(read_info);
    backwards_info3(read_info);
    backwards_info4(read_info);
    backwards_info5(read_info);
    backwards_info6(read_info);
    backwards_info7(read_info);
    backwards_info8(read_info);
    backwards_info9(read_info);
    backwards_info_pre4(read_info);
    backwards_info_pre5(read_info);
    backwards_info_pre8(read_info);
    backwards_info_pre9(read_info);
    backwards_legacy_v17_25(read_info);
    backwards_info10(read_info);
    backwards_info11(read_info);
    backwards_info12(read_info);
    backwards_info13(read_info);
    backwards_info14(read_info);
    backwards_info15();
    backwards_info16(read_info);
    backwards_info17(read_info);
    backwards_id1_1(read_info);
    backwards_id1_2(read_info);
    backwards_legacy_v18();
    backwards_legacy_v19();
    backwards_legacy_v20();

    if (g_overlay_3d)
    {
        g_init_mode = g_adapter;          // use previous adapter mode for overlays
        if (g_file_x_dots > g_logical_screen.x_dots || g_file_y_dots > g_logical_screen.y_dots)
        {
            stop_msg("Can't overlay with a larger image");
            g_init_mode = -1;
            return -1;
        }
    }
    else
    {
        const Display3DMode old_display_ed = g_display_3d;
        g_display_3d = g_loaded_3d ? Display3DMode::YES : Display3DMode::NONE;   // for <tab> display during next
        const int i = get_video_mode(&read_info, &blk_3_info);
        driver_check_memory();
        g_display_3d = old_display_ed;
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

    if (g_display_3d != Display3DMode::NONE)
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        set_fractal_type(FractalType::PLASMA);
        g_params[0] = 0;
        if (g_init_batch == BatchMode::NONE)
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
        if (const FractalType read_type{migrate_integer_types(read_info.fractal_type)};
            read_type == FractalType::L_SYSTEM)
        {
            g_l_system_name = blk_3_info.form_name;
        }
        else if (read_type == FractalType::IFS || read_type == FractalType::IFS_3D)
        {
            g_ifs_name = blk_3_info.form_name;
        }
        else
        {
            g_formula_name = blk_3_info.form_name;
            g_frm_uses_p1 = blk_3_info.uses_p1 != 0;
            g_frm_uses_p2 = blk_3_info.uses_p2 != 0;
            g_frm_uses_p3 = blk_3_info.uses_p3 != 0;
            g_frm_uses_ismand = blk_3_info.uses_ismand != 0;
            g_is_mandelbrot = blk_3_info.ismand != 0;
            g_frm_uses_p4 = blk_3_info.uses_p4 != 0;
            g_frm_uses_p5 = blk_3_info.uses_p5 != 0;
        }
        // perhaps in future add more here, check block_len for backward compatibility
    }

    g_iteration_ranges.clear(); // free prior ranges
    if (blk_4_info.got_data)
    {
        g_iteration_ranges = blk_4_info.range_data;
    }

    if (blk_5_info.got_data)
    {
        g_bf_math = BFMathType::BIG_NUM;
        init_bf_length(read_info.bf_length);
        std::memcpy(g_bf_x_min, blk_5_info.apm_data.data(), blk_5_info.apm_data.size());
    }
    else
    {
        g_bf_math = BFMathType::NONE;
    }

    if (blk_6_info.got_data)
    {
        GeneBase gene[NUM_GENES];
        copy_genes_from_bank(gene);
        if (read_info.info_version < 15)
        {
            // Increasing NUM_GENES moves ecount in the data structure
            // We added 4 to NUM_GENES, so ecount is at NUM_GENES-4
            blk_6_info.e_count = blk_6_info.mutate[NUM_GENES - 4];
        }
        if (blk_6_info.e_count != blk_6_info.image_grid_size *blk_6_info.image_grid_size
            && g_calc_status != CalcStatus::COMPLETED)
        {
            g_calc_status = CalcStatus::RESUMABLE;
            g_evolve_info.x_parameter_range = blk_6_info.x_parameter_range;
            g_evolve_info.y_parameter_range = blk_6_info.y_parameter_range;
            g_evolve_info.x_parameter_offset = blk_6_info.x_parameter_offset;
            g_evolve_info.y_parameter_offset = blk_6_info.y_parameter_offset;
            g_evolve_info.discrete_x_parameter_offset = blk_6_info.discrete_x_parameter_offset;
            g_evolve_info.discrete_y_parameter_offset = blk_6_info.discrete_y_parameter_offset;
            g_evolve_info.px           = blk_6_info.px;
            g_evolve_info.py           = blk_6_info.py;
            g_evolve_info.screen_x_offset       = blk_6_info.sx_offs;
            g_evolve_info.screen_y_offset       = blk_6_info.sy_offs;
            g_evolve_info.x_dots        = blk_6_info.x_dots;
            g_evolve_info.y_dots        = blk_6_info.y_dots;
            g_evolve_info.image_grid_size = blk_6_info.image_grid_size;
            g_evolve_info.evolving     = blk_6_info.evolving;
            g_evolve_info.this_generation_random_seed = blk_6_info.this_generation_random_seed;
            g_evolve_info.max_random_mutation = blk_6_info.max_random_mutation;
            g_evolve_info.count       = blk_6_info.e_count;
            g_have_evolve_info = true;
        }
        else
        {
            g_have_evolve_info = false;
            g_calc_status = CalcStatus::COMPLETED;
        }
        g_evolve_x_parameter_range = blk_6_info.x_parameter_range;
        g_evolve_y_parameter_range = blk_6_info.y_parameter_range;
        g_evolve_new_x_parameter_offset = blk_6_info.x_parameter_offset;
        g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
        g_evolve_new_y_parameter_offset = blk_6_info.y_parameter_offset;
        g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
        g_evolve_new_discrete_x_parameter_offset = static_cast<char>(blk_6_info.discrete_x_parameter_offset);
        g_evolve_discrete_x_parameter_offset = g_evolve_new_discrete_x_parameter_offset;
        g_evolve_new_discrete_y_parameter_offset = static_cast<char>(blk_6_info.discrete_y_parameter_offset);
        g_evolve_discrete_y_parameter_offset = g_evolve_new_discrete_y_parameter_offset;
        g_evolve_param_grid_x           = blk_6_info.px;
        g_evolve_param_grid_y           = blk_6_info.py;
        g_logical_screen.x_offset       = blk_6_info.sx_offs;
        g_logical_screen.y_offset       = blk_6_info.sy_offs;
        g_logical_screen.x_dots        = blk_6_info.x_dots;
        g_logical_screen.y_dots        = blk_6_info.y_dots;
        g_evolve_image_grid_size = blk_6_info.image_grid_size;
        g_evolve_this_generation_random_seed = blk_6_info.this_generation_random_seed;
        g_evolve_max_random_mutation = blk_6_info.max_random_mutation;
        g_evolving = static_cast<EvolutionModeFlags>(blk_6_info.evolving);
        g_viewport.enabled = g_evolving != EvolutionModeFlags::NONE;
        g_evolve_dist_per_x = g_evolve_x_parameter_range /(g_evolve_image_grid_size - 1);
        g_evolve_dist_per_y = g_evolve_y_parameter_range /(g_evolve_image_grid_size - 1);
        if (read_info.info_version > 14)
        {
            for (int i = 0; i < NUM_GENES; i++)
            {
                gene[i].mutate = static_cast<Variations>(blk_6_info.mutate[i]);
            }
        }
        else
        {
            for (int i = 0; i < 6; i++)
            {
                gene[i].mutate = static_cast<Variations>(blk_6_info.mutate[i]);
            }
            for (int i = 6; i < 10; i++)
            {
                gene[i].mutate = Variations::NONE;
            }
            for (int i = 10; i < NUM_GENES; i++)
            {
                gene[i].mutate = static_cast<Variations>(blk_6_info.mutate[i-4]);
            }
        }
        copy_genes_to_bank(gene);
        save_param_history();
    }
    else
    {
        g_evolving = EvolutionModeFlags::NONE;
    }

    if (blk_7_info.got_data)
    {
        g_orbit_corner.m_min.x = blk_7_info.ox_min;
        g_orbit_corner.m_max.x = blk_7_info.ox_max;
        g_orbit_corner.m_min.y = blk_7_info.oy_min;
        g_orbit_corner.m_max.y = blk_7_info.oy_max;
        g_orbit_corner.m_3rd.x = blk_7_info.ox_3rd;
        g_orbit_corner.m_3rd.y = blk_7_info.oy_3rd;
        g_keep_screen_coords = blk_7_info.keep_screen_coords != 0;
        g_draw_mode = static_cast<OrbitDrawMode>(blk_7_info.draw_mode);
        if (g_keep_screen_coords)
        {
            g_set_orbit_corners = true;
        }
    }

    g_show_file = ShowFile::LOAD_IMAGE; // trigger the file load
    return 0;
}

static void file_read(void *ptr, const size_t size, const size_t num, std::FILE *stream)
{
    if (std::fread(ptr, size, num, stream) != num)
    {
        throw std::system_error(errno, std::system_category(), "failed fread");
    }
}

// return true on error, false on success
bool find_fractal_info(const std::string &gif_file, FractalInfo *info,   //
    ExtBlock2 *blk_2_info, ExtBlock3 *blk_3_info, ExtBlock4 *blk_4_info, //
    ExtBlock5 *blk_5_info, ExtBlock6 *blk_6_info, ExtBlock7 *blk_7_info)
{
    Byte gif_start[18];
    char temp1[81];
    int block_len;
    int data_len;
    int hdr_offset;
    FormulaInfo formula_info;
    EvolutionInfo evolution_info;
    OrbitsInfo orbits_info;

    blk_2_info->got_data = false;
    blk_3_info->got_data = false;
    blk_4_info->got_data = false;
    blk_5_info->got_data = false;
    blk_6_info->got_data = false;
    blk_7_info->got_data = false;

    s_fp = std::fopen(gif_file.c_str(), "rb");
    if (s_fp == nullptr)
    {
        return true;
    }
    file_read(gif_start, 13, 1, s_fp);
    if (std::strncmp(reinterpret_cast<char *>(gif_start), "GIF", 3) != 0)
    {
        // not GIF, maybe old .tga?
        std::fclose(s_fp);
        return true;
    }

    GET16(gif_start[6], g_file_x_dots);
    GET16(gif_start[8], g_file_y_dots);
    g_file_colors = 2 << (gif_start[10] & 7);
    g_file_aspect_ratio = 0; // unknown
    if (gif_start[12])
    {
        // calc reasonably close value from gif header
        g_file_aspect_ratio = static_cast<float>(
            64.0 / (static_cast<double>(gif_start[12]) + 15.0) * static_cast<double>(g_file_y_dots) / static_cast<double>(g_file_x_dots));
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

    if (g_make_parameter_file && (gif_start[10] & 0x80) != 0)
    {
        for (int i = 0; i < g_file_colors; i++)
        {
            int k = 0;
            for (int j = 0; j < 3; j++)
            {
                k = std::getc(s_fp);
                if (k < 0)
                {
                    break;
                }
                g_dac_box[i][j] = static_cast<Byte>(k);
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
          11 bytes   alpha id, "fractintNNN" with fractint, NNN is secondary id
        n * {
           1 byte    length of block info in bytes
           x bytes   block info
            }
           1 byte    0, extension terminator
       To scan extension blocks, we first look in file at length of FractalInfo
       (the main extension block) from end of file, looking for a literal known
       to be at start of our block info.  Then we scan forward a bit, in case
       the file is from an earlier fractint vsn with shorter FractalInfo.
       If FractalInfo is found and is from vsn>=14, it includes the total length
       of all extension blocks; we then scan them all first to last to load
       any optional ones which are present.
       Defined extension blocks:
         fractint001     header, always present
         fractint002     resume info for interrupted resumable image
         fractint003     additional formula type info
         fractint004     ranges info
         fractint005     extended precision parameters
         fractint006     evolver params
         fractint007     orbits info
    */

    std::memset(info, 0, sizeof(FractalInfo));
    constexpr int FRACTAL_INFO_LEN = sizeof(FractalInfo) + (sizeof(FractalInfo) + 254) / 255;
    std::fseek(s_fp, -1 - FRACTAL_INFO_LEN, SEEK_END);
    /* TODO: revise this to read members one at a time so we get natural alignment
       of fields within the FractalInfo structure for the platform */
    file_read(info, 1, sizeof(FractalInfo), s_fp);
    if (std::strcmp(INFO_ID, info->info_id) == 0)
    {
        decode_fractal_info(info, 1);
        hdr_offset = -1-FRACTAL_INFO_LEN;
    }
    else
    {
        // didn't work 1st try, maybe an older vsn, maybe junk at eof, scan:
        char tmp_buff[110];
        hdr_offset = 0;
        int offset = 80; // don't even check last 80 bytes of file for id
        while (offset < FRACTAL_INFO_LEN+513)
        {
            // allow 512 garbage at eof
            offset += 100; // go back 100 bytes at a time
            std::fseek(s_fp, 0 - offset, SEEK_END);
            file_read(tmp_buff, 1, 110, s_fp); // read 10 extra for string compare
            for (int i = 0; i < 100; ++i)
            {
                if (std::strcmp(INFO_ID, &tmp_buff[i]) == 0)
                {
                    // found header?
                    std::strcpy(info->info_id, INFO_ID);
                    std::fseek(s_fp, hdr_offset = i - offset, SEEK_END);
                    /* TODO: revise this to read members one at a time so we get natural alignment
                        of fields within the FractalInfo structure for the platform */
                    file_read(info, 1, sizeof(FractalInfo), s_fp);
                    decode_fractal_info(info, 1);
                    offset = 10000; // force exit from outer loop
                    break;
                }
            }
        }
    }

    if (hdr_offset)
    {
        // we found INFO_ID

        if (info->info_version >= 4)
        {
            /* first reload main extension block, reasons:
                 might be over 255 chars, and thus earlier load might be bad
                 find exact endpoint, so scan back to start of ext blks works
               */
            std::fseek(s_fp, hdr_offset - 15, SEEK_END);
            int scan_extend = 1;
            while (scan_extend)
            {
                if (std::fgetc(s_fp) != '!' // if not what we expect just give up
                    || std::fread(temp1, 1, 13, s_fp) != 13
                    || std::strncmp(&temp1[2], "fractint", 8) != 0)
                {
                    break;
                }
                temp1[13] = 0;
                switch (std::atoi(&temp1[10]))   // e.g. "fractint002"
                {
                case GifExtensionId::FRACTAL_INFO: // "fractint001", the main extension block
                    if (scan_extend == 2)
                    {
                        // we've been here before, done now
                        scan_extend = 0;
                        break;
                    }
                    load_ext_blk(reinterpret_cast<char *>(info), sizeof(FractalInfo));
                    decode_fractal_info(info, 1);
                    scan_extend = 2;
                    // now we know total extension len, back up to first block
                    std::fseek(s_fp, 0L-info->tot_extend_len, SEEK_CUR);
                    break;
                case GifExtensionId::RESUME_INFO: // "fractint002", resume info
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    blk_2_info->resume_data.resize(data_len);
                    std::fseek(s_fp, 0 - block_len, SEEK_CUR);
                    load_ext_blk(reinterpret_cast<char *>(g_block), data_len);
                    // resume data is assumed to be platform native; no need to decode
                    std::copy(&g_block[0], &g_block[data_len], blk_2_info->resume_data.data());
                    blk_2_info->length = data_len;
                    blk_2_info->got_data = true;
                    break;
                case GifExtensionId::FORMULA_INFO: // "fractint003", formula info
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    // check data_len for backward compatibility
                    std::fseek(s_fp, 0 - block_len, SEEK_CUR);
                    load_ext_blk(reinterpret_cast<char *>(&formula_info), data_len);
                    // TODO: decode formula info?
                    std::strcpy(blk_3_info->form_name, formula_info.form_name);
                    blk_3_info->length = data_len;
                    blk_3_info->got_data = true;
                    if (data_len < sizeof(formula_info))
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
                        blk_3_info->uses_p1 = formula_info.uses_p1;
                        blk_3_info->uses_p2 = formula_info.uses_p2;
                        blk_3_info->uses_p3 = formula_info.uses_p3;
                        blk_3_info->uses_ismand = formula_info.uses_ismand;
                        blk_3_info->ismand = formula_info.ismand;
                        blk_3_info->uses_p4 = formula_info.uses_p4;
                        blk_3_info->uses_p5 = formula_info.uses_p5;
                    }
                    break;
                case GifExtensionId::RANGES_INFO: // "fractint004", ranges info
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    assert(data_len % 2 == 0);  // should specify an integral number of 16-bit ints
                    blk_4_info->length = data_len/2;
                    blk_4_info->range_data.resize(blk_4_info->length);
                    std::fseek(s_fp, -block_len, SEEK_CUR);
                    {
                        std::vector<char> buffer(data_len, 0);
                        load_ext_blk(buffer.data(), data_len);
                        for (int i = 0; i < blk_4_info->length; ++i)
                        {
                            // int16 stored in little-endian byte order
                            blk_4_info->range_data[i] = buffer[i*2 + 0] | buffer[i * 2 + 1] << 8;
                        }
                    }
                    blk_4_info->got_data = true;
                    break;
                case GifExtensionId::EXTENDED_PRECISION: // "fractint005", extended precision parameters
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    blk_5_info->apm_data.resize(data_len);
                    std::fseek(s_fp, 0 - block_len, SEEK_CUR);
                    load_ext_blk(blk_5_info->apm_data.data(), data_len);
                    // TODO: decode extended precision parameters?
                    blk_5_info->got_data = true;
                    break;
                case GifExtensionId::EVOLVER_INFO: // "fractint006", evolver params
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    std::fseek(s_fp, 0 - block_len, SEEK_CUR);
                    load_ext_blk(reinterpret_cast<char *>(&evolution_info), data_len);
                    decode_evolver_info(&evolution_info, 1);
                    blk_6_info->length = data_len;
                    blk_6_info->got_data = true;

                    blk_6_info->x_parameter_range = evolution_info.x_parameter_range;
                    blk_6_info->y_parameter_range = evolution_info.y_parameter_range;
                    blk_6_info->x_parameter_offset = evolution_info.x_parameter_offset;
                    blk_6_info->y_parameter_offset = evolution_info.y_parameter_offset;
                    blk_6_info->discrete_x_parameter_offset = static_cast<char>(evolution_info.discrete_x_parameter_offset);
                    blk_6_info->discrete_y_parameter_offset = static_cast<char>(evolution_info.discrete_y_parameter_offset);
                    blk_6_info->px              = evolution_info.px;
                    blk_6_info->py              = evolution_info.py;
                    blk_6_info->sx_offs          = evolution_info.screen_x_offset;
                    blk_6_info->sy_offs          = evolution_info.screen_y_offset;
                    blk_6_info->x_dots           = evolution_info.x_dots;
                    blk_6_info->y_dots           = evolution_info.y_dots;
                    blk_6_info->image_grid_size = evolution_info.image_grid_size;
                    blk_6_info->evolving        = evolution_info.evolving;
                    blk_6_info->this_generation_random_seed = evolution_info.this_generation_random_seed;
                    blk_6_info->max_random_mutation = evolution_info.max_random_mutation;
                    blk_6_info->e_count          = evolution_info.count;
                    for (int i = 0; i < NUM_GENES; i++)
                    {
                        blk_6_info->mutate[i]    = evolution_info.mutate[i];
                    }
                    break;
                case GifExtensionId::ORBITS_INFO: // "fractint007", orbits parameters
                    skip_ext_blk(&block_len, &data_len); // once to get lengths
                    std::fseek(s_fp, 0 - block_len, SEEK_CUR);
                    load_ext_blk(reinterpret_cast<char *>(&orbits_info), data_len);
                    decode_orbits_info(&orbits_info, 1);
                    blk_7_info->length = data_len;
                    blk_7_info->got_data = true;
                    blk_7_info->ox_min           = orbits_info.orbit_corner_min_x;
                    blk_7_info->ox_max           = orbits_info.orbit_corner_max_x;
                    blk_7_info->oy_min           = orbits_info.orbit_corner_min_y;
                    blk_7_info->oy_max           = orbits_info.orbit_corner_max_y;
                    blk_7_info->ox_3rd           = orbits_info.orbit_corner_3rd_x;
                    blk_7_info->oy_3rd           = orbits_info.orbit_corner_3rd_y;
                    blk_7_info->keep_screen_coords= orbits_info.keep_screen_coords;
                    blk_7_info->draw_mode        = orbits_info.draw_mode;
                    break;
                default:
                    skip_ext_blk(&block_len, &data_len);
                }
            }
        }

        std::fclose(s_fp);
        g_file_aspect_ratio = g_screen_aspect; // if not >= v15, this is correct
        return false;
    }

    std::strcpy(info->info_id, "GIFFILE");
    info->iterations = INITIAL_MAX_ITERATIONS;
    info->iterations_old = INITIAL_MAX_ITERATIONS;
    info->fractal_type = static_cast<short>(FractalType::PLASMA);
    info->x_min = -1;
    info->x_max = 1;
    info->y_min = -1;
    info->y_max = 1;
    info->x3rd = -1;
    info->y3rd = -1;
    info->c_real = 0;
    info->c_imag = 0;
    info->ax = 255;
    info->bx = 255;
    info->cx = 255;
    info->dx = 255;
    info->dot_mode = 0;
    info->x_dots = static_cast<short>(g_file_x_dots);
    info->y_dots = static_cast<short>(g_file_y_dots);
    info->colors = static_cast<short>(g_file_colors);
    info->info_version = 0; // this forces lots more init at calling end too

    // zero means we won
    std::fclose(s_fp);
    return false;
}

static void load_ext_blk(char *load_ptr, int load_len)
{
    int len;
    while ((len = std::fgetc(s_fp)) > 0)
    {
        while (--len >= 0)
        {
            if (--load_len >= 0)
            {
                *load_ptr++ = static_cast<char>(std::fgetc(s_fp));
            }
            else
            {
                std::fgetc(s_fp); // discard excess characters
            }
        }
    }
}

static void skip_ext_blk(int *block_len, int *data_len)
{
    int len;
    *data_len = 0;
    *block_len = 1;
    while ((len = std::fgetc(s_fp)) > 0)
    {
        std::fseek(s_fp, len, SEEK_CUR);
        *data_len += len;
        *block_len += len + 1;
    }
}

// switch obsolete fractal types to new generalizations
static void backwards_compat(const FractalInfo &info)
{
    switch (+g_fractal_type)
    {
    case DeprecatedFractalType::LAMBDA_SINE:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::SIN;
        break;
    case DeprecatedFractalType::LAMBDA_COS:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::COSXX;
        break;
    case DeprecatedFractalType::LAMBDA_EXP:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::EXP;
        break;
    case DeprecatedFractalType::MANDEL_SINE:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::SIN;
        break;
    case DeprecatedFractalType::MANDEL_COS:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::COSXX;
        break;
    case DeprecatedFractalType::MANDEL_EXP:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::EXP;
        break;
    case DeprecatedFractalType::MANDEL_SINH:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::SINH;
        break;
    case DeprecatedFractalType::LAMBDA_SINH:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::SINH;
        break;
    case DeprecatedFractalType::MANDEL_COSH:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::COSH;
        break;
    case DeprecatedFractalType::LAMBDA_COSH:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::COSH;
        break;
    case DeprecatedFractalType::MANDEL_SINE_L:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::SIN;
        break;
    case DeprecatedFractalType::LAMBDA_SINE_L:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::SIN;
        break;
    case DeprecatedFractalType::MANDEL_COS_L:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::COSXX;
        break;
    case DeprecatedFractalType::LAMBDA_COS_L:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::COSXX;
        break;
    case DeprecatedFractalType::MANDEL_SINH_L:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::SINH;
        break;
    case DeprecatedFractalType::LAMBDA_SINH_L:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::SINH;
        break;
    case DeprecatedFractalType::MANDEL_COSH_L:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::COSH;
        break;
    case DeprecatedFractalType::LAMBDA_COSH_L:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::COSH;
        break;
    case DeprecatedFractalType::MANDEL_EXP_L:
        set_fractal_type(FractalType::MANDEL_FN);
        g_trig_index[0] = TrigFn::EXP;
        break;
    case DeprecatedFractalType::LAMBDA_EXP_L:
        set_fractal_type(FractalType::LAMBDA_FN);
        g_trig_index[0] = TrigFn::EXP;
        break;
    case DeprecatedFractalType::DEM_M:
        set_fractal_type(FractalType::MANDEL);
        g_user.distance_estimator_value = (info.y_dots - 1) * 2;
        break;
    case DeprecatedFractalType::DEM_J:
        set_fractal_type(FractalType::JULIA);
        g_user.distance_estimator_value = (info.y_dots - 1) * 2;
        break;
    default:
        break;
    }
    if (g_fractal_type == FractalType::MANDEL_LAMBDA)
    {
        g_use_init_orbit = InitOrbitMode::PIXEL;
    }
    assert(g_cur_fractal_specific == get_fractal_specific(g_fractal_type));
    g_cur_fractal_specific = get_fractal_specific(g_fractal_type);
}

// switch old bifurcation fractal types to new generalizations
void set_if_old_bif()
{
    /* set functions if not set already, may need to check 'new_bifurcation_functions_loaded'
       before calling this routine. */

    switch (g_fractal_type)
    {
    case FractalType::BIFURCATION:
    case FractalType::BIF_STEWART:
    case FractalType::BIF_LAMBDA:
        set_trig_array(0, "ident");
        break;

    case FractalType::BIF_EQ_SIN_PI:
    case FractalType::BIF_PLUS_SIN_PI:
        set_trig_array(0, "sin");
        break;

    default:
        break;
    }
}

// miscellaneous function variable defaults
void set_function_param_defaults()
{
    switch (g_fractal_type)
    {
    case FractalType::POPCORN:
    case FractalType::POPCORN_JUL:
        set_trig_array(0, "sin");
        set_trig_array(1, "tan");
        set_trig_array(2, "sin");
        set_trig_array(3, "tan");
        break;

    case FractalType::LATOO:
        set_trig_array(0, "sin");
        set_trig_array(1, "sin");
        set_trig_array(2, "sin");
        set_trig_array(3, "sin");
        break;

    default:
        break;
    }
}

void backwards_legacy_v18()
{
    if (!g_new_bifurcation_functions_loaded)
    {
        set_if_old_bif(); // old bifs need function set
    }
    if (g_file_version < 1800                                                                     //
        && (g_fractal_type == FractalType::MANDEL_FN || g_fractal_type == FractalType::LAMBDA_FN) //
        && g_user.bailout_value == 0)
    {
        g_user.bailout_value = 2500;
    }
}

void backwards_legacy_v19()
{
    if (g_fractal_type == FractalType::MARKS_JULIA)
    {
        if (g_file_version < 1825)
        {
            if (g_params[2] == 0)
            {
                g_params[2] = 2;
            }
            else
            {
                g_params[2] += 1;
            }
        }
    }
    else if (g_fractal_type == FractalType::FORMULA)
    {
        if (g_file_version < 1824)
        {
            g_inversion.params.fill(0);
            g_inversion.invert = 0;
        }
    }

    // fractal has old bof60/61 problem with magnitude
    g_magnitude_calc = !fix_bof();
    // fractal uses old periodicity method
    g_use_old_periodicity = fix_period_bof();
    g_use_old_distance_estimator = g_file_version < 1827 && g_distance_estimator;
}

void backwards_legacy_v20()
{
    // Fractype == FP type is not seen from PAR file ?????
    g_bad_outside = g_file_version <= 1960                                                 //
        && (g_fractal_type == FractalType::MANDEL || g_fractal_type == FractalType::JULIA) //
        && g_outside_color <= REAL && g_outside_color >= SUM;

    if (g_file_version < 1961 && g_inside_color == EPS_CROSS)
    {
        g_close_proximity = 0.01;
    }

    g_bad_outside = false;
    if (!g_new_bifurcation_functions_loaded)
    {
        set_function_param_defaults();
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

} // namespace id::io
