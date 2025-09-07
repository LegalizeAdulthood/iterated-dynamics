// SPDX-License-Identifier: GPL-3.0-only
//
// This file contains the "big number" high precision versions of the
// fractal routines.

#include "engine/fractalb.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/type_has_param.h"
#include "fractals/divide_brot.h"
#include "fractals/fractalp.h"
#include "fractals/frasetup.h"
#include "math/biginit.h"
#include "math/fixed_pt.h"
#include "misc/id.h"
#include "ui/goodbye.h"
#include "ui/stop_msg.h"

#include <fmt/format.h>

#include <algorithm>
#include <array> // std::size
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using namespace id;

BFMathType g_bf_math{};

#ifndef NDEBUG
//********************************************************************
std::string bn_to_string(BigNum n, int dec)
{
    char msg[200];
    bn_to_str(msg, n, dec);
    return msg;
}

void show_var_bn(const char *s, BigNum n)
{
    std::string msg{s};
    msg += ' ' + bn_to_string(n, 40);
    msg.erase(79); // limit to 79 characters
    stop_msg(msg);
}

void show_corners_dbl(const char *s)
{
    if (stop_msg(fmt::format("{:s}\n"                                 //
                             "   x_min= {:.20f}    x_max= {:.20f}\n"  //
                             "   y_min= {:.20f}    y_max= {:.20f}\n"  //
                             "   x_3rd= {:.20f}    y_3rd= {:.20f}\n"  //
                             " delta_x= {:.20Lf} delta_y= {:.20Lf}\n" //
                             "delta_x2= {:.20Lf} delta_y2= {:.20Lf}", //
            s,                                                        //
            g_x_min, g_x_max,                                         //
            g_y_min, g_y_max,                                         //
            g_x_3rd, g_y_3rd,                                         //
            g_delta_x, g_delta_y,                                     //
            g_delta_x2, g_delta_y2)))
    {
        goodbye();
    }
}

// show floating point and bignumber corners
void show_corners_bn(const char *s)
{
    constexpr int dec = 20;
    if (stop_msg(fmt::format("{:s}\n"
                             "g_x_min_bn={:s}\n"
                             "g_x_min   ={:.20f}\n"
                             "\n"
                             "g_x_max_bn={:s}\n"
                             "g_x_max   ={:.20f}\n"
                             "\n"
                             "g_y_min_bn={:s}\n"
                             "g_y_min   ={:.20f}\n"
                             "\n"
                             "g_y_max_bn={:s}\n"
                             "g_y_max   ={:.20f}\n"
                             "\n"
                             "g_x_3rd_bn={:s}\n"
                             "g_x_3rd   ={:.20f}\n"
                             "\n"
                             "g_y_3rd_bn={:s}\n"
                             "g_y_3rd   ={:.20f}\n"
                             "\n",
            s,                                      //
            bn_to_string(g_x_min_bn, dec), g_x_min, //
            bn_to_string(g_x_max_bn, dec), g_x_max, //
            bn_to_string(g_y_min_bn, dec), g_y_min, //
            bn_to_string(g_y_max_bn, dec), g_y_max, //
            bn_to_string(g_x_3rd_bn, dec), g_x_3rd, //
            bn_to_string(g_y_3rd_bn, dec), g_y_3rd)))
    {
        goodbye();
    }
}

// show globals
void show_globals_bf(const char *s)
{
    if (stop_msg(fmt::format(
            "{:s}\n"                                                                             //
            "g_bn_step={:d} g_bn_length={:d} g_int_length={:d} g_r_length={:d} g_padding={:d}\n" //
            "g_shift_factor={:d} decimals={:d} g_bf_length={:d} g_r_bf_length={:d}\n"            //
            "g_bf_decimals={:d} ",                                                               //
            s,                                                                                   //
            g_bn_step, g_bn_length, g_int_length, g_r_length, g_padding,                         //
            g_shift_factor, g_decimals, g_bf_length, g_r_bf_length,                              //
            g_bf_decimals)))
    {
        goodbye();
    }
}

static std::string bf_to_string(BigFloat g_bf_x_min, int dec)
{
    char msg[100];
    bf_to_str(msg, g_bf_x_min, dec);
    return msg;
}

void show_corners_bf(const char *s)
{
    const int dec = std::min(g_decimals, 20);
    if (stop_msg(fmt::format("{:s}\n"
                             "bf_x_min={:s}\n"
                             "x_min= {:.20f} decimals {:d} g_bf_length {:d}\n"
                             "\n"
                             "bf_x_max={:s}\n"
                             "x_max= {:.20f}\n"
                             "\n"
                             "bf_y_min={:s}\n"
                             "y_min= {:.20f}\n"
                             "\n"
                             "bf_y_max={:s}\n"
                             "y_max= {:.20f}\n"
                             "\n"
                             "bf_x_3rd={:s}\n"
                             "xx_3rd= {:.20f}\n"
                             "\n"
                             "bf_y_3rd={:s}\n"
                             "y_3rd= {:.20f}\n"
                             "\n",
            s,                                              //
            bf_to_string(g_bf_x_min, dec).c_str(),          //
            g_x_min, g_decimals, g_bf_length,               //
            bf_to_string(g_bf_x_max, dec).c_str(), g_x_max, //
            bf_to_string(g_bf_y_min, dec).c_str(), g_y_min, //
            bf_to_string(g_bf_y_max, dec).c_str(), g_y_max, //
            bf_to_string(g_bf_x_3rd, dec).c_str(), g_x_3rd, //
            bf_to_string(g_bf_y_3rd, dec).c_str(), g_y_3rd)))
    {
        goodbye();
    }
}

void show_corners_bf_save(const char *s)
{
    constexpr int dec = 20;
    if (stop_msg(fmt::format(
        "{:s}\n"
        "bf_save_x_min={:s}\n"
        "x_min= {:.20f}\n"
        "\n"
        "bf_save_x_max={:s}\n"
        "x_max= {:.20f}\n"
        "\n"
        "bf_save_y_min={:s}\n"
        "y_min= {:.20f}\n"
        "\n"
        "bf_save_y_max={:s}\n"
        "y_max= {:.20f}\n"
        "\n"
        "bf_save_x_3rd={:s}\n"
        "x_3rd= {:.20f}\n"
        "\n"
        "bf_save_y_3rd={:s}\n"
        "y_3rd= {:.20f}\n"
        "\n",
        s,                                                   //
        bf_to_string(g_bf_save_x_min, dec).c_str(), g_x_min, //
        bf_to_string(g_bf_save_x_max, dec).c_str(), g_x_max, //
        bf_to_string(g_bf_save_y_min, dec).c_str(), g_y_min, //
        bf_to_string(g_bf_save_y_max, dec).c_str(), g_y_max, //
        bf_to_string(g_bf_save_x_3rd, dec).c_str(), g_x_3rd, //
        bf_to_string(g_bf_save_y_3rd, dec).c_str(), g_y_3rd)))
    {
        goodbye();
    }
}

static std::string bf_to_string_e(BigFloat value, int dec)
{
    char msg[100];
    bf_to_str_e(msg, value, dec);
    return msg;
}

void show_two_bf(const char *s1, BigFloat t1, const char *s2, BigFloat t2, int digits)
{
    if (stop_msg(fmt::format("\n"
                             "{:s}->{:s}\n"
                             "{:s}->{:s}",
            s1, bf_to_string_e(t1, digits), //
            s2, bf_to_string_e(t2, digits))))
    {
        goodbye();
    }
}

void show_three_bf(
    const char *s1, BigFloat t1, const char *s2, BigFloat t2, const char *s3, BigFloat t3, int digits)
{
    if (stop_msg(fmt::format("\n"
                             "{:s}->{:s}\n"
                             "{:s}->{:s}\n"
                             "{:s}->{:s}",
            s1, bf_to_string_e(t1, digits), //
            s2, bf_to_string_e(t2, digits), //
            s3, bf_to_string_e(t3, digits))))
    {
        goodbye();
    }
}

// for aspect ratio debugging
void show_aspect(const char *s)
{
    BigStackSaver saved;
    BigFloat bt1 = alloc_stack(g_r_bf_length + 2);
    BigFloat bt2 = alloc_stack(g_r_bf_length + 2);
    BigFloat aspect = alloc_stack(g_r_bf_length + 2);
    sub_bf(bt1, g_bf_x_max, g_bf_x_min);
    sub_bf(bt2, g_bf_y_max, g_bf_y_min);
    div_bf(aspect, bt2, bt1);
    if (stop_msg(fmt::format("aspect {:s}\n"
                             "float  {:13.10f}\n"
                             "bf     {:s}\n"
                             "\n",
            s,                                         //
            (g_y_max - g_y_min) / (g_x_max - g_x_min), //
            bf_to_string(aspect, 10))))
    {
        goodbye();
    }
}

// compare a double and bignumber
void compare_values(const char *s, LDouble x, BigNum bnx)
{
    constexpr int dec = 40;
    if (stop_msg(fmt::format("{:s}\n"
                             "bignum={:s}\n"
                             "double={:.20Lf}\n"
                             "\n",
            s,                      //
            bn_to_string(bnx, dec), //
            x)))
    {
        goodbye();
    }
}

// compare a double and bignumber
void compare_values_bf(const char *s, LDouble x, BigFloat bfx)
{
    constexpr int dec = 40;
    if (stop_msg(fmt::format("{:s}\n"
                             "bignum={:s}\n"
                             "double={:.20Lf}\n"
                             "\n",
            s,                        //
            bf_to_string_e(bfx, dec), //
            x)))
    {
        goodbye();
    }
}

//********************************************************************
void show_var_bf(const char *s, BigFloat n)
{
    std::string msg{bf_to_string_e(n, 40)};
    msg.erase(79);
    if (stop_msg(msg))
    {
        goodbye();
    }
}
#endif

void bf_corners_to_float()
{
    if (g_bf_math != BFMathType::NONE)
    {
        g_x_min = (double)bf_to_float(g_bf_x_min);
        g_y_min = (double)bf_to_float(g_bf_y_min);
        g_x_max = (double)bf_to_float(g_bf_x_max);
        g_y_max = (double)bf_to_float(g_bf_y_max);
        g_x_3rd = (double)bf_to_float(g_bf_x_3rd);
        g_y_3rd = (double)bf_to_float(g_bf_y_3rd);
    }
    for (int i = 0; i < MAX_PARAMS; i++)
    {
        if (type_has_param(g_fractal_type, i))
        {
            g_params[i] = (double)bf_to_float(g_bf_params[i]);
        }
    }
}

bool mandel_per_image_bn()
{
    BigStackSaver saved;
    // this should be set up dynamically based on corners
    BigNum bn_temp1 = alloc_stack(g_bn_length);
    BigNum bn_temp2 = alloc_stack(g_bn_length);

    bf_to_bn(g_x_min_bn, g_bf_x_min);
    bf_to_bn(g_x_max_bn, g_bf_x_max);
    bf_to_bn(g_y_min_bn, g_bf_y_min);
    bf_to_bn(g_y_max_bn, g_bf_y_max);
    bf_to_bn(g_x_3rd_bn, g_bf_x_3rd);
    bf_to_bn(g_y_3rd_bn, g_bf_y_3rd);

    g_bf_math = BFMathType::BIG_NUM;

    // g_delta_x_bn = (g_x_max_bn - g_x_3rd_bn)/(xdots-1)
    sub_bn(g_delta_x_bn, g_x_max_bn, g_x_3rd_bn);
    div_a_bn_int(g_delta_x_bn, (U16) (g_logical_screen_x_dots - 1));

    // g_delta_y_bn = (g_y_max_bn - g_y_3rd_bn)/(ydots-1)
    sub_bn(g_delta_y_bn, g_y_max_bn, g_y_3rd_bn);
    div_a_bn_int(g_delta_y_bn, (U16) (g_logical_screen_y_dots - 1));

    // g_delta2_x_bn = (g_x_3rd_bn - g_x_min_bn)/(ydots-1)
    sub_bn(g_delta2_x_bn, g_x_3rd_bn, g_x_min_bn);
    div_a_bn_int(g_delta2_x_bn, (U16) (g_logical_screen_y_dots - 1));

    // g_delta2_y_bn = (g_y_3rd_bn - g_y_min_bn)/(xdots-1)
    sub_bn(g_delta2_y_bn, g_y_3rd_bn, g_y_min_bn);
    div_a_bn_int(g_delta2_y_bn, (U16) (g_logical_screen_x_dots - 1));

    abs_bn(g_close_enough_bn, g_delta_x_bn);
    if (cmp_bn(abs_bn(bn_temp1, g_delta2_x_bn), g_close_enough_bn) > 0)
    {
        copy_bn(g_close_enough_bn, bn_temp1);
    }
    if (cmp_bn(abs_bn(bn_temp1, g_delta_y_bn), abs_bn(bn_temp2, g_delta2_y_bn)) > 0)
    {
        if (cmp_bn(bn_temp1, g_close_enough_bn) > 0)
        {
            copy_bn(g_close_enough_bn, bn_temp1);
        }
    }
    else if (cmp_bn(bn_temp2, g_close_enough_bn) > 0)
    {
        copy_bn(g_close_enough_bn, bn_temp2);
    }
    {
        int t = std::abs(g_periodicity_check);
        while (t--)
        {
            half_a_bn(g_close_enough_bn);
        }
    }

    if (g_std_calc_mode == CalcMode::PERTURBATION && bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
    {
        mandel_perturbation_per_image();
        // TODO: figure out crash if we don't do this
        g_std_calc_mode = CalcMode::SOLID_GUESS;
        g_calc_status = CalcStatus::COMPLETED;
        return true;
    }

    g_c_exponent = (int) g_params[2];
    switch (g_fractal_type)
    {
    case FractalType::JULIA:
        bf_to_bn(g_param_z_bn.x, g_bf_params[0]);
        bf_to_bn(g_param_z_bn.y, g_bf_params[1]);
        break;

    case FractalType::MANDEL_Z_POWER:
        init_big_pi();
        if ((double) g_c_exponent == g_params[2] && (g_c_exponent & 1)) // odd exponents
        {
            g_symmetry = SymmetryType::XY_AXIS_NO_PARAM;
        }
        if (g_params[3] != 0)
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;

    case FractalType::JULIA_Z_POWER:
        init_big_pi();
        bf_to_bn(g_param_z_bn.x, g_bf_params[0]);
        bf_to_bn(g_param_z_bn.y, g_bf_params[1]);
        if ((g_c_exponent & 1) || g_params[3] != 0.0 || (double) g_c_exponent != g_params[2])
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;

    case FractalType::DIVIDE_BROT5:
        init_big_pi();
        g_c_exponent = -((int) g_params[0] - 2); /* use negative here so only need it once */
        g_b_const = g_params[1] + 1.0e-20;
        break;

    default:
        break;
    }

    return true;
}

bool mandel_per_image_bf()
{
    // I suspect the following code should be somewhere in perform_worklist() to reset the setup routine to
    // floating point when zooming out. Somehow the math type is restored and the bigflt memory restored, but
    // the pointer to set up isn't.
    if (g_bf_math == BFMathType::NONE)
    {
        // kludge to prevent crash when math type = NONE and still call bigflt setup routine
        return mandel_per_image();
    }

    // this should be set up dynamically based on corners
    BigStackSaver saved;
    BigFloat bf_temp1{alloc_stack(g_bf_length + 2)};
    BigFloat bf_temp2{alloc_stack(g_bf_length + 2)};

    g_bf_math = BFMathType::BIG_FLT;

    // g_delta_x_bf = (g_bf_x_max - g_bf_x_3rd)/(xdots-1)
    sub_bf(g_delta_x_bf, g_bf_x_max, g_bf_x_3rd);
    div_a_bf_int(g_delta_x_bf, (U16)(g_logical_screen_x_dots - 1));

    // g_delta_y_bf = (g_bf_y_max - g_bf_y_3rd)/(ydots-1)
    sub_bf(g_delta_y_bf, g_bf_y_max, g_bf_y_3rd);
    div_a_bf_int(g_delta_y_bf, (U16)(g_logical_screen_y_dots - 1));

    // g_delta2_x_bf = (g_bf_x_3rd - g_bf_x_min)/(ydots-1)
    sub_bf(g_delta2_x_bf, g_bf_x_3rd, g_bf_x_min);
    div_a_bf_int(g_delta2_x_bf, (U16)(g_logical_screen_y_dots - 1));

    // g_delta2_y_bf = (g_bf_y_3rd - g_bf_y_min)/(xdots-1)
    sub_bf(g_delta2_y_bf, g_bf_y_3rd, g_bf_y_min);
    div_a_bf_int(g_delta2_y_bf, (U16)(g_logical_screen_x_dots - 1));

    abs_bf(g_close_enough_bf, g_delta_x_bf);
    if (cmp_bf(abs_bf(bf_temp1, g_delta2_x_bf), g_close_enough_bf) > 0)
    {
        copy_bf(g_close_enough_bf, bf_temp1);
    }
    if (cmp_bf(abs_bf(bf_temp1, g_delta_y_bf), abs_bf(bf_temp2, g_delta2_y_bf)) > 0)
    {
        if (cmp_bf(bf_temp1, g_close_enough_bf) > 0)
        {
            copy_bf(g_close_enough_bf, bf_temp1);
        }
    }
    else if (cmp_bf(bf_temp2, g_close_enough_bf) > 0)
    {
        copy_bf(g_close_enough_bf, bf_temp2);
    }
    {
        int t{std::abs(g_periodicity_check)};
        while (t--)
        {
            half_a_bf(g_close_enough_bf);
        }
    }

    // floating point code could probably be altered to handle many of
    // the situations that otherwise are using standard_fractal().
    // calcmandfp() can currently handle invert, any rqlim, potflag
    // zmag, epsilon cross, and all the current outside options
    g_c_exponent = (int)g_params[2];
    switch (g_fractal_type)
    {
    case FractalType::MANDEL:
    case FractalType::BURNING_SHIP:
        if (g_std_calc_mode == CalcMode::PERTURBATION &&
            bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
        {
            return mandel_perturbation_per_image();
        }
        break;

    case FractalType::JULIA:
        copy_bf(g_param_z_bf.x, g_bf_params[0]);
        copy_bf(g_param_z_bf.y, g_bf_params[1]);
        break;

    case FractalType::MANDEL_Z_POWER:
        if (g_std_calc_mode == CalcMode::PERTURBATION &&
            bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
        {
            // only allow integer values of real part
            if (const int degree = (int) g_params[2]; degree > 2)
            {
                return mandel_z_power_perturbation_per_image();
            }
            else if (degree == 2)  // NOLINT(readability-else-after-return)
            {
                return mandel_perturbation_per_image();
            }
        }

        init_big_pi();
        if ((double) g_c_exponent == g_params[2] && (g_c_exponent & 1)) // odd exponents
        {
            g_symmetry = SymmetryType::XY_AXIS_NO_PARAM;
        }
        if (g_params[3] != 0)
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;

    case FractalType::JULIA_Z_POWER:
        init_big_pi();
        copy_bf(g_param_z_bf.x, g_bf_params[0]);
        copy_bf(g_param_z_bf.y, g_bf_params[1]);
        if ((g_c_exponent & 1) || g_params[3] != 0.0 || (double)g_c_exponent != g_params[2])
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;

    case FractalType::DIVIDE_BROT5:
        init_big_pi();
        g_c_exponent = -((int) g_params[0] - 2); /* use negative here so only need it once */
        g_b_const = g_params[1] + 1.0e-20;
        break;

    default:
        break;
    }

    return true;
}

int mandel_per_pixel_bn()
{
    if (g_std_calc_mode == CalcMode::PERTURBATION &&
        bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
    {
        return true;
    }
    // parm.x = g_x_min + col*delx + row*delx2
    mult_bn_int(g_param_z_bn.x, g_delta_x_bn, (U16)g_col);
    mult_bn_int(g_bn_tmp, g_delta2_x_bn, (U16)g_row);

    add_a_bn(g_param_z_bn.x, g_bn_tmp);
    add_a_bn(g_param_z_bn.x, g_x_min_bn);

    // parm.y = g_y_max - row*dely - col*dely2;
    // note: in next four lines, g_old_z_bn is just used as a temporary variable
    mult_bn_int(g_old_z_bn.x, g_delta_y_bn, (U16)g_row);
    mult_bn_int(g_old_z_bn.y, g_delta2_y_bn, (U16)g_col);
    add_a_bn(g_old_z_bn.x, g_old_z_bn.y);
    sub_bn(g_param_z_bn.y, g_y_max_bn, g_old_z_bn.x);

    copy_bn(g_old_z_bn.x, g_param_z_bn.x);
    copy_bn(g_old_z_bn.y, g_param_z_bn.y);

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        float_to_bn(g_old_z_bn.x, g_params[0]); // initial perturbation of parameters set
        float_to_bn(g_old_z_bn.y, g_params[1]);
        g_color_iter = -1;
    }
    else
    {
        float_to_bn(g_new_z_bn.x, g_params[0]);
        float_to_bn(g_new_z_bn.y, g_params[1]);
        add_a_bn(g_old_z_bn.x, g_new_z_bn.x);
        add_a_bn(g_old_z_bn.y, g_new_z_bn.y);
    }

    // square has side effect - must copy first
    copy_bn(g_new_z_bn.x, g_old_z_bn.x);
    copy_bn(g_new_z_bn.y, g_old_z_bn.y);

    // Square these to g_r_length bytes of precision
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);

    return 1;                  // 1st iteration has been done
}

int mandel_per_pixel_bf()
{
    // I suspect the following code should be somewhere in perform_worklist() to reset the setup routine to
    // floating point when zooming out. Somehow the math type is restored and the bigflt memory restored, but
    // the pointer to set up isn't.
    if (g_bf_math == BFMathType::NONE) // kludge to prevent crash when math type = NONE and still call bigflt setup routine
    {
        return mandel_per_pixel();
    }
    // parm.x = g_x_min + col*delx + row*delx2
    mult_bf_int(g_param_z_bf.x, g_delta_x_bf, (U16)g_col);
    mult_bf_int(g_bf_tmp, g_delta2_x_bf, (U16)g_row);

    add_a_bf(g_param_z_bf.x, g_bf_tmp);
    add_a_bf(g_param_z_bf.x, g_bf_x_min);

    // parm.y = g_y_max - row*dely - col*dely2;
    // note: in next four lines, g_old_z_bf is just used as a temporary variable
    mult_bf_int(g_old_z_bf.x, g_delta_y_bf, (U16)g_row);
    mult_bf_int(g_old_z_bf.y, g_delta2_y_bf, (U16)g_col);
    add_a_bf(g_old_z_bf.x, g_old_z_bf.y);
    sub_bf(g_param_z_bf.y, g_bf_y_max, g_old_z_bf.x);

    copy_bf(g_old_z_bf.x, g_param_z_bf.x);
    copy_bf(g_old_z_bf.y, g_param_z_bf.y);

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        float_to_bf(g_old_z_bf.x, g_params[0]); // initial perturbation of parameters set
        float_to_bf(g_old_z_bf.y, g_params[1]);
        g_color_iter = -1;
    }
    else
    {
        float_to_bf(g_new_z_bf.x, g_params[0]);
        float_to_bf(g_new_z_bf.y, g_params[1]);
        add_a_bf(g_old_z_bf.x, g_new_z_bf.x);
        add_a_bf(g_old_z_bf.y, g_new_z_bf.y);
    }

    // square has side effect - must copy first
    copy_bf(g_new_z_bf.x, g_old_z_bf.x);
    copy_bf(g_new_z_bf.y, g_old_z_bf.y);

    // Square these to g_r_bf_length bytes of precision
    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);

    return 1;                  // 1st iteration has been done
}

int julia_per_pixel_bn()
{
    // old.x = g_x_min + col*delx + row*delx2
    mult_bn_int(g_old_z_bn.x, g_delta_x_bn, (U16)g_col);
    mult_bn_int(g_bn_tmp, g_delta2_x_bn, (U16)g_row);

    add_a_bn(g_old_z_bn.x, g_bn_tmp);
    add_a_bn(g_old_z_bn.x, g_x_min_bn);

    // old.y = g_y_max - row*dely - col*dely2;
    // note: in next four lines, g_new_z_bn is just used as a temporary variable
    mult_bn_int(g_new_z_bn.x, g_delta_y_bn, (U16)g_row);
    mult_bn_int(g_new_z_bn.y, g_delta2_y_bn, (U16)g_col);
    add_a_bn(g_new_z_bn.x, g_new_z_bn.y);
    sub_bn(g_old_z_bn.y, g_y_max_bn, g_new_z_bn.x);

    // square has side effect - must copy first
    copy_bn(g_new_z_bn.x, g_old_z_bn.x);
    copy_bn(g_new_z_bn.y, g_old_z_bn.y);

    // Square these to g_r_length bytes of precision
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);

    return 1;                  // 1st iteration has been done
}

int julia_per_pixel_bf()
{
    // old.x = g_x_min + col*delx + row*delx2
    mult_bf_int(g_old_z_bf.x, g_delta_x_bf, (U16)g_col);
    mult_bf_int(g_bf_tmp, g_delta2_x_bf, (U16)g_row);

    add_a_bf(g_old_z_bf.x, g_bf_tmp);
    add_a_bf(g_old_z_bf.x, g_bf_x_min);

    // old.y = g_y_max - row*dely - col*dely2;
    // note: in next four lines, g_new_z_bf is just used as a temporary variable
    mult_bf_int(g_new_z_bf.x, g_delta_y_bf, (U16)g_row);
    mult_bf_int(g_new_z_bf.y, g_delta2_y_bf, (U16)g_col);
    add_a_bf(g_new_z_bf.x, g_new_z_bf.y);
    sub_bf(g_old_z_bf.y, g_bf_y_max, g_new_z_bf.x);

    // square has side effect - must copy first
    copy_bf(g_new_z_bf.x, g_old_z_bf.x);
    copy_bf(g_new_z_bf.y, g_old_z_bf.y);

    // Square these to g_r_bf_length bytes of precision
    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);

    return 1;                  // 1st iteration has been done
}

int julia_orbit_bn()
{
    // Don't forget, with BigNum numbers, after multiplying or squaring
    // you must shift over by g_shift_factor to get the bn number.

    // g_tmp_sqr_x_bn and g_tmp_sqr_y_bn were previously squared before getting to
    // this function, so they must be shifted.

    // new.x = tmpsqrx - tmpsqry + parm.x;
    sub_a_bn(g_tmp_sqr_x_bn+g_shift_factor, g_tmp_sqr_y_bn+g_shift_factor);
    add_bn(g_new_z_bn.x, g_tmp_sqr_x_bn+g_shift_factor, g_param_z_bn.x);

    // new.y = 2 * g_old_z_bn.x * g_old_z_bn.y + parm.y;
    mult_bn(g_bn_tmp, g_old_z_bn.x, g_old_z_bn.y); // ok to use unsafe here
    double_a_bn(g_bn_tmp+g_shift_factor);
    add_bn(g_new_z_bn.y, g_bn_tmp+g_shift_factor, g_param_z_bn.y);

    return id::g_bailout_bignum();
}

int julia_orbit_bf()
{
    // new.x = tmpsqrx - tmpsqry + parm.x;
    sub_a_bf(g_tmp_sqr_x_bf, g_tmp_sqr_y_bf);
    add_bf(g_new_z_bf.x, g_tmp_sqr_x_bf, g_param_z_bf.x);

    // new.y = 2 * g_old_z_bf.x * g_old_z_bf.y + parm.y;
    mult_bf(g_bf_tmp, g_old_z_bf.x, g_old_z_bf.y); // ok to use unsafe here
    double_a_bf(g_bf_tmp);
    add_bf(g_new_z_bf.y, g_bf_tmp, g_param_z_bf.y);
    return id::g_bailout_bigfloat();
}

int julia_z_power_bn_fractal()
{
    BNComplex param2;
    int saved = save_stack();

    param2.x = alloc_stack(g_bn_length);
    param2.y = alloc_stack(g_bn_length);

    float_to_bn(param2.x, g_params[2]);
    float_to_bn(param2.y, g_params[3]);
    cmplx_pow_bn(&g_new_z_bn, &g_old_z_bn, &param2);
    add_bn(g_new_z_bn.x, g_param_z_bn.x, g_new_z_bn.x+g_shift_factor);
    add_bn(g_new_z_bn.y, g_param_z_bn.y, g_new_z_bn.y+g_shift_factor);
    restore_stack(saved);
    return id::g_bailout_bignum();
}

int julia_z_power_orbit_bf()
{
    BFComplex param2;
    int saved = save_stack();

    param2.x = alloc_stack(g_bf_length+2);
    param2.y = alloc_stack(g_bf_length+2);

    float_to_bf(param2.x, g_params[2]);
    float_to_bf(param2.y, g_params[3]);
    cmplx_pow_bf(&g_new_z_bf, &g_old_z_bf, &param2);
    add_bf(g_new_z_bf.x, g_param_z_bf.x, g_new_z_bf.x);
    add_bf(g_new_z_bf.y, g_param_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return id::g_bailout_bigfloat();
}

BFComplex *cmplx_log_bf(BFComplex *t, BFComplex *s)
{
    if (is_bf_zero(s->x) && is_bf_zero(s->y))
    {
        clear_bf(t->x);
        clear_bf(t->y);
    }
    else
    {
        square_bf(t->x, s->x);
        square_bf(t->y, s->y);
        add_a_bf(t->x, t->y);
        ln_bf(t->x, t->x);
        half_a_bf(t->x);
        atan2_bf(t->y, s->y, s->x);
    }
    return t;
}

BFComplex *cmplx_mul_bf(BFComplex *t, BFComplex *x, BFComplex *y)
{
    int saved = save_stack();
    BigFloat tmp1 = alloc_stack(g_r_bf_length + 2);
    mult_bf(t->x, x->x, y->x);
    mult_bf(t->y, x->y, y->y);
    sub_bf(t->x, t->x, t->y);

    mult_bf(tmp1, x->x, y->y);
    mult_bf(t->y, x->y, y->x);
    add_bf(t->y, tmp1, t->y);
    restore_stack(saved);
    return t;
}

BFComplex *cmplx_div_bf(BFComplex *t, BFComplex *x, BFComplex *y)
{
    int saved = save_stack();
    BigFloat tmp1 = alloc_stack(g_r_bf_length + 2);
    BigFloat denom = alloc_stack(g_r_bf_length + 2);

    square_bf(t->x, y->x);
    square_bf(t->y, y->y);
    add_bf(denom, t->x, t->y);

    if (is_bf_zero(denom))
    {
        g_overflow = true;
    }
    else
    {
        mult_bf(tmp1, x->x, y->x);
        mult_bf(t->x, x->y, y->y);
        add_bf(t->x, tmp1, t->x);
        div_bf(t->x, t->x, denom);

        mult_bf(tmp1, x->y, y->x);
        mult_bf(t->y, x->x, y->y);
        sub_bf(t->y, tmp1, t->y);
        div_bf(t->y, t->y, denom);
    }

    restore_stack(saved);
    return t;
}

BFComplex *cmplx_pow_bf(BFComplex *t, BFComplex *xx, BFComplex *yy)
{
    BFComplex tmp;
    int saved = save_stack();
    BigFloat e2x = alloc_stack(g_r_bf_length + 2);
    BigFloat sin_y = alloc_stack(g_r_bf_length + 2);
    BigFloat cos_y = alloc_stack(g_r_bf_length + 2);
    tmp.x = alloc_stack(g_r_bf_length+2);
    tmp.y = alloc_stack(g_r_bf_length+2);

    // 0 raised to anything is 0
    if (is_bf_zero(xx->x) && is_bf_zero(xx->y))
    {
        clear_bf(t->x);
        clear_bf(t->y);
        return t;
    }

    cmplx_log_bf(t, xx);
    cmplx_mul_bf(&tmp, t, yy);
    exp_bf(e2x, tmp.x);
    sin_cos_bf(sin_y, cos_y, tmp.y);
    mult_bf(t->x, e2x, cos_y);
    mult_bf(t->y, e2x, sin_y);
    restore_stack(saved);
    return t;
}

BNComplex *cmplx_log_bn(BNComplex *t, BNComplex *s)
{
    if (is_bn_zero(s->x) && is_bn_zero(s->y))
    {
        clear_bn(t->x);
        clear_bn(t->y);
    }
    else
    {
        add_bn(t->x, t->x + g_shift_factor, t->y + g_shift_factor);
        ln_bn(t->x, t->x);
        half_a_bn(t->x);
        atan2_bn(t->y, s->y, s->x);
    }
    return t;
}

BNComplex *cmplx_mul_bn(BNComplex *t, BNComplex *x, BNComplex *y)
{
    int saved = save_stack();
    BigNum tmp1 = alloc_stack(g_r_length);
    mult_bn(t->x, x->x, y->x);
    mult_bn(t->y, x->y, y->y);
    sub_bn(t->x, t->x + g_shift_factor, t->y + g_shift_factor);

    mult_bn(tmp1, x->x, y->y);
    mult_bn(t->y, x->y, y->x);
    add_bn(t->y, tmp1 + g_shift_factor, t->y + g_shift_factor);
    restore_stack(saved);
    return t;
}

BNComplex *cmplx_div_bn(BNComplex *t, BNComplex *x, BNComplex *y)
{
    int saved = save_stack();
    BigNum tmp1 = alloc_stack(g_r_length);
    BigNum tmp2 = alloc_stack(g_r_length);
    BigNum denom = alloc_stack(g_r_length);

    square_bn(tmp1, y->x);
    square_bn(tmp2, y->y);
    add_bn(denom, tmp1 + g_shift_factor, tmp2 + g_shift_factor);

    if (is_bn_zero(x->x) && is_bn_zero(x->y))
    {
        clear_bn(t->x);
        clear_bn(t->y);
    }
    else if (is_bn_zero(denom))
    {
        g_overflow = true;
    }
    else
    {
        mult_bn(tmp1, x->x, y->x);
        mult_bn(t->x, x->y, y->y);
        add_bn(tmp2, tmp1 + g_shift_factor, t->x + g_shift_factor);
        div_bn(t->x, tmp2, denom);

        mult_bn(tmp1, x->y, y->x);
        mult_bn(t->y, x->x, y->y);
        sub_bn(tmp2, tmp1 + g_shift_factor, t->y + g_shift_factor);
        div_bn(t->y, tmp2, denom);
    }

    restore_stack(saved);
    return t;
}

// note: ComplexPower_bn() returns need to be +g_shift_factor'ed
BNComplex *cmplx_pow_bn(BNComplex *t, BNComplex *xx, BNComplex *yy)
{
    BNComplex tmp;
    int saved = save_stack();
    BigNum e2x = alloc_stack(g_r_length);
    BigNum sin_y = alloc_stack(g_r_length);
    BigNum cos_y = alloc_stack(g_r_length);
    tmp.x = alloc_stack(g_r_length);
    tmp.y = alloc_stack(g_r_length);

    // 0 raised to anything is 0
    if (is_bn_zero(xx->x) && is_bn_zero(xx->y))
    {
        clear_bn(t->x);
        clear_bn(t->y);
        restore_stack(saved);
        return t;
    }

    cmplx_log_bn(t, xx);
    cmplx_mul_bn(&tmp, t, yy);
    exp_bn(e2x, tmp.x);
    sin_cos_bn(sin_y, cos_y, tmp.y);
    mult_bn(t->x, e2x, cos_y);
    mult_bn(t->y, e2x, sin_y);
    restore_stack(saved);
    return t;
}
