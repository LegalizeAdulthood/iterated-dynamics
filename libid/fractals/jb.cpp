// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/jb.h"

#include "engine/fractals.h"
#include "engine/get_julia_attractor.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/pickover_mandelbrot.h"
#include "io/loadmap.h"
#include "math/sqr.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "ui/get_3d_params.h"
#include "ui/spindac.h"
#include "ui/starfield.h"
#include "ui/stop_msg.h"

#include <algorithm>

namespace
{

struct Perspective
{
    double x;
    double y;
    double zx;
    double zy;
};

struct JuliBrot
{
    double x_per_inch{};
    double y_per_inch{};
    double inch_per_x_dot{};
    double inch_per_y_dot{};
    double x_pixel{};
    double y_pixel{};
    double init_z{};
    double delta_jx{};
    double delta_jy{};
    double delta_mx{};
    double delta_my{};
    double jx{};
    double jy{};
    double mx{};
    double my{};
    double x_offset{};
    double y_offset{};
    Perspective left_eye{};
    Perspective right_eye{};
    Perspective *per{};
    DComplex jb_c{};
};

} // namespace

static JuliBrot s_jb_fp{};
static int s_b_base{};
static float s_br_ratio_fp{1.0f};

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

static int s_z_pixel;
static int s_plotted;
static long s_n;

static int z_line(double x, double y);

bool julibrot_per_image()
{
    const char *map_name;

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
    s_jb_fp.left_eye.y = 0;
    s_jb_fp.right_eye.zx = g_julibrot_dist_fp;
    s_jb_fp.left_eye.zx = s_jb_fp.right_eye.zx;
    s_jb_fp.right_eye.zy = g_julibrot_dist_fp;
    s_jb_fp.left_eye.zy = s_jb_fp.right_eye.zy;
    s_b_base = 128;

    if (g_julibrot_3d_mode == Julibrot3DMode::RED_BLUE)
    {
        g_save_dac = SaveDAC::NO;
        map_name = g_glasses1_map.c_str();
    }
    else
    {
        map_name = g_altern_map_file.c_str();
    }
    if (g_save_dac != SaveDAC::YES)
    {
        if (validate_luts(map_name))
        {
            return false;
        }
        spin_dac(0, 1);               // load it, but don't spin
        if (g_save_dac == SaveDAC::NEXT_TIME)
        {
            g_save_dac = SaveDAC::YES;
        }
    }
    return true;
}

int julibrot_per_pixel()
{
    s_jb_fp.jx = ((s_jb_fp.per->x - s_jb_fp.x_pixel) * s_jb_fp.init_z / g_julibrot_dist_fp - s_jb_fp.x_pixel) * s_jb_fp.x_per_inch;
    s_jb_fp.jx += s_jb_fp.x_offset;
    s_jb_fp.delta_jx = (g_julibrot_depth_fp / g_julibrot_dist_fp) * (s_jb_fp.per->x - s_jb_fp.x_pixel) * s_jb_fp.x_per_inch / g_julibrot_z_dots;

    s_jb_fp.jy = ((s_jb_fp.per->y - s_jb_fp.y_pixel) * s_jb_fp.init_z / g_julibrot_dist_fp - s_jb_fp.y_pixel) * s_jb_fp.y_per_inch;
    s_jb_fp.jy += s_jb_fp.y_offset;
    s_jb_fp.delta_jy = g_julibrot_depth_fp / g_julibrot_dist_fp * (s_jb_fp.per->y - s_jb_fp.y_pixel) * s_jb_fp.y_per_inch / g_julibrot_z_dots;

    return 1;
}

int z_line(double x, double y)
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
    julibrot_per_pixel();
    for (s_z_pixel = 0; s_z_pixel < g_julibrot_z_dots; s_z_pixel++)
    {
        // Special initialization for Mandelbrot types
        if (g_new_orbit_type == FractalType::QUAT || g_new_orbit_type == FractalType::HYPER_CMPLX)
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
            if (get_fractal_specific(g_new_orbit_type)->orbit_calc())
            {
                break;
            }
        }
        if (s_n == g_max_iterations)
        {
            if (g_julibrot_3d_mode == Julibrot3DMode::RED_BLUE)
            {
                g_color = (int)(128L * s_z_pixel / g_julibrot_z_dots);
                if ((g_row + g_col) & 1)
                {
                    g_plot(g_col, g_row, 127 - g_color);
                }
                else
                {
                    g_color = (int)(g_color * s_br_ratio_fp);
                    g_color = std::max(g_color, 1);
                    g_color = std::min(g_color, 127);
                    g_plot(g_col, g_row, 127 + s_b_base - g_color);
                }
            }
            else
            {
                g_color = (int)(254L * s_z_pixel / g_julibrot_z_dots);
                g_plot(g_col, g_row, g_color + 1);
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

int calc_standard_4d_type()
{
    g_c_exponent = (int)g_params[2];

    if (g_new_orbit_type == FractalType::JULIA_Z_POWER)
    {
        if (g_params[3] == 0.0 && g_debug_flag != DebugFlags::FORCE_COMPLEX_POWER && (double)g_c_exponent == g_params[2])
        {
            get_fractal_specific(g_new_orbit_type)->orbit_calc = mandel_z_power_orbit;
        }
        else
        {
            get_fractal_specific(g_new_orbit_type)->orbit_calc = float_cmplx_z_power_fractal;
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
