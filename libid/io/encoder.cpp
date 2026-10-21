// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
//      GIF Encoder and associated routines
//
#include "io/encoder.h"

#include "engine/bailout_formula.h"
#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/engine_timer.h"
#include "engine/fractals.h"
#include "engine/ImageRegion.h"
#include "engine/Inversion.h"
#include "engine/log_map.h"
#include "engine/LogicalScreen.h"
#include "engine/OrbitPlot.h"
#include "engine/pixel.h"
#include "engine/pixel_limits.h"
#include "engine/Potential.h"
#include "engine/random_seed.h"
#include "engine/resume.h"
#include "engine/SolidGuess.h"
#include "engine/spindac.h"
#include "engine/StickyOrbits.h"
#include "engine/trig_fns.h"
#include "engine/UserData.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "fractals/formula.h"
#include "fractals/fractype.h"
#include "fractals/ifs.h"
#include "fractals/julibrot.h"
#include "fractals/lorenz.h"
#include "fractals/lsystem.h"
#include "fractals/parser.h"
#include "geometry/3d.h"
#include "geometry/line3d.h"
#include "geometry/plot3d.h"
#include "io/check_write_file.h"
#include "io/gif_extensions.h"
#include "io/gif_file.h"
#include "io/loadfile.h"
#include "misc/debug_flags.h"
#include "misc/id.h"
#include "misc/version.h"
#include "ui/big_while_loop.h"
#include "io/disk_storage.h"
#include "ui/evolve.h"

#include <gif_lib.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::math;
using namespace id::misc;
using namespace id::ui;

namespace id::io
{

std::filesystem::path g_save_filename{"fract001"};

void setup_save_info(FractalInfo *save_info)
{
    if (g_fractal_type != FractalType::FORMULA)
    {
        g_formula.max_function = 0;
    }
    // set save parameters in save structure
    static_assert(sizeof(INFO_ID) == sizeof(save_info->info_id));
    std::copy_n(INFO_ID, sizeof(save_info->info_id), save_info->info_id);
    save_info->info_version = FRACTAL_INFO_VERSION;
    save_info->iterations_old =
        static_cast<std::int16_t>(g_max_iterations <= std::numeric_limits<short>::max() ? g_max_iterations : std::numeric_limits<short>::max());
    save_info->fractal_type = static_cast<std::int16_t>(+g_fractal_type);
    save_info->x_min = g_image_region.m_min.x;
    save_info->x_max = g_image_region.m_max.x;
    save_info->y_min = g_image_region.m_min.y;
    save_info->y_max = g_image_region.m_max.y;
    save_info->c_real = g_params[0];
    save_info->c_imag = g_params[1];
    save_info->ax = 0;
    save_info->bx = 0;
    save_info->cx = 0;
    save_info->dx = 0;
    constexpr std::int16_t LEGACY_DOT_MODE{19};
    save_info->dot_mode = LEGACY_DOT_MODE;
    save_info->x_dots = static_cast<std::uint16_t>(g_video_entry.x_dots);
    save_info->y_dots = static_cast<std::uint16_t>(g_video_entry.y_dots);
    save_info->colors = static_cast<std::int16_t>(g_video_entry.colors);
    save_info->param3 = 0;        // pre version==7 fields
    save_info->param4 = 0;
    save_info->d_param3 = g_params[2];
    save_info->d_param4 = g_params[3];
    save_info->d_param5 = g_params[4];
    save_info->d_param6 = g_params[5];
    save_info->d_param7 = g_params[6];
    save_info->d_param8 = g_params[7];
    save_info->d_param9 = g_params[8];
    save_info->d_param10 = g_params[9];
    save_info->fill_color = static_cast<std::int16_t>(g_fill_color);
    save_info->potential[0] = static_cast<float>(g_potential.params[0]);
    save_info->potential[1] = static_cast<float>(g_potential.params[1]);
    save_info->potential[2] = static_cast<float>(g_potential.params[2]);
    save_info->random_seed_flag = static_cast<std::int16_t>(g_random_seed_flag ? 1 : 0);
    save_info->random_seed = static_cast<std::int16_t>(g_random_seed);
    if (g_inside_method != ColorMethod::COLOR)
    {
        save_info->inside = static_cast<std::int16_t>(+g_inside_method);
    }
    else
    {
        save_info->inside = static_cast<std::int16_t>(g_inside_color);
    }
    save_info->log_map_old = static_cast<std::int16_t>(g_log_map_flag <= std::numeric_limits<short>::max() ? g_log_map_flag : std::numeric_limits<short>::max());
    save_info->invert[0] = static_cast<float>(g_inversion.params[0]);
    save_info->invert[1] = static_cast<float>(g_inversion.params[1]);
    save_info->invert[2] = static_cast<float>(g_inversion.params[2]);
    save_info->decomp[0] = static_cast<std::int16_t>(g_decomp[0]);
    save_info->biomorph = static_cast<std::int16_t>(g_user.biomorph_value);
    save_info->symmetry = static_cast<std::int16_t>(g_force_symmetry);
    save_info->init3d[0] = static_cast<std::int16_t>(g_sphere ? 1 : 0);  // sphere? 1 = yes, 0 = no
    save_info->init3d[1] = static_cast<std::int16_t>(g_x_rot);            // rotate x-axis 60 degrees
    save_info->init3d[2] = static_cast<std::int16_t>(g_y_rot);            // rotate y-axis 90 degrees
    save_info->init3d[3] = static_cast<std::int16_t>(g_z_rot);            // rotate x-axis  0 degrees
    save_info->init3d[4] = static_cast<std::int16_t>(g_x_scale);          // scale x-axis, 90 percent
    save_info->init3d[5] = static_cast<std::int16_t>(g_y_scale);          // scale y-axis, 90 percent
    save_info->init3d[1] = static_cast<std::int16_t>(g_sphere_phi_min);   // longitude start, 180
    save_info->init3d[2] = static_cast<std::int16_t>(g_sphere_phi_max);   // longitude end ,   0
    save_info->init3d[3] = static_cast<std::int16_t>(g_sphere_theta_min); // latitude start,-90 degrees
    save_info->init3d[4] = static_cast<std::int16_t>(g_sphere_theta_max); // latitude stop,  90 degrees
    save_info->init3d[5] = static_cast<std::int16_t>(g_sphere_radius);    // should be user input
    save_info->init3d[6] = static_cast<std::int16_t>(g_rough);            // scale z-axis, 30 percent
    save_info->init3d[7] = static_cast<std::int16_t>(g_water_line);       // water level
    save_info->init3d[8] = static_cast<std::int16_t>(g_fill_type);        // fill type
    save_info->init3d[9] = static_cast<std::int16_t>(g_viewer_z);         // perspective view point
    save_info->init3d[10] = static_cast<std::int16_t>(g_shift_x);         // x shift
    save_info->init3d[11] = static_cast<std::int16_t>(g_shift_y);         // y shift
    save_info->init3d[12] = static_cast<std::int16_t>(g_light_x);         // x light vector coordinate
    save_info->init3d[13] = static_cast<std::int16_t>(g_light_y);         // y light vector coordinate
    save_info->init3d[14] = static_cast<std::int16_t>(g_light_z);         // z light vector coordinate
    save_info->init3d[15] = static_cast<std::int16_t>(g_light_avg);       // number of points to average
    save_info->preview_factor = static_cast<std::int16_t>(g_preview_factor);
    save_info->x_trans = static_cast<std::int16_t>(g_adjust_3d.x);
    save_info->y_trans = static_cast<std::int16_t>(g_adjust_3d.y);
    save_info->red_crop_left = static_cast<std::int16_t>(g_red_crop_left);
    save_info->red_crop_right = static_cast<std::int16_t>(g_red_crop_right);
    save_info->blue_crop_left = static_cast<short>(g_blue_crop_left);
    save_info->blue_crop_right = static_cast<std::int16_t>(g_blue_crop_right);
    save_info->red_bright = static_cast<std::int16_t>(g_red_bright);
    save_info->blue_bright = static_cast<std::int16_t>(g_blue_bright);
    save_info->x_adjust = static_cast<std::int16_t>(g_converge_x_adjust);
    save_info->y_adjust = static_cast<std::int16_t>(g_converge_y_adjust);
    save_info->eye_separation = static_cast<std::int16_t>(g_eye_separation);
    save_info->glasses_type = static_cast<std::int16_t>(g_glasses_type);
    if (g_outside_method != ColorMethod::COLOR)
    {
        save_info->outside = static_cast<std::int16_t>(+g_outside_method);
    }
    else
    {
        save_info->outside = static_cast<std::int16_t>(g_outside_color);
    }
    save_info->x3rd = g_image_region.m_3rd.x;
    save_info->y3rd = g_image_region.m_3rd.y;
    save_info->calc_status = static_cast<std::int16_t>(g_calc_status);
    save_info->std_calc_mode =
        g_three_pass && g_std_calc_mode == CalcMode::THREE_PASS ? 127 : static_cast<char>(g_std_calc_mode);
    save_info->dist_est_old =
        static_cast<std::int16_t>(g_distance_estimator <= 32000 ? g_distance_estimator : 32000);
    save_info->float_flag = static_cast<std::int16_t>(!g_version.legacy || g_user.legacy_float);
    save_info->bailout_old = static_cast<std::int16_t>(
        g_user.bailout_value >= 4 && g_user.bailout_value <= 32000 ? g_user.bailout_value : 0);
    save_info->calc_time = static_cast<std::int32_t>(g_calc_time);
    save_info->trig_index[0] = static_cast<Byte>(g_trig_index[0]);
    save_info->trig_index[1] = static_cast<Byte>(g_trig_index[1]);
    save_info->trig_index[2] = static_cast<Byte>(g_trig_index[2]);
    save_info->trig_index[3] = static_cast<Byte>(g_trig_index[3]);
    save_info->finite_attractor = static_cast<std::int16_t>(g_attractor.enabled ? 1 : 0);
    save_info->init_orbit[0] = g_init_orbit.x;
    save_info->init_orbit[1] = g_init_orbit.y;
    save_info->use_init_orbit = static_cast<char>(g_use_init_orbit);
    save_info->periodicity = static_cast<std::int16_t>(g_periodicity_check);
    save_info->pot16bit = static_cast<std::int16_t>(g_disk_16_bit ? 1 : 0);
    save_info->final_aspect_ratio = g_viewport.final_aspect_ratio;
    save_info->system = static_cast<std::int16_t>(SaveSystem::WINDOWS); // always "Windows" for saved files
    save_info->release = static_cast<std::int16_t>(20 * 100 + 4);       // legacy FRACTINT 20.04
    save_info->display_3d = static_cast<std::int16_t>(g_display_3d);
    save_info->ambient = static_cast<std::int16_t>(g_ambient);
    save_info->randomize = static_cast<std::int16_t>(g_randomize_3d);
    save_info->haze = static_cast<std::int16_t>(g_haze);
    save_info->transparent[0] = static_cast<std::int16_t>(g_transparent_color_3d[0]);
    save_info->transparent[1] = static_cast<std::int16_t>(g_transparent_color_3d[1]);
    save_info->rotate_lo = static_cast<std::int16_t>(g_color_cycle_range_lo);
    save_info->rotate_hi = static_cast<std::int16_t>(g_color_cycle_range_hi);
    save_info->dist_est_width = static_cast<std::int16_t>(g_distance_estimator_width_factor);
    save_info->julibrot_x_max = g_julibrot_x_max;
    save_info->julibrot_x_min = g_julibrot_x_min;
    save_info->julibrot_y_max = g_julibrot_y_max;
    save_info->julibrot_y_min = g_julibrot_y_min;
    save_info->julibrot_z_dots = static_cast<std::int16_t>(g_julibrot_z_dots);
    save_info->julibrot_origin_fp = g_julibrot_origin;
    save_info->julibrot_depth_fp = g_julibrot_depth;
    save_info->julibrot_height_fp = g_julibrot_height;
    save_info->julibrot_width_fp = g_julibrot_width;
    save_info->julibrot_dist_fp = g_julibrot_dist;
    save_info->eyes_fp = g_eyes;
    save_info->orbit_type = static_cast<std::int16_t>(g_new_orbit_type);
    save_info->juli3d_mode = static_cast<short>(g_julibrot_3d_mode);
    save_info->max_fn = static_cast<std::int16_t>(g_formula.max_function);
    save_info->inverse_julia = static_cast<std::int16_t>((+g_major_method << 8) + +g_inverse_julia_minor_method);
    save_info->bailout = static_cast<std::int32_t>(g_user.bailout_value);
    save_info->bailout_test = static_cast<std::int16_t>(g_bailout_test);
    save_info->iterations = static_cast<std::int32_t>(g_max_iterations);
    save_info->bf_length = static_cast<std::int16_t>(g_bn_length);
    save_info->bf_math = static_cast<std::int16_t>(g_bf_math);
    save_info->old_demm_colors = static_cast<std::int16_t>(g_old_demm_colors ? 1 : 0);
    save_info->log_map = static_cast<std::int32_t>(g_log_map_flag);
    save_info->dist_est = static_cast<std::int32_t>(g_distance_estimator);
    save_info->d_invert[0] = g_inversion.params[0];
    save_info->d_invert[1] = g_inversion.params[1];
    save_info->d_invert[2] = g_inversion.params[2];
    save_info->log_calc = static_cast<std::int16_t>(g_log_map_fly_calculate);
    save_info->stop_pass = static_cast<std::int16_t>(g_stop_pass);
    save_info->quick_calc = static_cast<std::int16_t>(g_quick_calc ? 1 : 0);
    save_info->close_prox = g_close_proximity;
    save_info->no_bof = static_cast<std::int16_t>(g_bof_match_book_images ? 0 : 1);
    save_info->orbit_interval = static_cast<std::int32_t>(g_orbit_interval);
    save_info->orbit_delay = static_cast<std::int16_t>(g_orbit_delay);
    save_info->math_tol[0] = g_math_tol[0]; // can't use std::copy_n because of gcc complaints about packed structs
    save_info->math_tol[1] = g_math_tol[1];
    if (g_version.legacy)
    {
        save_info->release = static_cast<std::int16_t>(g_version.major * 100 + g_version.minor);
        save_info->version_major = 0;
        save_info->version_minor = 0;
        save_info->version_patch = 0;
        save_info->version_tweak = 0;
    }
    else
    {
        save_info->release = static_cast<std::int16_t>(20 * 100 + 4); // legacy FRACTINT 20.04
        save_info->version_major = static_cast<std::uint8_t>(g_version.major);
        save_info->version_minor = static_cast<std::uint8_t>(g_version.minor);
        save_info->version_patch = static_cast<std::uint8_t>(g_version.patch);
        save_info->version_tweak = static_cast<std::uint8_t>(g_version.tweak);
    }
    // NOTE: can't use std::fill here on gcc due to packed constraints.
    for (int i = 0; i < static_cast<int>(std::size(save_info->future)); ++i) // NOLINT(modernize-loop-convert)
    {
        save_info->future[i] = 0;
    }
}

} // namespace id::io
