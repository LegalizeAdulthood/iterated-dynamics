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

using namespace id::io;
using namespace id::math;
using namespace id::misc;

namespace id::fractals
{

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

static JuliBrot s_jb{};
static int s_b_base{};
static float s_br_ratio{1.0F};

bool g_julibrot{}; // flag for julibrot

// these need to be accessed elsewhere for saving data
double g_julibrot_x_min{-.83};
double g_julibrot_y_min{-.25};
double g_julibrot_x_max{-.83};
double g_julibrot_y_max{.25};
//
int g_julibrot_z_dots{128};
float g_julibrot_origin{8.0F};
float g_julibrot_height{7.0F};
float g_julibrot_width{10.0F};
float g_julibrot_dist{24.0F};
float g_eyes{2.5F};
float g_julibrot_depth{8.0F};
Julibrot3DMode g_julibrot_3d_mode{};
FractalType g_new_orbit_type{FractalType::JULIA};
const char *g_julibrot_3d_options[]{
    to_string(Julibrot3DMode::MONOCULAR), //
    to_string(Julibrot3DMode::LEFT_EYE),  //
    to_string(Julibrot3DMode::RIGHT_EYE), //
    to_string(Julibrot3DMode::RED_BLUE)   //
};

static int s_z_pixel;
static bool s_plotted{};
static long s_n;

static void z_line(double x, double y);

bool julibrot_per_image()
{
    const char *map_name;

    if (g_colors < 255)
    {
        stop_msg("Sorry, but Julibrots require a 256-color video mode");
        return false;
    }

    s_jb.x_offset = (g_x_max + g_x_min) / 2;     // Calculate average
    s_jb.y_offset = (g_y_max + g_y_min) / 2;     // Calculate average
    s_jb.delta_mx = (g_julibrot_x_max - g_julibrot_x_min) / g_julibrot_z_dots;
    s_jb.delta_my = (g_julibrot_y_max - g_julibrot_y_min) / g_julibrot_z_dots;
    g_float_param = &s_jb.jb_c;
    s_jb.x_per_inch = (g_x_min - g_x_max) / g_julibrot_width;
    s_jb.y_per_inch = (g_y_max - g_y_min) / g_julibrot_height;
    s_jb.inch_per_x_dot = g_julibrot_width / g_logical_screen_x_dots;
    s_jb.inch_per_y_dot = g_julibrot_height / g_logical_screen_y_dots;
    s_jb.init_z = g_julibrot_origin - (g_julibrot_depth / 2);
    if (g_julibrot_3d_mode == Julibrot3DMode::MONOCULAR)
    {
        s_jb.right_eye.x = 0.0;
    }
    else
    {
        s_jb.right_eye.x = g_eyes / 2;
    }
    s_jb.left_eye.x = -s_jb.right_eye.x;
    s_jb.right_eye.y = 0;
    s_jb.left_eye.y = 0;
    s_jb.right_eye.zx = g_julibrot_dist;
    s_jb.left_eye.zx = s_jb.right_eye.zx;
    s_jb.right_eye.zy = g_julibrot_dist;
    s_jb.left_eye.zy = s_jb.right_eye.zy;
    s_b_base = 128;

    if (g_julibrot_3d_mode == Julibrot3DMode::RED_BLUE)
    {
        g_save_dac = SaveDAC::NO;
        map_name = GLASSES1_MAP_NAME.c_str();
    }
    else
    {
        map_name = ALTERN_MAP_NAME.c_str();
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
    s_jb.jx = ((s_jb.per->x - s_jb.x_pixel) * s_jb.init_z / g_julibrot_dist - s_jb.x_pixel) * s_jb.x_per_inch;
    s_jb.jx += s_jb.x_offset;
    s_jb.delta_jx = (g_julibrot_depth / g_julibrot_dist) * (s_jb.per->x - s_jb.x_pixel) * s_jb.x_per_inch / g_julibrot_z_dots;

    s_jb.jy = ((s_jb.per->y - s_jb.y_pixel) * s_jb.init_z / g_julibrot_dist - s_jb.y_pixel) * s_jb.y_per_inch;
    s_jb.jy += s_jb.y_offset;
    s_jb.delta_jy = g_julibrot_depth / g_julibrot_dist * (s_jb.per->y - s_jb.y_pixel) * s_jb.y_per_inch / g_julibrot_z_dots;

    return 1;
}

static void z_line(double x, double y)
{
    s_jb.x_pixel = x;
    s_jb.y_pixel = y;
    s_jb.mx = g_julibrot_x_min;
    s_jb.my = g_julibrot_y_min;
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
    julibrot_per_pixel();
    for (s_z_pixel = 0; s_z_pixel < g_julibrot_z_dots; s_z_pixel++)
    {
        // Special initialization for Mandelbrot types
        if (g_new_orbit_type == FractalType::QUAT || g_new_orbit_type == FractalType::HYPER_CMPLX)
        {
            g_old_z.x = 0.0;
            g_old_z.y = 0.0;
            s_jb.jb_c.x = 0.0;
            s_jb.jb_c.y = 0.0;
            g_quaternion_c = s_jb.jx;
            g_quaternion_ci = s_jb.jy;
            g_quaternion_cj = s_jb.mx;
            g_quaternion_ck = s_jb.my;
        }
        else
        {
            g_old_z.x = s_jb.jx;
            g_old_z.y = s_jb.jy;
            s_jb.jb_c.x = s_jb.mx;
            s_jb.jb_c.y = s_jb.my;
            g_quaternion_c = g_params[0];

            g_quaternion_ci = g_params[1];
            g_quaternion_cj = g_params[2];
            g_quaternion_ck = g_params[3];
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
                    g_color = (int)(g_color * s_br_ratio);
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
        s_jb.mx += s_jb.delta_mx;
        s_jb.my += s_jb.delta_my;
        s_jb.jx += s_jb.delta_jx;
        s_jb.jy += s_jb.delta_jy;
    }
}

Standard4D::Standard4D()
{
    g_c_exponent = (int) g_params[2];

    if (g_new_orbit_type == FractalType::JULIA_Z_POWER)
    {
        if (g_params[3] == 0.0 && g_debug_flag != DebugFlags::FORCE_COMPLEX_POWER &&
            (double) g_c_exponent == g_params[2])
        {
            get_fractal_specific(g_new_orbit_type)->orbit_calc = mandel_z_power_orbit;
        }
        else
        {
            get_fractal_specific(g_new_orbit_type)->orbit_calc = mandel_z_power_cmplx_orbit;
        }
        get_julia_attractor(g_params[0], g_params[1]); // another attractor?
    }
    m_y_dot = (g_logical_screen_y_dots >> 1) - 1;
    m_x_dot = 0;
    s_plotted = false;
    m_x = -g_julibrot_width / 2.0;
}

bool Standard4D::iterate()
{
    if (m_y_dot < 0)
    {
        return false;
    }

    g_col = m_x_dot;
    g_row = m_y_dot;
    z_line(m_x, m_y);
    g_col = g_logical_screen_x_dots - g_col - 1;
    g_row = g_logical_screen_y_dots - g_row - 1;
    z_line(-m_x, -m_y);
    ++m_x_dot;
    m_x += s_jb.inch_per_x_dot;
    if (m_x_dot == g_logical_screen_x_dots)
    {
        // next scanline
        if (!s_plotted && m_y != 0.0)
        {
            return false;
        }
        --m_y_dot;
        m_y -= s_jb.inch_per_y_dot;
        s_plotted = false;
        m_x_dot = 0;
        m_x = -g_julibrot_width / 2.0;
    }

    return true;
}

} // namespace id::fractals
