#include "convert_center_mag.h"

#include "port.h"
#include "prototyp.h"

#include "biginit.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "id.h"
#include "id_data.h"

#include <cmath>

#define rad_to_deg(x) ((x)*(180.0/PI)) // most people "think" in degrees

/*
convert corners to center/mag
Rotation angles indicate how much the IMAGE has been rotated, not the
zoom box.  Same goes for the Skew angles
*/

void cvtcentermag(double *Xctr, double *Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
{
    double Height;

    // simple normal case first
    if (g_x_3rd == g_x_min && g_y_3rd == g_y_min)
    {
        // no rotation or skewing, but stretching is allowed
        double Width = g_x_max - g_x_min;
        Height = g_y_max - g_y_min;
        *Xctr = (g_x_min + g_x_max)/2.0;
        *Yctr = (g_y_min + g_y_max)/2.0;
        *Magnification  = 2.0/Height;
        *Xmagfactor =  Height / (DEFAULT_ASPECT * Width);
        *Rotation = 0.0;
        *Skew = 0.0;
    }
    else
    {
        // set up triangle ABC, having sides abc
        // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
        double tmpx1 = g_x_max - g_x_min;
        double tmpy1 = g_y_max - g_y_min;
        double c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

        tmpx1 = g_x_max - g_x_3rd;
        tmpy1 = g_y_min - g_y_3rd;
        double a2 = tmpx1*tmpx1 + tmpy1*tmpy1;
        double a = std::sqrt(a2);
        *Rotation = -rad_to_deg(std::atan2(tmpy1, tmpx1));   // negative for image rotation

        double tmpx2 = g_x_min - g_x_3rd;
        double tmpy2 = g_y_max - g_y_3rd;
        double b2 = tmpx2*tmpx2 + tmpy2*tmpy2;
        double b = std::sqrt(b2);

        double tmpa = std::acos((a2+b2-c2)/(2*a*b)); // save tmpa for later use
        *Skew = 90.0 - rad_to_deg(tmpa);

        *Xctr = (g_x_min + g_x_max)*0.5;
        *Yctr = (g_y_min + g_y_max)*0.5;

        Height = b * std::sin(tmpa);

        *Magnification  = 2.0/Height; // 1/(h/2)
        *Xmagfactor = Height / (DEFAULT_ASPECT * a);

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0
            && g_debug_flag != debug_flags::allow_negative_cross_product)
        {
            *Skew = -*Skew;
            *Xmagfactor = -*Xmagfactor;
            *Magnification = -*Magnification;
        }
    }
    // just to make par file look nicer
    if (*Magnification < 0)
    {
        *Magnification = -*Magnification;
        *Rotation += 180;
    }
#ifdef DEBUG
    {
        double txmin, txmax, tx3rd, tymin, tymax, ty3rd;
        double error;
        txmin = xxmin;
        txmax = xxmax;
        tx3rd = xx3rd;
        tymin = yymin;
        tymax = yymax;
        ty3rd = yy3rd;
        cvtcorners(*Xctr, *Yctr, *Magnification, *Xmagfactor, *Rotation, *Skew);
        error = sqr(txmin - xxmin) +
                sqr(txmax - xxmax) +
                sqr(tx3rd - xx3rd) +
                sqr(tymin - yymin) +
                sqr(tymax - yymax) +
                sqr(ty3rd - yy3rd);
        if (error > .001)
        {
            showcornersdbl("cvtcentermag problem");
        }
        xxmin = txmin;
        xxmax = txmax;
        xx3rd = tx3rd;
        yymin = tymin;
        yymax = tymax;
        yy3rd = ty3rd;
    }
#endif
    return;
}


// convert corners to center/mag using bf
void cvtcentermagbf(bf_t Xctr, bf_t Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
{
    // needs to be LDBL or won't work past 307 (-DBL_MIN_10_EXP) or so digits
    LDBL Height;
    bf_t bfWidth;
    bf_t bftmpx;
    int saved;

    saved = save_stack();

    // simple normal case first
    // if (xx3rd == xxmin && yy3rd == yymin)
    if (!cmp_bf(g_bf_x_3rd, g_bf_x_min) && !cmp_bf(g_bf_y_3rd, g_bf_y_min))
    {
        // no rotation or skewing, but stretching is allowed
        bfWidth  = alloc_stack(bflength+2);
        bf_t bfHeight = alloc_stack(bflength+2);
        // Width  = xxmax - xxmin;
        sub_bf(bfWidth, g_bf_x_max, g_bf_x_min);
        LDBL Width = bftofloat(bfWidth);
        // Height = yymax - yymin;
        sub_bf(bfHeight, g_bf_y_max, g_bf_y_min);
        Height = bftofloat(bfHeight);
        // *Xctr = (xxmin + xxmax)/2;
        add_bf(Xctr, g_bf_x_min, g_bf_x_max);
        half_a_bf(Xctr);
        // *Yctr = (yymin + yymax)/2;
        add_bf(Yctr, g_bf_y_min, g_bf_y_max);
        half_a_bf(Yctr);
        *Magnification  = 2/Height;
        *Xmagfactor = (double)(Height / (DEFAULT_ASPECT * Width));
        *Rotation = 0.0;
        *Skew = 0.0;
    }
    else
    {
        bftmpx = alloc_stack(bflength+2);
        bf_t bftmpy = alloc_stack(bflength+2);

        // set up triangle ABC, having sides abc
        // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
        // IMPORTANT: convert from bf AFTER subtracting

        // tmpx = xxmax - xxmin;
        sub_bf(bftmpx, g_bf_x_max, g_bf_x_min);
        LDBL tmpx1 = bftofloat(bftmpx);
        // tmpy = yymax - yymin;
        sub_bf(bftmpy, g_bf_y_max, g_bf_y_min);
        LDBL tmpy1 = bftofloat(bftmpy);
        LDBL c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

        // tmpx = xxmax - xx3rd;
        sub_bf(bftmpx, g_bf_x_max, g_bf_x_3rd);
        tmpx1 = bftofloat(bftmpx);

        // tmpy = yymin - yy3rd;
        sub_bf(bftmpy, g_bf_y_min, g_bf_y_3rd);
        tmpy1 = bftofloat(bftmpy);
        LDBL a2 = tmpx1*tmpx1 + tmpy1*tmpy1;
        LDBL a = sqrtl(a2);

        // divide tmpx and tmpy by |tmpx| so that double version of atan2() can be used
        // atan2() only depends on the ratio, this puts it in double's range
        int signx = sign(tmpx1);
        LDBL tmpy = 0.0;
        if (signx)
        {
            tmpy = tmpy1/tmpx1 * signx;    // tmpy = tmpy / |tmpx|
        }
        *Rotation = (double)(-rad_to_deg(std::atan2((double)tmpy, signx)));   // negative for image rotation

        // tmpx = xxmin - xx3rd;
        sub_bf(bftmpx, g_bf_x_min, g_bf_x_3rd);
        LDBL tmpx2 = bftofloat(bftmpx);
        // tmpy = yymax - yy3rd;
        sub_bf(bftmpy, g_bf_y_max, g_bf_y_3rd);
        LDBL tmpy2 = bftofloat(bftmpy);
        LDBL b2 = tmpx2*tmpx2 + tmpy2*tmpy2;
        LDBL b = sqrtl(b2);

        double tmpa = std::acos((double)((a2+b2-c2)/(2*a*b))); // save tmpa for later use
        *Skew = 90 - rad_to_deg(tmpa);

        // these are the only two variables that must use big precision
        // *Xctr = (xxmin + xxmax)/2;
        add_bf(Xctr, g_bf_x_min, g_bf_x_max);
        half_a_bf(Xctr);
        // *Yctr = (yymin + yymax)/2;
        add_bf(Yctr, g_bf_y_min, g_bf_y_max);
        half_a_bf(Yctr);

        Height = b * std::sin(tmpa);
        *Magnification  = 2/Height; // 1/(h/2)
        *Xmagfactor = (double)(Height / (DEFAULT_ASPECT * a));

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0
            && g_debug_flag != debug_flags::allow_negative_cross_product)
        {
            *Skew = -*Skew;
            *Xmagfactor = -*Xmagfactor;
            *Magnification = -*Magnification;
        }
    }
    if (*Magnification < 0)
    {
        *Magnification = -*Magnification;
        *Rotation += 180;
    }
    restore_stack(saved);
}
