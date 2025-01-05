// SPDX-License-Identifier: GPL-3.0-only
//
// This file contains the "big number" high precision versions of the
// fractal routines.

#include "fractalb.h"

#include "bailout_formula.h"
#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "divide_brot.h"
#include "fixed_pt.h"
#include "fractalp.h"
#include "fractals.h"
#include "frasetup.h"
#include "goodbye.h"
#include "id.h"
#include "id_data.h"
#include "stop_msg.h"
#include "type_has_param.h"

#include <algorithm>
#include <array> // std::size
#include <cmath>
#include <cstdio>
#include <cstring>

BFMathType g_bf_math{};

#ifndef NDEBUG
//********************************************************************
void show_var_bn(char const *s, bn_t n)
{
    char msg[200];
    std::strcpy(msg, s);
    std::strcat(msg, " ");
    bn_to_str(msg + std::strlen(s), 40, n);
    msg[79] = 0;
    stop_msg(msg);
}

void show_corners_dbl(char const *s)
{
    char msg[400];
    std::snprintf(msg, std::size(msg),
        "%s\n"                               //
        "   x_min= %.20f    x_max= %.20f\n"  //
        "   y_min= %.20f    y_max= %.20f\n"  //
        "   x_3rd= %.20f    y_3rd= %.20f\n"  //
        " delta_x= %.20Lf delta_y= %.20Lf\n" //
        "delta_x2= %.20Lf delta_y2= %.20Lf", //
        s,                                   //
        g_x_min, g_x_max,                    //
        g_y_min, g_y_max,                    //
        g_x_3rd, g_y_3rd,                    //
        g_delta_x, g_delta_y,                //
        g_delta_x2, g_delta_y2);
    if (stop_msg(msg))
    {
        goodbye();
    }
}

// show floating point and bignumber corners
void show_corners_bn(char const *s)
{
    int dec = 20;
    char msg[100];
    char msg1[200];
    char msg3[400];
    bn_to_str(msg, dec, g_x_min_bn);
    std::snprintf(msg1, std::size(msg1), "g_x_min_bn=%s\nx_min= %.20f\n\n", msg, g_x_min);
    std::strcpy(msg3, s);
    std::strcat(msg3, "\n");
    std::strcat(msg3, msg1);
    bn_to_str(msg, dec, g_x_max_bn);
    std::snprintf(msg1, std::size(msg1), "g_x_max_bn=%s\nx_max= %.20f\n\n", msg, g_x_max);
    std::strcat(msg3, msg1);
    bn_to_str(msg, dec, g_y_min_bn);
    std::snprintf(msg1, std::size(msg1), "g_y_min_bn=%s\ny_min= %.20f\n\n", msg, g_y_min);
    std::strcat(msg3, msg1);
    bn_to_str(msg, dec, g_y_max_bn);
    std::snprintf(msg1, std::size(msg1), "g_y_max_bn=%s\ny_max= %.20f\n\n", msg, g_y_max);
    std::strcat(msg3, msg1);
    bn_to_str(msg, dec, g_x_3rd_bn);
    std::snprintf(msg1, std::size(msg1), "g_x_3rd_bn=%s\nx_3rd= %.20f\n\n", msg, g_x_3rd);
    std::strcat(msg3, msg1);
    bn_to_str(msg, dec, g_y_3rd_bn);
    std::snprintf(msg1, std::size(msg1), "g_y_3rd_bn=%s\ny_3rd= %.20f\n\n", msg, g_y_3rd);
    std::strcat(msg3, msg1);
    if (stop_msg(msg3))
    {
        goodbye();
    }
}

// show globals
void show_globals_bf(char const *s)
{
    char msg[300];
    std::snprintf(msg, std::size(msg),
        "%s\n"                                                                     //
        "g_bn_step=%d g_bn_length=%d g_int_length=%d g_r_length=%d g_padding=%d\n" //
        "g_shift_factor=%d decimals=%d g_bf_length=%d g_r_bf_length=%d \n"         //
        "g_bf_decimals=%d ",                                                          //
        s,                                                                         //
        g_bn_step, g_bn_length, g_int_length, g_r_length, g_padding,               //
        g_shift_factor, g_decimals, g_bf_length, g_r_bf_length,                    //
        g_bf_decimals);
    if (stop_msg(msg))
    {
        goodbye();
    }
}

void show_corners_bf(char const *s)
{
    int dec = g_decimals;
    char msg[100];
    char msg1[200];
    char msg3[600];
    dec = std::min(dec, 20);
    bf_to_str(msg, dec, g_bf_x_min);
    std::snprintf(msg1, std::size(msg1),
        "bf_x_min=%s\n"                                //
        "x_min= %.20f decimals %d g_bf_length %d\n\n", //
        msg,                                           //
        g_x_min, g_decimals, g_bf_length);
    std::strcpy(msg3, s);
    std::strcat(msg3, "\n");
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_x_max);
    std::snprintf(msg1, std::size(msg1), "bf_x_max=%s\nx_max= %.20f\n\n", msg, g_x_max);
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_y_min);
    std::snprintf(msg1, std::size(msg1), "bf_y_min=%s\ny_min= %.20f\n\n", msg, g_y_min);
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_y_max);
    std::snprintf(msg1, std::size(msg1), "bf_y_max=%s\ny_max= %.20f\n\n", msg, g_y_max);
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_x_3rd);
    std::snprintf(msg1, std::size(msg1), "bf_x_3rd=%s\nxx_3rd= %.20f\n\n", msg, g_x_3rd);
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_y_3rd);
    std::snprintf(msg1, std::size(msg1), "bf_y_3rd=%s\ny_3rd= %.20f\n\n", msg, g_y_3rd);
    std::strcat(msg3, msg1);
    if (stop_msg(msg3))
    {
        goodbye();
    }
}

void show_corners_bf_save(char const *s)
{
    int dec = 20;
    char msg[100];
    char msg1[200];
    char msg3[500];
    bf_to_str(msg, dec, g_bf_save_x_min);
    std::snprintf(msg1, std::size(msg1), "bf_save_x_min=%s\nx_min= %.20f\n\n", msg, g_x_min);
    std::strcpy(msg3, s);
    std::strcat(msg3, "\n");
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_save_x_max);
    std::snprintf(msg1, std::size(msg1), "bf_save_x_max=%s\nx_max= %.20f\n\n", msg, g_x_max);
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_save_y_min);
    std::snprintf(msg1, std::size(msg1), "bf_save_y_min=%s\ny_min= %.20f\n\n", msg, g_y_min);
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_save_y_max);
    std::snprintf(msg1, std::size(msg1), "bf_save_y_max=%s\ny_max= %.20f\n\n", msg, g_y_max);
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_save_x_3rd);
    std::snprintf(msg1, std::size(msg1), "bf_save_x_3rd=%s\nx_3rd= %.20f\n\n", msg, g_x_3rd);
    std::strcat(msg3, msg1);
    bf_to_str(msg, dec, g_bf_save_y_3rd);
    std::snprintf(msg1, std::size(msg1), "bf_save_y_3rd=%s\ny_3rd= %.20f\n\n", msg, g_y_3rd);
    std::strcat(msg3, msg1);
    if (stop_msg(msg3))
    {
        goodbye();
    }
}

void show_two_bf(char const *s1, bf_t t1, char const *s2, bf_t t2, int digits)
{
    char msg1[200];
    char msg2[200];
    char msg3[500];
    bf_to_str_e(msg1, digits, t1);
    bf_to_str_e(msg2, digits, t2);
    std::snprintf(msg3, std::size(msg3), "\n%s->%s\n%s->%s", s1, msg1, s2, msg2);
    if (stop_msg(msg3))
    {
        goodbye();
    }
}

void show_three_bf(char const *s1, bf_t t1, char const *s2, bf_t t2, char const *s3, bf_t t3, int digits)
{
    char msg1[200];
    char msg2[200];
    char msg3[200];
    char msg4[700];
    bf_to_str_e(msg1, digits, t1);
    bf_to_str_e(msg2, digits, t2);
    bf_to_str_e(msg3, digits, t3);
    std::snprintf(msg4, std::size(msg4), "\n%s->%s\n%s->%s\n%s->%s", s1, msg1, s2, msg2, s3, msg3);
    if (stop_msg(msg4))
    {
        goodbye();
    }
}

// for aspect ratio debugging
void show_aspect(char const *s)
{
    char msg[300];
    char str[100];
    int saved = save_stack();
    bf_t bt1 = alloc_stack(g_r_bf_length + 2);
    bf_t bt2 = alloc_stack(g_r_bf_length + 2);
    bf_t aspect = alloc_stack(g_r_bf_length + 2);
    sub_bf(bt1, g_bf_x_max, g_bf_x_min);
    sub_bf(bt2, g_bf_y_max, g_bf_y_min);
    div_bf(aspect, bt2, bt1);
    bf_to_str(str, 10, aspect);
    std::snprintf(msg, std::size(msg), "aspect %s\nfloat %13.10f\nbf    %s\n\n",
            s,
            (g_y_max-g_y_min)/(g_x_max-g_x_min),
            str);
    if (stop_msg(msg))
    {
        goodbye();
    }
    restore_stack(saved);
}

// compare a double and bignumber
void compare_values(char const *s, LDouble x, bn_t bnx)
{
    int dec = 40;
    char msg[100];
    char msg1[300];
    bn_to_str(msg, dec, bnx);
    std::snprintf(msg1, std::size(msg1), "%s\nbignum=%s\ndouble=%.20Lf\n\n", s, msg, x);
    if (stop_msg(msg1))
    {
        goodbye();
    }
}
// compare a double and bignumber
void compare_values_bf(char const *s, LDouble x, bf_t bfx)
{
    int dec = 40;
    char msg[300];
    char msg1[700];
    bf_to_str_e(msg, dec, bfx);
    std::snprintf(msg1, std::size(msg1), "%s\nbignum=%s\ndouble=%.20Lf\n\n", s, msg, x);
    if (stop_msg(msg1))
    {
        goodbye();
    }
}

//********************************************************************
void show_var_bf(char const *s, bf_t n)
{
    char msg[200];
    std::strcpy(msg, s);
    std::strcat(msg, " ");
    bf_to_str_e(msg+std::strlen(s), 40, n);
    msg[79] = 0;
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
        if (type_has_param(g_fractal_type, i, nullptr))
        {
            g_params[i] = (double)bf_to_float(g_bf_params[i]);
        }
    }
}

// --------------------------------------------------------------------
//    Bignumber Bailout Routines
// --------------------------------------------------------------------

// Note:
// No need to set magnitude
// as color schemes that need it calculate it later.

int  bn_mod_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    add_bn(g_bn_tmp, g_tmp_sqr_x_bn+g_shift_factor, g_tmp_sqr_y_bn+g_shift_factor);

    long longmagnitude = bn_to_int(g_bn_tmp);  // works with any fractal type
    if (longmagnitude >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

int  bn_real_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    long longtempsqrx = bn_to_int(g_tmp_sqr_x_bn + g_shift_factor);
    if (longtempsqrx >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

int  bn_imag_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    long longtempsqry = bn_to_int(g_tmp_sqr_y_bn + g_shift_factor);
    if (longtempsqry >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

int  bn_or_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    long longtempsqrx = bn_to_int(g_tmp_sqr_x_bn + g_shift_factor);
    long longtempsqry = bn_to_int(g_tmp_sqr_y_bn + g_shift_factor);
    if (longtempsqrx >= (long)g_magnitude_limit || longtempsqry >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

int  bn_and_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    long longtempsqrx = bn_to_int(g_tmp_sqr_x_bn + g_shift_factor);
    long longtempsqry = bn_to_int(g_tmp_sqr_y_bn + g_shift_factor);
    if (longtempsqrx >= (long)g_magnitude_limit && longtempsqry >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

int  bn_manh_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    // note: in next five lines, g_old_z_bn is just used as a temporary variable
    abs_bn(g_old_z_bn.x, g_new_z_bn.x);
    abs_bn(g_old_z_bn.y, g_new_z_bn.y);
    add_bn(g_bn_tmp, g_old_z_bn.x, g_old_z_bn.y);
    square_bn(g_old_z_bn.x, g_bn_tmp);
    long longtempmag = bn_to_int(g_old_z_bn.x + g_shift_factor);
    if (longtempmag >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

int  bn_manr_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    add_bn(g_bn_tmp, g_new_z_bn.x, g_new_z_bn.y); // don't need abs since we square it next
    // note: in next two lines, g_old_z_bn is just used as a temporary variable
    square_bn(g_old_z_bn.x, g_bn_tmp);
    long longtempmag = bn_to_int(g_old_z_bn.x + g_shift_factor);
    if (longtempmag >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

int  bf_mod_bailout()
{
    int saved = save_stack();
    bf_t tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    add_bf(g_bf_tmp, g_tmp_sqr_x_bf, g_tmp_sqr_y_bf);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_bf_tmp, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }
    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

int  bf_real_bailout()
{
    int saved = save_stack();
    bf_t tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_tmp_sqr_x_bf, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}


int  bf_imag_bailout()
{
    int saved = save_stack();
    bf_t tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_tmp_sqr_y_bf, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

int  bf_or_bailout()
{
    int saved = save_stack();
    bf_t tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_tmp_sqr_x_bf, tmp1) > 0 || cmp_bf(g_tmp_sqr_y_bf, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

int  bf_and_bailout()
{
    int saved = save_stack();
    bf_t tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_tmp_sqr_x_bf, tmp1) > 0 && cmp_bf(g_tmp_sqr_y_bf, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

int  bf_manh_bailout()
{
    int saved = save_stack();
    bf_t tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    // note: in next five lines, g_old_z_bf is just used as a temporary variable
    abs_bf(g_old_z_bf.x, g_new_z_bf.x);
    abs_bf(g_old_z_bf.y, g_new_z_bf.y);
    add_bf(g_bf_tmp, g_old_z_bf.x, g_old_z_bf.y);
    square_bf(g_old_z_bf.x, g_bf_tmp);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_old_z_bf.x, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

int  bf_manr_bailout()
{
    int saved = save_stack();
    bf_t tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    add_bf(g_bf_tmp, g_new_z_bf.x, g_new_z_bf.y); // don't need abs since we square it next
    // note: in next two lines, g_old_z_bf is just used as a temporary variable
    square_bf(g_old_z_bf.x, g_bf_tmp);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_old_z_bf.x, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

bool mandel_bn_setup()
{
    BigStackSaver saved;
    // this should be set up dynamically based on corners
    bn_t bntemp1 = alloc_stack(g_bn_length);
    bn_t bntemp2 = alloc_stack(g_bn_length);

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
    if (cmp_bn(abs_bn(bntemp1, g_delta2_x_bn), g_close_enough_bn) > 0)
    {
        copy_bn(g_close_enough_bn, bntemp1);
    }
    if (cmp_bn(abs_bn(bntemp1, g_delta_y_bn), abs_bn(bntemp2, g_delta2_y_bn)) > 0)
    {
        if (cmp_bn(bntemp1, g_close_enough_bn) > 0)
        {
            copy_bn(g_close_enough_bn, bntemp1);
        }
    }
    else if (cmp_bn(bntemp2, g_close_enough_bn) > 0)
    {
        copy_bn(g_close_enough_bn, bntemp2);
    }
    {
        int t = std::abs(g_periodicity_check);
        while (t--)
        {
            half_a_bn(g_close_enough_bn);
        }
    }

    if (g_std_calc_mode == 'p' && bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
    {
        mandel_perturbation_setup();
        // TODO: figure out crash if we don't do this
        g_std_calc_mode ='g';
        g_calc_status = CalcStatus::COMPLETED;
        return true;
    }

    g_c_exponent = (int) g_params[2];
    switch (g_fractal_type)
    {
    case FractalType::JULIA_FP:
        bf_to_bn(g_param_z_bn.x, g_bf_params[0]);
        bf_to_bn(g_param_z_bn.y, g_bf_params[1]);
        break;

    case FractalType::MANDEL_Z_POWER_FP:
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

    case FractalType::JULIA_Z_POWER_FP:
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

bool mandel_bf_setup()
{
    // I suspect the following code should be somewhere in perform_worklist() to reset the setup routine to
    // floating point when zooming out. Somehow the math type is restored and the bigflt memory restored, but
    // the pointer to setup isn't.
    if (g_bf_math == BFMathType::NONE)
    {
        // kludge to prevent crash when math type = NONE and still call bigflt setup routine
        return mandel_fp_setup();
    }

    // this should be set up dynamically based on corners
    BigStackSaver saved;
    bf_t bftemp1{alloc_stack(g_bf_length + 2)};
    bf_t bftemp2{alloc_stack(g_bf_length + 2)};

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
    if (cmp_bf(abs_bf(bftemp1, g_delta2_x_bf), g_close_enough_bf) > 0)
    {
        copy_bf(g_close_enough_bf, bftemp1);
    }
    if (cmp_bf(abs_bf(bftemp1, g_delta_y_bf), abs_bf(bftemp2, g_delta2_y_bf)) > 0)
    {
        if (cmp_bf(bftemp1, g_close_enough_bf) > 0)
        {
            copy_bf(g_close_enough_bf, bftemp1);
        }
    }
    else if (cmp_bf(bftemp2, g_close_enough_bf) > 0)
    {
        copy_bf(g_close_enough_bf, bftemp2);
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
    case FractalType::MANDEL_FP:
    case FractalType::BURNING_SHIP:
        if (g_std_calc_mode == 'p' && bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
        {
            return mandel_perturbation_setup();
        }
        break;

    case FractalType::JULIA_FP:
        copy_bf(g_param_z_bf.x, g_bf_params[0]);
        copy_bf(g_param_z_bf.y, g_bf_params[1]);
        break;

    case FractalType::MANDEL_Z_POWER_FP:
        if (g_std_calc_mode == 'p' && bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
        {
            // only allow integer values of real part
            if (const int degree = (int) g_params[2]; degree > 2)
            {
                return mandel_z_power_perturbation_setup();
            }
            else if (degree == 2)
            {
                return mandel_perturbation_setup();
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

    case FractalType::JULIA_Z_POWER_FP:
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

int mandel_bn_per_pixel()
{
    if (g_std_calc_mode == 'p' && bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
        return true;
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

int mandel_bf_per_pixel()
{
    // I suspect the following code should be somewhere in perform_worklist() to reset the setup routine to
    // floating point when zooming out. Somehow the math type is restored and the bigflt memory restored, but
    // the pointer to setup isn't.
    if (g_bf_math == BFMathType::NONE) // kludge to prevent crash when math type = NONE and still call bigflt setup routine
        return mandel_fp_per_pixel();
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

int
julia_bn_per_pixel()
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

int
julia_bf_per_pixel()
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

int
julia_bn_fractal()
{
    // Don't forget, with bn_t numbers, after multiplying or squaring
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

    return g_bailout_bignum();
}

int
julia_bf_fractal()
{
    // new.x = tmpsqrx - tmpsqry + parm.x;
    sub_a_bf(g_tmp_sqr_x_bf, g_tmp_sqr_y_bf);
    add_bf(g_new_z_bf.x, g_tmp_sqr_x_bf, g_param_z_bf.x);

    // new.y = 2 * g_old_z_bf.x * g_old_z_bf.y + parm.y;
    mult_bf(g_bf_tmp, g_old_z_bf.x, g_old_z_bf.y); // ok to use unsafe here
    double_a_bf(g_bf_tmp);
    add_bf(g_new_z_bf.y, g_bf_tmp, g_param_z_bf.y);
    return g_bailout_bigfloat();
}

int
julia_z_power_bn_fractal()
{
    BNComplex parm2;
    int saved = save_stack();

    parm2.x = alloc_stack(g_bn_length);
    parm2.y = alloc_stack(g_bn_length);

    float_to_bn(parm2.x, g_params[2]);
    float_to_bn(parm2.y, g_params[3]);
    cmplx_pow_bn(&g_new_z_bn, &g_old_z_bn, &parm2);
    add_bn(g_new_z_bn.x, g_param_z_bn.x, g_new_z_bn.x+g_shift_factor);
    add_bn(g_new_z_bn.y, g_param_z_bn.y, g_new_z_bn.y+g_shift_factor);
    restore_stack(saved);
    return g_bailout_bignum();
}

int
julia_z_power_bf_fractal()
{
    BFComplex parm2;
    int saved = save_stack();

    parm2.x = alloc_stack(g_bf_length+2);
    parm2.y = alloc_stack(g_bf_length+2);

    float_to_bf(parm2.x, g_params[2]);
    float_to_bf(parm2.y, g_params[3]);
    cmplx_pow_bf(&g_new_z_bf, &g_old_z_bf, &parm2);
    add_bf(g_new_z_bf.x, g_param_z_bf.x, g_new_z_bf.x);
    add_bf(g_new_z_bf.y, g_param_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return g_bailout_bigfloat();
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
    bf_t tmp1 = alloc_stack(g_r_bf_length + 2);
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
    bf_t tmp1 = alloc_stack(g_r_bf_length + 2);
    bf_t denom = alloc_stack(g_r_bf_length + 2);

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
    bf_t e2x = alloc_stack(g_r_bf_length + 2);
    bf_t siny = alloc_stack(g_r_bf_length + 2);
    bf_t cosy = alloc_stack(g_r_bf_length + 2);
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
    sin_cos_bf(siny, cosy, tmp.y);
    mult_bf(t->x, e2x, cosy);
    mult_bf(t->y, e2x, siny);
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
    bn_t tmp1 = alloc_stack(g_r_length);
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
    bn_t tmp1 = alloc_stack(g_r_length);
    bn_t tmp2 = alloc_stack(g_r_length);
    bn_t denom = alloc_stack(g_r_length);

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
    bn_t e2x = alloc_stack(g_r_length);
    bn_t siny = alloc_stack(g_r_length);
    bn_t cosy = alloc_stack(g_r_length);
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
    sin_cos_bn(siny, cosy, tmp.y);
    mult_bn(t->x, e2x, cosy);
    mult_bn(t->y, e2x, siny);
    restore_stack(saved);
    return t;
}
