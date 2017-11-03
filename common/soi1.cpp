/*
 * soi.c --  SOI
 *
 *  Simultaneous Orbit Iteration Image Generation Method. Computes
 *      rectangular regions by tracking the orbits of only a few key points.
 *
 * Copyright (c) 1994-1997 Michael R. Ganss. All Rights Reserved.
 *
 * This file is distributed under the same conditions as
 * AlmondBread. For further information see
 * <URL:http://www.cs.tu-berlin.de/~rms/AlmondBread>.
 *
 */
#include <float.h>
#include <time.h>
#include <string.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif
#include "port.h"
#include "prototyp.h"
#include "drivers.h"
#include "fractype.h"

#define FABS(x)  fabs(x)
#define FREXP(x, y) frexp(x, y)

#define EVERY 15
#define BASIN_COLOR 0

extern int rhombus_stack[10];
extern int rhombus_depth;
extern int max_rhombus_depth;
extern int minstackavail;
extern int minstack; // need this much stack to recurse
static double twidth;
static double equal;

static long iteration(double cr, double ci,
                      double re, double im,
                      long start)
{
    old.x = re;
    old.y = im;
    tempsqrx = sqr(old.x);
    tempsqry = sqr(old.y);
    g_float_param = &init;
    g_float_param->x = cr;
    g_float_param->y = ci;
    while (ORBITCALC() == 0 && start < maxit)
    {
        start++;
    }
    if (start >= maxit)
    {
        start = BASIN_COLOR;
    }
    return (start);
}

static void puthline(int x1, int y1, int x2, int color)
{
    int x;
    for (x = x1; x <= x2; x++)
    {
        (*plot)(x, y1, color);
    }
}

static void putbox(int x1, int y1, int x2, int y2, int color)
{
    for (; y1 <= y2; y1++)
    {
        puthline(x1, y1, x2, color);
    }
}

/* maximum side length beyond which we start regular scanning instead of
   subdividing */
#define SCAN 16

// pixel interleave used in scanning
#define INTERLEAVE 4

// compute the value of the interpolation polynomial at (x,y)
#define GET_REAL(x, y) \
    interpolate(cim1, midi, cim2, \
        interpolate(cre1, midr, cre2, zre1, zre5, zre2, x), \
        interpolate(cre1, midr, cre2, zre6, zre9, zre7, x), \
        interpolate(cre1, midr, cre2, zre3, zre8, zre4, x), y)
#define GET_IMAG(x, y) \
    interpolate(cre1, midr, cre2, \
        interpolate(cim1, midi, cim2, zim1, zim6, zim3, y), \
        interpolate(cim1, midi, cim2, zim5, zim9, zim8, y), \
        interpolate(cim1, midi, cim2, zim2, zim7, zim4, y), x)

/* compute the value of the interpolation polynomial at (x,y)
   from saved values before interpolation failed to stay within tolerance */
#define GET_SAVED_REAL(x, y) \
    interpolate(cim1, midi, cim2, \
        interpolate(cre1, midr, cre2, sr1, sr5, sr2, x), \
        interpolate(cre1, midr, cre2, sr6, sr9, sr7, x), \
        interpolate(cre1, midr, cre2, sr3, sr8, sr4, x), y)
#define GET_SAVED_IMAG(x, y) \
    interpolate(cre1, midr, cre2, \
        interpolate(cim1, midi, cim2, si1, si6, si3, y), \
        interpolate(cim1, midi, cim2, si5, si9, si8, y), \
        interpolate(cim1, midi, cim2, si2, si7, si4, y), x)

/* compute the value of the interpolation polynomial at (x,y)
   during scanning. Here, key values do not change, so we can precompute
   coefficients in one direction and simply evaluate the polynomial
   during scanning. */
#define GET_SCAN_REAL(x, y) \
    interpolate(cim1, midi, cim2, \
        EVALUATE(cre1, midr, state.br10, state.br11, state.br12, x), \
        EVALUATE(cre1, midr, state.br20, state.br21, state.br22, x), \
        EVALUATE(cre1, midr, state.br30, state.br31, state.br32, x), y)
#define GET_SCAN_IMAG(x, y) \
    interpolate(cre1, midr, cre2, \
        EVALUATE(cim1, midi, state.bi10, state.bi11, state.bi12, y), \
        EVALUATE(cim1, midi, state.bi20, state.bi21, state.bi22, y), \
        EVALUATE(cim1, midi, state.bi30, state.bi31, state.bi32, y), x)

/* compute coefficients of Newton polynomial (b0,..,b2) from
   (x0,w0),..,(x2,w2). */
#define INTERPOLATE(x0, x1, x2, w0, w1, w2, b0, b1, b2) \
    (b0) = (w0);                                        \
    (b1) = ((w1) - (w0))/((x1) - (x0));                 \
    (b2) = (((w2) - (w1))/((x2) - (x1)) - (b1))/((x2) - (x0))

// evaluate Newton polynomial given by (x0,b0),(x1,b1) at x:=t
#define EVALUATE(x0, x1, b0, b1, b2, t) \
    (((b2)*((t) - (x1)) + (b1))*((t) - (x0)) + (b0))

/* Newton Interpolation.
   It computes the value of the interpolation polynomial given by
   (x0,w0)..(x2,w2) at x:=t */
static double interpolate(double x0, double x1, double x2,
                        double w0, double w1, double w2,
                        double t)
{
    double b0 = w0, b1 = w1, b2 = w2, b;

    /*b0=(r0*b1-r1*b0)/(x1-x0);
    b1=(r1*b2-r2*b1)/(x2-x1);
    b0=(r0*b1-r2*b0)/(x2-x0);

    return (double)b0;*/
    b = (b1 - b0)/(x1 - x0);
    return (double)((((b2 - b1)/(x2 - x1) - b)/(x2 - x0))*(t - x1) + b)*(t - x0) + b0;
    /*
    if (t<x1)
      return w0+((t-x0)/(x1-x0))*(w1-w0);
    else
      return w1+((t-x1)/(x2-x1))*(w2-w1);*/
}

/* SOICompute - Perform simultaneous orbit iteration for a given rectangle

   Input: cre1..cim2 : values defining the four corners of the rectangle
          x1..y2     : corresponding pixel values
      zre1..zim9 : intermediate iterated values of the key points (key values)

      (cre1,cim1)               (cre2,cim1)
      (zre1,zim1)  (zre5,zim5)  (zre2,zim2)
           +------------+------------+
           |            |            |
           |            |            |
      (zre6,zim6)  (zre9,zim9)  (zre7,zim7)
           |            |            |
           |            |            |
           +------------+------------+
      (zre3,zim3)  (zre8,zim8)  (zre4,zim4)
      (cre1,cim2)               (cre2,cim2)

      iter       : current number of iterations
      */
static double zre1, zim1, zre2, zim2, zre3, zim3, zre4, zim4, zre5, zim5,
       zre6, zim6, zre7, zim7, zre8, zim8, zre9, zim9;
/*
   The purpose of this macro is to reduce the number of parameters of the
   function rhombus(), since this is a recursive function, and stack space
   under DOS is extremely limited.
*/

#define RHOMBUS(CRE1, CRE2, CIM1, CIM2, X1, X2, Y1, Y2, ZRE1, ZIM1, ZRE2, ZIM2, ZRE3, ZIM3, \
    ZRE4, ZIM4, ZRE5, ZIM5, ZRE6, ZIM6, ZRE7, ZIM7, ZRE8, ZIM8, ZRE9, ZIM9, ITER) \
    zre1 = (ZRE1);zim1 = (ZIM1);\
    zre2 = (ZRE2);zim2 = (ZIM2);\
    zre3 = (ZRE3);zim3 = (ZIM3);\
    zre4 = (ZRE4);zim4 = (ZIM4);\
    zre5 = (ZRE5);zim5 = (ZIM5);\
    zre6 = (ZRE6);zim6 = (ZIM6);\
    zre7 = (ZRE7);zim7 = (ZIM7);\
    zre8 = (ZRE8);zim8 = (ZIM8);\
    zre9 = (ZRE9);zim9 = (ZIM9);\
    status = rhombus((CRE1), (CRE2), (CIM1), (CIM2), (X1), (X2), (Y1), (Y2), (ITER)) != 0

namespace
{

struct soi_double_state
{
    int esc1;
    int esc2;
    int esc3;
    int esc4;
    int esc5;
    int esc6;
    int esc7;
    int esc8;
    int esc9;
    int tesc1;
    int tesc2;
    int tesc3;
    int tesc4;

    double re;
    double im;
    double restep;
    double imstep;
    double interstep;
    double helpre;
    double zre;
    double zim;
    double br10;
    double br11;
    double br12;
    double br20;
    double br21;
    double br22;
    double br30;
    double br31;
    double br32;
    double bi10;
    double bi11;
    double bi12;
    double bi20;
    double bi21;
    double bi22;
    double bi30;
    double bi31;
    double bi32;
    double l1;
    double l2;
    double rq1;
    double iq1;
    double rq2;
    double iq2;
    double rq3;
    double iq3;
    double rq4;
    double iq4;
    double rq5;
    double iq5;
    double rq6;
    double iq6;
    double rq7;
    double iq7;
    double rq8;
    double iq8;
    double rq9;
    double iq9;
    double cr1;
    double cr2;
    double ci1;
    double ci2;
    double tzr1;
    double tzi1;
    double tzr2;
    double tzi2;
    double tzr3;
    double tzi3;
    double tzr4;
    double tzi4;
    double trq1;
    double tiq1;
    double trq2;
    double tiq2;
    double trq3;
    double tiq3;
    double trq4;
    double tiq4;
};

soi_double_state state = { 0 };

}

static int rhombus(double cre1, double cre2, double cim1, double cim2,
                   int x1, int x2, int y1, int y2, long iter)
{
    // The following variables do not need their values saved
    // used in scanning
    static long savecolor, color, helpcolor;
    static int x, y, z, savex;

    // number of iterations before SOI iteration cycle
    static long before;
    static int avail;

    // the variables below need to have local copies for recursive calls
    // center of rectangle
    double midr = (cre1 + cre2)/2, midi = (cim1 + cim2)/2;

    double sr1;
    double si1;
    double sr2;
    double si2;
    double sr3;
    double si3;
    double sr4;
    double si4;
    double sr5;
    double si5;
    double sr6;
    double si6;
    double sr7;
    double si7;
    double sr8;
    double si8;
    double sr9;
    double si9;
    double re10;
    double re11;
    double re12;
    double re13;
    double re14;
    double re15;
    double re16;
    double re17;
    double re18;
    double re19;
    double re20;
    double re21;
    double im10;
    double im11;
    double im12;
    double im13;
    double im14;
    double im15;
    double im16;
    double im17;
    double im18;
    double im19;
    double im20;
    double im21;
    double re91;
    double re92;
    double re93;
    double re94;
    double im91;
    double im92;
    double im93;
    double im94;

    bool status = false;
    rhombus_depth++;
    avail = stackavail();
    if (avail < minstackavail)
    {
        minstackavail = avail;
    }
    if (rhombus_depth > max_rhombus_depth)
    {
        max_rhombus_depth = rhombus_depth;
    }
    rhombus_stack[rhombus_depth] = avail;

    if (driver_key_pressed())
    {
        status = true;
        goto rhombus_done;
    }
    if (iter > maxit)
    {
        putbox(x1, y1, x2, y2, 0);
        status = false;
        goto rhombus_done;
    }

    if ((y2 - y1 <= SCAN) || (avail < minstack))
    {
        // finish up the image by scanning the rectangle
scan:
        INTERPOLATE(cre1, midr, cre2, zre1, zre5, zre2, state.br10, state.br11, state.br12);
        INTERPOLATE(cre1, midr, cre2, zre6, zre9, zre7, state.br20, state.br21, state.br22);
        INTERPOLATE(cre1, midr, cre2, zre3, zre8, zre4, state.br30, state.br31, state.br32);

        INTERPOLATE(cim1, midi, cim2, zim1, zim6, zim3, state.bi10, state.bi11, state.bi12);
        INTERPOLATE(cim1, midi, cim2, zim5, zim9, zim8, state.bi20, state.bi21, state.bi22);
        INTERPOLATE(cim1, midi, cim2, zim2, zim7, zim4, state.bi30, state.bi31, state.bi32);

        state.restep = (cre2 - cre1)/(x2 - x1);
        state.imstep = (cim2 - cim1)/(y2 - y1);
        state.interstep = INTERLEAVE*state.restep;

        for (y = y1, state.im = cim1; y < y2; y++, state.im += state.imstep)
        {
            if (driver_key_pressed())
            {
                status = true;
                goto rhombus_done;
            }
            state.zre = GET_SCAN_REAL(cre1, state.im);
            state.zim = GET_SCAN_IMAG(cre1, state.im);
            savecolor = iteration(cre1, state.im, state.zre, state.zim, iter);
            if (savecolor < 0)
            {
                status = true;
                goto rhombus_done;
            }
            savex = x1;
            for (x = x1 + INTERLEAVE, state.re = cre1 + state.interstep; x < x2;
                    x += INTERLEAVE, state.re += state.interstep)
            {
                state.zre = GET_SCAN_REAL(state.re, state.im);
                state.zim = GET_SCAN_IMAG(state.re, state.im);

                color = iteration(state.re, state.im, state.zre, state.zim, iter);
                if (color < 0)
                {
                    status = true;
                    goto rhombus_done;
                }
                else if (color == savecolor)
                {
                    continue;
                }

                for (z = x - 1, state.helpre = state.re - state.restep; z > x - INTERLEAVE; z--, state.helpre -= state.restep)
                {
                    state.zre = GET_SCAN_REAL(state.helpre, state.im);
                    state.zim = GET_SCAN_IMAG(state.helpre, state.im);
                    helpcolor = iteration(state.helpre, state.im, state.zre, state.zim, iter);
                    if (helpcolor < 0)
                    {
                        status = true;
                        goto rhombus_done;
                    }
                    else if (helpcolor == savecolor)
                    {
                        break;
                    }
                    (*plot)(z, y, (int)(helpcolor&255));
                }

                if (savex < z)
                {
                    puthline(savex, y, z, (int)(savecolor&255));
                }
                else
                {
                    (*plot)(savex, y, (int)(savecolor&255));
                }

                savex = x;
                savecolor = color;
            }

            for (z = x2 - 1, state.helpre = cre2 - state.restep; z > savex; z--, state.helpre -= state.restep)
            {
                state.zre = GET_SCAN_REAL(state.helpre, state.im);
                state.zim = GET_SCAN_IMAG(state.helpre, state.im);
                helpcolor = iteration(state.helpre, state.im, state.zre, state.zim, iter);
                if (helpcolor < 0)
                {
                    status = true;
                    goto rhombus_done;
                }
                else if (helpcolor == savecolor)
                {
                    break;
                }

                (*plot)(z, y, (int)(helpcolor&255));
            }

            if (savex < z)
            {
                puthline(savex, y, z, (int)(savecolor&255));
            }
            else
            {
                (*plot)(savex, y, (int)(savecolor&255));
            }
        }
        status = false;
        goto rhombus_done;
    }

    state.rq1 = zre1*zre1;
    state.iq1 = zim1*zim1;
    state.rq2 = zre2*zre2;
    state.iq2 = zim2*zim2;
    state.rq3 = zre3*zre3;
    state.iq3 = zim3*zim3;
    state.rq4 = zre4*zre4;
    state.iq4 = zim4*zim4;
    state.rq5 = zre5*zre5;
    state.iq5 = zim5*zim5;
    state.rq6 = zre6*zre6;
    state.iq6 = zim6*zim6;
    state.rq7 = zre7*zre7;
    state.iq7 = zim7*zim7;
    state.rq8 = zre8*zre8;
    state.iq8 = zim8*zim8;
    state.rq9 = zre9*zre9;
    state.iq9 = zim9*zim9;

    state.cr1 = 0.75*cre1 + 0.25*cre2;
    state.cr2 = 0.25*cre1 + 0.75*cre2;
    state.ci1 = 0.75*cim1 + 0.25*cim2;
    state.ci2 = 0.25*cim1 + 0.75*cim2;

    state.tzr1 = GET_REAL(state.cr1, state.ci1);
    state.tzi1 = GET_IMAG(state.cr1, state.ci1);

    state.tzr2 = GET_REAL(state.cr2, state.ci1);
    state.tzi2 = GET_IMAG(state.cr2, state.ci1);

    state.tzr3 = GET_REAL(state.cr1, state.ci2);
    state.tzi3 = GET_IMAG(state.cr1, state.ci2);

    state.tzr4 = GET_REAL(state.cr2, state.ci2);
    state.tzi4 = GET_IMAG(state.cr2, state.ci2);

    state.trq1 = state.tzr1*state.tzr1;
    state.tiq1 = state.tzi1*state.tzi1;

    state.trq2 = state.tzr2*state.tzr2;
    state.tiq2 = state.tzi2*state.tzi2;

    state.trq3 = state.tzr3*state.tzr3;
    state.tiq3 = state.tzi3*state.tzi3;

    state.trq4 = state.tzr4*state.tzr4;
    state.tiq4 = state.tzi4*state.tzi4;

    before = iter;

    while (1)
    {
        sr1 = zre1;
        si1 = zim1;
        sr2 = zre2;
        si2 = zim2;
        sr3 = zre3;
        si3 = zim3;
        sr4 = zre4;
        si4 = zim4;
        sr5 = zre5;
        si5 = zim5;
        sr6 = zre6;
        si6 = zim6;
        sr7 = zre7;
        si7 = zim7;
        sr8 = zre8;
        si8 = zim8;
        sr9 = zre9;
        si9 = zim9;


#define SOI_ORBIT1(zr, rq, zi, iq, cr, ci, esc) \
    tempsqrx = rq;                              \
    tempsqry = iq;                              \
    old.x = zr;                                 \
    old.y = zi;                                 \
    g_float_param->x = cr;                      \
    g_float_param->y = ci;                      \
    (esc) = ORBITCALC();                        \
    (rq) = tempsqrx;                            \
    (iq) = tempsqry;                            \
    (zr) = new.x;                               \
    (zi) = new.y

#define SOI_ORBIT(zr, rq, zi, iq, cr, ci, esc)  \
    (zi) = ((zi) + (zi))*(zr) + (ci);           \
    (zr) = (rq) - (iq) + (cr);                  \
    (rq) = (zr)*(zr);                           \
    (iq) = (zi)*(zi);                           \
    (esc) = (((rq) + (iq)) > 16.0)?1:0

        // iterate key values
        SOI_ORBIT(zre1, state.rq1, zim1, state.iq1, cre1, cim1, state.esc1);
        /*
              zim1=(zim1+zim1)*zre1+cim1;
              zre1=rq1-iq1+cre1;
              rq1=zre1*zre1;
              iq1=zim1*zim1;
        */
        SOI_ORBIT(zre2, state.rq2, zim2, state.iq2, cre2, cim1, state.esc2);
        /*
              zim2=(zim2+zim2)*zre2+cim1;
              zre2=rq2-iq2+cre2;
              rq2=zre2*zre2;
              iq2=zim2*zim2;
        */
        SOI_ORBIT(zre3, state.rq3, zim3, state.iq3, cre1, cim2, state.esc3);
        /*
              zim3=(zim3+zim3)*zre3+cim2;
              zre3=rq3-iq3+cre1;
              rq3=zre3*zre3;
              iq3=zim3*zim3;
        */
        SOI_ORBIT(zre4, state.rq4, zim4, state.iq4, cre2, cim2, state.esc4);
        /*
              zim4=(zim4+zim4)*zre4+cim2;
              zre4=rq4-iq4+cre2;
              rq4=zre4*zre4;
              iq4=zim4*zim4;
        */
        SOI_ORBIT(zre5, state.rq5, zim5, state.iq5, midr, cim1, state.esc5);
        /*
              zim5=(zim5+zim5)*zre5+cim1;
              zre5=rq5-iq5+midr;
              rq5=zre5*zre5;
              iq5=zim5*zim5;
        */
        SOI_ORBIT(zre6, state.rq6, zim6, state.iq6, cre1, midi, state.esc6);
        /*
              zim6=(zim6+zim6)*zre6+midi;
              zre6=rq6-iq6+cre1;
              rq6=zre6*zre6;
              iq6=zim6*zim6;
        */
        SOI_ORBIT(zre7, state.rq7, zim7, state.iq7, cre2, midi, state.esc7);
        /*
              zim7=(zim7+zim7)*zre7+midi;
              zre7=rq7-iq7+cre2;
              rq7=zre7*zre7;
              iq7=zim7*zim7;
        */
        SOI_ORBIT(zre8, state.rq8, zim8, state.iq8, midr, cim2, state.esc8);
        /*
              zim8=(zim8+zim8)*zre8+cim2;
              zre8=rq8-iq8+midr;
              rq8=zre8*zre8;
              iq8=zim8*zim8;
        */
        SOI_ORBIT(zre9, state.rq9, zim9, state.iq9, midr, midi, state.esc9);
        /*
              zim9=(zim9+zim9)*zre9+midi;
              zre9=rq9-iq9+midr;
              rq9=zre9*zre9;
              iq9=zim9*zim9;
        */
        // iterate test point
        SOI_ORBIT(state.tzr1, state.trq1, state.tzi1, state.tiq1, state.cr1, state.ci1, state.tesc1);
        /*
              tzi1=(tzi1+tzi1)*tzr1+ci1;
              tzr1=trq1-tiq1+cr1;
              trq1=tzr1*tzr1;
              tiq1=tzi1*tzi1;
        */

        SOI_ORBIT(state.tzr2, state.trq2, state.tzi2, state.tiq2, state.cr2, state.ci1, state.tesc2);
        /*
              tzi2=(tzi2+tzi2)*tzr2+ci1;
              tzr2=trq2-tiq2+cr2;
              trq2=tzr2*tzr2;
              tiq2=tzi2*tzi2;
        */
        SOI_ORBIT(state.tzr3, state.trq3, state.tzi3, state.tiq3, state.cr1, state.ci2, state.tesc3);
        /*
              tzi3=(tzi3+tzi3)*tzr3+ci2;
              tzr3=trq3-tiq3+cr1;
              trq3=tzr3*tzr3;
              tiq3=tzi3*tzi3;
        */
        SOI_ORBIT(state.tzr4, state.trq4, state.tzi4, state.tiq4, state.cr2, state.ci2, state.tesc4);
        /*
              tzi4=(tzi4+tzi4)*tzr4+ci2;
              tzr4=trq4-tiq4+cr2;
              trq4=tzr4*tzr4;
              tiq4=tzi4*tzi4;
        */
        iter++;

        // if one of the iterated values bails out, subdivide
        /*
              if ((rq1+iq1)>16.0||
             (rq2+iq2)>16.0||
             (rq3+iq3)>16.0||
             (rq4+iq4)>16.0||
             (rq5+iq5)>16.0||
             (rq6+iq6)>16.0||
             (rq7+iq7)>16.0||
             (rq8+iq8)>16.0||
             (rq9+iq9)>16.0||
             (trq1+tiq1)>16.0||
             (trq2+tiq2)>16.0||
             (trq3+tiq3)>16.0||
             (trq4+tiq4)>16.0)
            break;
        */
        if (state.esc1||state.esc2||state.esc3||state.esc4||state.esc5||state.esc6||state.esc7||state.esc8||state.esc9||
                state.tesc1||state.tesc2||state.tesc3||state.tesc4)
        {
            break;
        }

        /* if maximum number of iterations is reached, the whole rectangle
        can be assumed part of M. This is of course best case behavior
        of SOI, we seldom get there */
        if (iter > maxit)
        {
            putbox(x1, y1, x2, y2, 0);
            status = false;
            goto rhombus_done;
        }

        /* now for all test points, check whether they exceed the
        allowed tolerance. if so, subdivide */
        state.l1 = GET_REAL(state.cr1, state.ci1);
        state.l1 = (state.tzr1 == 0.0)?
           (state.l1 == 0.0)?1.0:1000.0:
           state.l1/state.tzr1;
        if (FABS(1.0 - state.l1) > twidth)
        {
            break;
        }

        state.l2 = GET_IMAG(state.cr1, state.ci1);
        state.l2 = (state.tzi1 == 0.0)?
           (state.l2 == 0.0)?1.0:1000.0:
           state.l2/state.tzi1;
        if (FABS(1.0 - state.l2) > twidth)
        {
            break;
        }

        state.l1 = GET_REAL(state.cr2, state.ci1);
        state.l1 = (state.tzr2 == 0.0)?
           (state.l1 == 0.0)?1.0:1000.0:
           state.l1/state.tzr2;
        if (FABS(1.0 - state.l1) > twidth)
        {
            break;
        }

        state.l2 = GET_IMAG(state.cr2, state.ci1);
        state.l2 = (state.tzi2 == 0.0)?
           (state.l2 == 0.0)?1.0:1000.0:
           state.l2/state.tzi2;
        if (FABS(1.0 - state.l2) > twidth)
        {
            break;
        }

        state.l1 = GET_REAL(state.cr1, state.ci2);
        state.l1 = (state.tzr3 == 0.0)?
           (state.l1 == 0.0)?1.0:1000.0:
           state.l1/state.tzr3;
        if (FABS(1.0 - state.l1) > twidth)
        {
            break;
        }

        state.l2 = GET_IMAG(state.cr1, state.ci2);
        state.l2 = (state.tzi3 == 0.0)?
           (state.l2 == 0.0)?1.0:1000.0:
           state.l2/state.tzi3;
        if (FABS(1.0 - state.l2) > twidth)
        {
            break;
        }

        state.l1 = GET_REAL(state.cr2, state.ci2);
        state.l1 = (state.tzr4 == 0.0)?
           (state.l1 == 0.0)?1.0:1000.0:
           state.l1/state.tzr4;
        if (FABS(1.0 - state.l1) > twidth)
        {
            break;
        }

        state.l2 = GET_IMAG(state.cr2, state.ci2);
        state.l2 = (state.tzi4 == 0.0)?
           (state.l2 == 0.0)?1.0:1000.0:
           state.l2/state.tzi4;
        if (FABS(1.0 - state.l2) > twidth)
        {
            break;
        }
    }

    iter--;

    // this is a little heuristic I tried to improve performance.
    if (iter - before < 10)
    {
        zre1 = sr1;
        zim1 = si1;
        zre2 = sr2;
        zim2 = si2;
        zre3 = sr3;
        zim3 = si3;
        zre4 = sr4;
        zim4 = si4;
        zre5 = sr5;
        zim5 = si5;
        zre6 = sr6;
        zim6 = si6;
        zre7 = sr7;
        zim7 = si7;
        zre8 = sr8;
        zim8 = si8;
        zre9 = sr9;
        zim9 = si9;
        goto scan;
    }

    // compute key values for subsequent rectangles

    re10 = interpolate(cre1, midr, cre2, sr1, sr5, sr2, state.cr1);
    im10 = interpolate(cre1, midr, cre2, si1, si5, si2, state.cr1);

    re11 = interpolate(cre1, midr, cre2, sr1, sr5, sr2, state.cr2);
    im11 = interpolate(cre1, midr, cre2, si1, si5, si2, state.cr2);

    re20 = interpolate(cre1, midr, cre2, sr3, sr8, sr4, state.cr1);
    im20 = interpolate(cre1, midr, cre2, si3, si8, si4, state.cr1);

    re21 = interpolate(cre1, midr, cre2, sr3, sr8, sr4, state.cr2);
    im21 = interpolate(cre1, midr, cre2, si3, si8, si4, state.cr2);

    re15 = interpolate(cre1, midr, cre2, sr6, sr9, sr7, state.cr1);
    im15 = interpolate(cre1, midr, cre2, si6, si9, si7, state.cr1);

    re16 = interpolate(cre1, midr, cre2, sr6, sr9, sr7, state.cr2);
    im16 = interpolate(cre1, midr, cre2, si6, si9, si7, state.cr2);

    re12 = interpolate(cim1, midi, cim2, sr1, sr6, sr3, state.ci1);
    im12 = interpolate(cim1, midi, cim2, si1, si6, si3, state.ci1);

    re14 = interpolate(cim1, midi, cim2, sr2, sr7, sr4, state.ci1);
    im14 = interpolate(cim1, midi, cim2, si2, si7, si4, state.ci1);

    re17 = interpolate(cim1, midi, cim2, sr1, sr6, sr3, state.ci2);
    im17 = interpolate(cim1, midi, cim2, si1, si6, si3, state.ci2);

    re19 = interpolate(cim1, midi, cim2, sr2, sr7, sr4, state.ci2);
    im19 = interpolate(cim1, midi, cim2, si2, si7, si4, state.ci2);

    re13 = interpolate(cim1, midi, cim2, sr5, sr9, sr8, state.ci1);
    im13 = interpolate(cim1, midi, cim2, si5, si9, si8, state.ci1);

    re18 = interpolate(cim1, midi, cim2, sr5, sr9, sr8, state.ci2);
    im18 = interpolate(cim1, midi, cim2, si5, si9, si8, state.ci2);

    re91 = GET_SAVED_REAL(state.cr1, state.ci1);
    re92 = GET_SAVED_REAL(state.cr2, state.ci1);
    re93 = GET_SAVED_REAL(state.cr1, state.ci2);
    re94 = GET_SAVED_REAL(state.cr2, state.ci2);

    im91 = GET_SAVED_IMAG(state.cr1, state.ci1);
    im92 = GET_SAVED_IMAG(state.cr2, state.ci1);
    im93 = GET_SAVED_IMAG(state.cr1, state.ci2);
    im94 = GET_SAVED_IMAG(state.cr2, state.ci2);

    RHOMBUS(cre1, midr, cim1, midi, x1, ((x1 + x2) >> 1), y1, ((y1 + y2) >> 1),
            sr1, si1,
            sr5, si5,
            sr6, si6,
            sr9, si9,
            re10, im10,
            re12, im12,
            re13, im13,
            re15, im15,
            re91, im91,
            iter);
    RHOMBUS(midr, cre2, cim1, midi, (x1 + x2) >> 1, x2, y1, (y1 + y2) >> 1,
            sr5, si5,
            sr2, si2,
            sr9, si9,
            sr7, si7,
            re11, im11,
            re13, im13,
            re14, im14,
            re16, im16,
            re92, im92,
            iter);
    RHOMBUS(cre1, midr, midi, cim2, x1, (x1 + x2) >> 1, (y1 + y2) >> 1, y2,
            sr6, si6,
            sr9, si9,
            sr3, si3,
            sr8, si8,
            re15, im15,
            re17, im17,
            re18, im18,
            re20, im20,
            re93, im93,
            iter);
    RHOMBUS(midr, cre2, midi, cim2, (x1 + x2) >> 1, x2, (y1 + y2) >> 1, y2,
            sr9, si9,
            sr7, si7,
            sr8, si8,
            sr4, si4,
            re16, im16,
            re18, im18,
            re19, im19,
            re21, im21,
            re94, im94,
            iter);
rhombus_done:
    rhombus_depth--;
    return status ? 1 : 0;
}

void soi()
{
    // cppcheck-suppress unreadVariable
    bool status;
    double tolerance = 0.1;
    double stepx, stepy;
    double xxminl, xxmaxl, yyminl, yymaxl;
    minstackavail = 30000;
    rhombus_depth = -1;
    max_rhombus_depth = 0;
    if (bf_math != bf_math_type::NONE)
    {
        xxminl = (double)bftofloat(bfxmin);
        yyminl = (double)bftofloat(bfymin);
        xxmaxl = (double)bftofloat(bfxmax);
        yymaxl = (double)bftofloat(bfymax);
    }
    else
    {
        xxminl = xxmin;
        yyminl = yymin;
        xxmaxl = xxmax;
        yymaxl = yymax;
    }
    twidth = tolerance/(xdots - 1);
    stepx = (xxmaxl - xxminl)/xdots;
    stepy = (yyminl - yymaxl)/ydots;
    equal = (stepx < stepy ? stepx : stepy);

    RHOMBUS(xxminl, xxmaxl, yymaxl, yyminl,
            0, xdots, 0, ydots,
            xxminl, yymaxl,
            xxmaxl, yymaxl,
            xxminl, yyminl,
            xxmaxl, yyminl,
            (xxmaxl + xxminl)/2, yymaxl,
            xxminl, (yymaxl + yyminl)/2,
            xxmaxl, (yymaxl + yyminl)/2,
            (xxmaxl + xxminl)/2, yyminl,
            (xxminl + xxmaxl)/2, (yymaxl + yyminl)/2,
            1);
}
