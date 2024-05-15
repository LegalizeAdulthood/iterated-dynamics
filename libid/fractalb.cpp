/* -----------------------------------------------------------------

This file contains the "big number" high precision versions of the
fractal routines.

--------------------------------------------------------------------   */
#include "port.h"
#include "prototyp.h"

#include "fractalb.h"

#include "bailout_formula.h"
#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "divide_brot.h"
#include "fixed_pt.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "goodbye.h"
#include "helpdefs.h"
#include "id.h"
#include "id_data.h"
#include "stop_msg.h"
#include "type_has_param.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <string>

bf_math_type bf_math = bf_math_type::NONE;

#ifndef NDEBUG
//********************************************************************
void show_var_bn(char const *s, bn_t n)
{
    char msg[200];
    std::strcpy(msg, s);
    std::strcat(msg, " ");
    bntostr(msg + std::strlen(s), 40, n);
    msg[79] = 0;
    stopmsg(STOPMSG_NONE, msg);
}

void showcornersdbl(char const *s)
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
    if (stopmsg(STOPMSG_NONE, msg))
    {
        goodbye();
    }
}

// show floating point and bignumber corners
void showcorners(char const *s)
{
    int dec = 20;
    char msg[100];
    char msg1[200];
    char msg3[400];
    bntostr(msg, dec, bnxmin);
    std::snprintf(msg1, std::size(msg1), "bnxmin=%s\nx_min= %.20f\n\n", msg, g_x_min);
    std::strcpy(msg3, s);
    std::strcat(msg3, "\n");
    std::strcat(msg3, msg1);
    bntostr(msg, dec, bnxmax);
    std::snprintf(msg1, std::size(msg1), "bnxmax=%s\nx_max= %.20f\n\n", msg, g_x_max);
    std::strcat(msg3, msg1);
    bntostr(msg, dec, bnymin);
    std::snprintf(msg1, std::size(msg1), "bnymin=%s\ny_min= %.20f\n\n", msg, g_y_min);
    std::strcat(msg3, msg1);
    bntostr(msg, dec, bnymax);
    std::snprintf(msg1, std::size(msg1), "bnymax=%s\ny_max= %.20f\n\n", msg, g_y_max);
    std::strcat(msg3, msg1);
    bntostr(msg, dec, bnx3rd);
    std::snprintf(msg1, std::size(msg1), "bnx3rd=%s\nx_3rd= %.20f\n\n", msg, g_x_3rd);
    std::strcat(msg3, msg1);
    bntostr(msg, dec, bny3rd);
    std::snprintf(msg1, std::size(msg1), "bny3rd=%s\ny_3rd= %.20f\n\n", msg, g_y_3rd);
    std::strcat(msg3, msg1);
    if (stopmsg(STOPMSG_NONE, msg3))
    {
        goodbye();
    }
}

// show globals
void showbfglobals(char const *s)
{
    char msg[300];
    std::snprintf(msg, std::size(msg),
        "%s\n"                                                       //
        "bnstep=%d bnlength=%d intlength=%d rlength=%d padding=%d\n" //
        "shiftfactor=%d decimals=%d bflength=%d rbflength=%d \n"     //
        "bfdecimals=%d ",                                            //
        s,                                                           //
        bnstep, bnlength, intlength, rlength, padding,               //
        shiftfactor, g_decimals, bflength, rbflength,                //
        bfdecimals);
    if (stopmsg(STOPMSG_NONE, msg))
    {
        goodbye();
    }
}

void showcornersbf(char const *s)
{
    int dec = g_decimals;
    char msg[100];
    char msg1[200];
    char msg3[600];
    if (dec > 20)
    {
        dec = 20;
    }
    bftostr(msg, dec, g_bf_x_min);
    std::snprintf(msg1, std::size(msg1),
        "bf_x_min=%s\n"                                //
        "x_min= %.20f decimals %d bflength %d\n\n", //
        msg,                                           //
        g_x_min, g_decimals, bflength);
    std::strcpy(msg3, s);
    std::strcat(msg3, "\n");
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_x_max);
    std::snprintf(msg1, std::size(msg1), "bf_x_max=%s\nx_max= %.20f\n\n", msg, g_x_max);
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_y_min);
    std::snprintf(msg1, std::size(msg1), "bf_y_min=%s\ny_min= %.20f\n\n", msg, g_y_min);
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_y_max);
    std::snprintf(msg1, std::size(msg1), "bf_y_max=%s\ny_max= %.20f\n\n", msg, g_y_max);
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_x_3rd);
    std::snprintf(msg1, std::size(msg1), "bf_x_3rd=%s\nxx_3rd= %.20f\n\n", msg, g_x_3rd);
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_y_3rd);
    std::snprintf(msg1, std::size(msg1), "bf_y_3rd=%s\ny_3rd= %.20f\n\n", msg, g_y_3rd);
    std::strcat(msg3, msg1);
    if (stopmsg(STOPMSG_NONE, msg3))
    {
        goodbye();
    }
}

void showcornersbfs(char const *s)
{
    int dec = 20;
    char msg[100];
    char msg1[200];
    char msg3[500];
    bftostr(msg, dec, g_bf_save_x_min);
    std::snprintf(msg1, std::size(msg1), "bf_save_x_min=%s\nx_min= %.20f\n\n", msg, g_x_min);
    std::strcpy(msg3, s);
    std::strcat(msg3, "\n");
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_save_x_max);
    std::snprintf(msg1, std::size(msg1), "bf_save_x_max=%s\nx_max= %.20f\n\n", msg, g_x_max);
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_save_y_min);
    std::snprintf(msg1, std::size(msg1), "bf_save_y_min=%s\ny_min= %.20f\n\n", msg, g_y_min);
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_save_y_max);
    std::snprintf(msg1, std::size(msg1), "bf_save_y_max=%s\ny_max= %.20f\n\n", msg, g_y_max);
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_save_x_3rd);
    std::snprintf(msg1, std::size(msg1), "bf_save_x_3rd=%s\nx_3rd= %.20f\n\n", msg, g_x_3rd);
    std::strcat(msg3, msg1);
    bftostr(msg, dec, g_bf_save_y_3rd);
    std::snprintf(msg1, std::size(msg1), "bf_save_y_3rd=%s\ny_3rd= %.20f\n\n", msg, g_y_3rd);
    std::strcat(msg3, msg1);
    if (stopmsg(STOPMSG_NONE, msg3))
    {
        goodbye();
    }
}

void show_two_bf(char const *s1, bf_t t1, char const *s2, bf_t t2, int digits)
{
    char msg1[200];
    char msg2[200];
    char msg3[500];
    bftostr_e(msg1, digits, t1);
    bftostr_e(msg2, digits, t2);
    std::snprintf(msg3, std::size(msg3), "\n%s->%s\n%s->%s", s1, msg1, s2, msg2);
    if (stopmsg(STOPMSG_NONE, msg3))
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
    bftostr_e(msg1, digits, t1);
    bftostr_e(msg2, digits, t2);
    bftostr_e(msg3, digits, t3);
    std::snprintf(msg4, std::size(msg4), "\n%s->%s\n%s->%s\n%s->%s", s1, msg1, s2, msg2, s3, msg3);
    if (stopmsg(STOPMSG_NONE, msg4))
    {
        goodbye();
    }
}

// for aspect ratio debugging
void showaspect(char const *s)
{
    bf_t bt1;
    bf_t bt2;
    bf_t aspect;
    char msg[300];
    char str[100];
    int saved;
    saved = save_stack();
    bt1    = alloc_stack(rbflength+2);
    bt2    = alloc_stack(rbflength+2);
    aspect = alloc_stack(rbflength+2);
    sub_bf(bt1, g_bf_x_max, g_bf_x_min);
    sub_bf(bt2, g_bf_y_max, g_bf_y_min);
    div_bf(aspect, bt2, bt1);
    bftostr(str, 10, aspect);
    std::snprintf(msg, std::size(msg), "aspect %s\nfloat %13.10f\nbf    %s\n\n",
            s,
            (g_y_max-g_y_min)/(g_x_max-g_x_min),
            str);
    if (stopmsg(STOPMSG_NONE, msg))
    {
        goodbye();
    }
    restore_stack(saved);
}

// compare a double and bignumber
void comparevalues(char const *s, LDBL x, bn_t bnx)
{
    int dec = 40;
    char msg[100];
    char msg1[300];
    bntostr(msg, dec, bnx);
    std::snprintf(msg1, std::size(msg1), "%s\nbignum=%s\ndouble=%.20Lf\n\n", s, msg, x);
    if (stopmsg(STOPMSG_NONE, msg1))
    {
        goodbye();
    }
}
// compare a double and bignumber
void comparevaluesbf(char const *s, LDBL x, bf_t bfx)
{
    int dec = 40;
    char msg[300];
    char msg1[700];
    bftostr_e(msg, dec, bfx);
    std::snprintf(msg1, std::size(msg1), "%s\nbignum=%s\ndouble=%.20Lf\n\n", s, msg, x);
    if (stopmsg(STOPMSG_NONE, msg1))
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
    bftostr_e(msg+std::strlen(s), 40, n);
    msg[79] = 0;
    if (stopmsg(STOPMSG_NONE, msg))
    {
        goodbye();
    }
}
#endif

void bfcornerstofloat()
{
    if (bf_math != bf_math_type::NONE)
    {
        g_x_min = (double)bftofloat(g_bf_x_min);
        g_y_min = (double)bftofloat(g_bf_y_min);
        g_x_max = (double)bftofloat(g_bf_x_max);
        g_y_max = (double)bftofloat(g_bf_y_max);
        g_x_3rd = (double)bftofloat(g_bf_x_3rd);
        g_y_3rd = (double)bftofloat(g_bf_y_3rd);
    }
    for (int i = 0; i < MAX_PARAMS; i++)
    {
        if (typehasparm(g_fractal_type, i, nullptr))
        {
            g_params[i] = (double)bftofloat(bfparms[i]);
        }
    }
}

// --------------------------------------------------------------------
//    Bignumber Bailout Routines
// --------------------------------------------------------------------

// mandel_bntoint() can only be used for intlength of 1
#define mandel_bntoint(n) (*(n + bnlength - 1)) // assumes intlength of 1

// Note:
// No need to set magnitude
// as color schemes that need it calculate it later.

int  bnMODbailout()
{
    long longmagnitude;

    square_bn(bntmpsqrx, bnnew.x);
    square_bn(bntmpsqry, bnnew.y);
    add_bn(bntmp, bntmpsqrx+shiftfactor, bntmpsqry+shiftfactor);

    longmagnitude = bntoint(bntmp);  // works with any fractal type
    if (longmagnitude >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(bnold.x, bnnew.x);
    copy_bn(bnold.y, bnnew.y);
    return 0;
}

int  bnREALbailout()
{
    long longtempsqrx;

    square_bn(bntmpsqrx, bnnew.x);
    square_bn(bntmpsqry, bnnew.y);
    longtempsqrx = bntoint(bntmpsqrx+shiftfactor);
    if (longtempsqrx >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(bnold.x, bnnew.x);
    copy_bn(bnold.y, bnnew.y);
    return 0;
}


int  bnIMAGbailout()
{
    long longtempsqry;

    square_bn(bntmpsqrx, bnnew.x);
    square_bn(bntmpsqry, bnnew.y);
    longtempsqry = bntoint(bntmpsqry+shiftfactor);
    if (longtempsqry >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(bnold.x, bnnew.x);
    copy_bn(bnold.y, bnnew.y);
    return 0;
}

int  bnORbailout()
{
    long longtempsqrx;
    long longtempsqry;

    square_bn(bntmpsqrx, bnnew.x);
    square_bn(bntmpsqry, bnnew.y);
    longtempsqrx = bntoint(bntmpsqrx+shiftfactor);
    longtempsqry = bntoint(bntmpsqry+shiftfactor);
    if (longtempsqrx >= (long)g_magnitude_limit || longtempsqry >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(bnold.x, bnnew.x);
    copy_bn(bnold.y, bnnew.y);
    return 0;
}

int  bnANDbailout()
{
    long longtempsqrx;
    long longtempsqry;

    square_bn(bntmpsqrx, bnnew.x);
    square_bn(bntmpsqry, bnnew.y);
    longtempsqrx = bntoint(bntmpsqrx+shiftfactor);
    longtempsqry = bntoint(bntmpsqry+shiftfactor);
    if (longtempsqrx >= (long)g_magnitude_limit && longtempsqry >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(bnold.x, bnnew.x);
    copy_bn(bnold.y, bnnew.y);
    return 0;
}

int  bnMANHbailout()
{
    long longtempmag;

    square_bn(bntmpsqrx, bnnew.x);
    square_bn(bntmpsqry, bnnew.y);
    // note: in next five lines, bnold is just used as a temporary variable
    abs_bn(bnold.x, bnnew.x);
    abs_bn(bnold.y, bnnew.y);
    add_bn(bntmp, bnold.x, bnold.y);
    square_bn(bnold.x, bntmp);
    longtempmag = bntoint(bnold.x+shiftfactor);
    if (longtempmag >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(bnold.x, bnnew.x);
    copy_bn(bnold.y, bnnew.y);
    return 0;
}

int  bnMANRbailout()
{
    long longtempmag;

    square_bn(bntmpsqrx, bnnew.x);
    square_bn(bntmpsqry, bnnew.y);
    add_bn(bntmp, bnnew.x, bnnew.y); // don't need abs since we square it next
    // note: in next two lines, bnold is just used as a temporary variable
    square_bn(bnold.x, bntmp);
    longtempmag = bntoint(bnold.x+shiftfactor);
    if (longtempmag >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(bnold.x, bnnew.x);
    copy_bn(bnold.y, bnnew.y);
    return 0;
}

int  bfMODbailout()
{
    long longmagnitude;

    square_bf(bftmpsqrx, bfnew.x);
    square_bf(bftmpsqry, bfnew.y);
    add_bf(bftmp, bftmpsqrx, bftmpsqry);

    longmagnitude = bftoint(bftmp);
    if (longmagnitude >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bf(bfold.x, bfnew.x);
    copy_bf(bfold.y, bfnew.y);
    return 0;
}

int  bfREALbailout()
{
    long longtempsqrx;

    square_bf(bftmpsqrx, bfnew.x);
    square_bf(bftmpsqry, bfnew.y);
    longtempsqrx = bftoint(bftmpsqrx);
    if (longtempsqrx >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bf(bfold.x, bfnew.x);
    copy_bf(bfold.y, bfnew.y);
    return 0;
}


int  bfIMAGbailout()
{
    long longtempsqry;

    square_bf(bftmpsqrx, bfnew.x);
    square_bf(bftmpsqry, bfnew.y);
    longtempsqry = bftoint(bftmpsqry);
    if (longtempsqry >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bf(bfold.x, bfnew.x);
    copy_bf(bfold.y, bfnew.y);
    return 0;
}

int  bfORbailout()
{
    long longtempsqrx;
    long longtempsqry;

    square_bf(bftmpsqrx, bfnew.x);
    square_bf(bftmpsqry, bfnew.y);
    longtempsqrx = bftoint(bftmpsqrx);
    longtempsqry = bftoint(bftmpsqry);
    if (longtempsqrx >= (long)g_magnitude_limit || longtempsqry >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bf(bfold.x, bfnew.x);
    copy_bf(bfold.y, bfnew.y);
    return 0;
}

int  bfANDbailout()
{
    long longtempsqrx;
    long longtempsqry;

    square_bf(bftmpsqrx, bfnew.x);
    square_bf(bftmpsqry, bfnew.y);
    longtempsqrx = bftoint(bftmpsqrx);
    longtempsqry = bftoint(bftmpsqry);
    if (longtempsqrx >= (long)g_magnitude_limit && longtempsqry >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bf(bfold.x, bfnew.x);
    copy_bf(bfold.y, bfnew.y);
    return 0;
}

int  bfMANHbailout()
{
    long longtempmag;

    square_bf(bftmpsqrx, bfnew.x);
    square_bf(bftmpsqry, bfnew.y);
    // note: in next five lines, bfold is just used as a temporary variable
    abs_bf(bfold.x, bfnew.x);
    abs_bf(bfold.y, bfnew.y);
    add_bf(bftmp, bfold.x, bfold.y);
    square_bf(bfold.x, bftmp);
    longtempmag = bftoint(bfold.x);
    if (longtempmag >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bf(bfold.x, bfnew.x);
    copy_bf(bfold.y, bfnew.y);
    return 0;
}

int  bfMANRbailout()
{
    long longtempmag;

    square_bf(bftmpsqrx, bfnew.x);
    square_bf(bftmpsqry, bfnew.y);
    add_bf(bftmp, bfnew.x, bfnew.y); // don't need abs since we square it next
    // note: in next two lines, bfold is just used as a temporary variable
    square_bf(bfold.x, bftmp);
    longtempmag = bftoint(bfold.x);
    if (longtempmag >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bf(bfold.x, bfnew.x);
    copy_bf(bfold.y, bfnew.y);
    return 0;
}

bool MandelbnSetup()
{
    // this should be set up dynamically based on corners
    bn_t bntemp1;
    bn_t bntemp2;
    int saved;
    saved = save_stack();
    bntemp1 = alloc_stack(bnlength);
    bntemp2 = alloc_stack(bnlength);

    bftobn(bnxmin, g_bf_x_min);
    bftobn(bnxmax, g_bf_x_max);
    bftobn(bnymin, g_bf_y_min);
    bftobn(bnymax, g_bf_y_max);
    bftobn(bnx3rd, g_bf_x_3rd);
    bftobn(bny3rd, g_bf_y_3rd);

    bf_math = bf_math_type::BIGNUM;

    // bnxdel = (bnxmax - bnx3rd)/(xdots-1)
    sub_bn(bnxdel, bnxmax, bnx3rd);
    div_a_bn_int(bnxdel, (U16)(g_logical_screen_x_dots - 1));

    // bnydel = (bnymax - bny3rd)/(ydots-1)
    sub_bn(bnydel, bnymax, bny3rd);
    div_a_bn_int(bnydel, (U16)(g_logical_screen_y_dots - 1));

    // bnxdel2 = (bnx3rd - bnxmin)/(ydots-1)
    sub_bn(bnxdel2, bnx3rd, bnxmin);
    div_a_bn_int(bnxdel2, (U16)(g_logical_screen_y_dots - 1));

    // bnydel2 = (bny3rd - bnymin)/(xdots-1)
    sub_bn(bnydel2, bny3rd, bnymin);
    div_a_bn_int(bnydel2, (U16)(g_logical_screen_x_dots - 1));

    abs_bn(bnclosenuff, bnxdel);
    if (cmp_bn(abs_bn(bntemp1, bnxdel2), bnclosenuff) > 0)
    {
        copy_bn(bnclosenuff, bntemp1);
    }
    if (cmp_bn(abs_bn(bntemp1, bnydel), abs_bn(bntemp2, bnydel2)) > 0)
    {
        if (cmp_bn(bntemp1, bnclosenuff) > 0)
        {
            copy_bn(bnclosenuff, bntemp1);
        }
    }
    else if (cmp_bn(bntemp2, bnclosenuff) > 0)
    {
        copy_bn(bnclosenuff, bntemp2);
    }
    {
        int t;
        t = std::abs(g_periodicity_check);
        while (t--)
        {
            half_a_bn(bnclosenuff);
        }
    }

    g_c_exponent = (int)g_params[2];
    switch (g_fractal_type)
    {
    case fractal_type::JULIAFP:
        bftobn(bnparm.x, bfparms[0]);
        bftobn(bnparm.y, bfparms[1]);
        break;
    case fractal_type::FPMANDELZPOWER:
        init_big_pi();
        if ((double)g_c_exponent == g_params[2] && (g_c_exponent & 1))   // odd exponents
        {
            g_symmetry = symmetry_type::XY_AXIS_NO_PARAM;
        }
        if (g_params[3] != 0)
        {
            g_symmetry = symmetry_type::NONE;
        }
        break;
    case fractal_type::FPJULIAZPOWER:
        init_big_pi();
        bftobn(bnparm.x, bfparms[0]);
        bftobn(bnparm.y, bfparms[1]);
        if ((g_c_exponent & 1) || g_params[3] != 0.0 || (double)g_c_exponent != g_params[2])
        {
            g_symmetry = symmetry_type::NONE;
        }
        break;
    case fractal_type::DIVIDE_BROT5:
        init_big_pi();
        g_c_exponent = -((int) g_params[0] - 2); /* use negative here so only need it once */
        g_b_const = g_params[1] + 1.0e-20;
        break;
    default:
        break;
    }

    restore_stack(saved);
    return true;
}

bool MandelbfSetup()
{
    // this should be set up dynamically based on corners
    bf_t bftemp1;
    bf_t bftemp2;
    int saved;
    saved = save_stack();
    bftemp1 = alloc_stack(bflength+2);
    bftemp2 = alloc_stack(bflength+2);

    bf_math = bf_math_type::BIGFLT;

    // bfxdel = (g_bf_x_max - g_bf_x_3rd)/(xdots-1)
    sub_bf(bfxdel, g_bf_x_max, g_bf_x_3rd);
    div_a_bf_int(bfxdel, (U16)(g_logical_screen_x_dots - 1));

    // bfydel = (g_bf_y_max - g_bf_y_3rd)/(ydots-1)
    sub_bf(bfydel, g_bf_y_max, g_bf_y_3rd);
    div_a_bf_int(bfydel, (U16)(g_logical_screen_y_dots - 1));

    // bfxdel2 = (g_bf_x_3rd - g_bf_x_min)/(ydots-1)
    sub_bf(bfxdel2, g_bf_x_3rd, g_bf_x_min);
    div_a_bf_int(bfxdel2, (U16)(g_logical_screen_y_dots - 1));

    // bfydel2 = (g_bf_y_3rd - g_bf_y_min)/(xdots-1)
    sub_bf(bfydel2, g_bf_y_3rd, g_bf_y_min);
    div_a_bf_int(bfydel2, (U16)(g_logical_screen_x_dots - 1));

    abs_bf(bfclosenuff, bfxdel);
    if (cmp_bf(abs_bf(bftemp1, bfxdel2), bfclosenuff) > 0)
    {
        copy_bf(bfclosenuff, bftemp1);
    }
    if (cmp_bf(abs_bf(bftemp1, bfydel), abs_bf(bftemp2, bfydel2)) > 0)
    {
        if (cmp_bf(bftemp1, bfclosenuff) > 0)
        {
            copy_bf(bfclosenuff, bftemp1);
        }
    }
    else if (cmp_bf(bftemp2, bfclosenuff) > 0)
    {
        copy_bf(bfclosenuff, bftemp2);
    }
    {
        int t;
        t = std::abs(g_periodicity_check);
        while (t--)
        {
            half_a_bf(bfclosenuff);
        }
    }

    g_c_exponent = (int)g_params[2];
    switch (g_fractal_type)
    {
    case fractal_type::JULIAFP:
        copy_bf(bfparm.x, bfparms[0]);
        copy_bf(bfparm.y, bfparms[1]);
        break;
    case fractal_type::FPMANDELZPOWER:
        init_big_pi();
        if ((double)g_c_exponent == g_params[2] && (g_c_exponent & 1))   // odd exponents
        {
            g_symmetry = symmetry_type::XY_AXIS_NO_PARAM;
        }
        if (g_params[3] != 0)
        {
            g_symmetry = symmetry_type::NONE;
        }
        break;
    case fractal_type::FPJULIAZPOWER:
        init_big_pi();
        copy_bf(bfparm.x, bfparms[0]);
        copy_bf(bfparm.y, bfparms[1]);
        if ((g_c_exponent & 1) || g_params[3] != 0.0 || (double)g_c_exponent != g_params[2])
        {
            g_symmetry = symmetry_type::NONE;
        }
        break;
    case fractal_type::DIVIDE_BROT5:
        init_big_pi();
        g_c_exponent = -((int) g_params[0] - 2); /* use negative here so only need it once */
        g_b_const = g_params[1] + 1.0e-20;
        break;
    default:
        break;
    }

    restore_stack(saved);
    return true;
}

int mandelbn_per_pixel()
{
    // parm.x = g_x_min + col*delx + row*delx2
    mult_bn_int(bnparm.x, bnxdel, (U16)g_col);
    mult_bn_int(bntmp, bnxdel2, (U16)g_row);

    add_a_bn(bnparm.x, bntmp);
    add_a_bn(bnparm.x, bnxmin);

    // parm.y = g_y_max - row*dely - col*dely2;
    // note: in next four lines, bnold is just used as a temporary variable
    mult_bn_int(bnold.x, bnydel, (U16)g_row);
    mult_bn_int(bnold.y, bnydel2, (U16)g_col);
    add_a_bn(bnold.x, bnold.y);
    sub_bn(bnparm.y, bnymax, bnold.x);

    copy_bn(bnold.x, bnparm.x);
    copy_bn(bnold.y, bnparm.y);

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        floattobn(bnold.x, g_params[0]); // initial pertubation of parameters set
        floattobn(bnold.y, g_params[1]);
        g_color_iter = -1;
    }
    else
    {
        floattobn(bnnew.x, g_params[0]);
        floattobn(bnnew.y, g_params[1]);
        add_a_bn(bnold.x, bnnew.x);
        add_a_bn(bnold.y, bnnew.y);
    }

    // square has side effect - must copy first
    copy_bn(bnnew.x, bnold.x);
    copy_bn(bnnew.y, bnold.y);

    // Square these to rlength bytes of precision
    square_bn(bntmpsqrx, bnnew.x);
    square_bn(bntmpsqry, bnnew.y);

    return 1;                  // 1st iteration has been done
}

int mandelbf_per_pixel()
{
    // parm.x = g_x_min + col*delx + row*delx2
    mult_bf_int(bfparm.x, bfxdel, (U16)g_col);
    mult_bf_int(bftmp, bfxdel2, (U16)g_row);

    add_a_bf(bfparm.x, bftmp);
    add_a_bf(bfparm.x, g_bf_x_min);

    // parm.y = g_y_max - row*dely - col*dely2;
    // note: in next four lines, bfold is just used as a temporary variable
    mult_bf_int(bfold.x, bfydel, (U16)g_row);
    mult_bf_int(bfold.y, bfydel2, (U16)g_col);
    add_a_bf(bfold.x, bfold.y);
    sub_bf(bfparm.y, g_bf_y_max, bfold.x);

    copy_bf(bfold.x, bfparm.x);
    copy_bf(bfold.y, bfparm.y);

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        floattobf(bfold.x, g_params[0]); // initial pertubation of parameters set
        floattobf(bfold.y, g_params[1]);
        g_color_iter = -1;
    }
    else
    {
        floattobf(bfnew.x, g_params[0]);
        floattobf(bfnew.y, g_params[1]);
        add_a_bf(bfold.x, bfnew.x);
        add_a_bf(bfold.y, bfnew.y);
    }

    // square has side effect - must copy first
    copy_bf(bfnew.x, bfold.x);
    copy_bf(bfnew.y, bfold.y);

    // Square these to rbflength bytes of precision
    square_bf(bftmpsqrx, bfnew.x);
    square_bf(bftmpsqry, bfnew.y);

    return 1;                  // 1st iteration has been done
}

int
juliabn_per_pixel()
{
    // old.x = g_x_min + col*delx + row*delx2
    mult_bn_int(bnold.x, bnxdel, (U16)g_col);
    mult_bn_int(bntmp, bnxdel2, (U16)g_row);

    add_a_bn(bnold.x, bntmp);
    add_a_bn(bnold.x, bnxmin);

    // old.y = g_y_max - row*dely - col*dely2;
    // note: in next four lines, bnnew is just used as a temporary variable
    mult_bn_int(bnnew.x, bnydel, (U16)g_row);
    mult_bn_int(bnnew.y, bnydel2, (U16)g_col);
    add_a_bn(bnnew.x, bnnew.y);
    sub_bn(bnold.y, bnymax, bnnew.x);

    // square has side effect - must copy first
    copy_bn(bnnew.x, bnold.x);
    copy_bn(bnnew.y, bnold.y);

    // Square these to rlength bytes of precision
    square_bn(bntmpsqrx, bnnew.x);
    square_bn(bntmpsqry, bnnew.y);

    return 1;                  // 1st iteration has been done
}

int
juliabf_per_pixel()
{
    // old.x = g_x_min + col*delx + row*delx2
    mult_bf_int(bfold.x, bfxdel, (U16)g_col);
    mult_bf_int(bftmp, bfxdel2, (U16)g_row);

    add_a_bf(bfold.x, bftmp);
    add_a_bf(bfold.x, g_bf_x_min);

    // old.y = g_y_max - row*dely - col*dely2;
    // note: in next four lines, bfnew is just used as a temporary variable
    mult_bf_int(bfnew.x, bfydel, (U16)g_row);
    mult_bf_int(bfnew.y, bfydel2, (U16)g_col);
    add_a_bf(bfnew.x, bfnew.y);
    sub_bf(bfold.y, g_bf_y_max, bfnew.x);

    // square has side effect - must copy first
    copy_bf(bfnew.x, bfold.x);
    copy_bf(bfnew.y, bfold.y);

    // Square these to rbflength bytes of precision
    square_bf(bftmpsqrx, bfnew.x);
    square_bf(bftmpsqry, bfnew.y);

    return 1;                  // 1st iteration has been done
}

int
JuliabnFractal()
{
    // Don't forget, with bn_t numbers, after multiplying or squaring
    // you must shift over by shiftfactor to get the bn number.

    // bntmpsqrx and bntmpsqry were previously squared before getting to
    // this function, so they must be shifted.

    // new.x = tmpsqrx - tmpsqry + parm.x;
    sub_a_bn(bntmpsqrx+shiftfactor, bntmpsqry+shiftfactor);
    add_bn(bnnew.x, bntmpsqrx+shiftfactor, bnparm.x);

    // new.y = 2 * bnold.x * bnold.y + parm.y;
    mult_bn(bntmp, bnold.x, bnold.y); // ok to use unsafe here
    double_a_bn(bntmp+shiftfactor);
    add_bn(bnnew.y, bntmp+shiftfactor, bnparm.y);

    return g_bailout_bignum();
}

int
JuliabfFractal()
{
    // new.x = tmpsqrx - tmpsqry + parm.x;
    sub_a_bf(bftmpsqrx, bftmpsqry);
    add_bf(bfnew.x, bftmpsqrx, bfparm.x);

    // new.y = 2 * bfold.x * bfold.y + parm.y;
    mult_bf(bftmp, bfold.x, bfold.y); // ok to use unsafe here
    double_a_bf(bftmp);
    add_bf(bfnew.y, bftmp, bfparm.y);
    return g_bailout_bigfloat();
}

int
JuliaZpowerbnFractal()
{
    BNComplex parm2;
    int saved;
    saved = save_stack();

    parm2.x = alloc_stack(bnlength);
    parm2.y = alloc_stack(bnlength);

    floattobn(parm2.x, g_params[2]);
    floattobn(parm2.y, g_params[3]);
    ComplexPower_bn(&bnnew, &bnold, &parm2);
    add_bn(bnnew.x, bnparm.x, bnnew.x+shiftfactor);
    add_bn(bnnew.y, bnparm.y, bnnew.y+shiftfactor);
    restore_stack(saved);
    return g_bailout_bignum();
}

int
JuliaZpowerbfFractal()
{
    BFComplex parm2;
    int saved;
    saved = save_stack();

    parm2.x = alloc_stack(bflength+2);
    parm2.y = alloc_stack(bflength+2);

    floattobf(parm2.x, g_params[2]);
    floattobf(parm2.y, g_params[3]);
    ComplexPower_bf(&bfnew, &bfold, &parm2);
    add_bf(bfnew.x, bfparm.x, bfnew.x);
    add_bf(bfnew.y, bfparm.y, bfnew.y);
    restore_stack(saved);
    return g_bailout_bigfloat();
}

DComplex cmplxbntofloat(BNComplex *s)
{
    DComplex t;
    t.x = (double)bntofloat(s->x);
    t.y = (double)bntofloat(s->y);
    return t;
}

DComplex cmplxbftofloat(BFComplex *s)
{
    DComplex t;
    t.x = (double)bftofloat(s->x);
    t.y = (double)bftofloat(s->y);
    return t;
}

BFComplex *cmplxlog_bf(BFComplex *t, BFComplex *s)
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

BFComplex *cplxmul_bf(BFComplex *t, BFComplex *x, BFComplex *y)
{
    bf_t tmp1;
    int saved;
    saved = save_stack();
    tmp1 = alloc_stack(rbflength+2);
    mult_bf(t->x, x->x, y->x);
    mult_bf(t->y, x->y, y->y);
    sub_bf(t->x, t->x, t->y);

    mult_bf(tmp1, x->x, y->y);
    mult_bf(t->y, x->y, y->x);
    add_bf(t->y, tmp1, t->y);
    restore_stack(saved);
    return t;
}

BFComplex *cplxdiv_bf(BFComplex *t, BFComplex *x, BFComplex *y)
{
    bf_t tmp1, denom;
    int saved;
    saved = save_stack();
    tmp1 = alloc_stack(rbflength + 2);
    denom = alloc_stack(rbflength + 2);

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

BFComplex *ComplexPower_bf(BFComplex *t, BFComplex *xx, BFComplex *yy)
{
    BFComplex tmp;
    bf_t e2x;
    bf_t siny;
    bf_t cosy;
    int saved;
    saved = save_stack();
    e2x  = alloc_stack(rbflength+2);
    siny = alloc_stack(rbflength+2);
    cosy = alloc_stack(rbflength+2);
    tmp.x = alloc_stack(rbflength+2);
    tmp.y = alloc_stack(rbflength+2);

    // 0 raised to anything is 0
    if (is_bf_zero(xx->x) && is_bf_zero(xx->y))
    {
        clear_bf(t->x);
        clear_bf(t->y);
        return t;
    }

    cmplxlog_bf(t, xx);
    cplxmul_bf(&tmp, t, yy);
    exp_bf(e2x, tmp.x);
    sincos_bf(siny, cosy, tmp.y);
    mult_bf(t->x, e2x, cosy);
    mult_bf(t->y, e2x, siny);
    restore_stack(saved);
    return t;
}

BNComplex *cmplxlog_bn(BNComplex *t, BNComplex *s)
{
    if (is_bn_zero(s->x) && is_bn_zero(s->y))
    {
        clear_bn(t->x);
        clear_bn(t->y);
    }
    else
    {
        add_bn(t->x, t->x + shiftfactor, t->y + shiftfactor);
        ln_bn(t->x, t->x);
        half_a_bn(t->x);
        atan2_bn(t->y, s->y, s->x);
    }
    return t;
}

BNComplex *cplxmul_bn(BNComplex *t, BNComplex *x, BNComplex *y)
{
    bn_t tmp1;
    int saved;
    saved = save_stack();
    tmp1 = alloc_stack(rlength);
    mult_bn(t->x, x->x, y->x);
    mult_bn(t->y, x->y, y->y);
    sub_bn(t->x, t->x + shiftfactor, t->y + shiftfactor);

    mult_bn(tmp1, x->x, y->y);
    mult_bn(t->y, x->y, y->x);
    add_bn(t->y, tmp1 + shiftfactor, t->y + shiftfactor);
    restore_stack(saved);
    return t;
}

BNComplex *cplxdiv_bn(BNComplex *t, BNComplex *x, BNComplex *y)
{
    bn_t tmp1, tmp2, denom;
    int saved;
    saved = save_stack();
    tmp1 = alloc_stack(rlength);
    tmp2 = alloc_stack(rlength);
    denom = alloc_stack(rlength);

    square_bn(tmp1, y->x);
    square_bn(tmp2, y->y);
    add_bn(denom, tmp1 + shiftfactor, tmp2 + shiftfactor);

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
        add_bn(tmp2, tmp1 + shiftfactor, t->x + shiftfactor);
        div_bn(t->x, tmp2, denom);

        mult_bn(tmp1, x->y, y->x);
        mult_bn(t->y, x->x, y->y);
        sub_bn(tmp2, tmp1 + shiftfactor, t->y + shiftfactor);
        div_bn(t->y, tmp2, denom);
    }

    restore_stack(saved);
    return t;
}

// note: ComplexPower_bn() returns need to be +shiftfactor'ed
BNComplex *ComplexPower_bn(BNComplex *t, BNComplex *xx, BNComplex *yy)
{
    BNComplex tmp;
    bn_t e2x;
    bn_t siny;
    bn_t cosy;
    int saved;
    saved = save_stack();
    e2x  = alloc_stack(bnlength);
    siny = alloc_stack(bnlength);
    cosy = alloc_stack(bnlength);
    tmp.x = alloc_stack(rlength);
    tmp.y = alloc_stack(rlength);

    // 0 raised to anything is 0
    if (is_bn_zero(xx->x) && is_bn_zero(xx->y))
    {
        clear_bn(t->x);
        clear_bn(t->y);
        return t;
    }

    cmplxlog_bn(t, xx);
    cplxmul_bn(&tmp, t, yy);
    exp_bn(e2x, tmp.x);
    sincos_bn(siny, cosy, tmp.y);
    mult_bn(t->x, e2x, cosy);
    mult_bn(t->y, e2x, siny);
    restore_stack(saved);
    return t;
}
