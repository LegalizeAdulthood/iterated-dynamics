#include "port.h"
#include "prototyp.h"

#include "jb.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "drivers.h"
#include "fixed_pt.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "get_3d_params.h"
#include "get_julia_attractor.h"
#include "helpdefs.h"
#include "id_data.h"
#include "loadmap.h"
#include "pickover_mandelbrot.h"
#include "spindac.h"
#include "sqr.h"
#include "starfield.h"
#include "stop_msg.h"

struct Perspective
{
    long x;
    long y;
    long zx;
    long zy;
};

struct Perspectivefp
{
    double x;
    double y;
    double zx;
    double zy;
};

static long s_mx_min{};
static long s_my_min{};
static long s_x_per_inch{};
static long s_y_per_inch{};
static long s_inch_per_x_dot{};
static long s_inch_per_y_dot{};
static double s_x_per_inch_fp{};
static double s_y_per_inch_fp{};
static double s_inch_per_x_dot_fp{};
static double s_inch_per_y_dot_fp{};
static int s_b_base{};
static long s_x_pixel{};
static long s_y_pixel{};
static double s_x_pixel_fp{};
static double s_y_pixel_fp{};
static long s_init_z{};
static long s_delta_jx{};
static long s_delta_jy{};
static long s_delta_mx{};
static long s_delta_my{};
static double s_init_z_fp{};
static double s_delta_jx_fp{};
static double s_delta_jy_fp{};
static double s_delta_mx_fp{};
static double s_delta_my_fp{};
static long s_jx{};
static long s_jy{};
static long s_mx{};
static long s_my{};
static long s_x_offset{};
static long s_y_offset{};
static double s_jx_fp{};
static double s_jy_fp{};
static double s_mx_fp{};
static double s_my_fp{};
static double s_x_offset_fp{};
static double s_y_offset_fp{};
static Perspective s_left_eye{};
static Perspective s_right_eye{};
static Perspective *s_per{};
static Perspectivefp s_left_eye_fp{};
static Perspectivefp s_right_eye_fp{};
static Perspectivefp *s_per_fp{};
static LComplex s_jb_c{};
static DComplex s_jb_c_fp{};
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
julibrot_3d_mode g_julibrot_3d_mode{};
fractal_type g_new_orbit_type{fractal_type::JULIA};
const char *g_julibrot_3d_options[]{
    to_string(julibrot_3d_mode::MONOCULAR), //
    to_string(julibrot_3d_mode::LEFT_EYE),  //
    to_string(julibrot_3d_mode::RIGHT_EYE), //
    to_string(julibrot_3d_mode::RED_BLUE)   //
};

bool JulibrotSetup()
{
    char const *mapname;

    if (g_colors < 255)
    {
        stopmsg("Sorry, but Julibrots require a 256-color video mode");
        return false;
    }

    s_x_offset_fp = (g_x_max + g_x_min) / 2;     // Calculate average
    s_y_offset_fp = (g_y_max + g_y_min) / 2;     // Calculate average
    s_delta_mx_fp = (g_julibrot_x_max - g_julibrot_x_min) / g_julibrot_z_dots;
    s_delta_my_fp = (g_julibrot_y_max - g_julibrot_y_min) / g_julibrot_z_dots;
    g_float_param = &s_jb_c_fp;
    s_x_per_inch_fp = (g_x_min - g_x_max) / g_julibrot_width_fp;
    s_y_per_inch_fp = (g_y_max - g_y_min) / g_julibrot_height_fp;
    s_inch_per_x_dot_fp = g_julibrot_width_fp / g_logical_screen_x_dots;
    s_inch_per_y_dot_fp = g_julibrot_height_fp / g_logical_screen_y_dots;
    s_init_z_fp = g_julibrot_origin_fp - (g_julibrot_depth_fp / 2);
    if (g_julibrot_3d_mode == julibrot_3d_mode::MONOCULAR)
    {
        s_right_eye_fp.x = 0.0;
    }
    else
    {
        s_right_eye_fp.x = g_eyes_fp / 2;
    }
    s_left_eye_fp.x = -s_right_eye_fp.x;
    s_right_eye_fp.y = 0;
    s_left_eye_fp.y = s_right_eye_fp.y;
    s_right_eye_fp.zx = g_julibrot_dist_fp;
    s_left_eye_fp.zx = s_right_eye_fp.zx;
    s_right_eye_fp.zy = g_julibrot_dist_fp;
    s_left_eye_fp.zy = s_right_eye_fp.zy;
    s_b_base = 128;

    if (g_fractal_specific[+g_fractal_type].isinteger > 0)
    {
        long jxmin;
        long jxmax;
        long jymin;
        long jymax;
        long mxmax;
        long mymax;
        if (g_fractal_specific[+g_new_orbit_type].isinteger == 0)
        {
            stopmsg("Julibrot orbit type isinteger mismatch");
        }
        if (g_fractal_specific[+g_new_orbit_type].isinteger > 1)
        {
            g_bit_shift = g_fractal_specific[+g_new_orbit_type].isinteger;
        }
        s_fg = (double)(1L << g_bit_shift);
        s_fg16 = (double)(1L << 16);
        jxmin = (long)(g_x_min * s_fg);
        jxmax = (long)(g_x_max * s_fg);
        s_x_offset = (jxmax + jxmin) / 2;    // Calculate average
        jymin = (long)(g_y_min * s_fg);
        jymax = (long)(g_y_max * s_fg);
        s_y_offset = (jymax + jymin) / 2;    // Calculate average
        s_mx_min = (long)(g_julibrot_x_min * s_fg);
        mxmax = (long)(g_julibrot_x_max * s_fg);
        s_my_min = (long)(g_julibrot_y_min * s_fg);
        mymax = (long)(g_julibrot_y_max * s_fg);
        long origin = (long)(g_julibrot_origin_fp * s_fg16);
        s_depth = (long)(g_julibrot_depth_fp * s_fg16);
        s_width = (long)(g_julibrot_width_fp * s_fg16);
        s_dist = (long)(g_julibrot_dist_fp * s_fg16);
        s_eyes = (long)(g_eyes_fp * s_fg16);
        s_br_ratio = (long)(s_br_ratio_fp * s_fg16);
        s_delta_mx = (mxmax - s_mx_min) / g_julibrot_z_dots;
        s_delta_my = (mymax - s_my_min) / g_julibrot_z_dots;
        g_long_param = &s_jb_c;

        s_x_per_inch = (long)((g_x_min - g_x_max) / g_julibrot_width_fp * s_fg);
        s_y_per_inch = (long)((g_y_max - g_y_min) / g_julibrot_height_fp * s_fg);
        s_inch_per_x_dot = (long)((g_julibrot_width_fp / g_logical_screen_x_dots) * s_fg16);
        s_inch_per_y_dot = (long)((g_julibrot_height_fp / g_logical_screen_y_dots) * s_fg16);
        s_init_z = origin - (s_depth / 2);
        if (g_julibrot_3d_mode == julibrot_3d_mode::MONOCULAR)
        {
            s_right_eye.x = 0L;
        }
        else
        {
            s_right_eye.x = s_eyes / 2;
        }
        s_left_eye.x = -s_right_eye.x;
        s_right_eye.y = 0L;
        s_left_eye.y = s_right_eye.y;
        s_right_eye.zx = s_dist;
        s_left_eye.zx = s_right_eye.zx;
        s_right_eye.zy = s_dist;
        s_left_eye.zy = s_right_eye.zy;
        s_b_base = (int)(128.0 * s_br_ratio_fp);
    }

    if (g_julibrot_3d_mode == julibrot_3d_mode::RED_BLUE)
    {
        g_save_dac = 0;
        mapname = g_glasses1_map.c_str();
    }
    else
    {
        mapname = g_altern_map_file.c_str();
    }
    if (g_save_dac != 1)
    {
        if (ValidateLuts(mapname))
        {
            return false;
        }
        spindac(0, 1);               // load it, but don't spin
        if (g_save_dac == 2)
        {
            g_save_dac = 1;
        }
    }
    return true;
}


int jb_per_pixel()
{
    s_jx = multiply(s_per->x - s_x_pixel, s_init_z, 16);
    s_jx = divide(s_jx, s_dist, 16) - s_x_pixel;
    s_jx = multiply(s_jx << (g_bit_shift - 16), s_x_per_inch, g_bit_shift);
    s_jx += s_x_offset;
    s_delta_jx = divide(s_depth, s_dist, 16);
    s_delta_jx = multiply(s_delta_jx, s_per->x - s_x_pixel, 16) << (g_bit_shift - 16);
    s_delta_jx = multiply(s_delta_jx, s_x_per_inch, g_bit_shift) / g_julibrot_z_dots;

    s_jy = multiply(s_per->y - s_y_pixel, s_init_z, 16);
    s_jy = divide(s_jy, s_dist, 16) - s_y_pixel;
    s_jy = multiply(s_jy << (g_bit_shift - 16), s_y_per_inch, g_bit_shift);
    s_jy += s_y_offset;
    s_delta_jy = divide(s_depth, s_dist, 16);
    s_delta_jy = multiply(s_delta_jy, s_per->y - s_y_pixel, 16) << (g_bit_shift - 16);
    s_delta_jy = multiply(s_delta_jy, s_y_per_inch, g_bit_shift) / g_julibrot_z_dots;

    return 1;
}

int jbfp_per_pixel()
{
    s_jx_fp = ((s_per_fp->x - s_x_pixel_fp) * s_init_z_fp / g_julibrot_dist_fp - s_x_pixel_fp) * s_x_per_inch_fp;
    s_jx_fp += s_x_offset_fp;
    s_delta_jx_fp = (g_julibrot_depth_fp / g_julibrot_dist_fp) * (s_per_fp->x - s_x_pixel_fp) * s_x_per_inch_fp / g_julibrot_z_dots;

    s_jy_fp = ((s_per_fp->y - s_y_pixel_fp) * s_init_z_fp / g_julibrot_dist_fp - s_y_pixel_fp) * s_y_per_inch_fp;
    s_jy_fp += s_y_offset_fp;
    s_delta_jy_fp = g_julibrot_depth_fp / g_julibrot_dist_fp * (s_per_fp->y - s_y_pixel_fp) * s_y_per_inch_fp / g_julibrot_z_dots;

    return 1;
}

static int zpixel;
static int plotted;
static long n;

int zline(long x, long y)
{
    s_x_pixel = x;
    s_y_pixel = y;
    s_mx = s_mx_min;
    s_my = s_my_min;
    switch (g_julibrot_3d_mode)
    {
    case julibrot_3d_mode::MONOCULAR:
    case julibrot_3d_mode::LEFT_EYE:
        s_per = &s_left_eye;
        break;
    case julibrot_3d_mode::RIGHT_EYE:
        s_per = &s_right_eye;
        break;
    case julibrot_3d_mode::RED_BLUE:
        if ((g_row + g_col) & 1)
        {
            s_per = &s_left_eye;
        }
        else
        {
            s_per = &s_right_eye;
        }
        break;
    }
    jb_per_pixel();
    for (zpixel = 0; zpixel < g_julibrot_z_dots; zpixel++)
    {
        g_l_old_z.x = s_jx;
        g_l_old_z.y = s_jy;
        s_jb_c.x = s_mx;
        s_jb_c.y = s_my;
        if (driver_key_pressed())
        {
            return -1;
        }
        g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
        g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
        for (n = 0; n < g_max_iterations; n++)
        {
            if (g_fractal_specific[+g_new_orbit_type].orbitcalc())
            {
                break;
            }
        }
        if (n == g_max_iterations)
        {
            if (g_julibrot_3d_mode == julibrot_3d_mode::RED_BLUE)
            {
                g_color = (int)(128l * zpixel / g_julibrot_z_dots);
                if ((g_row + g_col) & 1)
                {

                    (*g_plot)(g_col, g_row, 127 - g_color);
                }
                else
                {
                    g_color = (int)(multiply((long) g_color << 16, s_br_ratio, 16) >> 16);
                    if (g_color < 1)
                    {
                        g_color = 1;
                    }
                    if (g_color > 127)
                    {
                        g_color = 127;
                    }
                    (*g_plot)(g_col, g_row, 127 + s_b_base - g_color);
                }
            }
            else
            {
                g_color = (int)(254l * zpixel / g_julibrot_z_dots);
                (*g_plot)(g_col, g_row, g_color + 1);
            }
            plotted = 1;
            break;
        }
        s_mx += s_delta_mx;
        s_my += s_delta_my;
        s_jx += s_delta_jx;
        s_jy += s_delta_jy;
    }
    return 0;
}

int zlinefp(double x, double y)
{
    s_x_pixel_fp = x;
    s_y_pixel_fp = y;
    s_mx_fp = g_julibrot_x_min;
    s_my_fp = g_julibrot_y_min;
    switch (g_julibrot_3d_mode)
    {
    case julibrot_3d_mode::MONOCULAR:
    case julibrot_3d_mode::LEFT_EYE:
        s_per_fp = &s_left_eye_fp;
        break;
    case julibrot_3d_mode::RIGHT_EYE:
        s_per_fp = &s_right_eye_fp;
        break;
    case julibrot_3d_mode::RED_BLUE:
        if ((g_row + g_col) & 1)
        {
            s_per_fp = &s_left_eye_fp;
        }
        else
        {
            s_per_fp = &s_right_eye_fp;
        }
        break;
    }
    jbfp_per_pixel();
    for (zpixel = 0; zpixel < g_julibrot_z_dots; zpixel++)
    {
        // Special initialization for Mandelbrot types
        if (g_new_orbit_type == fractal_type::QUATFP || g_new_orbit_type == fractal_type::HYPERCMPLXFP)
        {
            g_old_z.x = 0.0;
            g_old_z.y = 0.0;
            s_jb_c_fp.x = 0.0;
            s_jb_c_fp.y = 0.0;
            g_quaternion_c = s_jx_fp;
            g_quaternion_ci = s_jy_fp;
            g_quaternion_cj = s_mx_fp;
            g_quaternion_ck = s_my_fp;
        }
        else
        {
            g_old_z.x = s_jx_fp;
            g_old_z.y = s_jy_fp;
            s_jb_c_fp.x = s_mx_fp;
            s_jb_c_fp.y = s_my_fp;
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

        for (n = 0; n < g_max_iterations; n++)
        {
            if (g_fractal_specific[+g_new_orbit_type].orbitcalc())
            {
                break;
            }
        }
        if (n == g_max_iterations)
        {
            if (g_julibrot_3d_mode == julibrot_3d_mode::RED_BLUE)
            {
                g_color = (int)(128l * zpixel / g_julibrot_z_dots);
                if ((g_row + g_col) & 1)
                {
                    (*g_plot)(g_col, g_row, 127 - g_color);
                }
                else
                {
                    g_color = (int)(g_color * s_br_ratio_fp);
                    if (g_color < 1)
                    {
                        g_color = 1;
                    }
                    if (g_color > 127)
                    {
                        g_color = 127;
                    }
                    (*g_plot)(g_col, g_row, 127 + s_b_base - g_color);
                }
            }
            else
            {
                g_color = (int)(254l * zpixel / g_julibrot_z_dots);
                (*g_plot)(g_col, g_row, g_color + 1);
            }
            plotted = 1;
            break;
        }
        s_mx_fp += s_delta_mx_fp;
        s_my_fp += s_delta_my_fp;
        s_jx_fp += s_delta_jx_fp;
        s_jy_fp += s_delta_jy_fp;
    }
    return 0;
}

int Std4dFractal()
{
    long x;
    g_c_exponent = (int)g_params[2];
    if (g_new_orbit_type == fractal_type::LJULIAZPOWER)
    {
        if (g_c_exponent < 1)
        {
            g_c_exponent = 1;
        }
        if (g_params[3] == 0.0 && g_debug_flag != debug_flags::force_complex_power && (double)g_c_exponent == g_params[2])
        {
            g_fractal_specific[+g_new_orbit_type].orbitcalc = longZpowerFractal;
        }
        else
        {
            g_fractal_specific[+g_new_orbit_type].orbitcalc = longCmplxZpowerFractal;
        }
    }

    long y = 0;
    for (int ydot = (g_logical_screen_y_dots >> 1) - 1; ydot >= 0; ydot--, y -= s_inch_per_y_dot)
    {
        plotted = 0;
        x = -(s_width >> 1);
        for (int xdot = 0; xdot < g_logical_screen_x_dots; xdot++, x += s_inch_per_x_dot)
        {
            g_col = xdot;
            g_row = ydot;
            if (zline(x, y) < 0)
            {
                return -1;
            }
            g_col = g_logical_screen_x_dots - g_col - 1;
            g_row = g_logical_screen_y_dots - g_row - 1;
            if (zline(-x, -y) < 0)
            {
                return -1;
            }
        }
        if (plotted == 0)
        {
            if (y == 0)
            {
                plotted = -1;  // no points first pass; don't give up
            }
            else
            {
                break;
            }
        }
    }
    return 0;
}

int Std4dfpFractal()
{
    double x;
    g_c_exponent = (int)g_params[2];

    if (g_new_orbit_type == fractal_type::FPJULIAZPOWER)
    {
        if (g_params[3] == 0.0 && g_debug_flag != debug_flags::force_complex_power && (double)g_c_exponent == g_params[2])
        {
            g_fractal_specific[+g_new_orbit_type].orbitcalc = floatZpowerFractal;
        }
        else
        {
            g_fractal_specific[+g_new_orbit_type].orbitcalc = floatCmplxZpowerFractal;
        }
        get_julia_attractor(g_params[0], g_params[1]);  // another attractor?
    }

    double y = 0.0;
    for (int ydot = (g_logical_screen_y_dots >> 1) - 1; ydot >= 0; ydot--, y -= s_inch_per_y_dot_fp)
    {
        plotted = 0;
        x = -g_julibrot_width_fp / 2;
        for (int xdot = 0; xdot < g_logical_screen_x_dots; xdot++, x += s_inch_per_x_dot_fp)
        {
            g_col = xdot;
            g_row = ydot;
            if (zlinefp(x, y) < 0)
            {
                return -1;
            }
            g_col = g_logical_screen_x_dots - g_col - 1;
            g_row = g_logical_screen_y_dots - g_row - 1;
            if (zlinefp(-x, -y) < 0)
            {
                return -1;
            }
        }
        if (plotted == 0)
        {
            if (y == 0)
            {
                plotted = -1;  // no points first pass; don't give up
            }
            else
            {
                break;
            }
        }
    }
    return 0;
}
