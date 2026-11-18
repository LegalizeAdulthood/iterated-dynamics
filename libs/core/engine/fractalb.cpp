// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// This file contains the "big number" high precision versions of the
// fractal routines.

#include <engine/fractalb.h>

#include <engine/bailout_formula.h>
#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/type_has_param.h>
#include <fractals/divide_brot.h>
#include <fractals/fractalp.h>
#include <fractals/frasetup.h>
#include <math/big_init.h>
#include <math/BigComplex.h>
#include <math/BigFloat.h>
#include <math/BigNum.h>
#include <math/fixed_pt.h>

#include <fmt/format.h>

#include <algorithm>
#include <array> // std::size
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::math
{
BFMathType g_bf_math{};
}

namespace id::engine
{

std::vector<std::string> g_precision_diagnostics;

//********************************************************************
std::string format_var_bn(const char *s, const BigNum &n)
{
    std::string msg{s};
    msg += ' ' + n.to_string(40);
    msg.resize(std::min(msg.size(), size_t{79}));
    return msg;
}

std::string format_corners_dbl(const char *s)
{
    return fmt::format("{:s}\n"                                 //
                       "   x_min= {:.20f}    x_max= {:.20f}\n"  //
                       "   y_min= {:.20f}    y_max= {:.20f}\n"  //
                       "   x_3rd= {:.20f}    y_3rd= {:.20f}\n"  //
                       " delta_x= {:.20Lf} delta_y= {:.20Lf}\n" //
                       "delta_x2= {:.20Lf} delta_y2= {:.20Lf}", //
        s,                                                      //
        g_image_region.m_min.x, g_image_region.m_max.x,         //
        g_image_region.m_min.y, g_image_region.m_max.y,         //
        g_image_region.m_3rd.x, g_image_region.m_3rd.y,         //
        g_delta_x, g_delta_y,                                   //
        g_delta_x2, g_delta_y2);
}

// show floating point and bignumber corners
std::string format_corners_bn(const char *s)
{
    constexpr int dec{20};
    return fmt::format("{:s}\n"
                       "x_min_bn={:s}\n"
                       "x_min   ={:.20f}\n"
                       "\n"
                       "x_max_bn={:s}\n"
                       "x_max   ={:.20f}\n"
                       "\n"
                       "y_min_bn={:s}\n"
                       "y_min   ={:.20f}\n"
                       "\n"
                       "y_max_bn={:s}\n"
                       "y_max   ={:.20f}\n"
                       "\n"
                       "x_3rd_bn={:s}\n"
                       "x_3rd   ={:.20f}\n"
                       "\n"
                       "y_3rd_bn={:s}\n"
                       "y_3rd   ={:.20f}\n"
                       "\n",
        s,                                                           //
        math::bn_to_string(g_x_min_bn, dec), g_image_region.m_min.x, //
        math::bn_to_string(g_x_max_bn, dec), g_image_region.m_max.x, //
        math::bn_to_string(g_y_min_bn, dec), g_image_region.m_min.y, //
        math::bn_to_string(g_y_max_bn, dec), g_image_region.m_max.y, //
        math::bn_to_string(g_x_3rd_bn, dec), g_image_region.m_3rd.x, //
        math::bn_to_string(g_y_3rd_bn, dec), g_image_region.m_3rd.y);
}

// show globals
std::string format_globals_bf(const char *s)
{
    return fmt::format("{:s}\n"                                                                   //
                       "bn_step={:d} bn_length={:d} int_length={:d} r_length={:d} padding={:d}\n" //
                       "shift_factor={:d} decimals={:d} bf_length={:d} r_bf_length={:d}\n"        //
                       "bf_decimals={:d} ",                                                       //
        s,                                                                                        //
        g_bn_step, g_bn_length, g_int_length, g_r_length, g_padding,                              //
        g_shift_factor, g_decimals, g_bf_length, g_r_bf_length,                                   //
        g_bf_decimals);
}

std::string format_corners_bf(const char *s)
{
    const int dec{std::min(g_decimals, 20)};
    return fmt::format("{:s}\n"
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
        s,                                                     //
        bf_to_string(g_bf_x_min, dec),                         //
        g_image_region.m_min.x, g_decimals, g_bf_length,       //
        bf_to_string(g_bf_x_max, dec), g_image_region.m_max.x, //
        bf_to_string(g_bf_y_min, dec), g_image_region.m_min.y, //
        bf_to_string(g_bf_y_max, dec), g_image_region.m_max.y, //
        bf_to_string(g_bf_x_3rd, dec), g_image_region.m_3rd.x, //
        bf_to_string(g_bf_y_3rd, dec), g_image_region.m_3rd.y);
}

std::string format_corners_bf_save(const char *s)
{
    constexpr int dec{20};
    return fmt::format("{:s}\n"
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
        s,                                                          //
        bf_to_string(g_bf_save_x_min, dec), g_image_region.m_min.x, //
        bf_to_string(g_bf_save_x_max, dec), g_image_region.m_max.x, //
        bf_to_string(g_bf_save_y_min, dec), g_image_region.m_min.y, //
        bf_to_string(g_bf_save_y_max, dec), g_image_region.m_max.y, //
        bf_to_string(g_bf_save_x_3rd, dec), g_image_region.m_3rd.x, //
        bf_to_string(g_bf_save_y_3rd, dec), g_image_region.m_3rd.y);
}

std::string format_two_bf(const char *s1, const BigFloat &t1, const char *s2, const BigFloat &t2, const int digits)
{
    return fmt::format("\n"
                       "{:s}->{:s}\n"
                       "{:s}->{:s}",
        s1, bf_to_string_e(t1, digits), //
        s2, bf_to_string_e(t2, digits));
}

std::string format_three_bf(const char *s1, const BigFloat &t1, const char *s2, const BigFloat &t2, const char *s3,
    const BigFloat &t3, const int digits)
{
    return fmt::format("\n"
                       "{:s}->{:s}\n"
                       "{:s}->{:s}\n"
                       "{:s}->{:s}",
        s1, bf_to_string_e(t1, digits), //
        s2, bf_to_string_e(t2, digits), //
        s3, bf_to_string_e(t3, digits));
}

// for aspect ratio debugging
std::string format_aspect(const char *s)
{
    BigStackSaver saved;
    BigFloat bt1{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat bt2{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat aspect{bf_arena(BigLifetime::TEMPORARY)};
    sub_bf(bt1, g_bf_x_max, g_bf_x_min);
    sub_bf(bt2, g_bf_y_max, g_bf_y_min);
    div_bf(aspect.data(), bt2.data(), bt1.data());
    return fmt::format("aspect {:s}\n"
                       "float  {:13.10f}\n"
                       "bf     {:s}\n"
                       "\n",
        s,                                                //
        g_image_region.height() / g_image_region.width(), //
        bf_to_string(aspect, 10));
}

// compare a double and bignumber
std::string format_compare_values(const char *s, const LDouble x, const BigNum &bnx)
{
    constexpr int dec{40};
    return fmt::format("{:s}\n"
                       "bignum={:s}\n"
                       "double={:.20Lf}\n"
                       "\n",
        s,                  //
        bnx.to_string(dec), //
        x);
}

// compare a double and bignumber
std::string format_compare_values_bf(const char *s, const LDouble x, const BigFloat &bfx)
{
    constexpr int dec{40};
    return fmt::format("{:s}\n"
                       "bignum={:s}\n"
                       "double={:.20Lf}\n"
                       "\n",
        s,                               //
        bf_to_string_e(bfx, dec), //
        x);
}

//********************************************************************
std::string format_var_bf(const char *, const BigFloat &n)
{
    std::string msg{bf_to_string_e(n, 40)};
    msg.resize(std::min(msg.size(), size_t{79}));
    return msg;
}

void bf_corners_to_float()
{
    if (g_bf_math != BFMathType::NONE)
    {
        g_image_region.m_min.x = static_cast<double>(bf_to_float(g_bf_x_min));
        g_image_region.m_min.y = static_cast<double>(bf_to_float(g_bf_y_min));
        g_image_region.m_max.x = static_cast<double>(bf_to_float(g_bf_x_max));
        g_image_region.m_max.y = static_cast<double>(bf_to_float(g_bf_y_max));
        g_image_region.m_3rd.x = static_cast<double>(bf_to_float(g_bf_x_3rd));
        g_image_region.m_3rd.y = static_cast<double>(bf_to_float(g_bf_y_3rd));
    }
    for (int i = 0; i < MAX_PARAMS; i++)
    {
        if (type_has_param(g_fractal_type, i))
        {
            g_params[i] = static_cast<double>(bf_to_float(g_bf_params[i]));
        }
    }
}

bool mandel_per_image_bn()
{
    BigStackSaver saved;
    // this should be set up dynamically based on corners
    BigNum bn_temp1{bn_arena(BigLifetime::TEMPORARY)};
    BigNum bn_temp2{bn_arena(BigLifetime::TEMPORARY)};

    bf_to_bn(g_x_min_bn.data(), g_bf_x_min.data());
    bf_to_bn(g_x_max_bn.data(), g_bf_x_max.data());
    bf_to_bn(g_y_min_bn.data(), g_bf_y_min.data());
    bf_to_bn(g_y_max_bn.data(), g_bf_y_max.data());
    bf_to_bn(g_x_3rd_bn.data(), g_bf_x_3rd.data());
    bf_to_bn(g_y_3rd_bn.data(), g_bf_y_3rd.data());

    g_bf_math = BFMathType::BIG_NUM;

    // g_delta_x_bn = (g_x_max_bn - g_x_3rd_bn)/(xdots-1)
    g_delta_x_bn = g_x_max_bn - g_x_3rd_bn;
    g_delta_x_bn /= static_cast<U16>(g_logical_screen.x_dots - 1);

    // g_delta_y_bn = (g_y_max_bn - g_y_3rd_bn)/(ydots-1)
    g_delta_y_bn = g_y_max_bn - g_y_3rd_bn;
    g_delta_y_bn /= static_cast<U16>(g_logical_screen.y_dots - 1);

    // g_delta2_x_bn = (g_x_3rd_bn - g_x_min_bn)/(ydots-1)
    g_delta2_x_bn = g_x_3rd_bn - g_x_min_bn;
    g_delta2_x_bn /= static_cast<U16>(g_logical_screen.y_dots - 1);

    // g_delta2_y_bn = (g_y_3rd_bn - g_y_min_bn)/(xdots-1)
    g_delta2_y_bn = g_y_3rd_bn - g_y_min_bn;
    g_delta2_y_bn /= static_cast<U16>(g_logical_screen.x_dots - 1);

    abs_bn(g_close_enough_bn, g_delta_x_bn);
    if (cmp_bn(abs_bn(bn_temp1, g_delta2_x_bn), g_close_enough_bn) > 0)
    {
        g_close_enough_bn = bn_temp1;
    }
    if (cmp_bn(abs_bn(bn_temp1, g_delta_y_bn), abs_bn(bn_temp2, g_delta2_y_bn)) > 0)
    {
        if (bn_temp1 > g_close_enough_bn)
        {
            g_close_enough_bn = bn_temp1;
        }
    }
    else if (bn_temp2 > g_close_enough_bn)
    {
        g_close_enough_bn = bn_temp2;
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
        return false;
    }

    g_c_exponent = static_cast<int>(g_params[2]);
    switch (g_fractal_type)
    {
    case FractalType::JULIA:
        bf_to_bn(g_param_z_bn.x.data(), g_bf_params[0].data());
        bf_to_bn(g_param_z_bn.y.data(), g_bf_params[1].data());
        break;

    case FractalType::MANDEL_Z_POWER:
        init_big_pi();
        if (static_cast<double>(g_c_exponent) == g_params[2] && g_c_exponent & 1) // odd exponents
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
        bf_to_bn(g_param_z_bn.x.data(), g_bf_params[0].data());
        bf_to_bn(g_param_z_bn.y.data(), g_bf_params[1].data());
        if (g_c_exponent & 1 || g_params[3] != 0.0 || static_cast<double>(g_c_exponent) != g_params[2])
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;

    case FractalType::DIVIDE_BROT5:
        init_big_pi();
        g_c_exponent = -(static_cast<int>(g_params[0]) - 2); /* use negative here so only need it once */
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
    BigFloat bf_temp1{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat bf_temp2{bf_arena(BigLifetime::TEMPORARY)};

    g_bf_math = BFMathType::BIG_FLT;

    // g_delta_x_bf = (g_bf_x_max - g_bf_x_3rd)/(xdots-1)
    sub_bf(g_delta_x_bf, g_bf_x_max, g_bf_x_3rd);
    g_delta_x_bf /= static_cast<U16>(g_logical_screen.x_dots - 1);

    // g_delta_y_bf = (g_bf_y_max - g_bf_y_3rd)/(ydots-1)
    sub_bf(g_delta_y_bf, g_bf_y_max, g_bf_y_3rd);
    g_delta_y_bf /= static_cast<U16>(g_logical_screen.y_dots - 1);

    // g_delta2_x_bf = (g_bf_x_3rd - g_bf_x_min)/(ydots-1)
    sub_bf(g_delta2_x_bf, g_bf_x_3rd, g_bf_x_min);
    g_delta2_x_bf /= static_cast<U16>(g_logical_screen.y_dots - 1);

    // g_delta2_y_bf = (g_bf_y_3rd - g_bf_y_min)/(xdots-1)
    sub_bf(g_delta2_y_bf, g_bf_y_3rd, g_bf_y_min);
    g_delta2_y_bf /= static_cast<U16>(g_logical_screen.x_dots - 1);

    abs_bf(g_close_enough_bf, g_delta_x_bf);
    if (cmp_bf(abs_bf(bf_temp1, g_delta2_x_bf), g_close_enough_bf) > 0)
    {
        g_close_enough_bf = bf_temp1;
    }
    if (cmp_bf(abs_bf(bf_temp1, g_delta_y_bf), abs_bf(bf_temp2, g_delta2_y_bf)) > 0)
    {
        if (bf_temp1 > g_close_enough_bf)
        {
            g_close_enough_bf = bf_temp1;
        }
    }
    else if (bf_temp2 > g_close_enough_bf)
    {
        g_close_enough_bf = bf_temp2;
    }
    {
        int t{std::abs(g_periodicity_check)};
        while (t--)
        {
            g_close_enough_bf = g_close_enough_bf.half();
        }
    }

    // floating point code could probably be altered to handle many of
    // the situations that otherwise are using standard_fractal().
    // calcmandfp() can currently handle invert, any rqlim, potflag
    // zmag, epsilon cross, and all the current outside options
    g_c_exponent = static_cast<int>(g_params[2]);
    switch (g_fractal_type)
    {
    case FractalType::MANDEL:
    case FractalType::BURNING_SHIP:
        if (g_std_calc_mode == CalcMode::PERTURBATION &&
            bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
        {
            return false;
        }
        break;

    case FractalType::JULIA:
        copy_bf(g_param_z_bf.x, g_bf_params[0]);
        copy_bf(g_param_z_bf.y, g_bf_params[1]);
        break;

    case FractalType::MANDEL_Z_POWER:
        init_big_pi();
        if (static_cast<double>(g_c_exponent) == g_params[2] && g_c_exponent & 1) // odd exponents
        {
            g_symmetry = SymmetryType::XY_AXIS_NO_PARAM;
        }
        if (g_params[3] != 0)
        {
            g_symmetry = SymmetryType::NONE;
        }
        if (g_std_calc_mode == CalcMode::PERTURBATION && bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
        {
            return false;
        }
        break;

    case FractalType::JULIA_Z_POWER:
        init_big_pi();
        copy_bf(g_param_z_bf.x, g_bf_params[0]);
        copy_bf(g_param_z_bf.y, g_bf_params[1]);
        if (g_c_exponent & 1 || g_params[3] != 0.0 || static_cast<double>(g_c_exponent) != g_params[2])
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;

    case FractalType::DIVIDE_BROT5:
        init_big_pi();
        g_c_exponent = -(static_cast<int>(g_params[0]) - 2); /* use negative here so only need it once */
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
    // parm.x = x_min + col*delx + row*delx2
    g_param_z_bn.x = g_delta_x_bn * static_cast<U16>(g_col);
    mult_bn_int(g_bn_tmp, g_delta2_x_bn, static_cast<U16>(g_row));

    add_a_bn(g_param_z_bn.x, g_bn_tmp);
    g_param_z_bn.x += g_x_min_bn;

    // parm.y = y_max - row*dely - col*dely2;
    // note: in next four lines, g_old_z_bn is just used as a temporary variable
    mult_bn_int(g_old_z_bn.x, g_delta_y_bn, static_cast<U16>(g_row));
    mult_bn_int(g_old_z_bn.y, g_delta2_y_bn, static_cast<U16>(g_col));
    add_a_bn(g_old_z_bn.x, g_old_z_bn.y);
    sub_bn(g_param_z_bn.y, g_y_max_bn, g_old_z_bn.x);

    if (g_use_init_orbit == InitOrbitMode::VALUE)
    {
        g_old_z_bn.x = g_init_orbit.x;
        g_old_z_bn.y = g_init_orbit.y;
    }
    else
    {
        copy_bn(g_old_z_bn.x, g_param_z_bn.x);
        copy_bn(g_old_z_bn.y, g_param_z_bn.y);
    }

    if ((g_inside_method == ColorMethod::BOF60 || g_inside_method == ColorMethod::BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        float_to_bn(g_old_z_bn.x, g_params[0]); // initial perturbation of parameters set
        float_to_bn(g_old_z_bn.y, g_params[1]);
        g_color_iter = -1;
    }
    else
    {
        g_new_z_bn.x = g_params[0];
        g_new_z_bn.y = g_params[1];
        add_a_bn(g_old_z_bn.x, g_new_z_bn.x);
        add_a_bn(g_old_z_bn.y, g_new_z_bn.y);
    }

    // square has side effect - must copy first
    copy_bn(g_new_z_bn.x, g_old_z_bn.x);
    copy_bn(g_new_z_bn.y, g_old_z_bn.y);

    // Store canonical g_bn_length-byte squares in the caches.
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
    // parm.x = x_min + col*delx + row*delx2
    g_param_z_bf.x = g_delta_x_bf * static_cast<U16>(g_col);
    g_bf_tmp = g_delta2_x_bf * static_cast<U16>(g_row);

    g_param_z_bf.x += g_bf_tmp;
    add_a_bf(g_param_z_bf.x, g_bf_x_min);

    // parm.y = y_max - row*dely - col*dely2;
    // note: in next four lines, g_old_z_bf is just used as a temporary variable
    g_old_z_bf.x = g_delta_y_bf * static_cast<U16>(g_row);
    g_old_z_bf.y = g_delta2_y_bf * static_cast<U16>(g_col);
    g_old_z_bf.x += g_old_z_bf.y;
    sub_bf(g_param_z_bf.y, g_bf_y_max, g_old_z_bf.x);

    if ((g_fractal_type == FractalType::MANDEL || g_fractal_type == FractalType::MANDEL_Z_POWER) &&
        g_use_init_orbit == InitOrbitMode::VALUE)
    {
        g_old_z_bf.x = g_init_orbit.x;
        g_old_z_bf.y = g_init_orbit.y;
    }
    else
    {
        g_old_z_bf.x = g_param_z_bf.x;
        g_old_z_bf.y = g_param_z_bf.y;
    }

    if ((g_inside_method == ColorMethod::BOF60 || g_inside_method == ColorMethod::BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        g_old_z_bf.x = g_params[0]; // initial perturbation of parameters set
        g_old_z_bf.y = g_params[1];
        g_color_iter = -1;
    }
    else
    {
        g_new_z_bf.x = g_params[0];
        g_new_z_bf.y = g_params[1];
        g_old_z_bf.x += g_new_z_bf.x;
        g_old_z_bf.y += g_new_z_bf.y;
    }

    // square has side effect - must copy first
    g_new_z_bf.x = g_old_z_bf.x;
    g_new_z_bf.y = g_old_z_bf.y;

    // Square these to g_r_bf_length bytes of precision
    g_tmp_sqr_x_bf = sqr(g_new_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_new_z_bf.y);

    return 1;                  // 1st iteration has been done
}

int julia_per_pixel_bn()
{
    // old.x = x_min + col*delx + row*delx2
    mult_bn_int(g_old_z_bn.x, g_delta_x_bn, static_cast<U16>(g_col));
    mult_bn_int(g_bn_tmp, g_delta2_x_bn, static_cast<U16>(g_row));

    add_a_bn(g_old_z_bn.x, g_bn_tmp);
    add_a_bn(g_old_z_bn.x, g_x_min_bn);

    // old.y = y_max - row*dely - col*dely2;
    // note: in next four lines, g_new_z_bn is just used as a temporary variable
    g_new_z_bn.x = g_delta_y_bn * static_cast<U16>(g_row);
    g_new_z_bn.y = g_delta2_y_bn * static_cast<U16>(g_col);
    g_new_z_bn.x += g_new_z_bn.y;
    sub_bn(g_old_z_bn.y, g_y_max_bn, g_new_z_bn.x);

    // square has side effect - must copy first
    copy_bn(g_new_z_bn.x, g_old_z_bn.x);
    copy_bn(g_new_z_bn.y, g_old_z_bn.y);

    // Store canonical g_bn_length-byte squares in the caches.
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);

    return 1;                  // 1st iteration has been done
}

int julia_per_pixel_bf()
{
    // old.x = x_min + col*delx + row*delx2
    g_old_z_bf.x = g_delta_x_bf * static_cast<U16>(g_col);
    g_bf_tmp = g_delta2_x_bf * static_cast<U16>(g_row);

    g_old_z_bf.x += g_bf_tmp;
    add_a_bf(g_old_z_bf.x, g_bf_x_min);

    // old.y = y_max - row*dely - col*dely2;
    // note: in next four lines, g_new_z_bf is just used as a temporary variable
    g_new_z_bf.x = g_delta_y_bf * static_cast<U16>(g_row);
    g_new_z_bf.y = g_delta2_y_bf * static_cast<U16>(g_col);
    g_new_z_bf.x += g_new_z_bf.y;
    sub_bf(g_old_z_bf.y, g_bf_y_max, g_new_z_bf.x);

    // square has side effect - must copy first
    g_new_z_bf.x = g_old_z_bf.x;
    g_new_z_bf.y = g_old_z_bf.y;

    // Square these to g_r_bf_length bytes of precision
    g_tmp_sqr_x_bf = sqr(g_new_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_new_z_bf.y);

    return 1;                  // 1st iteration has been done
}

int julia_orbit_bn()
{
    BigStackSaver saved;
    BigNum product{bn_arena(BigLifetime::TEMPORARY)};

    // new.x = tmpsqrx - tmpsqry + parm.x;
    sub_a_bn(g_tmp_sqr_x_bn, g_tmp_sqr_y_bn);
    add_bn(g_new_z_bn.x, g_tmp_sqr_x_bn, g_param_z_bn.x);

    // new.y = 2 * g_old_z_bn.x * g_old_z_bn.y + parm.y;
    mult_bn(product, g_old_z_bn.x, g_old_z_bn.y);
    double_a_bn(product);
    add_bn(g_new_z_bn.y, product, g_param_z_bn.y);

    return g_bailout_bignum();
}

int julia_orbit_bf()
{
    // new.x = tmpsqrx - tmpsqry + parm.x;
    g_tmp_sqr_x_bf -= g_tmp_sqr_y_bf;
    g_new_z_bf.x = g_tmp_sqr_x_bf + g_param_z_bf.x;

    // new.y = 2 * g_old_z_bf.x * g_old_z_bf.y + parm.y;
    g_bf_tmp = g_old_z_bf.x * g_old_z_bf.y; // ok to use unsafe here
    double_a_bf(g_bf_tmp);
    g_new_z_bf.y = g_bf_tmp + g_param_z_bf.y;
    return g_bailout_bigfloat();
}

int julia_z_power_orbit_bf()
{
    BigStackSaver saved;
    BFComplex param2{BigFloat{bf_arena(BigLifetime::TEMPORARY)}, BigFloat{bf_arena(BigLifetime::TEMPORARY)}};

    param2.x = g_params[2];
    param2.y = g_params[3];
    g_new_z_bf = math::pow(g_old_z_bf, param2);
    g_new_z_bf.x = g_param_z_bf.x + g_new_z_bf.x;
    g_new_z_bf.y = g_param_z_bf.y + g_new_z_bf.y;
    return g_bailout_bigfloat();
}

} // namespace id::engine
