/*
FRACTALS.C, FRACTALP.C and CALCFRAC.C actually calculate the fractal
images (well, SOMEBODY had to do it!).  The modules are set up so that
all logic that is independent of any fractal-specific code is in
CALCFRAC.C, the code that IS fractal-specific is in FRACTALS.C, and the
structure that ties (we hope!) everything together is in FRACTALP.C.
Original author Tim Wegner, but just about ALL the authors have
contributed SOME code to this routine at one time or another, or
contributed to one of the many massive restructurings.

The Fractal-specific routines are divided into three categories:

1. Routines that are called once-per-orbit to calculate the orbit
   value. These have names like "XxxxFractal", and their function
   pointers are stored in fractalspecific[fractype].orbitcalc. EVERY
   new fractal type needs one of these. Return 0 to continue iterations,
   1 if we're done. Results for integer fractals are left in 'lnew.x' and
   'lnew.y', for floating point fractals in 'new.x' and 'new.y'.

2. Routines that are called once per pixel to set various variables
   prior to the orbit calculation. These have names like xxx_per_pixel
   and are fairly generic - chances are one is right for your new type.
   They are stored in fractalspecific[fractype].per_pixel.

3. Routines that are called once per screen to set various variables.
   These have names like XxxxSetup, and are stored in
   fractalspecific[fractype].per_image.

4. The main fractal routine. Usually this will be standard_fractal(),
   but if you have written a stand-alone fractal routine independent
   of the standard_fractal mechanisms, your routine name goes here,
   stored in fractalspecific[fractype].calctype.per_image.

Adding a new fractal type should be simply a matter of adding an item
to the 'fractalspecific' structure, writing (or re-using one of the existing)
an appropriate setup, per_image, per_pixel, and orbit routines.

--------------------------------------------------------------------   */
#include <float.h>
#include <limits.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "externs.h"


#define NEWTONDEGREELIMIT  100

namespace
{

const long l16triglim = 8L << 16;       // domain limit of fast trig functions

} // namespace

LComplex g_l_coefficient, g_l_old_z, g_l_new_z, g_l_param, g_l_init, g_l_temp, ltmp2, g_l_param2;
long g_l_temp_sqr_x, g_l_temp_sqr_y;
int g_max_color;
int g_degree, g_basin;
double g_newton_r_over_d, g_degree_minus_1_over_degree, g_threshold;
DComplex tmp2;
DComplex g_marks_coefficient;
DComplex  staticroots[16]; // roots array for degree 16 or less
std::vector<DComplex> g_roots;
std::vector<MPC> g_mpc_roots;
long g_fudge_half;
DComplex g_power_z;
int     g_bit_shift_less_1;                  // bit shift less 1
bool g_overflow = false;

#define modulus(z)       (sqr((z).x)+sqr((z).y))
#define conjugate(pz)   ((pz)->y = 0.0 - (pz)->y)
#define distance(z1, z2)  (sqr((z1).x-(z2).x)+sqr((z1).y-(z2).y))
#define pMPsqr(z) (*pMPmul((z), (z)))
#define MPdistance(z1, z2)  (*pMPadd(pMPsqr(*pMPsub((z1).x, (z2).x)), pMPsqr(*pMPsub((z1).y, (z2).y))))

int g_c_exponent;


// These are local but I don't want to pass them as parameters
DComplex g_param_z1, g_param_z2;
DComplex *g_float_param;
LComplex *g_long_param; // used here and in jb.c

// --------------------------------------------------------------------
//              These variables are external for speed's sake only
// --------------------------------------------------------------------

double g_sin_x, g_cos_x;
double siny, cosy;
double tmpexp;
double tempsqrx, tempsqry;

double foldxinitx, foldyinity, foldxinity, foldyinitx;
long oldxinitx, oldyinity, oldxinity, oldyinitx;
long longtmp;

// These are for quaternions
double g_quaternion_c, g_quaternion_ci, g_quaternion_cj, g_quaternino_ck;

// temporary variables for trig use
long lcosx, lsinx;
long lcosy, lsiny;

/*
**  details of finite attractors (required for Magnet Fractals)
**  (can also be used in "coloring in" the lakes of Julia types)
*/

/*
**  pre-calculated values for fractal types Magnet2M & Magnet2J
*/
DComplex  T_Cm1;        // 3 * (floatparm - 1)
DComplex  T_Cm2;        // 3 * (floatparm - 2)
DComplex  T_Cm1Cm2;     // (floatparm - 1) * (floatparm - 2)

void FloatPreCalcMagnet2() // precalculation for Magnet2 (M & J) for speed
{
    T_Cm1.x = g_float_param->x - 1.0;
    T_Cm1.y = g_float_param->y;
    T_Cm2.x = g_float_param->x - 2.0;
    T_Cm2.y = g_float_param->y;
    T_Cm1Cm2.x = (T_Cm1.x * T_Cm2.x) - (T_Cm1.y * T_Cm2.y);
    T_Cm1Cm2.y = (T_Cm1.x * T_Cm2.y) + (T_Cm1.y * T_Cm2.x);
    T_Cm1.x += T_Cm1.x + T_Cm1.x;
    T_Cm1.y += T_Cm1.y + T_Cm1.y;
    T_Cm2.x += T_Cm2.x + T_Cm2.x;
    T_Cm2.y += T_Cm2.y + T_Cm2.y;
}

// --------------------------------------------------------------------
//              Bailout Routines Macros
// --------------------------------------------------------------------

int (*floatbailout)();
int (*longbailout)();
int (*bignumbailout)();
int (*bigfltbailout)();

int  fpMODbailout()
{
    tempsqrx = sqr(g_new_z.x);
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    if (g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpREALbailout()
{
    tempsqrx = sqr(g_new_z.x);
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    if (tempsqrx >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpIMAGbailout()
{
    tempsqrx = sqr(g_new_z.x);
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    if (tempsqry >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpORbailout()
{
    tempsqrx = sqr(g_new_z.x);
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    if (tempsqrx >= g_magnitude_limit || tempsqry >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpANDbailout()
{
    tempsqrx = sqr(g_new_z.x);
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    if (tempsqrx >= g_magnitude_limit && tempsqry >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpMANHbailout()
{
    double manhmag;
    tempsqrx = sqr(g_new_z.x);
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    manhmag = fabs(g_new_z.x) + fabs(g_new_z.y);
    if ((manhmag * manhmag) >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpMANRbailout()
{
    double manrmag;
    tempsqrx = sqr(g_new_z.x);
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    manrmag = g_new_z.x + g_new_z.y; // don't need abs() since we square it next
    if ((manrmag * manrmag) >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

#define FLOATTRIGBAILOUT()  \
    if (fabs(g_old_z.y) >= rqlim2) \
        return 1;

#define LONGTRIGBAILOUT()  \
    if (labs(g_l_old_z.y) >= g_l_limit2) \
    { \
        return 1; \
    }

#define LONGXYTRIGBAILOUT()  \
    if (labs(g_l_old_z.x) >= g_l_limit2 || labs(g_l_old_z.y) >= g_l_limit2)\
        { return 1;}

#define FLOATXYTRIGBAILOUT()  \
    if (fabs(g_old_z.x) >= rqlim2 || fabs(g_old_z.y) >= rqlim2) \
        return 1;

#define FLOATHTRIGBAILOUT()  \
    if (fabs(g_old_z.x) >= rqlim2) \
        return 1;

#define LONGHTRIGBAILOUT()  \
    if (labs(g_l_old_z.x) >= g_l_limit2) \
    { \
        return 1; \
    }

#define TRIG16CHECK(X)  \
    if (labs((X)) > l16triglim) \
    { \
        return 1; \
    }

#define OLD_FLOATEXPBAILOUT()  \
    if (fabs(g_old_z.y) >= 1.0e8) \
        return 1;\
    if (fabs(g_old_z.x) >= 6.4e2) \
        return 1;

#define FLOATEXPBAILOUT()  \
    if (fabs(g_old_z.y) >= 1.0e3) \
        return 1;\
    if (fabs(g_old_z.x) >= 8) \
        return 1;

#define LONGEXPBAILOUT()  \
    if (labs(g_l_old_z.y) >= (1000L << g_bit_shift)) \
        return 1;\
    if (labs(g_l_old_z.x) >=    (8L << g_bit_shift)) \
        return 1;

#define LTRIGARG(X)    \
    if (labs((X)) > l16triglim)\
    {\
        double tmp;\
        tmp = (X);\
        tmp /= g_fudge_factor;\
        tmp = fmod(tmp, PI*2.0);\
        tmp *= g_fudge_factor;\
        (X) = (long)tmp;\
    }\
 
static int  Halleybailout()
{
    if (fabs(modulus(g_new_z)-modulus(g_old_z)) < g_param_z2.x)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

#if !defined(XFRACT)
#define MPCmod(m) (*pMPadd(*pMPmul((m).x, (m).x), *pMPmul((m).y, (m).y)))
MPC mpcold, mpcnew, mpctmp, mpctmp1;
MP g_mp_temp_param2_x;

static int  MPCHalleybailout()
{
    static MP mptmpbailout;
    mptmpbailout = *MPabs(*pMPsub(MPCmod(mpcnew), MPCmod(mpcold)));
    if (pMPcmp(mptmpbailout, g_mp_temp_param2_x) < 0)
    {
        return 1;
    }
    mpcold = mpcnew;
    return 0;
}
#endif

// --------------------------------------------------------------------
//              Fractal (once per iteration) routines
// --------------------------------------------------------------------
static double xt, yt, t2;

/* Raise complex number (base) to the (exp) power, storing the result
** in complex (result).
*/
void cpower(DComplex *base, int exp, DComplex *result)
{
    if (exp < 0)
    {
        cpower(base, -exp, result);
        CMPLXrecip(*result, *result);
        return;
    }

    xt = base->x;
    yt = base->y;

    if (exp & 1)
    {
        result->x = xt;
        result->y = yt;
    }
    else
    {
        result->x = 1.0;
        result->y = 0.0;
    }

    exp >>= 1;
    while (exp)
    {
        t2 = xt * xt - yt * yt;
        yt = 2 * xt * yt;
        xt = t2;

        if (exp & 1)
        {
            t2 = xt * result->x - yt * result->y;
            result->y = result->y * xt + yt * result->x;
            result->x = t2;
        }
        exp >>= 1;
    }
}

#if !defined(XFRACT)
// long version
static long lxt, lyt, lt2;
int
lcpower(LComplex *base, int exp, LComplex *result, int bitshift)
{
    if (exp < 0)
    {
        g_overflow = lcpower(base, -exp, result, bitshift) != 0;
        LCMPLXrecip(*result, *result);
        return g_overflow ? 1 : 0;
    }

    g_overflow = false;
    lxt = base->x;
    lyt = base->y;

    if (exp & 1)
    {
        result->x = lxt;
        result->y = lyt;
    }
    else
    {
        result->x = 1L << bitshift;
        result->y = 0L;
    }

    exp >>= 1;
    while (exp)
    {
        lt2 = multiply(lxt, lxt, bitshift) - multiply(lyt, lyt, bitshift);
        lyt = multiply(lxt, lyt, g_bit_shift_less_1);
        if (g_overflow)
        {
            return g_overflow;
        }
        lxt = lt2;

        if (exp & 1)
        {
            lt2 = multiply(lxt, result->x, bitshift) - multiply(lyt, result->y, bitshift);
            result->y = multiply(result->y, lxt, bitshift) + multiply(lyt, result->x, bitshift);
            result->x = lt2;
        }
        exp >>= 1;
    }
    if (result->x == 0 && result->y == 0)
    {
        g_overflow = true;
    }
    return g_overflow;
}
#endif

int complex_div(DComplex arg1, DComplex arg2, DComplex *pz);
int complex_mult(DComplex arg1, DComplex arg2, DComplex *pz);

// Distance of complex z from unit circle
#define DIST1(z) (((z).x-1.0)*((z).x-1.0)+((z).y)*((z).y))
#define LDIST1(z) (lsqr((((z).x)-g_fudge_factor)) + lsqr(((z).y)))

int NewtonFractal2()
{
    static char start = 1;
    if (start)
    {
        start = 0;
    }
    cpower(&g_old_z, g_degree-1, &g_tmp_z);
    complex_mult(g_tmp_z, g_old_z, &g_new_z);

    if (DIST1(g_new_z) < g_threshold)
    {
        if (fractype == fractal_type::NEWTBASIN || fractype == fractal_type::MPNEWTBASIN)
        {
            long tmpcolor;
            tmpcolor = -1;
            /* this code determines which degree-th root of root the
               Newton formula converges to. The roots of a 1 are
               distributed on a circle of radius 1 about the origin. */
            for (int i = 0; i < g_degree; i++)
            {
                /* color in alternating shades with iteration according to
                   which root of 1 it converged to */
                if (distance(g_roots[i], g_old_z) < g_threshold)
                {
                    if (g_basin == 2)
                    {
                        tmpcolor = 1+(i&7)+((g_color_iter&1) << 3);
                    }
                    else
                    {
                        tmpcolor = 1+i;
                    }
                    break;
                }
            }
            if (tmpcolor == -1)
            {
                g_color_iter = g_max_color;
            }
            else
            {
                g_color_iter = tmpcolor;
            }
        }
        return 1;
    }
    g_new_z.x = g_degree_minus_1_over_degree * g_new_z.x + g_newton_r_over_d;
    g_new_z.y *= g_degree_minus_1_over_degree;

    // Watch for divide underflow
    t2 = g_tmp_z.x*g_tmp_z.x + g_tmp_z.y*g_tmp_z.y;
    if (t2 < FLT_MIN)
    {
        return 1;
    }
    else
    {
        t2 = 1.0 / t2;
        g_old_z.x = t2 * (g_new_z.x * g_tmp_z.x + g_new_z.y * g_tmp_z.y);
        g_old_z.y = t2 * (g_new_z.y * g_tmp_z.x - g_new_z.x * g_tmp_z.y);
    }
    return 0;
}

int
complex_mult(DComplex arg1, DComplex arg2, DComplex *pz)
{
    pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
    pz->y = arg1.x*arg2.y+arg1.y*arg2.x;
    return 0;
}

int
complex_div(DComplex numerator, DComplex denominator, DComplex *pout)
{
    double mod = modulus(denominator);
    if (mod < FLT_MIN)
    {
        return 1;
    }
    conjugate(&denominator);
    complex_mult(numerator, denominator, pout);
    pout->x = pout->x/mod;
    pout->y = pout->y/mod;
    return 0;
}

#if !defined(XFRACT)
MP g_newton_mp_r_over_d, g_mp_degree_minus_1_over_degree, g_mp_threshold;
MP g_mp_temp2;
MP g_mp_one;
#endif

int MPCNewtonFractal()
{
#if !defined(XFRACT)
    g_mp_overflow = 0;
    mpctmp   = MPCpow(mpcold, g_degree-1);

    mpcnew.x = *pMPsub(*pMPmul(mpctmp.x, mpcold.x), *pMPmul(mpctmp.y, mpcold.y));
    mpcnew.y = *pMPadd(*pMPmul(mpctmp.x, mpcold.y), *pMPmul(mpctmp.y, mpcold.x));
    mpctmp1.x = *pMPsub(mpcnew.x, g_mpc_one.x);
    mpctmp1.y = *pMPsub(mpcnew.y, g_mpc_one.y);
    if (pMPcmp(MPCmod(mpctmp1), g_mp_threshold) < 0)
    {
        if (fractype == fractal_type::MPNEWTBASIN)
        {
            long tmpcolor;
            tmpcolor = -1;
            for (int i = 0; i < g_degree; i++)
                if (pMPcmp(MPdistance(g_mpc_roots[i], mpcold), g_mp_threshold) < 0)
                {
                    if (g_basin == 2)
                    {
                        tmpcolor = 1+(i&7) + ((g_color_iter&1) << 3);
                    }
                    else
                    {
                        tmpcolor = 1+i;
                    }
                    break;
                }
            if (tmpcolor == -1)
            {
                g_color_iter = g_max_color;
            }
            else
            {
                g_color_iter = tmpcolor;
            }
        }
        return 1;
    }

    mpcnew.x = *pMPadd(*pMPmul(g_mp_degree_minus_1_over_degree, mpcnew.x), g_newton_mp_r_over_d);
    mpcnew.y = *pMPmul(mpcnew.y, g_mp_degree_minus_1_over_degree);
    g_mp_temp2 = MPCmod(mpctmp);
    g_mp_temp2 = *pMPdiv(g_mp_one, g_mp_temp2);
    mpcold.x = *pMPmul(g_mp_temp2, (*pMPadd(*pMPmul(mpcnew.x, mpctmp.x), *pMPmul(mpcnew.y, mpctmp.y))));
    mpcold.y = *pMPmul(g_mp_temp2, (*pMPsub(*pMPmul(mpcnew.y, mpctmp.x), *pMPmul(mpcnew.x, mpctmp.y))));
    g_new_z.x = *pMP2d(mpcold.x);
    g_new_z.y = *pMP2d(mpcold.y);
    return g_mp_overflow;
#else
    return 0;
#endif
}

int
Barnsley1Fractal()
{
#if !defined(XFRACT)
    /* Barnsley's Mandelbrot type M1 from "Fractals
    Everywhere" by Michael Barnsley, p. 322 */

    // calculate intermediate products
    oldxinitx   = multiply(g_l_old_z.x, g_long_param->x, g_bit_shift);
    oldyinity   = multiply(g_l_old_z.y, g_long_param->y, g_bit_shift);
    oldxinity   = multiply(g_l_old_z.x, g_long_param->y, g_bit_shift);
    oldyinitx   = multiply(g_l_old_z.y, g_long_param->x, g_bit_shift);
    // orbit calculation
    if (g_l_old_z.x >= 0)
    {
        g_l_new_z.x = (oldxinitx - g_long_param->x - oldyinity);
        g_l_new_z.y = (oldyinitx - g_long_param->y + oldxinity);
    }
    else
    {
        g_l_new_z.x = (oldxinitx + g_long_param->x - oldyinity);
        g_l_new_z.y = (oldyinitx + g_long_param->y + oldxinity);
    }
    return longbailout();
#else
    return 0;
#endif
}

int
Barnsley1FPFractal()
{
    /* Barnsley's Mandelbrot type M1 from "Fractals
    Everywhere" by Michael Barnsley, p. 322 */
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step

    // calculate intermediate products
    foldxinitx = g_old_z.x * g_float_param->x;
    foldyinity = g_old_z.y * g_float_param->y;
    foldxinity = g_old_z.x * g_float_param->y;
    foldyinitx = g_old_z.y * g_float_param->x;
    // orbit calculation
    if (g_old_z.x >= 0)
    {
        g_new_z.x = (foldxinitx - g_float_param->x - foldyinity);
        g_new_z.y = (foldyinitx - g_float_param->y + foldxinity);
    }
    else
    {
        g_new_z.x = (foldxinitx + g_float_param->x - foldyinity);
        g_new_z.y = (foldyinitx + g_float_param->y + foldxinity);
    }
    return floatbailout();
}

int
Barnsley2Fractal()
{
#if !defined(XFRACT)
    /* An unnamed Mandelbrot/Julia function from "Fractals
    Everywhere" by Michael Barnsley, p. 331, example 4.2 */
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step

    // calculate intermediate products
    oldxinitx   = multiply(g_l_old_z.x, g_long_param->x, g_bit_shift);
    oldyinity   = multiply(g_l_old_z.y, g_long_param->y, g_bit_shift);
    oldxinity   = multiply(g_l_old_z.x, g_long_param->y, g_bit_shift);
    oldyinitx   = multiply(g_l_old_z.y, g_long_param->x, g_bit_shift);

    // orbit calculation
    if (oldxinity + oldyinitx >= 0)
    {
        g_l_new_z.x = oldxinitx - g_long_param->x - oldyinity;
        g_l_new_z.y = oldyinitx - g_long_param->y + oldxinity;
    }
    else
    {
        g_l_new_z.x = oldxinitx + g_long_param->x - oldyinity;
        g_l_new_z.y = oldyinitx + g_long_param->y + oldxinity;
    }
    return longbailout();
#else
    return 0;
#endif
}

int
Barnsley2FPFractal()
{
    /* An unnamed Mandelbrot/Julia function from "Fractals
    Everywhere" by Michael Barnsley, p. 331, example 4.2 */

    // calculate intermediate products
    foldxinitx = g_old_z.x * g_float_param->x;
    foldyinity = g_old_z.y * g_float_param->y;
    foldxinity = g_old_z.x * g_float_param->y;
    foldyinitx = g_old_z.y * g_float_param->x;

    // orbit calculation
    if (foldxinity + foldyinitx >= 0)
    {
        g_new_z.x = foldxinitx - g_float_param->x - foldyinity;
        g_new_z.y = foldyinitx - g_float_param->y + foldxinity;
    }
    else
    {
        g_new_z.x = foldxinitx + g_float_param->x - foldyinity;
        g_new_z.y = foldyinitx + g_float_param->y + foldxinity;
    }
    return floatbailout();
}

int
JuliaFractal()
{
#if !defined(XFRACT)
    /* used for C prototype of fast integer math routines for classic
       Mandelbrot and Julia */
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return longbailout();
#else
    {
        static bool been_here = false;
        if (!been_here)
        {
            stopmsg(STOPMSG_NONE,
                "This integer fractal type is unimplemented;\n"
                "Use float=yes to get a real image.");
            been_here = true;
        }
        return 0;
    }
#endif
}

int
JuliafpFractal()
{
    // floating point version of classical Mandelbrot/Julia
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step
    g_new_z.x = tempsqrx - tempsqry + g_float_param->x;
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y + g_float_param->y;
    return floatbailout();
}

int
LambdaFPFractal()
{
    // variation of classical Mandelbrot/Julia
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step

    tempsqrx = g_old_z.x - tempsqrx + tempsqry;
    tempsqry = -(g_old_z.y * g_old_z.x);
    tempsqry += tempsqry + g_old_z.y;

    g_new_z.x = g_float_param->x * tempsqrx - g_float_param->y * tempsqry;
    g_new_z.y = g_float_param->x * tempsqry + g_float_param->y * tempsqrx;
    return floatbailout();
}

int
LambdaFractal()
{
#if !defined(XFRACT)
    // variation of classical Mandelbrot/Julia

    // in complex math) temp = Z * (1-Z)
    g_l_temp_sqr_x = g_l_old_z.x - g_l_temp_sqr_x + g_l_temp_sqr_y;
    g_l_temp_sqr_y = g_l_old_z.y
                - multiply(g_l_old_z.y, g_l_old_z.x, g_bit_shift_less_1);
    // (in complex math) Z = Lambda * Z
    g_l_new_z.x = multiply(g_long_param->x, g_l_temp_sqr_x, g_bit_shift)
             - multiply(g_long_param->y, g_l_temp_sqr_y, g_bit_shift);
    g_l_new_z.y = multiply(g_long_param->x, g_l_temp_sqr_y, g_bit_shift)
             + multiply(g_long_param->y, g_l_temp_sqr_x, g_bit_shift);
    return longbailout();
#else
    return 0;
#endif
}

int
SierpinskiFractal()
{
#if !defined(XFRACT)
    /* following code translated from basic - see "Fractals
    Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */
    g_l_new_z.x = (g_l_old_z.x << 1);              // new.x = 2 * old.x
    g_l_new_z.y = (g_l_old_z.y << 1);              // new.y = 2 * old.y
    if (g_l_old_z.y > g_l_temp.y)  // if old.y > .5
    {
        g_l_new_z.y = g_l_new_z.y - g_l_temp.x;    // new.y = 2 * old.y - 1
    }
    else if (g_l_old_z.x > g_l_temp.y)     // if old.x > .5
    {
        g_l_new_z.x = g_l_new_z.x - g_l_temp.x;    // new.x = 2 * old.x - 1
    }
    // end barnsley code
    return longbailout();
#else
    return 0;
#endif
}

int
SierpinskiFPFractal()
{
    /* following code translated from basic - see "Fractals
    Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */

    g_new_z.x = g_old_z.x + g_old_z.x;
    g_new_z.y = g_old_z.y + g_old_z.y;
    if (g_old_z.y > .5)
    {
        g_new_z.y = g_new_z.y - 1;
    }
    else if (g_old_z.x > .5)
    {
        g_new_z.x = g_new_z.x - 1;
    }

    // end barnsley code
    return floatbailout();
}

int
LambdaexponentFractal()
{
    // found this in  "Science of Fractal Images"
    if (g_save_release > 2002)
    {
        // need braces since these are macros
        FLOATEXPBAILOUT();
    }
    else
    {
        OLD_FLOATEXPBAILOUT();
    }
    FPUsincos(&g_old_z.y, &siny, &cosy);

    if (g_old_z.x >= g_magnitude_limit && cosy >= 0.0)
    {
        return 1;
    }
    tmpexp = exp(g_old_z.x);
    g_tmp_z.x = tmpexp*cosy;
    g_tmp_z.y = tmpexp*siny;

    //multiply by lamda
    g_new_z.x = g_float_param->x*g_tmp_z.x - g_float_param->y*g_tmp_z.y;
    g_new_z.y = g_float_param->y*g_tmp_z.x + g_float_param->x*g_tmp_z.y;
    g_old_z = g_new_z;
    return 0;
}

int
LongLambdaexponentFractal()
{
#if !defined(XFRACT)
    // found this in  "Science of Fractal Images"
    LONGEXPBAILOUT();

    SinCos086(g_l_old_z.y, &lsiny,  &lcosy);

    if (g_l_old_z.x >= g_l_limit && lcosy >= 0L)
    {
        return 1;
    }
    longtmp = Exp086(g_l_old_z.x);

    g_l_temp.x = multiply(longtmp,      lcosy,   g_bit_shift);
    g_l_temp.y = multiply(longtmp,      lsiny,   g_bit_shift);

    g_l_new_z.x  = multiply(g_long_param->x, g_l_temp.x, g_bit_shift)
              - multiply(g_long_param->y, g_l_temp.y, g_bit_shift);
    g_l_new_z.y  = multiply(g_long_param->x, g_l_temp.y, g_bit_shift)
              + multiply(g_long_param->y, g_l_temp.x, g_bit_shift);
    g_l_old_z = g_l_new_z;
    return 0;
#else
    return 0;
#endif
}

int
FloatTrigPlusExponentFractal()
{
    // another Scientific American biomorph type
    // z(n+1) = e**z(n) + trig(z(n)) + C

    if (fabs(g_old_z.x) >= 6.4e2)
    {
        return 1; // DOMAIN errors
    }
    tmpexp = exp(g_old_z.x);
    FPUsincos(&g_old_z.y, &siny, &cosy);
    CMPLXtrig0(g_old_z, g_new_z);

    //new =   trig(old) + e**old + C
    g_new_z.x += tmpexp*cosy + g_float_param->x;
    g_new_z.y += tmpexp*siny + g_float_param->y;
    return floatbailout();
}

int
LongTrigPlusExponentFractal()
{
#if !defined(XFRACT)
    // calculate exp(z)

    // domain check for fast transcendental functions
    TRIG16CHECK(g_l_old_z.x);
    TRIG16CHECK(g_l_old_z.y);

    longtmp = Exp086(g_l_old_z.x);
    SinCos086(g_l_old_z.y, &lsiny,  &lcosy);
    LCMPLXtrig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += multiply(longtmp,    lcosy,   g_bit_shift) + g_long_param->x;
    g_l_new_z.y += multiply(longtmp,    lsiny,   g_bit_shift) + g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

int
MarksLambdaFractal()
{
    // Mark Peterson's variation of "lambda" function

    // Z1 = (C^(exp-1) * Z**2) + C
#if !defined(XFRACT)
    g_l_temp.x = g_l_temp_sqr_x - g_l_temp_sqr_y;
    g_l_temp.y = multiply(g_l_old_z.x , g_l_old_z.y , g_bit_shift_less_1);

    g_l_new_z.x = multiply(g_l_coefficient.x, g_l_temp.x, g_bit_shift)
             - multiply(g_l_coefficient.y, g_l_temp.y, g_bit_shift) + g_long_param->x;
    g_l_new_z.y = multiply(g_l_coefficient.x, g_l_temp.y, g_bit_shift)
             + multiply(g_l_coefficient.y, g_l_temp.x, g_bit_shift) + g_long_param->y;

    return longbailout();
#else
    return 0;
#endif
}

int
MarksLambdafpFractal()
{
    // Mark Peterson's variation of "lambda" function

    // Z1 = (C^(exp-1) * Z**2) + C
    g_tmp_z.x = tempsqrx - tempsqry;
    g_tmp_z.y = g_old_z.x * g_old_z.y *2;

    g_new_z.x = g_marks_coefficient.x * g_tmp_z.x - g_marks_coefficient.y * g_tmp_z.y + g_float_param->x;
    g_new_z.y = g_marks_coefficient.x * g_tmp_z.y + g_marks_coefficient.y * g_tmp_z.x + g_float_param->y;

    return floatbailout();
}


long XXOne, g_fudge_one, g_fudge_two;

int
UnityFractal()
{
#if !defined(XFRACT)
    XXOne = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift) + multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    if ((XXOne > g_fudge_two) || (labs(XXOne - g_fudge_one) < g_l_delta_min))
    {
        return 1;
    }
    g_l_old_z.y = multiply(g_fudge_two - XXOne, g_l_old_z.x, g_bit_shift);
    g_l_old_z.x = multiply(g_fudge_two - XXOne, g_l_old_z.y, g_bit_shift);
    g_l_new_z = g_l_old_z;
    return 0;
#else
    return 0;
#endif
}

int
UnityfpFractal()
{
    double XXOne;
    XXOne = sqr(g_old_z.x) + sqr(g_old_z.y);
    if ((XXOne > 2.0) || (fabs(XXOne - 1.0) < g_delta_min))
    {
        return 1;
    }
    g_old_z.y = (2.0 - XXOne)* g_old_z.x;
    g_old_z.x = (2.0 - XXOne)* g_old_z.y;
    g_new_z = g_old_z;
    return 0;
}

int
Mandel4Fractal()
{
    /*
       this routine calculates the Mandelbrot/Julia set based on the
       polynomial z**4 + lambda
     */

    // first, compute (x + iy)**2
#if !defined(XFRACT)
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1);
    if (longbailout())
    {
        return 1;
    }

    // then, compute ((x + iy)**2)**2 + lambda
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

int
Mandel4fpFractal()
{
    // first, compute (x + iy)**2
    g_new_z.x  = tempsqrx - tempsqry;
    g_new_z.y = g_old_z.x*g_old_z.y*2;
    if (floatbailout())
    {
        return 1;
    }

    // then, compute ((x + iy)**2)**2 + lambda
    g_new_z.x  = tempsqrx - tempsqry + g_float_param->x;
    g_new_z.y =  g_old_z.x*g_old_z.y*2 + g_float_param->y;
    return floatbailout();
}

int
floatZtozPluszpwrFractal()
{
    cpower(&g_old_z, (int)g_params[2], &g_new_z);
    g_old_z = ComplexPower(g_old_z, g_old_z);
    g_new_z.x = g_new_z.x + g_old_z.x +g_float_param->x;
    g_new_z.y = g_new_z.y + g_old_z.y +g_float_param->y;
    return floatbailout();
}

int
longZpowerFractal()
{
#if !defined(XFRACT)
    if (lcpower(&g_l_old_z, g_c_exponent, &g_l_new_z, g_bit_shift))
    {
        g_l_new_z.y = 8L << g_bit_shift;
        g_l_new_z.x = g_l_new_z.y;
    }
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

int
longCmplxZpowerFractal()
{
#if !defined(XFRACT)
    DComplex x, y;

    x.x = (double)g_l_old_z.x / g_fudge_factor;
    x.y = (double)g_l_old_z.y / g_fudge_factor;
    y.x = (double)g_l_param2.x / g_fudge_factor;
    y.y = (double)g_l_param2.y / g_fudge_factor;
    x = ComplexPower(x, y);
    if (fabs(x.x) < g_fudge_limit && fabs(x.y) < g_fudge_limit)
    {
        g_l_new_z.x = (long)(x.x * g_fudge_factor);
        g_l_new_z.y = (long)(x.y * g_fudge_factor);
    }
    else
    {
        g_overflow = true;
    }
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

int
floatZpowerFractal()
{
    cpower(&g_old_z, g_c_exponent, &g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return floatbailout();
}

int
floatCmplxZpowerFractal()
{
    g_new_z = ComplexPower(g_old_z, g_param_z2);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return floatbailout();
}

int
Barnsley3Fractal()
{
    /* An unnamed Mandelbrot/Julia function from "Fractals
    Everywhere" by Michael Barnsley, p. 292, example 4.1 */

    // calculate intermediate products
#if !defined(XFRACT)
    oldxinitx   = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    oldyinity   = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    oldxinity   = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift);

    // orbit calculation
    if (g_l_old_z.x > 0)
    {
        g_l_new_z.x = oldxinitx   - oldyinity - g_fudge_factor;
        g_l_new_z.y = oldxinity << 1;
    }
    else
    {
        g_l_new_z.x = oldxinitx - oldyinity - g_fudge_factor
                 + multiply(g_long_param->x, g_l_old_z.x, g_bit_shift);
        g_l_new_z.y = oldxinity <<1;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_l_new_z.y += multiply(g_long_param->y, g_l_old_z.x, g_bit_shift);
    }
    return longbailout();
#else
    return 0;
#endif
}

int
Barnsley3FPFractal()
{
    /* An unnamed Mandelbrot/Julia function from "Fractals
    Everywhere" by Michael Barnsley, p. 292, example 4.1 */


    // calculate intermediate products
    foldxinitx  = g_old_z.x * g_old_z.x;
    foldyinity  = g_old_z.y * g_old_z.y;
    foldxinity  = g_old_z.x * g_old_z.y;

    // orbit calculation
    if (g_old_z.x > 0)
    {
        g_new_z.x = foldxinitx - foldyinity - 1.0;
        g_new_z.y = foldxinity * 2;
    }
    else
    {
        g_new_z.x = foldxinitx - foldyinity -1.0 + g_float_param->x * g_old_z.x;
        g_new_z.y = foldxinity * 2;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_new_z.y += g_float_param->y * g_old_z.x;
    }
    return floatbailout();
}

int
TrigPlusZsquaredFractal()
{
#if !defined(XFRACT)
    // From Scientific American, July 1989
    // A Biomorph
    // z(n+1) = trig(z(n))+z(n)**2+C
    LCMPLXtrig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y += multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

int
TrigPlusZsquaredfpFractal()
{
    // From Scientific American, July 1989
    // A Biomorph
    // z(n+1) = trig(z(n))+z(n)**2+C

    CMPLXtrig0(g_old_z, g_new_z);
    g_new_z.x += tempsqrx - tempsqry + g_float_param->x;
    g_new_z.y += 2.0 * g_old_z.x * g_old_z.y + g_float_param->y;
    return floatbailout();
}

int
Richard8fpFractal()
{
    //  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50}
    CMPLXtrig0(g_old_z, g_new_z);
    g_new_z.x += g_tmp_z.x;
    g_new_z.y += g_tmp_z.y;
    return floatbailout();
}

int
Richard8Fractal()
{
#if !defined(XFRACT)
    //  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50}
    LCMPLXtrig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += g_l_temp.x;
    g_l_new_z.y += g_l_temp.y;
    return longbailout();
#else
    return 0;
#endif
}

int
PopcornFractal_Old()
{
    g_tmp_z = g_old_z;
    g_tmp_z.x *= 3.0;
    g_tmp_z.y *= 3.0;
    FPUsincos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    FPUsincos(&g_tmp_z.y, &siny, &cosy);
    g_tmp_z.x = g_sin_x/g_cos_x + g_old_z.x;
    g_tmp_z.y = siny/cosy + g_old_z.y;
    FPUsincos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    FPUsincos(&g_tmp_z.y, &siny, &cosy);
    g_new_z.x = g_old_z.x - g_param_z1.x*siny;
    g_new_z.y = g_old_z.y - g_param_z1.x*g_sin_x;
    if (g_plot == noplot)
    {
        plot_orbit(g_new_z.x, g_new_z.y, 1+g_row%g_colors);
        g_old_z = g_new_z;
    }
    else
    {
        tempsqrx = sqr(g_new_z.x);
    }
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    if (g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int
PopcornFractal()
{
    g_tmp_z = g_old_z;
    g_tmp_z.x *= 3.0;
    g_tmp_z.y *= 3.0;
    FPUsincos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    FPUsincos(&g_tmp_z.y, &siny, &cosy);
    g_tmp_z.x = g_sin_x/g_cos_x + g_old_z.x;
    g_tmp_z.y = siny/cosy + g_old_z.y;
    FPUsincos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    FPUsincos(&g_tmp_z.y, &siny, &cosy);
    g_new_z.x = g_old_z.x - g_param_z1.x*siny;
    g_new_z.y = g_old_z.y - g_param_z1.x*g_sin_x;
    if (g_plot == noplot)
    {
        plot_orbit(g_new_z.x, g_new_z.y, 1+g_row%g_colors);
        g_old_z = g_new_z;
    }
    tempsqrx = sqr(g_new_z.x);
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    if (g_magnitude >= g_magnitude_limit
            || fabs(g_new_z.x) > rqlim2 || fabs(g_new_z.y) > rqlim2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int
LPopcornFractal_Old()
{
#if !defined(XFRACT)
    g_l_temp = g_l_old_z;
    g_l_temp.x *= 3L;
    g_l_temp.y *= 3L;
    LTRIGARG(g_l_temp.x);
    LTRIGARG(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_temp.x = divide(lsinx, lcosx, g_bit_shift) + g_l_old_z.x;
    g_l_temp.y = divide(lsiny, lcosy, g_bit_shift) + g_l_old_z.y;
    LTRIGARG(g_l_temp.x);
    LTRIGARG(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_new_z.x = g_l_old_z.x - multiply(g_l_param.x, lsiny, g_bit_shift);
    g_l_new_z.y = g_l_old_z.y - multiply(g_l_param.x, lsinx, g_bit_shift);
    if (g_plot == noplot)
    {
        iplot_orbit(g_l_new_z.x, g_l_new_z.y, 1+g_row%g_colors);
        g_l_old_z = g_l_new_z;
    }
    else
    {
        g_l_temp_sqr_x = lsqr(g_l_new_z.x);
        g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    }
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_limit || g_l_magnitude < 0 || labs(g_l_new_z.x) > g_l_limit2
            || labs(g_l_new_z.y) > g_l_limit2)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
#else
    return 0;
#endif
}

int
LPopcornFractal()
{
#if !defined(XFRACT)
    g_l_temp = g_l_old_z;
    g_l_temp.x *= 3L;
    g_l_temp.y *= 3L;
    LTRIGARG(g_l_temp.x);
    LTRIGARG(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_temp.x = divide(lsinx, lcosx, g_bit_shift) + g_l_old_z.x;
    g_l_temp.y = divide(lsiny, lcosy, g_bit_shift) + g_l_old_z.y;
    LTRIGARG(g_l_temp.x);
    LTRIGARG(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_new_z.x = g_l_old_z.x - multiply(g_l_param.x, lsiny, g_bit_shift);
    g_l_new_z.y = g_l_old_z.y - multiply(g_l_param.x, lsinx, g_bit_shift);
    if (g_plot == noplot)
    {
        iplot_orbit(g_l_new_z.x, g_l_new_z.y, 1+g_row%g_colors);
        g_l_old_z = g_l_new_z;
    }
    // else
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_limit || g_l_magnitude < 0
            || labs(g_l_new_z.x) > g_l_limit2
            || labs(g_l_new_z.y) > g_l_limit2)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
#else
    return 0;
#endif
}

// Popcorn generalization

int
PopcornFractalFn()
{
    DComplex tmpx;
    DComplex tmpy;

    // tmpx contains the generalized value of the old real "x" equation
    CMPLXtimesreal(g_param_z2, g_old_z.y, g_tmp_z);  // tmp = (C * old.y)
    CMPLXtrig1(g_tmp_z, tmpx);             // tmpx = trig1(tmp)
    tmpx.x += g_old_z.y;                  // tmpx = old.y + trig1(tmp)
    CMPLXtrig0(tmpx, g_tmp_z);             // tmp = trig0(tmpx)
    CMPLXmult(g_tmp_z, g_param_z1, tmpx);         // tmpx = tmp * h

    // tmpy contains the generalized value of the old real "y" equation
    CMPLXtimesreal(g_param_z2, g_old_z.x, g_tmp_z);  // tmp = (C * old.x)
    CMPLXtrig3(g_tmp_z, tmpy);             // tmpy = trig3(tmp)
    tmpy.x += g_old_z.x;                  // tmpy = old.x + trig1(tmp)
    CMPLXtrig2(tmpy, g_tmp_z);             // tmp = trig2(tmpy)

    CMPLXmult(g_tmp_z, g_param_z1, tmpy);         // tmpy = tmp * h

    g_new_z.x = g_old_z.x - tmpx.x - tmpy.y;
    g_new_z.y = g_old_z.y - tmpy.x - tmpx.y;

    if (g_plot == noplot)
    {
        plot_orbit(g_new_z.x, g_new_z.y, 1+g_row%g_colors);
        g_old_z = g_new_z;
    }

    tempsqrx = sqr(g_new_z.x);
    tempsqry = sqr(g_new_z.y);
    g_magnitude = tempsqrx + tempsqry;
    if (g_magnitude >= g_magnitude_limit
            || fabs(g_new_z.x) > rqlim2 || fabs(g_new_z.y) > rqlim2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

#define FIX_OVERFLOW(arg)           \
    if (g_overflow)                   \
    {                               \
        (arg).x = g_fudge_factor;   \
        (arg).y = 0;                \
        g_overflow = false;           \
   }

int
LPopcornFractalFn()
{
#if !defined(XFRACT)
    LComplex ltmpx, ltmpy;

    g_overflow = false;

    // ltmpx contains the generalized value of the old real "x" equation
    LCMPLXtimesreal(g_l_param2, g_l_old_z.y, g_l_temp); // tmp = (C * old.y)
    LCMPLXtrig1(g_l_temp, ltmpx);             // tmpx = trig1(tmp)
    FIX_OVERFLOW(ltmpx);
    ltmpx.x += g_l_old_z.y;                   // tmpx = old.y + trig1(tmp)
    LCMPLXtrig0(ltmpx, g_l_temp);             // tmp = trig0(tmpx)
    FIX_OVERFLOW(g_l_temp);
    LCMPLXmult(g_l_temp, g_l_param, ltmpx);        // tmpx = tmp * h

    // ltmpy contains the generalized value of the old real "y" equation
    LCMPLXtimesreal(g_l_param2, g_l_old_z.x, g_l_temp); // tmp = (C * old.x)
    LCMPLXtrig3(g_l_temp, ltmpy);             // tmpy = trig3(tmp)
    FIX_OVERFLOW(ltmpy);
    ltmpy.x += g_l_old_z.x;                   // tmpy = old.x + trig1(tmp)
    LCMPLXtrig2(ltmpy, g_l_temp);             // tmp = trig2(tmpy)
    FIX_OVERFLOW(g_l_temp);
    LCMPLXmult(g_l_temp, g_l_param, ltmpy);        // tmpy = tmp * h

    g_l_new_z.x = g_l_old_z.x - ltmpx.x - ltmpy.y;
    g_l_new_z.y = g_l_old_z.y - ltmpy.x - ltmpx.y;

    if (g_plot == noplot)
    {
        iplot_orbit(g_l_new_z.x, g_l_new_z.y, 1+g_row%g_colors);
        g_l_old_z = g_l_new_z;
    }
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_limit || g_l_magnitude < 0
            || labs(g_l_new_z.x) > g_l_limit2
            || labs(g_l_new_z.y) > g_l_limit2)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
#else
    return 0;
#endif
}

int MarksCplxMand()
{
    g_tmp_z.x = tempsqrx - tempsqry;
    g_tmp_z.y = 2*g_old_z.x*g_old_z.y;
    FPUcplxmul(&g_tmp_z, &g_marks_coefficient, &g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return floatbailout();
}

int SpiderfpFractal()
{
    // Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 }
    g_new_z.x = tempsqrx - tempsqry + g_tmp_z.x;
    g_new_z.y = 2 * g_old_z.x * g_old_z.y + g_tmp_z.y;
    g_tmp_z.x = g_tmp_z.x/2 + g_new_z.x;
    g_tmp_z.y = g_tmp_z.y/2 + g_new_z.y;
    return floatbailout();
}

int
SpiderFractal()
{
#if !defined(XFRACT)
    // Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 }
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_l_temp.x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_l_temp.y;
    g_l_temp.x = (g_l_temp.x >> 1) + g_l_new_z.x;
    g_l_temp.y = (g_l_temp.y >> 1) + g_l_new_z.y;
    return longbailout();
#else
    return 0;
#endif
}

int
TetratefpFractal()
{
    // Tetrate(XAXIS) { c=z=pixel: z=c^z, |z|<=(P1+3) }
    g_new_z = ComplexPower(*g_float_param, g_old_z);
    return floatbailout();
}

int
ZXTrigPlusZFractal()
{
#if !defined(XFRACT)
    // z = (p1*z*trig(z))+p2*z
    LCMPLXtrig0(g_l_old_z, g_l_temp);          // ltmp  = trig(old)
    LCMPLXmult(g_l_param, g_l_temp, g_l_temp);      // ltmp  = p1*trig(old)
    LCMPLXmult(g_l_old_z, g_l_temp, ltmp2);      // ltmp2 = p1*old*trig(old)
    LCMPLXmult(g_l_param2, g_l_old_z, g_l_temp);     // ltmp  = p2*old
    LCMPLXadd(ltmp2, g_l_temp, g_l_new_z);       // lnew  = p1*trig(old) + p2*old
    return longbailout();
#else
    return 0;
#endif
}

int
ScottZXTrigPlusZFractal()
{
#if !defined(XFRACT)
    // z = (z*trig(z))+z
    LCMPLXtrig0(g_l_old_z, g_l_temp);          // ltmp  = trig(old)
    LCMPLXmult(g_l_old_z, g_l_temp, g_l_new_z);       // lnew  = old*trig(old)
    LCMPLXadd(g_l_new_z, g_l_old_z, g_l_new_z);        // lnew  = trig(old) + old
    return longbailout();
#else
    return 0;
#endif
}

int
SkinnerZXTrigSubZFractal()
{
#if !defined(XFRACT)
    // z = (z*trig(z))-z
    LCMPLXtrig0(g_l_old_z, g_l_temp);          // ltmp  = trig(old)
    LCMPLXmult(g_l_old_z, g_l_temp, g_l_new_z);       // lnew  = old*trig(old)
    LCMPLXsub(g_l_new_z, g_l_old_z, g_l_new_z);        // lnew  = trig(old) - old
    return longbailout();
#else
    return 0;
#endif
}

int
ZXTrigPlusZfpFractal()
{
    // z = (p1*z*trig(z))+p2*z
    CMPLXtrig0(g_old_z, g_tmp_z);          // tmp  = trig(old)
    CMPLXmult(g_param_z1, g_tmp_z, g_tmp_z);      // tmp  = p1*trig(old)
    CMPLXmult(g_old_z, g_tmp_z, tmp2);      // tmp2 = p1*old*trig(old)
    CMPLXmult(g_param_z2, g_old_z, g_tmp_z);     // tmp  = p2*old
    CMPLXadd(tmp2, g_tmp_z, g_new_z);       // new  = p1*trig(old) + p2*old
    return floatbailout();
}

int
ScottZXTrigPlusZfpFractal()
{
    // z = (z*trig(z))+z
    CMPLXtrig0(g_old_z, g_tmp_z);         // tmp  = trig(old)
    CMPLXmult(g_old_z, g_tmp_z, g_new_z);       // new  = old*trig(old)
    CMPLXadd(g_new_z, g_old_z, g_new_z);        // new  = trig(old) + old
    return floatbailout();
}

int
SkinnerZXTrigSubZfpFractal()
{
    // z = (z*trig(z))-z
    CMPLXtrig0(g_old_z, g_tmp_z);         // tmp  = trig(old)
    CMPLXmult(g_old_z, g_tmp_z, g_new_z);       // new  = old*trig(old)
    CMPLXsub(g_new_z, g_old_z, g_new_z);        // new  = trig(old) - old
    return floatbailout();
}

int
Sqr1overTrigFractal()
{
#if !defined(XFRACT)
    // z = sqr(1/trig(z))
    LCMPLXtrig0(g_l_old_z, g_l_old_z);
    LCMPLXrecip(g_l_old_z, g_l_old_z);
    LCMPLXsqr(g_l_old_z, g_l_new_z);
    return longbailout();
#else
    return 0;
#endif
}

int
Sqr1overTrigfpFractal()
{
    // z = sqr(1/trig(z))
    CMPLXtrig0(g_old_z, g_old_z);
    CMPLXrecip(g_old_z, g_old_z);
    CMPLXsqr(g_old_z, g_new_z);
    return floatbailout();
}

int
TrigPlusTrigFractal()
{
#if !defined(XFRACT)
    // z = trig(0,z)*p1+trig1(z)*p2
    LCMPLXtrig0(g_l_old_z, g_l_temp);
    LCMPLXmult(g_l_param, g_l_temp, g_l_temp);
    LCMPLXtrig1(g_l_old_z, ltmp2);
    LCMPLXmult(g_l_param2, ltmp2, g_l_old_z);
    LCMPLXadd(g_l_temp, g_l_old_z, g_l_new_z);
    return longbailout();
#else
    return 0;
#endif
}

int
TrigPlusTrigfpFractal()
{
    // z = trig0(z)*p1+trig1(z)*p2
    CMPLXtrig0(g_old_z, g_tmp_z);
    CMPLXmult(g_param_z1, g_tmp_z, g_tmp_z);
    CMPLXtrig1(g_old_z, g_old_z);
    CMPLXmult(g_param_z2, g_old_z, g_old_z);
    CMPLXadd(g_tmp_z, g_old_z, g_new_z);
    return floatbailout();
}

/* The following four fractals are based on the idea of parallel
   or alternate calculations.  The shift is made when the mod
   reaches a given value.  */

int
LambdaTrigOrTrigFractal()
{
#if !defined(XFRACT)
    /* z = trig0(z)*p1 if mod(old) < p2.x and
           trig1(z)*p1 if mod(old) >= p2.x */
    if ((LCMPLXmod(g_l_old_z)) < g_l_param2.x)
    {
        LCMPLXtrig0(g_l_old_z, g_l_temp);
        LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);
    }
    else
    {
        LCMPLXtrig1(g_l_old_z, g_l_temp);
        LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);
    }
    return longbailout();
#else
    return 0;
#endif
}

int
LambdaTrigOrTrigfpFractal()
{
    /* z = trig0(z)*p1 if mod(old) < p2.x and
           trig1(z)*p1 if mod(old) >= p2.x */
    if (CMPLXmod(g_old_z) < g_param_z2.x)
    {
        CMPLXtrig0(g_old_z, g_old_z);
        FPUcplxmul(g_float_param, &g_old_z, &g_new_z);
    }
    else
    {
        CMPLXtrig1(g_old_z, g_old_z);
        FPUcplxmul(g_float_param, &g_old_z, &g_new_z);
    }
    return floatbailout();
}

int
JuliaTrigOrTrigFractal()
{
#if !defined(XFRACT)
    /* z = trig0(z)+p1 if mod(old) < p2.x and
           trig1(z)+p1 if mod(old) >= p2.x */
    if (LCMPLXmod(g_l_old_z) < g_l_param2.x)
    {
        LCMPLXtrig0(g_l_old_z, g_l_temp);
        LCMPLXadd(*g_long_param, g_l_temp, g_l_new_z);
    }
    else
    {
        LCMPLXtrig1(g_l_old_z, g_l_temp);
        LCMPLXadd(*g_long_param, g_l_temp, g_l_new_z);
    }
    return longbailout();
#else
    return 0;
#endif
}

int
JuliaTrigOrTrigfpFractal()
{
    /* z = trig0(z)+p1 if mod(old) < p2.x and
           trig1(z)+p1 if mod(old) >= p2.x */
    if (CMPLXmod(g_old_z) < g_param_z2.x)
    {
        CMPLXtrig0(g_old_z, g_old_z);
        CMPLXadd(*g_float_param, g_old_z, g_new_z);
    }
    else
    {
        CMPLXtrig1(g_old_z, g_old_z);
        CMPLXadd(*g_float_param, g_old_z, g_new_z);
    }
    return floatbailout();
}

int g_halley_a_plus_one, g_halley_a_plus_one_times_degree;
MP g_halley_mp_a_plus_one, g_halley_mp_a_plus_one_times_degree;
MPC g_mpc_temp_param;

int MPCHalleyFractal()
{
#if !defined(XFRACT)
    //  X(X^a - 1) = 0, Halley Map
    //  a = parm.x,  relaxation coeff. = parm.y,  epsilon = parm2.x

    MPC mpcXtoAlessOne, mpcXtoA;
    MPC mpcXtoAplusOne; // a-1, a, a+1
    MPC mpcFX, mpcF1prime, mpcF2prime, mpcHalnumer1;
    MPC mpcHalnumer2, mpcHaldenom, mpctmp;

    g_mp_overflow = 0;
    mpcXtoAlessOne.x = mpcold.x;
    mpcXtoAlessOne.y = mpcold.y;
    for (int ihal = 2; ihal < g_degree; ihal++)
    {
        mpctmp.x = *pMPsub(*pMPmul(mpcXtoAlessOne.x, mpcold.x), *pMPmul(mpcXtoAlessOne.y, mpcold.y));
        mpctmp.y = *pMPadd(*pMPmul(mpcXtoAlessOne.x, mpcold.y), *pMPmul(mpcXtoAlessOne.y, mpcold.x));
        mpcXtoAlessOne.x = mpctmp.x;
        mpcXtoAlessOne.y = mpctmp.y;
    }
    mpcXtoA.x = *pMPsub(*pMPmul(mpcXtoAlessOne.x, mpcold.x), *pMPmul(mpcXtoAlessOne.y, mpcold.y));
    mpcXtoA.y = *pMPadd(*pMPmul(mpcXtoAlessOne.x, mpcold.y), *pMPmul(mpcXtoAlessOne.y, mpcold.x));
    mpcXtoAplusOne.x = *pMPsub(*pMPmul(mpcXtoA.x, mpcold.x), *pMPmul(mpcXtoA.y, mpcold.y));
    mpcXtoAplusOne.y = *pMPadd(*pMPmul(mpcXtoA.x, mpcold.y), *pMPmul(mpcXtoA.y, mpcold.x));

    mpcFX.x = *pMPsub(mpcXtoAplusOne.x, mpcold.x);
    mpcFX.y = *pMPsub(mpcXtoAplusOne.y, mpcold.y); // FX = X^(a+1) - X  = F

    mpcF2prime.x = *pMPmul(g_halley_mp_a_plus_one_times_degree, mpcXtoAlessOne.x); // mpAp1deg in setup
    mpcF2prime.y = *pMPmul(g_halley_mp_a_plus_one_times_degree, mpcXtoAlessOne.y);        // F"

    mpcF1prime.x = *pMPsub(*pMPmul(g_halley_mp_a_plus_one, mpcXtoA.x), g_mp_one);
    mpcF1prime.y = *pMPmul(g_halley_mp_a_plus_one, mpcXtoA.y);                   //  F'

    mpctmp.x = *pMPsub(*pMPmul(mpcF2prime.x, mpcFX.x), *pMPmul(mpcF2prime.y, mpcFX.y));
    mpctmp.y = *pMPadd(*pMPmul(mpcF2prime.x, mpcFX.y), *pMPmul(mpcF2prime.y, mpcFX.x));
    //  F * F"

    mpcHaldenom.x = *pMPadd(mpcF1prime.x, mpcF1prime.x);
    mpcHaldenom.y = *pMPadd(mpcF1prime.y, mpcF1prime.y);      //  2 * F'

    mpcHalnumer1 = MPCdiv(mpctmp, mpcHaldenom);        //  F"F/2F'
    mpctmp.x = *pMPsub(mpcF1prime.x, mpcHalnumer1.x);
    mpctmp.y = *pMPsub(mpcF1prime.y, mpcHalnumer1.y); //  F' - F"F/2F'
    mpcHalnumer2 = MPCdiv(mpcFX, mpctmp);

    mpctmp   =  MPCmul(g_mpc_temp_param, mpcHalnumer2);  // mpctmpparm is
    // relaxation coef.
    mpcnew = MPCsub(mpcold, mpctmp);
    g_new_z    = MPC2cmplx(mpcnew);
    return MPCHalleybailout()||g_mp_overflow;
#else
    return 0;
#endif
}

int
HalleyFractal()
{
    //  X(X^a - 1) = 0, Halley Map
    //  a = parm.x = degree, relaxation coeff. = parm.y, epsilon = parm2.x
    DComplex XtoAlessOne, XtoA, XtoAplusOne; // a-1, a, a+1
    DComplex FX, F1prime, F2prime, Halnumer1, Halnumer2, Haldenom;
    DComplex relax;

    XtoAlessOne = g_old_z;
    for (int ihal = 2; ihal < g_degree; ihal++)
    {
        FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoAlessOne);
    }
    FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoA);
    FPUcplxmul(&g_old_z, &XtoA, &XtoAplusOne);

    CMPLXsub(XtoAplusOne, g_old_z, FX);        // FX = X^(a+1) - X  = F
    F2prime.x = g_halley_a_plus_one_times_degree * XtoAlessOne.x; // g_halley_a_plus_one_times_degree in setup
    F2prime.y = g_halley_a_plus_one_times_degree * XtoAlessOne.y;        // F"

    F1prime.x = g_halley_a_plus_one * XtoA.x - 1.0;
    F1prime.y = g_halley_a_plus_one * XtoA.y;                             //  F'

    FPUcplxmul(&F2prime, &FX, &Halnumer1);                  //  F * F"
    Haldenom.x = F1prime.x + F1prime.x;
    Haldenom.y = F1prime.y + F1prime.y;                     //  2 * F'

    FPUcplxdiv(&Halnumer1, &Haldenom, &Halnumer1);         //  F"F/2F'
    CMPLXsub(F1prime, Halnumer1, Halnumer2);          //  F' - F"F/2F'
    FPUcplxdiv(&FX, &Halnumer2, &Halnumer2);
    // parm.y is relaxation coef.
    relax.x = g_param_z1.y;
    relax.y = g_params[3];
    FPUcplxmul(&relax, &Halnumer2, &Halnumer2);
    g_new_z.x = g_old_z.x - Halnumer2.x;
    g_new_z.y = g_old_z.y - Halnumer2.y;
    return Halleybailout();
}

int
LongPhoenixFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    g_l_temp.x = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift);
    g_l_new_z.x = g_l_temp_sqr_x-g_l_temp_sqr_y+g_long_param->x+multiply(g_long_param->y, ltmp2.x, g_bit_shift);
    g_l_new_z.y = (g_l_temp.x + g_l_temp.x) + multiply(g_long_param->y, ltmp2.y, g_bit_shift);
    ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return longbailout();
#else
    return 0;
#endif
}

int
PhoenixFractal()
{
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    g_tmp_z.x = g_old_z.x * g_old_z.y;
    g_new_z.x = tempsqrx - tempsqry + g_float_param->x + (g_float_param->y * tmp2.x);
    g_new_z.y = (g_tmp_z.x + g_tmp_z.x) + (g_float_param->y * tmp2.y);
    tmp2 = g_old_z; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixFractalcplx()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    g_l_temp.x = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift);
    g_l_new_z.x = g_l_temp_sqr_x-g_l_temp_sqr_y+g_long_param->x+multiply(g_l_param2.x, ltmp2.x, g_bit_shift)-multiply(g_l_param2.y, ltmp2.y, g_bit_shift);
    g_l_new_z.y = (g_l_temp.x + g_l_temp.x)+g_long_param->y+multiply(g_l_param2.x, ltmp2.y, g_bit_shift)+multiply(g_l_param2.y, ltmp2.x, g_bit_shift);
    ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return longbailout();
#else
    return 0;
#endif
}

int
PhoenixFractalcplx()
{
    // z(n+1) = z(n)^2 + p1 + p2*y(n),  y(n+1) = z(n)
    g_tmp_z.x = g_old_z.x * g_old_z.y;
    g_new_z.x = tempsqrx - tempsqry + g_float_param->x + (g_param_z2.x * tmp2.x) - (g_param_z2.y * tmp2.y);
    g_new_z.y = (g_tmp_z.x + g_tmp_z.x) + g_float_param->y + (g_param_z2.x * tmp2.y) + (g_param_z2.y * tmp2.x);
    tmp2 = g_old_z; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixPlusFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    LComplex loldplus, lnewminus;
    loldplus = g_l_old_z;
    g_l_temp = g_l_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        LCMPLXmult(g_l_old_z, g_l_temp, g_l_temp); // = old^(degree-1)
    }
    loldplus.x += g_long_param->x;
    LCMPLXmult(g_l_temp, loldplus, lnewminus);
    g_l_new_z.x = lnewminus.x + multiply(g_long_param->y, ltmp2.x, g_bit_shift);
    g_l_new_z.y = lnewminus.y + multiply(g_long_param->y, ltmp2.y, g_bit_shift);
    ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return longbailout();
#else
    return 0;
#endif
}

int
PhoenixPlusFractal()
{
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    DComplex oldplus, newminus;
    oldplus = g_old_z;
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        FPUcplxmul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-1)
    }
    oldplus.x += g_float_param->x;
    FPUcplxmul(&g_tmp_z, &oldplus, &newminus);
    g_new_z.x = newminus.x + (g_float_param->y * tmp2.x);
    g_new_z.y = newminus.y + (g_float_param->y * tmp2.y);
    tmp2 = g_old_z; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixMinusFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    LComplex loldsqr, lnewminus;
    LCMPLXmult(g_l_old_z, g_l_old_z, loldsqr);
    g_l_temp = g_l_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        LCMPLXmult(g_l_old_z, g_l_temp, g_l_temp); // = old^(degree-2)
    }
    loldsqr.x += g_long_param->x;
    LCMPLXmult(g_l_temp, loldsqr, lnewminus);
    g_l_new_z.x = lnewminus.x + multiply(g_long_param->y, ltmp2.x, g_bit_shift);
    g_l_new_z.y = lnewminus.y + multiply(g_long_param->y, ltmp2.y, g_bit_shift);
    ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return longbailout();
#else
    return 0;
#endif
}

int
PhoenixMinusFractal()
{
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    DComplex oldsqr, newminus;
    FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        FPUcplxmul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-2)
    }
    oldsqr.x += g_float_param->x;
    FPUcplxmul(&g_tmp_z, &oldsqr, &newminus);
    g_new_z.x = newminus.x + (g_float_param->y * tmp2.x);
    g_new_z.y = newminus.y + (g_float_param->y * tmp2.y);
    tmp2 = g_old_z; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixCplxPlusFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    LComplex loldplus, lnewminus;
    loldplus = g_l_old_z;
    g_l_temp = g_l_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        LCMPLXmult(g_l_old_z, g_l_temp, g_l_temp); // = old^(degree-1)
    }
    loldplus.x += g_long_param->x;
    loldplus.y += g_long_param->y;
    LCMPLXmult(g_l_temp, loldplus, lnewminus);
    LCMPLXmult(g_l_param2, ltmp2, g_l_temp);
    g_l_new_z.x = lnewminus.x + g_l_temp.x;
    g_l_new_z.y = lnewminus.y + g_l_temp.y;
    ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return longbailout();
#else
    return 0;
#endif
}

int
PhoenixCplxPlusFractal()
{
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    DComplex oldplus, newminus;
    oldplus = g_old_z;
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        FPUcplxmul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-1)
    }
    oldplus.x += g_float_param->x;
    oldplus.y += g_float_param->y;
    FPUcplxmul(&g_tmp_z, &oldplus, &newminus);
    FPUcplxmul(&g_param_z2, &tmp2, &g_tmp_z);
    g_new_z.x = newminus.x + g_tmp_z.x;
    g_new_z.y = newminus.y + g_tmp_z.y;
    tmp2 = g_old_z; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixCplxMinusFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    LComplex loldsqr, lnewminus;
    LCMPLXmult(g_l_old_z, g_l_old_z, loldsqr);
    g_l_temp = g_l_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        LCMPLXmult(g_l_old_z, g_l_temp, g_l_temp); // = old^(degree-2)
    }
    loldsqr.x += g_long_param->x;
    loldsqr.y += g_long_param->y;
    LCMPLXmult(g_l_temp, loldsqr, lnewminus);
    LCMPLXmult(g_l_param2, ltmp2, g_l_temp);
    g_l_new_z.x = lnewminus.x + g_l_temp.x;
    g_l_new_z.y = lnewminus.y + g_l_temp.y;
    ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return longbailout();
#else
    return 0;
#endif
}

int
PhoenixCplxMinusFractal()
{
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    DComplex oldsqr, newminus;
    FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        FPUcplxmul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-2)
    }
    oldsqr.x += g_float_param->x;
    oldsqr.y += g_float_param->y;
    FPUcplxmul(&g_tmp_z, &oldsqr, &newminus);
    FPUcplxmul(&g_param_z2, &tmp2, &g_tmp_z);
    g_new_z.x = newminus.x + g_tmp_z.x;
    g_new_z.y = newminus.y + g_tmp_z.y;
    tmp2 = g_old_z; // set tmp2 to Y value
    return floatbailout();
}

int
ScottTrigPlusTrigFractal()
{
#if !defined(XFRACT)
    // z = trig0(z)+trig1(z)
    LCMPLXtrig0(g_l_old_z, g_l_temp);
    LCMPLXtrig1(g_l_old_z, g_l_old_z);
    LCMPLXadd(g_l_temp, g_l_old_z, g_l_new_z);
    return longbailout();
#else
    return 0;
#endif
}

int
ScottTrigPlusTrigfpFractal()
{
    // z = trig0(z)+trig1(z)
    CMPLXtrig0(g_old_z, g_tmp_z);
    CMPLXtrig1(g_old_z, tmp2);
    CMPLXadd(g_tmp_z, tmp2, g_new_z);
    return floatbailout();
}

int
SkinnerTrigSubTrigFractal()
{
#if !defined(XFRACT)
    // z = trig(0, z)-trig1(z)
    LCMPLXtrig0(g_l_old_z, g_l_temp);
    LCMPLXtrig1(g_l_old_z, ltmp2);
    LCMPLXsub(g_l_temp, ltmp2, g_l_new_z);
    return longbailout();
#else
    return 0;
#endif
}

int
SkinnerTrigSubTrigfpFractal()
{
    // z = trig0(z)-trig1(z)
    CMPLXtrig0(g_old_z, g_tmp_z);
    CMPLXtrig1(g_old_z, tmp2);
    CMPLXsub(g_tmp_z, tmp2, g_new_z);
    return floatbailout();
}

int
TrigXTrigfpFractal()
{
    // z = trig0(z)*trig1(z)
    CMPLXtrig0(g_old_z, g_tmp_z);
    CMPLXtrig1(g_old_z, g_old_z);
    CMPLXmult(g_tmp_z, g_old_z, g_new_z);
    return floatbailout();
}

#if !defined(XFRACT)
// call float version of fractal if integer math overflow
static int TryFloatFractal(int (*fpFractal)())
{
    g_overflow = false;
    // lold had better not be changed!
    g_old_z.x = g_l_old_z.x;
    g_old_z.x /= g_fudge_factor;
    g_old_z.y = g_l_old_z.y;
    g_old_z.y /= g_fudge_factor;
    tempsqrx = sqr(g_old_z.x);
    tempsqry = sqr(g_old_z.y);
    fpFractal();
    if (g_save_release < 1900)
    {
        // for backwards compatibility
        g_l_new_z.x = (long)(g_new_z.x/g_fudge_factor); // this error has been here a long time
        g_l_new_z.y = (long)(g_new_z.y/g_fudge_factor);
    }
    else
    {
        g_l_new_z.x = (long)(g_new_z.x*g_fudge_factor);
        g_l_new_z.y = (long)(g_new_z.y*g_fudge_factor);
    }
    return 0;
}
#endif

int
TrigXTrigFractal()
{
#if !defined(XFRACT)
    LComplex ltmp2;
    // z = trig0(z)*trig1(z)
    LCMPLXtrig0(g_l_old_z, g_l_temp);
    LCMPLXtrig1(g_l_old_z, ltmp2);
    LCMPLXmult(g_l_temp, ltmp2, g_l_new_z);
    if (g_overflow)
    {
        TryFloatFractal(TrigXTrigfpFractal);
    }
    return longbailout();
#else
    return 0;
#endif
}

//******************************************************************
//  Next six orbit functions are one type - extra functions are
//    special cases written for speed.
//******************************************************************

int
TrigPlusSqrFractal() // generalization of Scott and Skinner types
{
#if !defined(XFRACT)
    // { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT }
    LCMPLXtrig0(g_l_old_z, g_l_temp);     // ltmp = trig(lold)
    LCMPLXmult(g_l_param, g_l_temp, g_l_new_z); // lnew = lparm*trig(lold)
    LCMPLXsqr_old(g_l_temp);         // ltmp = sqr(lold)
    LCMPLXmult(g_l_param2, g_l_temp, g_l_temp);// ltmp = lparm2*sqr(lold)
    LCMPLXadd(g_l_new_z, g_l_temp, g_l_new_z);   // lnew = lparm*trig(lold)+lparm2*sqr(lold)
    return longbailout();
#else
    return 0;
#endif
}

int
TrigPlusSqrfpFractal() // generalization of Scott and Skinner types
{
    // { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT }
    CMPLXtrig0(g_old_z, g_tmp_z);     // tmp = trig(old)
    CMPLXmult(g_param_z1, g_tmp_z, g_new_z); // new = parm*trig(old)
    CMPLXsqr_old(g_tmp_z);        // tmp = sqr(old)
    CMPLXmult(g_param_z2, g_tmp_z, tmp2); // tmp = parm2*sqr(old)
    CMPLXadd(g_new_z, tmp2, g_new_z);    // new = parm*trig(old)+parm2*sqr(old)
    return floatbailout();
}

int
ScottTrigPlusSqrFractal()
{
#if !defined(XFRACT)
    //  { z=pixel: z=trig(z)+sqr(z), |z|<BAILOUT }
    LCMPLXtrig0(g_l_old_z, g_l_new_z);    // lnew = trig(lold)
    LCMPLXsqr_old(g_l_temp);        // lold = sqr(lold)
    LCMPLXadd(g_l_temp, g_l_new_z, g_l_new_z);  // lnew = trig(lold)+sqr(lold)
    return longbailout();
#else
    return 0;
#endif
}

int
ScottTrigPlusSqrfpFractal() // float version
{
    // { z=pixel: z=sin(z)+sqr(z), |z|<BAILOUT }
    CMPLXtrig0(g_old_z, g_new_z);       // new = trig(old)
    CMPLXsqr_old(g_tmp_z);          // tmp = sqr(old)
    CMPLXadd(g_new_z, g_tmp_z, g_new_z);      // new = trig(old)+sqr(old)
    return floatbailout();
}

int
SkinnerTrigSubSqrFractal()
{
#if !defined(XFRACT)
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    LCMPLXtrig0(g_l_old_z, g_l_new_z);    // lnew = trig(lold)
    LCMPLXsqr_old(g_l_temp);        // lold = sqr(lold)
    LCMPLXsub(g_l_new_z, g_l_temp, g_l_new_z);  // lnew = trig(lold)-sqr(lold)
    return longbailout();
#else
    return 0;
#endif
}

int
SkinnerTrigSubSqrfpFractal()
{
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    CMPLXtrig0(g_old_z, g_new_z);       // new = trig(old)
    CMPLXsqr_old(g_tmp_z);          // old = sqr(old)
    CMPLXsub(g_new_z, g_tmp_z, g_new_z);      // new = trig(old)-sqr(old)
    return floatbailout();
}

int
TrigZsqrdfpFractal()
{
    // { z=pixel: z=trig(z*z), |z|<TEST }
    CMPLXsqr_old(g_tmp_z);
    CMPLXtrig0(g_tmp_z, g_new_z);
    return floatbailout();
}

int
TrigZsqrdFractal() // this doesn't work very well
{
#if !defined(XFRACT)
    // { z=pixel: z=trig(z*z), |z|<TEST }
    long l16triglim_2 = 8L << 15;
    LCMPLXsqr_old(g_l_temp);
    if ((labs(g_l_temp.x) > l16triglim_2 || labs(g_l_temp.y) > l16triglim_2) &&
            g_save_release > 1900)
    {
        g_overflow = true;
    }
    else
    {
        LCMPLXtrig0(g_l_temp, g_l_new_z);
    }
    if (g_overflow)
    {
        TryFloatFractal(TrigZsqrdfpFractal);
    }
    return longbailout();
#else
    return 0;
#endif
}

int
SqrTrigFractal()
{
#if !defined(XFRACT)
    // { z=pixel: z=sqr(trig(z)), |z|<TEST}
    LCMPLXtrig0(g_l_old_z, g_l_temp);
    LCMPLXsqr(g_l_temp, g_l_new_z);
    return longbailout();
#else
    return 0;
#endif
}

int
SqrTrigfpFractal()
{
    // SZSB(XYAXIS) { z=pixel, TEST=(p1+3): z=sin(z)*sin(z), |z|<TEST}
    CMPLXtrig0(g_old_z, g_tmp_z);
    CMPLXsqr(g_tmp_z, g_new_z);
    return floatbailout();
}

int
Magnet1Fractal()    //    Z = ((Z**2 + C - 1)/(2Z + C - 2))**2
{
    //  In "Beauty of Fractals", code by Kev Allen.
    DComplex top, bot, tmp;
    double div;

    top.x = tempsqrx - tempsqry + g_float_param->x - 1; // top = Z**2+C-1
    top.y = g_old_z.x * g_old_z.y;
    top.y = top.y + top.y + g_float_param->y;

    bot.x = g_old_z.x + g_old_z.x + g_float_param->x - 2;       // bot = 2*Z+C-2
    bot.y = g_old_z.y + g_old_z.y + g_float_param->y;

    div = bot.x*bot.x + bot.y*bot.y;                // tmp = top/bot
    if (div < FLT_MIN)
    {
        return 1;
    }
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    g_new_z.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      // Z = tmp**2
    g_new_z.y = tmp.x * tmp.y;
    g_new_z.y += g_new_z.y;

    return floatbailout();
}

int
Magnet2Fractal()  // Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2)  ) /
//       (3Z**2 + 3(C-2)Z + (C-1)(C-2)+1) )**2
{
    //   In "Beauty of Fractals", code by Kev Allen.
    DComplex top, bot, tmp;
    double div;

    top.x = g_old_z.x * (tempsqrx-tempsqry-tempsqry-tempsqry + T_Cm1.x)
            - g_old_z.y * T_Cm1.y + T_Cm1Cm2.x;
    top.y = g_old_z.y * (tempsqrx+tempsqrx+tempsqrx-tempsqry + T_Cm1.x)
            + g_old_z.x * T_Cm1.y + T_Cm1Cm2.y;

    bot.x = tempsqrx - tempsqry;
    bot.x = bot.x + bot.x + bot.x
            + g_old_z.x * T_Cm2.x - g_old_z.y * T_Cm2.y
            + T_Cm1Cm2.x + 1.0;
    bot.y = g_old_z.x * g_old_z.y;
    bot.y += bot.y;
    bot.y = bot.y + bot.y + bot.y
            + g_old_z.x * T_Cm2.y + g_old_z.y * T_Cm2.x
            + T_Cm1Cm2.y;

    div = bot.x*bot.x + bot.y*bot.y;                // tmp = top/bot
    if (div < FLT_MIN)
    {
        return 1;
    }
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    g_new_z.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      // Z = tmp**2
    g_new_z.y = tmp.x * tmp.y;
    g_new_z.y += g_new_z.y;

    return floatbailout();
}

int
LambdaTrigFractal()
{
#if !defined(XFRACT)
    LONGXYTRIGBAILOUT();
    LCMPLXtrig0(g_l_old_z, g_l_temp);           // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);   // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
#else
    return 0;
#endif
}

int
LambdaTrigfpFractal()
{
    FLOATXYTRIGBAILOUT();
    CMPLXtrig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

// bailouts are different for different trig functions
int
LambdaTrigFractal1()
{
#if !defined(XFRACT)
    LONGTRIGBAILOUT(); // sin,cos
    LCMPLXtrig0(g_l_old_z, g_l_temp);           // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);   // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
#else
    return 0;
#endif
}

int
LambdaTrigfpFractal1()
{
    FLOATTRIGBAILOUT(); // sin,cos
    CMPLXtrig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

int
LambdaTrigFractal2()
{
#if !defined(XFRACT)
    LONGHTRIGBAILOUT(); // sinh,cosh
    LCMPLXtrig0(g_l_old_z, g_l_temp);           // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);   // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
#else
    return 0;
#endif
}

int
LambdaTrigfpFractal2()
{
#if !defined(XFRACT)
    FLOATHTRIGBAILOUT(); // sinh,cosh
    CMPLXtrig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
#else
    return 0;
#endif
}

int
ManOWarFractal()
{
#if !defined(XFRACT)
    // From Art Matrix via Lee Skinner
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_l_temp.x + g_long_param->x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_l_temp.y + g_long_param->y;
    g_l_temp = g_l_old_z;
    return longbailout();
#else
    return 0;
#endif
}

int
ManOWarfpFractal()
{
    // From Art Matrix via Lee Skinner
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step
    g_new_z.x = tempsqrx - tempsqry + g_tmp_z.x + g_float_param->x;
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y + g_tmp_z.y + g_float_param->y;
    g_tmp_z = g_old_z;
    return floatbailout();
}

/*
   MarksMandelPwr (XAXIS) {
      z = pixel, c = z ^ (z - 1):
         z = c * sqr(z) + pixel,
      |z| <= 4
   }
*/

int
MarksMandelPwrfpFractal()
{
    CMPLXtrig0(g_old_z, g_new_z);
    CMPLXmult(g_tmp_z, g_new_z, g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return floatbailout();
}

int
MarksMandelPwrFractal()
{
#if !defined(XFRACT)
    LCMPLXtrig0(g_l_old_z, g_l_new_z);
    LCMPLXmult(g_l_temp, g_l_new_z, g_l_new_z);
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

/* I was coding Marksmandelpower and failed to use some temporary
   variables. The result was nice, and since my name is not on any fractal,
   I thought I would immortalize myself with this error!
                Tim Wegner */

int
TimsErrorfpFractal()
{
    CMPLXtrig0(g_old_z, g_new_z);
    g_new_z.x = g_new_z.x * g_tmp_z.x - g_new_z.y * g_tmp_z.y;
    g_new_z.y = g_new_z.x * g_tmp_z.y - g_new_z.y * g_tmp_z.x;
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return floatbailout();
}

int
TimsErrorFractal()
{
#if !defined(XFRACT)
    LCMPLXtrig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x = multiply(g_l_new_z.x, g_l_temp.x, g_bit_shift)-multiply(g_l_new_z.y, g_l_temp.y, g_bit_shift);
    g_l_new_z.y = multiply(g_l_new_z.x, g_l_temp.y, g_bit_shift)-multiply(g_l_new_z.y, g_l_temp.x, g_bit_shift);
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

int
CirclefpFractal()
{
    long i;
    i = (long)(g_params[0]*(tempsqrx+tempsqry));
    g_color_iter = i%g_colors;
    return 1;
}
/*
CirclelongFractal()
{
   long i;
   i = multiply(lparm.x,(g_l_temp_sqr_x+g_l_temp_sqr_y),g_bit_shift);
   i = i >> g_bit_shift;
   g_color_iter = i%colors);
   return 1;
}
*/

// --------------------------------------------------------------------
//              Initialization (once per pixel) routines
// --------------------------------------------------------------------

// this code translated to asm - lives in newton.asm
// transform points with reciprocal function
void invertz2(DComplex *z)
{
    z->x = g_dx_pixel();
    z->y = g_dy_pixel();
    z->x -= g_f_x_center;
    z->y -= g_f_y_center;  // Normalize values to center of circle

    tempsqrx = sqr(z->x) + sqr(z->y);  // Get old radius
    if (fabs(tempsqrx) > FLT_MIN)
    {
        tempsqrx = g_f_radius / tempsqrx;
    }
    else
    {
        tempsqrx = FLT_MAX;   // a big number, but not TOO big
    }
    z->x *= tempsqrx;
    z->y *= tempsqrx;      // Perform inversion
    z->x += g_f_x_center;
    z->y += g_f_y_center; // Renormalize
}

int long_julia_per_pixel()
{
#if !defined(XFRACT)
    // integer julia types
    // lambda
    // barnsleyj1
    // barnsleyj2
    // sierpinski
    if (g_invert != 0)
    {
        // invert
        invertz2(&g_old_z);

        // watch out for overflow
        if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 127)
        {
            g_old_z.x = 8;  // value to bail out in one iteration
            g_old_z.y = 8;
        }

        // convert to fudged longs
        g_l_old_z.x = (long)(g_old_z.x*g_fudge_factor);
        g_l_old_z.y = (long)(g_old_z.y*g_fudge_factor);
    }
    else
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
    }
    return 0;
#else
    return 0;
#endif
}

int long_richard8_per_pixel()
{
#if !defined(XFRACT)
    long_mandel_per_pixel();
    LCMPLXtrig1(*g_long_param, g_l_temp);
    LCMPLXmult(g_l_temp, g_l_param2, g_l_temp);
    return 1;
#else
    return 0;
#endif
}

int long_mandel_per_pixel()
{
#if !defined(XFRACT)
    // integer mandel types
    // barnsleym1
    // barnsleym2
    g_l_init.x = g_l_x_pixel();
    if (g_save_release >= 2004)
    {
        g_l_init.y = g_l_y_pixel();
    }

    if (g_invert != 0)
    {
        // invert
        invertz2(&g_init);

        // watch out for overflow
        if (sqr(g_init.x)+sqr(g_init.y) >= 127)
        {
            g_init.x = 8;  // value to bail out in one iteration
            g_init.y = 8;
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else
    {
        g_l_old_z = g_l_init;
    }

    g_l_old_z.x += g_l_param.x;    // initial pertubation of parameters set
    g_l_old_z.y += g_l_param.y;
    return 1; // 1st iteration has been done
#else
    return 0;
#endif
}

int julia_per_pixel()
{
    // julia

    if (g_invert != 0)
    {
        // invert
        invertz2(&g_old_z);

        // watch out for overflow
        if (g_bit_shift <= 24)
        {
            if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 127)
            {
                g_old_z.x = 8;  // value to bail out in one iteration
                g_old_z.y = 8;
            }
        }
        if (g_bit_shift >  24)
        {
            if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 4.0)
            {
                g_old_z.x = 2;  // value to bail out in one iteration
                g_old_z.y = 2;
            }
        }

        // convert to fudged longs
        g_l_old_z.x = (long)(g_old_z.x*g_fudge_factor);
        g_l_old_z.y = (long)(g_old_z.y*g_fudge_factor);
    }
    else
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
    }

    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    g_l_temp = g_l_old_z;
    return 0;
}

int
marks_mandelpwr_per_pixel()
{
#if !defined(XFRACT)
    mandel_per_pixel();
    g_l_temp = g_l_old_z;
    g_l_temp.x -= g_fudge_factor;
    LCMPLXpwr(g_l_old_z, g_l_temp, g_l_temp);
    return 1;
#else
    return 0;
#endif
}

int mandel_per_pixel()
{
    // mandel

    if (g_invert != 0)
    {
        invertz2(&g_init);

        // watch out for overflow
        if (g_bit_shift <= 24)
        {
            if (sqr(g_init.x)+sqr(g_init.y) >= 127)
            {
                g_init.x = 8;  // value to bail out in one iteration
                g_init.y = 8;
            }
        }
        if (g_bit_shift >  24)
        {
            if (sqr(g_init.x)+sqr(g_init.y) >= 4)
            {
                g_init.x = 2;  // value to bail out in one iteration
                g_init.y = 2;
            }
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }
    else
    {
        g_l_init.x = g_l_x_pixel();
        if (g_save_release >= 2004)
        {
            g_l_init.y = g_l_y_pixel();
        }
    }
    switch (fractype)
    {
    case fractal_type::MANDELLAMBDA:              // Critical Value 0.5 + 0.0i
        g_l_old_z.x = g_fudge_half;
        g_l_old_z.y = 0;
        break;
    default:
        g_l_old_z = g_l_init;
        break;
    }

    // alter init value
    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else if (g_use_init_orbit == init_orbit_mode::pixel)
    {
        g_l_old_z = g_l_init;
    }

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        g_l_old_z.x = g_l_param.x; // initial pertubation of parameters set
        g_l_old_z.y = g_l_param.y;
        g_color_iter = -1;
    }
    else
    {
        g_l_old_z.x += g_l_param.x; // initial pertubation of parameters set
        g_l_old_z.y += g_l_param.y;
    }
    g_l_temp = g_l_init; // for spider
    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    return 1; // 1st iteration has been done
}

int marksmandel_per_pixel()
{
#if !defined(XFRACT)
    // marksmandel
    if (g_invert != 0)
    {
        invertz2(&g_init);

        // watch out for overflow
        if (sqr(g_init.x)+sqr(g_init.y) >= 127)
        {
            g_init.x = 8;  // value to bail out in one iteration
            g_init.y = 8;
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }
    else
    {
        g_l_init.x = g_l_x_pixel();
        if (g_save_release >= 2004)
        {
            g_l_init.y = g_l_y_pixel();
        }
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else
    {
        g_l_old_z = g_l_init;
    }

    g_l_old_z.x += g_l_param.x;    // initial pertubation of parameters set
    g_l_old_z.y += g_l_param.y;

    if (g_c_exponent > 3)
    {
        lcpower(&g_l_old_z, g_c_exponent-1, &g_l_coefficient, g_bit_shift);
    }
    else if (g_c_exponent == 3)
    {
        g_l_coefficient.x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift)
                         - multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
        g_l_coefficient.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1);
    }
    else if (g_c_exponent == 2)
    {
        g_l_coefficient = g_l_old_z;
    }
    else if (g_c_exponent < 2)
    {
        g_l_coefficient.x = 1L << g_bit_shift;
        g_l_coefficient.y = 0L;
    }

    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
#endif
    return 1; // 1st iteration has been done
}

int marksmandelfp_per_pixel()
{
    // marksmandel

    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        if (g_save_release >= 2004)
        {
            g_init.y = g_dy_pixel();
        }
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_old_z = g_init_orbit;
    }
    else
    {
        g_old_z = g_init;
    }

    g_old_z.x += g_param_z1.x;      // initial pertubation of parameters set
    g_old_z.y += g_param_z1.y;

    tempsqrx = sqr(g_old_z.x);
    tempsqry = sqr(g_old_z.y);

    if (g_c_exponent > 3)
    {
        cpower(&g_old_z, g_c_exponent-1, &g_marks_coefficient);
    }
    else if (g_c_exponent == 3)
    {
        g_marks_coefficient.x = tempsqrx - tempsqry;
        g_marks_coefficient.y = g_old_z.x * g_old_z.y * 2;
    }
    else if (g_c_exponent == 2)
    {
        g_marks_coefficient = g_old_z;
    }
    else if (g_c_exponent < 2)
    {
        g_marks_coefficient.x = 1.0;
        g_marks_coefficient.y = 0.0;
    }

    return 1; // 1st iteration has been done
}

int
marks_mandelpwrfp_per_pixel()
{
    mandelfp_per_pixel();
    g_tmp_z = g_old_z;
    g_tmp_z.x -= 1;
    CMPLXpwr(g_old_z, g_tmp_z, g_tmp_z);
    return 1;
}

int mandelfp_per_pixel()
{
    // floating point mandelbrot
    // mandelfp

    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        if (g_save_release >= 2004)
        {
            g_init.y = g_dy_pixel();
        }
    }
    switch (fractype)
    {
    case fractal_type::MAGNET2M:
        FloatPreCalcMagnet2();
    case fractal_type::MAGNET1M:
        g_old_z.y = 0.0;        // Critical Val Zero both, but neither
        g_old_z.x = g_old_z.y;      // is of the form f(Z,C) = Z*g(Z)+C
        break;
    case fractal_type::MANDELLAMBDAFP:            // Critical Value 0.5 + 0.0i
        g_old_z.x = 0.5;
        g_old_z.y = 0.0;
        break;
    default:
        g_old_z = g_init;
        break;
    }

    // alter init value
    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_old_z = g_init_orbit;
    }
    else if (g_use_init_orbit == init_orbit_mode::pixel)
    {
        g_old_z = g_init;
    }

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        g_old_z.x = g_param_z1.x; // initial pertubation of parameters set
        g_old_z.y = g_param_z1.y;
        g_color_iter = -1;
    }
    else
    {
        g_old_z.x += g_param_z1.x;
        g_old_z.y += g_param_z1.y;
    }
    g_tmp_z = g_init; // for spider
    tempsqrx = sqr(g_old_z.x);  // precalculated value for regular Mandelbrot
    tempsqry = sqr(g_old_z.y);
    return 1; // 1st iteration has been done
}

int juliafp_per_pixel()
{
    // floating point julia
    // juliafp
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    tempsqrx = sqr(g_old_z.x);  // precalculated value for regular Julia
    tempsqry = sqr(g_old_z.y);
    g_tmp_z = g_old_z;
    return 0;
}

int MPCjulia_per_pixel()
{
#if !defined(XFRACT)
    // floating point julia
    // juliafp
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    mpcold.x = *pd2MP(g_old_z.x);
    mpcold.y = *pd2MP(g_old_z.y);
    return 0;
#else
    return 0;
#endif
}

int
otherrichard8fp_per_pixel()
{
    othermandelfp_per_pixel();
    CMPLXtrig1(*g_float_param, g_tmp_z);
    CMPLXmult(g_tmp_z, g_param_z2, g_tmp_z);
    return 1;
}

int othermandelfp_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        if (g_save_release >= 2004)
        {
            g_init.y = g_dy_pixel();
        }
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_old_z = g_init_orbit;
    }
    else
    {
        g_old_z = g_init;
    }

    g_old_z.x += g_param_z1.x;      // initial pertubation of parameters set
    g_old_z.y += g_param_z1.y;

    return 1; // 1st iteration has been done
}

int MPCHalley_per_pixel()
{
#if !defined(XFRACT)
    // MPC halley
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        if (g_save_release >= 2004)
        {
            g_init.y = g_dy_pixel();
        }
    }

    mpcold.x = *pd2MP(g_init.x);
    mpcold.y = *pd2MP(g_init.y);

    return 0;
#else
    return 0;
#endif
}

int Halley_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        if (g_save_release >= 2004)
        {
            g_init.y = g_dy_pixel();
        }
    }

    g_old_z = g_init;

    return 0; // 1st iteration is not done
}

int otherjuliafp_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    return 0;
}

#define Q0 0
#define Q1 0

int quaternionjulfp_per_pixel()
{
    g_old_z.x = g_dx_pixel();
    g_old_z.y = g_dy_pixel();
    g_float_param->x = g_params[4];
    g_float_param->y = g_params[5];
    g_quaternion_c  = g_params[0];
    g_quaternion_ci = g_params[1];
    g_quaternion_cj = g_params[2];
    g_quaternino_ck = g_params[3];
    return 0;
}

int quaternionfp_per_pixel()
{
    g_old_z.x = 0;
    g_old_z.y = 0;
    g_float_param->x = 0;
    g_float_param->y = 0;
    g_quaternion_c  = g_dx_pixel();
    g_quaternion_ci = g_dy_pixel();
    g_quaternion_cj = g_params[2];
    g_quaternino_ck = g_params[3];
    return 0;
}

int MarksCplxMandperp()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        if (g_save_release >= 2004)
        {
            g_init.y = g_dy_pixel();
        }
    }
    g_old_z.x = g_init.x + g_param_z1.x; // initial pertubation of parameters set
    g_old_z.y = g_init.y + g_param_z1.y;
    tempsqrx = sqr(g_old_z.x);  // precalculated value
    tempsqry = sqr(g_old_z.y);
    g_marks_coefficient = ComplexPower(g_init, g_power_z);
    return 1;
}

int long_phoenix_per_pixel()
{
#if !defined(XFRACT)
    if (g_invert != 0)
    {
        // invert
        invertz2(&g_old_z);

        // watch out for overflow
        if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 127)
        {
            g_old_z.x = 8;  // value to bail out in one iteration
            g_old_z.y = 8;
        }

        // convert to fudged longs
        g_l_old_z.x = (long)(g_old_z.x*g_fudge_factor);
        g_l_old_z.y = (long)(g_old_z.y*g_fudge_factor);
    }
    else
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
    }
    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    ltmp2.x = 0; // use ltmp2 as the complex Y value
    ltmp2.y = 0;
    return 0;
#else
    return 0;
#endif
}

int phoenix_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    tempsqrx = sqr(g_old_z.x);  // precalculated value
    tempsqry = sqr(g_old_z.y);
    tmp2.x = 0; // use tmp2 as the complex Y value
    tmp2.y = 0;
    return 0;
}
int long_mandphoenix_per_pixel()
{
#if !defined(XFRACT)
    g_l_init.x = g_l_x_pixel();
    if (g_save_release >= 2004)
    {
        g_l_init.y = g_l_y_pixel();
    }

    if (g_invert != 0)
    {
        // invert
        invertz2(&g_init);

        // watch out for overflow
        if (sqr(g_init.x)+sqr(g_init.y) >= 127)
        {
            g_init.x = 8;  // value to bail out in one iteration
            g_init.y = 8;
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else
    {
        g_l_old_z = g_l_init;
    }

    g_l_old_z.x += g_l_param.x;    // initial pertubation of parameters set
    g_l_old_z.y += g_l_param.y;
    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    ltmp2.x = 0;
    ltmp2.y = 0;
    return 1; // 1st iteration has been done
#else
    return 0;
#endif
}
int mandphoenix_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        if (g_save_release >= 2004)
        {
            g_init.y = g_dy_pixel();
        }
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_old_z = g_init_orbit;
    }
    else
    {
        g_old_z = g_init;
    }

    g_old_z.x += g_param_z1.x;      // initial pertubation of parameters set
    g_old_z.y += g_param_z1.y;
    tempsqrx = sqr(g_old_z.x);  // precalculated value
    tempsqry = sqr(g_old_z.y);
    tmp2.x = 0;
    tmp2.y = 0;
    return 1; // 1st iteration has been done
}

int
QuaternionFPFractal()
{
    double a0, a1, a2, a3, n0, n1, n2, n3;
    a0 = g_old_z.x;
    a1 = g_old_z.y;
    a2 = g_float_param->x;
    a3 = g_float_param->y;

    n0 = a0*a0-a1*a1-a2*a2-a3*a3 + g_quaternion_c;
    n1 = 2*a0*a1 + g_quaternion_ci;
    n2 = 2*a0*a2 + g_quaternion_cj;
    n3 = 2*a0*a3 + g_quaternino_ck;
    // Check bailout
    g_magnitude = a0*a0+a1*a1+a2*a2+a3*a3;
    if (g_magnitude > g_magnitude_limit)
    {
        return 1;
    }
    g_new_z.x = n0;
    g_old_z.x = g_new_z.x;
    g_new_z.y = n1;
    g_old_z.y = g_new_z.y;
    g_float_param->x = n2;
    g_float_param->y = n3;
    return 0;
}

int
HyperComplexFPFractal()
{
    DHyperComplex hold, hnew;
    hold.x = g_old_z.x;
    hold.y = g_old_z.y;
    hold.z = g_float_param->x;
    hold.t = g_float_param->y;

    HComplexTrig0(&hold, &hnew);

    hnew.x += g_quaternion_c;
    hnew.y += g_quaternion_ci;
    hnew.z += g_quaternion_cj;
    hnew.t += g_quaternino_ck;

    g_new_z.x = hnew.x;
    g_old_z.x = g_new_z.x;
    g_new_z.y = hnew.y;
    g_old_z.y = g_new_z.y;
    g_float_param->x = hnew.z;
    g_float_param->y = hnew.t;

    // Check bailout
    g_magnitude = sqr(g_old_z.x)+sqr(g_old_z.y)+sqr(g_float_param->x)+sqr(g_float_param->y);
    if (g_magnitude > g_magnitude_limit)
    {
        return 1;
    }
    return 0;
}

int
VLfpFractal() // Beauty of Fractals pp. 125 - 127
{
    double a, b, ab, half, u, w, xy;

    half = g_params[0] / 2.0;
    xy = g_old_z.x * g_old_z.y;
    u = g_old_z.x - xy;
    w = -g_old_z.y + xy;
    a = g_old_z.x + g_params[1] * u;
    b = g_old_z.y + g_params[1] * w;
    ab = a * b;
    g_new_z.x = g_old_z.x + half * (u + (a - ab));
    g_new_z.y = g_old_z.y + half * (w + (-b + ab));
    return floatbailout();
}

int
EscherfpFractal() // Science of Fractal Images pp. 185, 187
{
    DComplex oldtest, newtest, testsqr;
    double testsize = 0.0;
    long testiter = 0;

    g_new_z.x = tempsqrx - tempsqry; // standard Julia with C == (0.0, 0.0i)
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y;
    oldtest.x = g_new_z.x * 15.0;    // scale it
    oldtest.y = g_new_z.y * 15.0;
    testsqr.x = sqr(oldtest.x);  // set up to test with user-specified ...
    testsqr.y = sqr(oldtest.y);  //    ... Julia as the target set
    while (testsize <= g_magnitude_limit && testiter < g_max_iterations) // nested Julia loop
    {
        newtest.x = testsqr.x - testsqr.y + g_params[0];
        newtest.y = 2.0 * oldtest.x * oldtest.y + g_params[1];
        testsqr.x = sqr(newtest.x);
        testsqr.y = sqr(newtest.y);
        testsize = testsqr.x + testsqr.y;
        oldtest = newtest;
        testiter++;
    }
    if (testsize > g_magnitude_limit)
    {
        return floatbailout(); // point not in target set
    }
    else   // make distinct level sets if point stayed in target set
    {
        g_color_iter = ((3L * g_color_iter) % 255L) + 1L;
        return 1;
    }
}

/* re-use static roots variable
   memory for mandelmix4 */

#define A staticroots[ 0]
#define B staticroots[ 1]
#define C staticroots[ 2]
#define D staticroots[ 3]
#define F staticroots[ 4]
#define G staticroots[ 5]
#define H staticroots[ 6]
#define J staticroots[ 7]
#define K staticroots[ 8]
#define L staticroots[ 9]
#define Z staticroots[10]

bool MandelbrotMix4Setup()
{
    int sign_array = 0;
    A.x = g_params[0];
    A.y = 0.0;    // a=real(p1),
    B.x = g_params[1];
    B.y = 0.0;    // b=imag(p1),
    D.x = g_params[2];
    D.y = 0.0;    // d=real(p2),
    F.x = g_params[3];
    F.y = 0.0;    // f=imag(p2),
    K.x = g_params[4]+1.0;
    K.y = 0.0;    // k=real(p3)+1,
    L.x = g_params[5]+100.0;
    L.y = 0.0;    // l=imag(p3)+100,
    CMPLXrecip(F, G);                // g=1/f,
    CMPLXrecip(D, H);                // h=1/d,
    CMPLXsub(F, B, g_tmp_z);              // tmp = f-b
    CMPLXrecip(g_tmp_z, J);              // j = 1/(f-b)
    CMPLXneg(A, g_tmp_z);
    CMPLXmult(g_tmp_z, B, g_tmp_z);           // z=(-a*b*g*h)^j,
    CMPLXmult(g_tmp_z, G, g_tmp_z);
    CMPLXmult(g_tmp_z, H, g_tmp_z);

    /*
       This code kludge attempts to duplicate the behavior
       of the parser in determining the sign of zero of the
       imaginary part of the argument of the power function. The
       reason this is important is that the complex arctangent
       returns PI in one case and -PI in the other, depending
       on the sign bit of zero, and we wish the results to be
       compatible with Jim Muth's mix4 formula using the parser.

       First create a number encoding the signs of a, b, g , h. Our
       kludge assumes that those signs determine the behavior.
     */
    if (A.x < 0.0)
    {
        sign_array += 8;
    }
    if (B.x < 0.0)
    {
        sign_array += 4;
    }
    if (G.x < 0.0)
    {
        sign_array += 2;
    }
    if (H.x < 0.0)
    {
        sign_array += 1;
    }
    if (g_tmp_z.y == 0.0) // we know tmp.y IS zero but ...
    {
        switch (sign_array)
        {
        /*
           Add to this list the magic numbers of any cases
           in which the fractal does not match the formula version
         */
        case 15: // 1111
        case 10: // 1010
        case  6: // 0110
        case  5: // 0101
        case  3: // 0011
        case  0: // 0000
            g_tmp_z.y = -g_tmp_z.y; // swap sign bit
        default: // do nothing - remaining cases already OK
            ;
        }
        // in case our kludge failed, let the user fix it
        if (g_debug_flag == debug_flags::mandelbrot_mix4_flip_sign)
        {
            g_tmp_z.y = -g_tmp_z.y;
        }
    }

    CMPLXpwr(g_tmp_z, J, g_tmp_z);   // note: z is old
    // in case our kludge failed, let the user fix it
    if (g_params[6] < 0.0)
    {
        g_tmp_z.y = -g_tmp_z.y;
    }

    if (g_bail_out == 0)
    {
        g_magnitude_limit = L.x;
        rqlim2 = g_magnitude_limit*g_magnitude_limit;
    }
    return true;
}

int MandelbrotMix4fp_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }
    g_old_z = g_tmp_z;
    CMPLXtrig0(g_init, C);        // c=fn1(pixel):
    return 0; // 1st iteration has been NOT been done
}

int
MandelbrotMix4fpFractal() // from formula by Jim Muth
{
    // z=k*((a*(z^b))+(d*(z^f)))+c,
    DComplex z_b, z_f;
    CMPLXpwr(g_old_z, B, z_b);     // (z^b)
    CMPLXpwr(g_old_z, F, z_f);     // (z^f)
    g_new_z.x = K.x*A.x*z_b.x + K.x*D.x*z_f.x + C.x;
    g_new_z.y = K.x*A.x*z_b.y + K.x*D.x*z_f.y + C.y;
    return floatbailout();
}
#undef A
#undef B
#undef C
#undef D
#undef F
#undef G
#undef H
#undef J
#undef K
#undef L

/*
 * The following functions calculate the real and imaginary complex
 * coordinates of the point in the complex plane corresponding to
 * the screen coordinates (col,row) at the current zoom corners
 * settings. The functions come in two flavors. One looks up the pixel
 * values using the precalculated grid arrays dx0, dx1, dy0, and dy1,
 * which has a speed advantage but is limited to MAXPIXELS image
 * dimensions. The other calculates the complex coordinates at a
 * cost of two additions and two multiplications for each component,
 * but works at any resolution.
 *
 * The function call overhead
 * appears to be negligible. It also appears that the speed advantage
 * of the lookup vs the calculation is negligible on machines with
 * coprocessors. Bert Tyler's original implementation was designed for
 * machines with no coprocessor; on those machines the saving was
 * significant. For the time being, the table lookup capability will
 * be maintained.
 */

// Real component, grid lookup version - requires dx0/dx1 arrays
static double dxpixel_grid()
{
    return g_grid_x0[g_col]+g_grid_x1[g_row];
}

// Real component, calculation version - does not require arrays
static double dxpixel_calc()
{
    return (double)(xxmin + g_col*g_delta_x + g_row*g_delta_x2);
}

// Imaginary component, grid lookup version - requires dy0/dy1 arrays
static double dypixel_grid()
{
    return g_grid_y0[g_row]+g_grid_y1[g_col];
}

// Imaginary component, calculation version - does not require arrays
static double dypixel_calc()
{
    return (double)(yymax - g_row*g_delta_y - g_col*g_delta_y2);
}

// Real component, grid lookup version - requires lx0/lx1 arrays
static long lxpixel_grid()
{
    return g_l_x0[g_col]+g_l_x1[g_row];
}

// Real component, calculation version - does not require arrays
static long lxpixel_calc()
{
    return xmin + g_col*g_l_delta_x + g_row*g_l_delta_x2;
}

// Imaginary component, grid lookup version - requires ly0/ly1 arrays
static long lypixel_grid()
{
    return g_l_y0[g_row]+g_l_y1[g_col];
}

// Imaginary component, calculation version - does not require arrays
static long lypixel_calc()
{
    return ymax - g_row*g_l_delta_y - g_col*g_l_delta_y2;
}

double (*g_dx_pixel)() = dxpixel_calc;
double (*g_dy_pixel)() = dypixel_calc;
long (*g_l_x_pixel)() = lxpixel_calc;
long (*g_l_y_pixel)() = lypixel_calc;

void set_pixel_calc_functions()
{
    if (g_use_grid)
    {
        g_dx_pixel = dxpixel_grid;
        g_dy_pixel = dypixel_grid;
        g_l_x_pixel = lxpixel_grid;
        g_l_y_pixel = lypixel_grid;
    }
    else
    {
        g_dx_pixel = dxpixel_calc;
        g_dy_pixel = dypixel_calc;
        g_l_x_pixel = lxpixel_calc;
        g_l_y_pixel = lypixel_calc;
    }
}
