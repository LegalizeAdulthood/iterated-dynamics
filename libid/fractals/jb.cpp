// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/jb.h"

#include "debug_flags.h"
#include "drivers.h"
#include "engine/fractals.h"
#include "engine/get_julia_attractor.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/pickover_mandelbrot.h"
#include "io/loadmap.h"
#include "math/fixed_pt.h"
#include "math/sqr.h"
#include "spindac.h"
#include "starfield.h"
#include "ui/get_3d_params.h"
#include "ui/stop_msg.h"

#include <algorithm>

template <typename T>
struct PerspectiveT
{
    T x;
    T y;
    T zx;
    T zy;
};

using Perspective = PerspectiveT<long>;
using PerspectiveFP = PerspectiveT<double>;

template <typename T>
struct JuliBrot
{
    T x_per_inch{};
    T y_per_inch{};
    T inch_per_x_dot{};
    T inch_per_y_dot{};
    T x_pixel{};
    T y_pixel{};
    T init_z{};
    T delta_jx{};
    T delta_jy{};
    T delta_mx{};
    T delta_my{};
    T jx{};
    T jy{};
    T mx{};
    T my{};
    T x_offset{};
    T y_offset{};
    PerspectiveT<T> left_eye{};
    PerspectiveT<T> right_eye{};
    PerspectiveT<T> *per{};
    id::Complex<T> jb_c{};
};

static JuliBrot<long> s_jb{};
static JuliBrot<double> s_jb_fp{};
static long s_mx_min{};
static long s_my_min{};
static int s_b_base{};
static double s_fg{};
static double s_fg16{};
static float s_br_ratio_fp{1.0f};
static long s_width{};
static long s_dist{};
static long s_depth{};
static long s_br_ratio{};
static long s_eyes{};

bool g_julibrot{}; // flag for julibrot

// these need to be accessed elsewhere for saving data
double g_julibrot_x_min{-.83};
double g_julibrot_y_min{-.25};
double g_julibrot_x_max{-.83};
double g_julibrot_y_max{.25};
//
int g_julibrot_z_dots{128};
float g_julibrot_origin_fp{8.0f};
float g_julibrot_height_fp{7.0f};
float g_julibrot_width_fp{10.0f};
float g_julibrot_dist_fp{24.0f};
float g_eyes_fp{2.5f};
float g_julibrot_depth_fp{8.0f};
Julibrot3DMode g_julibrot_3d_mode{};
FractalType g_new_orbit_type{FractalType::JULIA};
const char *g_julibrot_3d_options[]{
    to_string(Julibrot3DMode::MONOCULAR), //
    to_string(Julibrot3DMode::LEFT_EYE),  //
    to_string(Julibrot3DMode::RIGHT_EYE), //
    to_string(Julibrot3DMode::RED_BLUE)   //
};

bool julibrot_setup()
{
    char const *map_name;

    if (g_colors < 255)
    {
        stop_msg("Sorry, but Julibrots require a 256-color video mode");
        return false;
    }

    s_jb_fp.x_offset = (g_x_max + g_x_min) / 2;     // Calculate average
    s_jb_fp.y_offset = (g_y_max + g_y_min) / 2;     // Calculate average
    s_jb_fp.delta_mx = (g_julibrot_x_max - g_julibrot_x_min) / g_julibrot_z_dots;
    s_jb_fp.delta_my = (g_julibrot_y_max - g_julibrot_y_min) / g_julibrot_z_dots;
    g_float_param = &s_jb_fp.jb_c;
    s_jb_fp.x_per_inch = (g_x_min - g_x_max) / g_julibrot_width_fp;
    s_jb_fp.y_per_inch = (g_y_max - g_y_min) / g_julibrot_height_fp;
    s_jb_fp.inch_per_x_dot = g_julibrot_width_fp / g_logical_screen_x_dots;
    s_jb_fp.inch_per_y_dot = g_julibrot_height_fp / g_logical_screen_y_dots;
    s_jb_fp.init_z = g_julibrot_origin_fp - (g_julibrot_depth_fp / 2);
    if (g_julibrot_3d_mode == Julibrot3DMode::MONOCULAR)
    {
        s_jb_fp.right_eye.x = 0.0;
    }
    else
    {
        s_jb_fp.right_eye.x = g_eyes_fp / 2;
    }
    s_jb_fp.left_eye.x = -s_jb_fp.right_eye.x;
    s_jb_fp.right_eye.y = 0;
    s_jb_fp.left_eye.y = s_jb_fp.right_eye.y;
    s_jb_fp.right_eye.zx = g_julibrot_dist_fp;
    s_jb_fp.left_eye.zx = s_jb_fp.right_eye.zx;
    s_jb_fp.right_eye.zy = g_julibrot_dist_fp;
    s_jb_fp.left_eye.zy = s_jb_fp.right_eye.zy;
    s_b_base = 128;

    if (g_fractal_specific[+g_fractal_type].is_integer > 0)
    {
        if (g_fractal_specific[+g_new_orbit_type].is_integer == 0)
        {
            stop_msg("Julibrot orbit type isinteger mismatch");
        }
        if (g_fractal_specific[+g_new_orbit_type].is_integer > 1)
        {
            g_bit_shift = g_fractal_specific[+g_new_orbit_type].is_integer;
        }
        s_fg = (double)(1L << g_bit_shift);
        s_fg16 = (double)(1L << 16);
        long j_x_min = (long) (g_x_min * s_fg);
        long j_x_max = (long) (g_x_max * s_fg);
        s_jb.x_offset = (j_x_max + j_x_min) / 2;    // Calculate average
        long j_y_min = (long) (g_y_min * s_fg);
        long j_y_max = (long) (g_y_max * s_fg);
        s_jb.y_offset = (j_y_max + j_y_min) / 2;    // Calculate average
        s_mx_min = (long)(g_julibrot_x_min * s_fg);
        long m_x_max = (long) (g_julibrot_x_max * s_fg);
        s_my_min = (long)(g_julibrot_y_min * s_fg);
        long m_y_max = (long) (g_julibrot_y_max * s_fg);
        long origin = (long)(g_julibrot_origin_fp * s_fg16);
        s_depth = (long)(g_julibrot_depth_fp * s_fg16);
        s_width = (long)(g_julibrot_width_fp * s_fg16);
        s_dist = (long)(g_julibrot_dist_fp * s_fg16);
        s_eyes = (long)(g_eyes_fp * s_fg16);
        s_br_ratio = (long)(s_br_ratio_fp * s_fg16);
        s_jb.delta_mx = (m_x_max - s_mx_min) / g_julibrot_z_dots;
        s_jb.delta_my = (m_y_max - s_my_min) / g_julibrot_z_dots;
        g_long_param = &s_jb.jb_c;

        s_jb.x_per_inch = (long)((g_x_min - g_x_max) / g_julibrot_width_fp * s_fg);
        s_jb.y_per_inch = (long)((g_y_max - g_y_min) / g_julibrot_height_fp * s_fg);
        s_jb.inch_per_x_dot = (long)((g_julibrot_width_fp / g_logical_screen_x_dots) * s_fg16);
        s_jb.inch_per_y_dot = (long)((g_julibrot_height_fp / g_logical_screen_y_dots) * s_fg16);
        s_jb.init_z = origin - (s_depth / 2);
        if (g_julibrot_3d_mode == Julibrot3DMode::MONOCULAR)
        {
            s_jb.right_eye.x = 0L;
        }
        else
        {
            s_jb.right_eye.x = s_eyes / 2;
        }
        s_jb.left_eye.x = -s_jb.right_eye.x;
        s_jb.right_eye.y = 0L;
        s_jb.left_eye.y = s_jb.right_eye.y;
        s_jb.right_eye.zx = s_dist;
        s_jb.left_eye.zx = s_jb.right_eye.zx;
        s_jb.right_eye.zy = s_dist;
        s_jb.left_eye.zy = s_jb.right_eye.zy;
        s_b_base = (int)(128.0 * s_br_ratio_fp);
    }

    if (g_julibrot_3d_mode == Julibrot3DMode::RED_BLUE)
    {
        g_save_dac = 0;
        map_name = g_glasses1_map.c_str();
    }
    else
    {
        map_name = g_altern_map_file.data();
    }
    if (g_save_dac != 1)
    {
        if (validate_luts(map_name))
        {
            return false;
        }
        spin_dac(0, 1);               // load it, but don't spin
        if (g_save_dac == 2)
        {
            g_save_dac = 1;
        }
    }
    return true;
}

int jb_per_pixel()
{
    s_jb.jx = multiply(s_jb.per->x - s_jb.x_pixel, s_jb.init_z, 16);
    s_jb.jx = divide(s_jb.jx, s_dist, 16) - s_jb.x_pixel;
    s_jb.jx = multiply(s_jb.jx << (g_bit_shift - 16), s_jb.x_per_inch, g_bit_shift);
    s_jb.jx += s_jb.x_offset;
    s_jb.delta_jx = divide(s_depth, s_dist, 16);
    s_jb.delta_jx = multiply(s_jb.delta_jx, s_jb.per->x - s_jb.x_pixel, 16) << (g_bit_shift - 16);
    s_jb.delta_jx = multiply(s_jb.delta_jx, s_jb.x_per_inch, g_bit_shift) / g_julibrot_z_dots;

    s_jb.jy = multiply(s_jb.per->y - s_jb.y_pixel, s_jb.init_z, 16);
    s_jb.jy = divide(s_jb.jy, s_dist, 16) - s_jb.y_pixel;
    s_jb.jy = multiply(s_jb.jy << (g_bit_shift - 16), s_jb.y_per_inch, g_bit_shift);
    s_jb.jy += s_jb.y_offset;
    s_jb.delta_jy = divide(s_depth, s_dist, 16);
    s_jb.delta_jy = multiply(s_jb.delta_jy, s_jb.per->y - s_jb.y_pixel, 16) << (g_bit_shift - 16);
    s_jb.delta_jy = multiply(s_jb.delta_jy, s_jb.y_per_inch, g_bit_shift) / g_julibrot_z_dots;

    return 1;
}

int jb_fp_per_pixel()
{
    s_jb_fp.jx = ((s_jb_fp.per->x - s_jb_fp.x_pixel) * s_jb_fp.init_z / g_julibrot_dist_fp - s_jb_fp.x_pixel) * s_jb_fp.x_per_inch;
    s_jb_fp.jx += s_jb_fp.x_offset;
    s_jb_fp.delta_jx = (g_julibrot_depth_fp / g_julibrot_dist_fp) * (s_jb_fp.per->x - s_jb_fp.x_pixel) * s_jb_fp.x_per_inch / g_julibrot_z_dots;

    s_jb_fp.jy = ((s_jb_fp.per->y - s_jb_fp.y_pixel) * s_jb_fp.init_z / g_julibrot_dist_fp - s_jb_fp.y_pixel) * s_jb_fp.y_per_inch;
    s_jb_fp.jy += s_jb_fp.y_offset;
    s_jb_fp.delta_jy = g_julibrot_depth_fp / g_julibrot_dist_fp * (s_jb_fp.per->y - s_jb_fp.y_pixel) * s_jb_fp.y_per_inch / g_julibrot_z_dots;

    return 1;
}

static int s_z_pixel;
static int s_plotted;
static long s_n;

int z_line(long x, long y)
{
    s_jb.x_pixel = x;
    s_jb.y_pixel = y;
    s_jb.mx = s_mx_min;
    s_jb.my = s_my_min;
    switch (g_julibrot_3d_mode)
    {
    case Julibrot3DMode::MONOCULAR:
    case Julibrot3DMode::LEFT_EYE:
        s_jb.per = &s_jb.left_eye;
        break;
    case Julibrot3DMode::RIGHT_EYE:
        s_jb.per = &s_jb.right_eye;
        break;
    case Julibrot3DMode::RED_BLUE:
        if ((g_row + g_col) & 1)
        {
            s_jb.per = &s_jb.left_eye;
        }
        else
        {
            s_jb.per = &s_jb.right_eye;
        }
        break;
    }
    jb_per_pixel();
    for (s_z_pixel = 0; s_z_pixel < g_julibrot_z_dots; s_z_pixel++)
    {
        g_l_old_z.x = s_jb.jx;
        g_l_old_z.y = s_jb.jy;
        s_jb.jb_c.x = s_jb.mx;
        s_jb.jb_c.y = s_jb.my;
        if (driver_key_pressed())
        {
            return -1;
        }
        g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
        g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
        for (s_n = 0; s_n < g_max_iterations; s_n++)
        {
            if (g_fractal_specific[+g_new_orbit_type].orbit_calc())
            {
                break;
            }
        }
        if (s_n == g_max_iterations)
        {
            if (g_julibrot_3d_mode == Julibrot3DMode::RED_BLUE)
            {
                g_color = (int)(128l * s_z_pixel / g_julibrot_z_dots);
                if ((g_row + g_col) & 1)
                {

                    (*g_plot)(g_col, g_row, 127 - g_color);
                }
                else
                {
                    g_color = (int)(multiply((long) g_color << 16, s_br_ratio, 16) >> 16);
                    g_color = std::max(g_color, 1);
                    g_color = std::min(g_color, 127);
                    (*g_plot)(g_col, g_row, 127 + s_b_base - g_color);
                }
            }
            else
            {
                g_color = (int)(254l * s_z_pixel / g_julibrot_z_dots);
                (*g_plot)(g_col, g_row, g_color + 1);
            }
            s_plotted = 1;
            break;
        }
        s_jb.mx += s_jb.delta_mx;
        s_jb.my += s_jb.delta_my;
        s_jb.jx += s_jb.delta_jx;
        s_jb.jy += s_jb.delta_jy;
    }
    return 0;
}

int z_line_fp(double x, double y)
{
    s_jb_fp.x_pixel = x;
    s_jb_fp.y_pixel = y;
    s_jb_fp.mx = g_julibrot_x_min;
    s_jb_fp.my = g_julibrot_y_min;
    switch (g_julibrot_3d_mode)
    {
    case Julibrot3DMode::MONOCULAR:
    case Julibrot3DMode::LEFT_EYE:
        s_jb_fp.per = &s_jb_fp.left_eye;
        break;
    case Julibrot3DMode::RIGHT_EYE:
        s_jb_fp.per = &s_jb_fp.right_eye;
        break;
    case Julibrot3DMode::RED_BLUE:
        if ((g_row + g_col) & 1)
        {
            s_jb_fp.per = &s_jb_fp.left_eye;
        }
        else
        {
            s_jb_fp.per = &s_jb_fp.right_eye;
        }
        break;
    }
    jb_fp_per_pixel();
    for (s_z_pixel = 0; s_z_pixel < g_julibrot_z_dots; s_z_pixel++)
    {
        // Special initialization for Mandelbrot types
        if (g_new_orbit_type == FractalType::QUAT_FP || g_new_orbit_type == FractalType::HYPER_CMPLX_FP)
        {
            g_old_z.x = 0.0;
            g_old_z.y = 0.0;
            s_jb_fp.jb_c.x = 0.0;
            s_jb_fp.jb_c.y = 0.0;
            g_quaternion_c = s_jb_fp.jx;
            g_quaternion_ci = s_jb_fp.jy;
            g_quaternion_cj = s_jb_fp.mx;
            g_quaternion_ck = s_jb_fp.my;
        }
        else
        {
            g_old_z.x = s_jb_fp.jx;
            g_old_z.y = s_jb_fp.jy;
            s_jb_fp.jb_c.x = s_jb_fp.mx;
            s_jb_fp.jb_c.y = s_jb_fp.my;
            g_quaternion_c = g_params[0];
            g_quaternion_ci = g_params[1];
            g_quaternion_cj = g_params[2];
            g_quaternion_ck = g_params[3];
        }
        if (driver_key_pressed())
        {
            return -1;
        }
        g_temp_sqr_x = sqr(g_old_z.x);
        g_temp_sqr_y = sqr(g_old_z.y);

        for (s_n = 0; s_n < g_max_iterations; s_n++)
        {
            if (g_fractal_specific[+g_new_orbit_type].orbit_calc())
            {
                break;
            }
        }
        if (s_n == g_max_iterations)
        {
            if (g_julibrot_3d_mode == Julibrot3DMode::RED_BLUE)
            {
                g_color = (int)(128l * s_z_pixel / g_julibrot_z_dots);
                if ((g_row + g_col) & 1)
                {
                    (*g_plot)(g_col, g_row, 127 - g_color);
                }
                else
                {
                    g_color = (int)(g_color * s_br_ratio_fp);
                    g_color = std::max(g_color, 1);
                    g_color = std::min(g_color, 127);
                    (*g_plot)(g_col, g_row, 127 + s_b_base - g_color);
                }
            }
            else
            {
                g_color = (int)(254l * s_z_pixel / g_julibrot_z_dots);
                (*g_plot)(g_col, g_row, g_color + 1);
            }
            s_plotted = 1;
            break;
        }
        s_jb_fp.mx += s_jb_fp.delta_mx;
        s_jb_fp.my += s_jb_fp.delta_my;
        s_jb_fp.jx += s_jb_fp.delta_jx;
        s_jb_fp.jy += s_jb_fp.delta_jy;
    }
    return 0;
}

int std_4d_fractal()
{
    g_c_exponent = (int)g_params[2];
    if (g_new_orbit_type == FractalType::JULIA_Z_POWER_L)
    {
        g_c_exponent = std::max(g_c_exponent, 1);
        if (g_params[3] == 0.0 && g_debug_flag != DebugFlags::FORCE_COMPLEX_POWER && (double)g_c_exponent == g_params[2])
        {
            g_fractal_specific[+g_new_orbit_type].orbit_calc = long_z_power_fractal;
        }
        else
        {
            g_fractal_specific[+g_new_orbit_type].orbit_calc = long_cmplx_z_power_fractal;
        }
    }

    long y = 0;
    for (int y_dot = (g_logical_screen_y_dots >> 1) - 1; y_dot >= 0; y_dot--, y -= s_jb.inch_per_y_dot)
    {
        s_plotted = 0;
        long x = -(s_width >> 1);
        for (int x_dot = 0; x_dot < g_logical_screen_x_dots; x_dot++, x += s_jb.inch_per_x_dot)
        {
            g_col = x_dot;
            g_row = y_dot;
            if (z_line(x, y) < 0)
            {
                return -1;
            }
            g_col = g_logical_screen_x_dots - g_col - 1;
            g_row = g_logical_screen_y_dots - g_row - 1;
            if (z_line(-x, -y) < 0)
            {
                return -1;
            }
        }
        if (s_plotted == 0)
        {
            if (y == 0)
            {
                s_plotted = -1;  // no points first pass; don't give up
            }
            else
            {
                break;
            }
        }
    }
    return 0;
}

int std_4d_fp_fractal()
{
    g_c_exponent = (int)g_params[2];

    if (g_new_orbit_type == FractalType::JULIA_Z_POWER_FP)
    {
        if (g_params[3] == 0.0 && g_debug_flag != DebugFlags::FORCE_COMPLEX_POWER && (double)g_c_exponent == g_params[2])
        {
            g_fractal_specific[+g_new_orbit_type].orbit_calc = float_z_power_fractal;
        }
        else
        {
            g_fractal_specific[+g_new_orbit_type].orbit_calc = float_cmplx_z_power_fractal;
        }
        get_julia_attractor(g_params[0], g_params[1]);  // another attractor?
    }

    double y = 0.0;
    for (int y_dot = (g_logical_screen_y_dots >> 1) - 1; y_dot >= 0; y_dot--, y -= s_jb_fp.inch_per_y_dot)
    {
        s_plotted = 0;
        double x = -g_julibrot_width_fp / 2;
        for (int x_dot = 0; x_dot < g_logical_screen_x_dots; x_dot++, x += s_jb_fp.inch_per_x_dot)
        {
            g_col = x_dot;
            g_row = y_dot;
            if (z_line_fp(x, y) < 0)
            {
                return -1;
            }
            g_col = g_logical_screen_x_dots - g_col - 1;
            g_row = g_logical_screen_y_dots - g_row - 1;
            if (z_line_fp(-x, -y) < 0)
            {
                return -1;
            }
        }
        if (s_plotted == 0)
        {
            if (y == 0)
            {
                s_plotted = -1;  // no points first pass; don't give up
            }
            else
            {
                break;
            }
        }
    }
    return 0;
}
