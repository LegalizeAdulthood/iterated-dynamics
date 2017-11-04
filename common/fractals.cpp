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

LComplex g_l_coefficient, g_l_old, g_l_new, g_l_param, g_l_init, g_l_temp, ltmp2, g_l_param2;
long g_l_temp_sqr_x, g_l_temp_sqr_y;
int maxcolor;
int root, degree, g_basin;
double roverd, g_degree_minus_1_over_degree, threshold;
DComplex tmp2;
DComplex g_marks_coefficient;
DComplex  staticroots[16]; // roots array for degree 16 or less
std::vector<DComplex> roots;
std::vector<MPC> MPCroots;
long g_fudge_half;
DComplex pwr;
int     bitshiftless1;                  // bit shift less 1
bool overflow = false;

#define modulus(z)       (sqr((z).x)+sqr((z).y))
#define conjugate(pz)   ((pz)->y = 0.0 - (pz)->y)
#define distance(z1, z2)  (sqr((z1).x-(z2).x)+sqr((z1).y-(z2).y))
#define pMPsqr(z) (*pMPmul((z), (z)))
#define MPdistance(z1, z2)  (*pMPadd(pMPsqr(*pMPsub((z1).x, (z2).x)), pMPsqr(*pMPsub((z1).y, (z2).y))))

double twopi = PI*2.0;
int g_c_exponent;


// These are local but I don't want to pass them as parameters
DComplex parm, parm2;
DComplex *g_float_param;
LComplex *g_long_param; // used here and in jb.c

// --------------------------------------------------------------------
//              These variables are external for speed's sake only
// --------------------------------------------------------------------

double sinx, cosx;
double siny, cosy;
double tmpexp;
double tempsqrx, tempsqry;

double foldxinitx, foldyinity, foldxinity, foldyinitx;
long oldxinitx, oldyinity, oldxinity, oldyinitx;
long longtmp;

// These are for quaternions
double qc, qci, qcj, qck;

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
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    if (magnitude >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

int  fpREALbailout()
{
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    if (tempsqrx >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

int  fpIMAGbailout()
{
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    if (tempsqry >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

int  fpORbailout()
{
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    if (tempsqrx >= rqlim || tempsqry >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

int  fpANDbailout()
{
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    if (tempsqrx >= rqlim && tempsqry >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

int  fpMANHbailout()
{
    double manhmag;
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    manhmag = fabs(g_new.x) + fabs(g_new.y);
    if ((manhmag * manhmag) >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

int  fpMANRbailout()
{
    double manrmag;
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    manrmag = g_new.x + g_new.y; // don't need abs() since we square it next
    if ((manrmag * manrmag) >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

#define FLOATTRIGBAILOUT()  \
    if (fabs(old.y) >= rqlim2) \
        return 1;

#define LONGTRIGBAILOUT()  \
    if (labs(g_l_old.y) >= g_l_limit2) \
    { \
        return 1; \
    }

#define LONGXYTRIGBAILOUT()  \
    if (labs(g_l_old.x) >= g_l_limit2 || labs(g_l_old.y) >= g_l_limit2)\
        { return 1;}

#define FLOATXYTRIGBAILOUT()  \
    if (fabs(old.x) >= rqlim2 || fabs(old.y) >= rqlim2) \
        return 1;

#define FLOATHTRIGBAILOUT()  \
    if (fabs(old.x) >= rqlim2) \
        return 1;

#define LONGHTRIGBAILOUT()  \
    if (labs(g_l_old.x) >= g_l_limit2) \
    { \
        return 1; \
    }

#define TRIG16CHECK(X)  \
    if (labs((X)) > l16triglim) \
    { \
        return 1; \
    }

#define OLD_FLOATEXPBAILOUT()  \
    if (fabs(old.y) >= 1.0e8) \
        return 1;\
    if (fabs(old.x) >= 6.4e2) \
        return 1;

#define FLOATEXPBAILOUT()  \
    if (fabs(old.y) >= 1.0e3) \
        return 1;\
    if (fabs(old.x) >= 8) \
        return 1;

#define LONGEXPBAILOUT()  \
    if (labs(g_l_old.y) >= (1000L << bitshift)) \
        return 1;\
    if (labs(g_l_old.x) >=    (8L << bitshift)) \
        return 1;

#define LTRIGARG(X)    \
    if (labs((X)) > l16triglim)\
    {\
        double tmp;\
        tmp = (X);\
        tmp /= g_fudge_factor;\
        tmp = fmod(tmp, twopi);\
        tmp *= g_fudge_factor;\
        (X) = (long)tmp;\
    }\
 
static int  Halleybailout()
{
    if (fabs(modulus(g_new)-modulus(old)) < parm2.x)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

#if !defined(XFRACT)
#define MPCmod(m) (*pMPadd(*pMPmul((m).x, (m).x), *pMPmul((m).y, (m).y)))
MPC mpcold, mpcnew, mpctmp, mpctmp1;
MP mptmpparm2x;

static int  MPCHalleybailout()
{
    static MP mptmpbailout;
    mptmpbailout = *MPabs(*pMPsub(MPCmod(mpcnew), MPCmod(mpcold)));
    if (pMPcmp(mptmpbailout, mptmpparm2x) < 0)
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
        overflow = lcpower(base, -exp, result, bitshift) != 0;
        LCMPLXrecip(*result, *result);
        return overflow ? 1 : 0;
    }

    overflow = false;
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
        lyt = multiply(lxt, lyt, bitshiftless1);
        if (overflow)
        {
            return overflow;
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
        overflow = true;
    }
    return overflow;
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
    cpower(&old, degree-1, &tmp);
    complex_mult(tmp, old, &g_new);

    if (DIST1(g_new) < threshold)
    {
        if (fractype == fractal_type::NEWTBASIN || fractype == fractal_type::MPNEWTBASIN)
        {
            long tmpcolor;
            tmpcolor = -1;
            /* this code determines which degree-th root of root the
               Newton formula converges to. The roots of a 1 are
               distributed on a circle of radius 1 about the origin. */
            for (int i = 0; i < degree; i++)
            {
                /* color in alternating shades with iteration according to
                   which root of 1 it converged to */
                if (distance(roots[i], old) < threshold)
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
                g_color_iter = maxcolor;
            }
            else
            {
                g_color_iter = tmpcolor;
            }
        }
        return 1;
    }
    g_new.x = g_degree_minus_1_over_degree * g_new.x + roverd;
    g_new.y *= g_degree_minus_1_over_degree;

    // Watch for divide underflow
    t2 = tmp.x*tmp.x + tmp.y*tmp.y;
    if (t2 < FLT_MIN)
    {
        return 1;
    }
    else
    {
        t2 = 1.0 / t2;
        old.x = t2 * (g_new.x * tmp.x + g_new.y * tmp.y);
        old.y = t2 * (g_new.y * tmp.x - g_new.x * tmp.y);
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
MP mproverd, mpd1overd, mpthreshold;
MP mpt2;
MP mpone;
#endif

int MPCNewtonFractal()
{
#if !defined(XFRACT)
    MPOverflow = 0;
    mpctmp   = MPCpow(mpcold, degree-1);

    mpcnew.x = *pMPsub(*pMPmul(mpctmp.x, mpcold.x), *pMPmul(mpctmp.y, mpcold.y));
    mpcnew.y = *pMPadd(*pMPmul(mpctmp.x, mpcold.y), *pMPmul(mpctmp.y, mpcold.x));
    mpctmp1.x = *pMPsub(mpcnew.x, MPCone.x);
    mpctmp1.y = *pMPsub(mpcnew.y, MPCone.y);
    if (pMPcmp(MPCmod(mpctmp1), mpthreshold) < 0)
    {
        if (fractype == fractal_type::MPNEWTBASIN)
        {
            long tmpcolor;
            tmpcolor = -1;
            for (int i = 0; i < degree; i++)
                if (pMPcmp(MPdistance(MPCroots[i], mpcold), mpthreshold) < 0)
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
                g_color_iter = maxcolor;
            }
            else
            {
                g_color_iter = tmpcolor;
            }
        }
        return 1;
    }

    mpcnew.x = *pMPadd(*pMPmul(mpd1overd, mpcnew.x), mproverd);
    mpcnew.y = *pMPmul(mpcnew.y, mpd1overd);
    mpt2 = MPCmod(mpctmp);
    mpt2 = *pMPdiv(mpone, mpt2);
    mpcold.x = *pMPmul(mpt2, (*pMPadd(*pMPmul(mpcnew.x, mpctmp.x), *pMPmul(mpcnew.y, mpctmp.y))));
    mpcold.y = *pMPmul(mpt2, (*pMPsub(*pMPmul(mpcnew.y, mpctmp.x), *pMPmul(mpcnew.x, mpctmp.y))));
    g_new.x = *pMP2d(mpcold.x);
    g_new.y = *pMP2d(mpcold.y);
    return MPOverflow;
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
    oldxinitx   = multiply(g_l_old.x, g_long_param->x, bitshift);
    oldyinity   = multiply(g_l_old.y, g_long_param->y, bitshift);
    oldxinity   = multiply(g_l_old.x, g_long_param->y, bitshift);
    oldyinitx   = multiply(g_l_old.y, g_long_param->x, bitshift);
    // orbit calculation
    if (g_l_old.x >= 0)
    {
        g_l_new.x = (oldxinitx - g_long_param->x - oldyinity);
        g_l_new.y = (oldyinitx - g_long_param->y + oldxinity);
    }
    else
    {
        g_l_new.x = (oldxinitx + g_long_param->x - oldyinity);
        g_l_new.y = (oldyinitx + g_long_param->y + oldxinity);
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
    foldxinitx = old.x * g_float_param->x;
    foldyinity = old.y * g_float_param->y;
    foldxinity = old.x * g_float_param->y;
    foldyinitx = old.y * g_float_param->x;
    // orbit calculation
    if (old.x >= 0)
    {
        g_new.x = (foldxinitx - g_float_param->x - foldyinity);
        g_new.y = (foldyinitx - g_float_param->y + foldxinity);
    }
    else
    {
        g_new.x = (foldxinitx + g_float_param->x - foldyinity);
        g_new.y = (foldyinitx + g_float_param->y + foldxinity);
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
    oldxinitx   = multiply(g_l_old.x, g_long_param->x, bitshift);
    oldyinity   = multiply(g_l_old.y, g_long_param->y, bitshift);
    oldxinity   = multiply(g_l_old.x, g_long_param->y, bitshift);
    oldyinitx   = multiply(g_l_old.y, g_long_param->x, bitshift);

    // orbit calculation
    if (oldxinity + oldyinitx >= 0)
    {
        g_l_new.x = oldxinitx - g_long_param->x - oldyinity;
        g_l_new.y = oldyinitx - g_long_param->y + oldxinity;
    }
    else
    {
        g_l_new.x = oldxinitx + g_long_param->x - oldyinity;
        g_l_new.y = oldyinitx + g_long_param->y + oldxinity;
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
    foldxinitx = old.x * g_float_param->x;
    foldyinity = old.y * g_float_param->y;
    foldxinity = old.x * g_float_param->y;
    foldyinitx = old.y * g_float_param->x;

    // orbit calculation
    if (foldxinity + foldyinitx >= 0)
    {
        g_new.x = foldxinitx - g_float_param->x - foldyinity;
        g_new.y = foldyinitx - g_float_param->y + foldxinity;
    }
    else
    {
        g_new.x = foldxinitx + g_float_param->x - foldyinity;
        g_new.y = foldyinitx + g_float_param->y + foldxinity;
    }
    return floatbailout();
}

int
JuliaFractal()
{
#if !defined(XFRACT)
    /* used for C prototype of fast integer math routines for classic
       Mandelbrot and Julia */
    g_l_new.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new.y = multiply(g_l_old.x, g_l_old.y, bitshiftless1) + g_long_param->y;
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
    g_new.x = tempsqrx - tempsqry + g_float_param->x;
    g_new.y = 2.0 * old.x * old.y + g_float_param->y;
    return floatbailout();
}

int
LambdaFPFractal()
{
    // variation of classical Mandelbrot/Julia
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step

    tempsqrx = old.x - tempsqrx + tempsqry;
    tempsqry = -(old.y * old.x);
    tempsqry += tempsqry + old.y;

    g_new.x = g_float_param->x * tempsqrx - g_float_param->y * tempsqry;
    g_new.y = g_float_param->x * tempsqry + g_float_param->y * tempsqrx;
    return floatbailout();
}

int
LambdaFractal()
{
#if !defined(XFRACT)
    // variation of classical Mandelbrot/Julia

    // in complex math) temp = Z * (1-Z)
    g_l_temp_sqr_x = g_l_old.x - g_l_temp_sqr_x + g_l_temp_sqr_y;
    g_l_temp_sqr_y = g_l_old.y
                - multiply(g_l_old.y, g_l_old.x, bitshiftless1);
    // (in complex math) Z = Lambda * Z
    g_l_new.x = multiply(g_long_param->x, g_l_temp_sqr_x, bitshift)
             - multiply(g_long_param->y, g_l_temp_sqr_y, bitshift);
    g_l_new.y = multiply(g_long_param->x, g_l_temp_sqr_y, bitshift)
             + multiply(g_long_param->y, g_l_temp_sqr_x, bitshift);
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
    g_l_new.x = (g_l_old.x << 1);              // new.x = 2 * old.x
    g_l_new.y = (g_l_old.y << 1);              // new.y = 2 * old.y
    if (g_l_old.y > g_l_temp.y)  // if old.y > .5
    {
        g_l_new.y = g_l_new.y - g_l_temp.x;    // new.y = 2 * old.y - 1
    }
    else if (g_l_old.x > g_l_temp.y)     // if old.x > .5
    {
        g_l_new.x = g_l_new.x - g_l_temp.x;    // new.x = 2 * old.x - 1
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

    g_new.x = old.x + old.x;
    g_new.y = old.y + old.y;
    if (old.y > .5)
    {
        g_new.y = g_new.y - 1;
    }
    else if (old.x > .5)
    {
        g_new.x = g_new.x - 1;
    }

    // end barnsley code
    return floatbailout();
}

int
LambdaexponentFractal()
{
    // found this in  "Science of Fractal Images"
    if (save_release > 2002)
    {
        // need braces since these are macros
        FLOATEXPBAILOUT();
    }
    else
    {
        OLD_FLOATEXPBAILOUT();
    }
    FPUsincos(&old.y, &siny, &cosy);

    if (old.x >= rqlim && cosy >= 0.0)
    {
        return 1;
    }
    tmpexp = exp(old.x);
    tmp.x = tmpexp*cosy;
    tmp.y = tmpexp*siny;

    //multiply by lamda
    g_new.x = g_float_param->x*tmp.x - g_float_param->y*tmp.y;
    g_new.y = g_float_param->y*tmp.x + g_float_param->x*tmp.y;
    old = g_new;
    return 0;
}

int
LongLambdaexponentFractal()
{
#if !defined(XFRACT)
    // found this in  "Science of Fractal Images"
    LONGEXPBAILOUT();

    SinCos086(g_l_old.y, &lsiny,  &lcosy);

    if (g_l_old.x >= g_l_limit && lcosy >= 0L)
    {
        return 1;
    }
    longtmp = Exp086(g_l_old.x);

    g_l_temp.x = multiply(longtmp,      lcosy,   bitshift);
    g_l_temp.y = multiply(longtmp,      lsiny,   bitshift);

    g_l_new.x  = multiply(g_long_param->x, g_l_temp.x, bitshift)
              - multiply(g_long_param->y, g_l_temp.y, bitshift);
    g_l_new.y  = multiply(g_long_param->x, g_l_temp.y, bitshift)
              + multiply(g_long_param->y, g_l_temp.x, bitshift);
    g_l_old = g_l_new;
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

    if (fabs(old.x) >= 6.4e2)
    {
        return 1; // DOMAIN errors
    }
    tmpexp = exp(old.x);
    FPUsincos(&old.y, &siny, &cosy);
    CMPLXtrig0(old, g_new);

    //new =   trig(old) + e**old + C
    g_new.x += tmpexp*cosy + g_float_param->x;
    g_new.y += tmpexp*siny + g_float_param->y;
    return floatbailout();
}

int
LongTrigPlusExponentFractal()
{
#if !defined(XFRACT)
    // calculate exp(z)

    // domain check for fast transcendental functions
    TRIG16CHECK(g_l_old.x);
    TRIG16CHECK(g_l_old.y);

    longtmp = Exp086(g_l_old.x);
    SinCos086(g_l_old.y, &lsiny,  &lcosy);
    LCMPLXtrig0(g_l_old, g_l_new);
    g_l_new.x += multiply(longtmp,    lcosy,   bitshift) + g_long_param->x;
    g_l_new.y += multiply(longtmp,    lsiny,   bitshift) + g_long_param->y;
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
    g_l_temp.y = multiply(g_l_old.x , g_l_old.y , bitshiftless1);

    g_l_new.x = multiply(g_l_coefficient.x, g_l_temp.x, bitshift)
             - multiply(g_l_coefficient.y, g_l_temp.y, bitshift) + g_long_param->x;
    g_l_new.y = multiply(g_l_coefficient.x, g_l_temp.y, bitshift)
             + multiply(g_l_coefficient.y, g_l_temp.x, bitshift) + g_long_param->y;

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
    tmp.x = tempsqrx - tempsqry;
    tmp.y = old.x * old.y *2;

    g_new.x = g_marks_coefficient.x * tmp.x - g_marks_coefficient.y * tmp.y + g_float_param->x;
    g_new.y = g_marks_coefficient.x * tmp.y + g_marks_coefficient.y * tmp.x + g_float_param->y;

    return floatbailout();
}


long XXOne, g_fudge_one, g_fudge_two;

int
UnityFractal()
{
#if !defined(XFRACT)
    XXOne = multiply(g_l_old.x, g_l_old.x, bitshift) + multiply(g_l_old.y, g_l_old.y, bitshift);
    if ((XXOne > g_fudge_two) || (labs(XXOne - g_fudge_one) < delmin))
    {
        return 1;
    }
    g_l_old.y = multiply(g_fudge_two - XXOne, g_l_old.x, bitshift);
    g_l_old.x = multiply(g_fudge_two - XXOne, g_l_old.y, bitshift);
    g_l_new = g_l_old;
    return 0;
#else
    return 0;
#endif
}

int
UnityfpFractal()
{
    double XXOne;
    XXOne = sqr(old.x) + sqr(old.y);
    if ((XXOne > 2.0) || (fabs(XXOne - 1.0) < ddelmin))
    {
        return 1;
    }
    old.y = (2.0 - XXOne)* old.x;
    old.x = (2.0 - XXOne)* old.y;
    g_new = old;
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
    g_l_new.x  = g_l_temp_sqr_x - g_l_temp_sqr_y;
    g_l_new.y = multiply(g_l_old.x, g_l_old.y, bitshiftless1);
    if (longbailout())
    {
        return 1;
    }

    // then, compute ((x + iy)**2)**2 + lambda
    g_l_new.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new.y = multiply(g_l_old.x, g_l_old.y, bitshiftless1) + g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

int
Mandel4fpFractal()
{
    // first, compute (x + iy)**2
    g_new.x  = tempsqrx - tempsqry;
    g_new.y = old.x*old.y*2;
    if (floatbailout())
    {
        return 1;
    }

    // then, compute ((x + iy)**2)**2 + lambda
    g_new.x  = tempsqrx - tempsqry + g_float_param->x;
    g_new.y =  old.x*old.y*2 + g_float_param->y;
    return floatbailout();
}

int
floatZtozPluszpwrFractal()
{
    cpower(&old, (int)param[2], &g_new);
    old = ComplexPower(old, old);
    g_new.x = g_new.x + old.x +g_float_param->x;
    g_new.y = g_new.y + old.y +g_float_param->y;
    return floatbailout();
}

int
longZpowerFractal()
{
#if !defined(XFRACT)
    if (lcpower(&g_l_old, g_c_exponent, &g_l_new, bitshift))
    {
        g_l_new.y = 8L << bitshift;
        g_l_new.x = g_l_new.y;
    }
    g_l_new.x += g_long_param->x;
    g_l_new.y += g_long_param->y;
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

    x.x = (double)g_l_old.x / g_fudge_factor;
    x.y = (double)g_l_old.y / g_fudge_factor;
    y.x = (double)g_l_param2.x / g_fudge_factor;
    y.y = (double)g_l_param2.y / g_fudge_factor;
    x = ComplexPower(x, y);
    if (fabs(x.x) < g_fudge_limit && fabs(x.y) < g_fudge_limit)
    {
        g_l_new.x = (long)(x.x * g_fudge_factor);
        g_l_new.y = (long)(x.y * g_fudge_factor);
    }
    else
    {
        overflow = true;
    }
    g_l_new.x += g_long_param->x;
    g_l_new.y += g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

int
floatZpowerFractal()
{
    cpower(&old, g_c_exponent, &g_new);
    g_new.x += g_float_param->x;
    g_new.y += g_float_param->y;
    return floatbailout();
}

int
floatCmplxZpowerFractal()
{
    g_new = ComplexPower(old, parm2);
    g_new.x += g_float_param->x;
    g_new.y += g_float_param->y;
    return floatbailout();
}

int
Barnsley3Fractal()
{
    /* An unnamed Mandelbrot/Julia function from "Fractals
    Everywhere" by Michael Barnsley, p. 292, example 4.1 */

    // calculate intermediate products
#if !defined(XFRACT)
    oldxinitx   = multiply(g_l_old.x, g_l_old.x, bitshift);
    oldyinity   = multiply(g_l_old.y, g_l_old.y, bitshift);
    oldxinity   = multiply(g_l_old.x, g_l_old.y, bitshift);

    // orbit calculation
    if (g_l_old.x > 0)
    {
        g_l_new.x = oldxinitx   - oldyinity - g_fudge_factor;
        g_l_new.y = oldxinity << 1;
    }
    else
    {
        g_l_new.x = oldxinitx - oldyinity - g_fudge_factor
                 + multiply(g_long_param->x, g_l_old.x, bitshift);
        g_l_new.y = oldxinity <<1;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_l_new.y += multiply(g_long_param->y, g_l_old.x, bitshift);
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
    foldxinitx  = old.x * old.x;
    foldyinity  = old.y * old.y;
    foldxinity  = old.x * old.y;

    // orbit calculation
    if (old.x > 0)
    {
        g_new.x = foldxinitx - foldyinity - 1.0;
        g_new.y = foldxinity * 2;
    }
    else
    {
        g_new.x = foldxinitx - foldyinity -1.0 + g_float_param->x * old.x;
        g_new.y = foldxinity * 2;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_new.y += g_float_param->y * old.x;
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
    LCMPLXtrig0(g_l_old, g_l_new);
    g_l_new.x += g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new.y += multiply(g_l_old.x, g_l_old.y, bitshiftless1) + g_long_param->y;
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

    CMPLXtrig0(old, g_new);
    g_new.x += tempsqrx - tempsqry + g_float_param->x;
    g_new.y += 2.0 * old.x * old.y + g_float_param->y;
    return floatbailout();
}

int
Richard8fpFractal()
{
    //  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50}
    CMPLXtrig0(old, g_new);
    g_new.x += tmp.x;
    g_new.y += tmp.y;
    return floatbailout();
}

int
Richard8Fractal()
{
#if !defined(XFRACT)
    //  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50}
    LCMPLXtrig0(g_l_old, g_l_new);
    g_l_new.x += g_l_temp.x;
    g_l_new.y += g_l_temp.y;
    return longbailout();
#else
    return 0;
#endif
}

int
PopcornFractal_Old()
{
    tmp = old;
    tmp.x *= 3.0;
    tmp.y *= 3.0;
    FPUsincos(&tmp.x, &sinx, &cosx);
    FPUsincos(&tmp.y, &siny, &cosy);
    tmp.x = sinx/cosx + old.x;
    tmp.y = siny/cosy + old.y;
    FPUsincos(&tmp.x, &sinx, &cosx);
    FPUsincos(&tmp.y, &siny, &cosy);
    g_new.x = old.x - parm.x*siny;
    g_new.y = old.y - parm.x*sinx;
    if (plot == noplot)
    {
        plot_orbit(g_new.x, g_new.y, 1+row%g_colors);
        old = g_new;
    }
    else
    {
        tempsqrx = sqr(g_new.x);
    }
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    if (magnitude >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

int
PopcornFractal()
{
    tmp = old;
    tmp.x *= 3.0;
    tmp.y *= 3.0;
    FPUsincos(&tmp.x, &sinx, &cosx);
    FPUsincos(&tmp.y, &siny, &cosy);
    tmp.x = sinx/cosx + old.x;
    tmp.y = siny/cosy + old.y;
    FPUsincos(&tmp.x, &sinx, &cosx);
    FPUsincos(&tmp.y, &siny, &cosy);
    g_new.x = old.x - parm.x*siny;
    g_new.y = old.y - parm.x*sinx;
    if (plot == noplot)
    {
        plot_orbit(g_new.x, g_new.y, 1+row%g_colors);
        old = g_new;
    }
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    if (magnitude >= rqlim
            || fabs(g_new.x) > rqlim2 || fabs(g_new.y) > rqlim2)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

int
LPopcornFractal_Old()
{
#if !defined(XFRACT)
    g_l_temp = g_l_old;
    g_l_temp.x *= 3L;
    g_l_temp.y *= 3L;
    LTRIGARG(g_l_temp.x);
    LTRIGARG(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_temp.x = divide(lsinx, lcosx, bitshift) + g_l_old.x;
    g_l_temp.y = divide(lsiny, lcosy, bitshift) + g_l_old.y;
    LTRIGARG(g_l_temp.x);
    LTRIGARG(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_new.x = g_l_old.x - multiply(g_l_param.x, lsiny, bitshift);
    g_l_new.y = g_l_old.y - multiply(g_l_param.x, lsinx, bitshift);
    if (plot == noplot)
    {
        iplot_orbit(g_l_new.x, g_l_new.y, 1+row%g_colors);
        g_l_old = g_l_new;
    }
    else
    {
        g_l_temp_sqr_x = lsqr(g_l_new.x);
        g_l_temp_sqr_y = lsqr(g_l_new.y);
    }
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_limit || g_l_magnitude < 0 || labs(g_l_new.x) > g_l_limit2
            || labs(g_l_new.y) > g_l_limit2)
    {
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
#else
    return 0;
#endif
}

int
LPopcornFractal()
{
#if !defined(XFRACT)
    g_l_temp = g_l_old;
    g_l_temp.x *= 3L;
    g_l_temp.y *= 3L;
    LTRIGARG(g_l_temp.x);
    LTRIGARG(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_temp.x = divide(lsinx, lcosx, bitshift) + g_l_old.x;
    g_l_temp.y = divide(lsiny, lcosy, bitshift) + g_l_old.y;
    LTRIGARG(g_l_temp.x);
    LTRIGARG(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_new.x = g_l_old.x - multiply(g_l_param.x, lsiny, bitshift);
    g_l_new.y = g_l_old.y - multiply(g_l_param.x, lsinx, bitshift);
    if (plot == noplot)
    {
        iplot_orbit(g_l_new.x, g_l_new.y, 1+row%g_colors);
        g_l_old = g_l_new;
    }
    // else
    g_l_temp_sqr_x = lsqr(g_l_new.x);
    g_l_temp_sqr_y = lsqr(g_l_new.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_limit || g_l_magnitude < 0
            || labs(g_l_new.x) > g_l_limit2
            || labs(g_l_new.y) > g_l_limit2)
    {
        return 1;
    }
    g_l_old = g_l_new;
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
    CMPLXtimesreal(parm2, old.y, tmp);  // tmp = (C * old.y)
    CMPLXtrig1(tmp, tmpx);             // tmpx = trig1(tmp)
    tmpx.x += old.y;                  // tmpx = old.y + trig1(tmp)
    CMPLXtrig0(tmpx, tmp);             // tmp = trig0(tmpx)
    CMPLXmult(tmp, parm, tmpx);         // tmpx = tmp * h

    // tmpy contains the generalized value of the old real "y" equation
    CMPLXtimesreal(parm2, old.x, tmp);  // tmp = (C * old.x)
    CMPLXtrig3(tmp, tmpy);             // tmpy = trig3(tmp)
    tmpy.x += old.x;                  // tmpy = old.x + trig1(tmp)
    CMPLXtrig2(tmpy, tmp);             // tmp = trig2(tmpy)

    CMPLXmult(tmp, parm, tmpy);         // tmpy = tmp * h

    g_new.x = old.x - tmpx.x - tmpy.y;
    g_new.y = old.y - tmpy.x - tmpx.y;

    if (plot == noplot)
    {
        plot_orbit(g_new.x, g_new.y, 1+row%g_colors);
        old = g_new;
    }

    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    if (magnitude >= rqlim
            || fabs(g_new.x) > rqlim2 || fabs(g_new.y) > rqlim2)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

#define FIX_OVERFLOW(arg)           \
    if (overflow)                   \
    {                               \
        (arg).x = g_fudge_factor;   \
        (arg).y = 0;                \
        overflow = false;           \
   }

int
LPopcornFractalFn()
{
#if !defined(XFRACT)
    LComplex ltmpx, ltmpy;

    overflow = false;

    // ltmpx contains the generalized value of the old real "x" equation
    LCMPLXtimesreal(g_l_param2, g_l_old.y, g_l_temp); // tmp = (C * old.y)
    LCMPLXtrig1(g_l_temp, ltmpx);             // tmpx = trig1(tmp)
    FIX_OVERFLOW(ltmpx);
    ltmpx.x += g_l_old.y;                   // tmpx = old.y + trig1(tmp)
    LCMPLXtrig0(ltmpx, g_l_temp);             // tmp = trig0(tmpx)
    FIX_OVERFLOW(g_l_temp);
    LCMPLXmult(g_l_temp, g_l_param, ltmpx);        // tmpx = tmp * h

    // ltmpy contains the generalized value of the old real "y" equation
    LCMPLXtimesreal(g_l_param2, g_l_old.x, g_l_temp); // tmp = (C * old.x)
    LCMPLXtrig3(g_l_temp, ltmpy);             // tmpy = trig3(tmp)
    FIX_OVERFLOW(ltmpy);
    ltmpy.x += g_l_old.x;                   // tmpy = old.x + trig1(tmp)
    LCMPLXtrig2(ltmpy, g_l_temp);             // tmp = trig2(tmpy)
    FIX_OVERFLOW(g_l_temp);
    LCMPLXmult(g_l_temp, g_l_param, ltmpy);        // tmpy = tmp * h

    g_l_new.x = g_l_old.x - ltmpx.x - ltmpy.y;
    g_l_new.y = g_l_old.y - ltmpy.x - ltmpx.y;

    if (plot == noplot)
    {
        iplot_orbit(g_l_new.x, g_l_new.y, 1+row%g_colors);
        g_l_old = g_l_new;
    }
    g_l_temp_sqr_x = lsqr(g_l_new.x);
    g_l_temp_sqr_y = lsqr(g_l_new.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_limit || g_l_magnitude < 0
            || labs(g_l_new.x) > g_l_limit2
            || labs(g_l_new.y) > g_l_limit2)
    {
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
#else
    return 0;
#endif
}

int MarksCplxMand()
{
    tmp.x = tempsqrx - tempsqry;
    tmp.y = 2*old.x*old.y;
    FPUcplxmul(&tmp, &g_marks_coefficient, &g_new);
    g_new.x += g_float_param->x;
    g_new.y += g_float_param->y;
    return floatbailout();
}

int SpiderfpFractal()
{
    // Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 }
    g_new.x = tempsqrx - tempsqry + tmp.x;
    g_new.y = 2 * old.x * old.y + tmp.y;
    tmp.x = tmp.x/2 + g_new.x;
    tmp.y = tmp.y/2 + g_new.y;
    return floatbailout();
}

int
SpiderFractal()
{
#if !defined(XFRACT)
    // Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 }
    g_l_new.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_l_temp.x;
    g_l_new.y = multiply(g_l_old.x, g_l_old.y, bitshiftless1) + g_l_temp.y;
    g_l_temp.x = (g_l_temp.x >> 1) + g_l_new.x;
    g_l_temp.y = (g_l_temp.y >> 1) + g_l_new.y;
    return longbailout();
#else
    return 0;
#endif
}

int
TetratefpFractal()
{
    // Tetrate(XAXIS) { c=z=pixel: z=c^z, |z|<=(P1+3) }
    g_new = ComplexPower(*g_float_param, old);
    return floatbailout();
}

int
ZXTrigPlusZFractal()
{
#if !defined(XFRACT)
    // z = (p1*z*trig(z))+p2*z
    LCMPLXtrig0(g_l_old, g_l_temp);          // ltmp  = trig(old)
    LCMPLXmult(g_l_param, g_l_temp, g_l_temp);      // ltmp  = p1*trig(old)
    LCMPLXmult(g_l_old, g_l_temp, ltmp2);      // ltmp2 = p1*old*trig(old)
    LCMPLXmult(g_l_param2, g_l_old, g_l_temp);     // ltmp  = p2*old
    LCMPLXadd(ltmp2, g_l_temp, g_l_new);       // lnew  = p1*trig(old) + p2*old
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
    LCMPLXtrig0(g_l_old, g_l_temp);          // ltmp  = trig(old)
    LCMPLXmult(g_l_old, g_l_temp, g_l_new);       // lnew  = old*trig(old)
    LCMPLXadd(g_l_new, g_l_old, g_l_new);        // lnew  = trig(old) + old
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
    LCMPLXtrig0(g_l_old, g_l_temp);          // ltmp  = trig(old)
    LCMPLXmult(g_l_old, g_l_temp, g_l_new);       // lnew  = old*trig(old)
    LCMPLXsub(g_l_new, g_l_old, g_l_new);        // lnew  = trig(old) - old
    return longbailout();
#else
    return 0;
#endif
}

int
ZXTrigPlusZfpFractal()
{
    // z = (p1*z*trig(z))+p2*z
    CMPLXtrig0(old, tmp);          // tmp  = trig(old)
    CMPLXmult(parm, tmp, tmp);      // tmp  = p1*trig(old)
    CMPLXmult(old, tmp, tmp2);      // tmp2 = p1*old*trig(old)
    CMPLXmult(parm2, old, tmp);     // tmp  = p2*old
    CMPLXadd(tmp2, tmp, g_new);       // new  = p1*trig(old) + p2*old
    return floatbailout();
}

int
ScottZXTrigPlusZfpFractal()
{
    // z = (z*trig(z))+z
    CMPLXtrig0(old, tmp);         // tmp  = trig(old)
    CMPLXmult(old, tmp, g_new);       // new  = old*trig(old)
    CMPLXadd(g_new, old, g_new);        // new  = trig(old) + old
    return floatbailout();
}

int
SkinnerZXTrigSubZfpFractal()
{
    // z = (z*trig(z))-z
    CMPLXtrig0(old, tmp);         // tmp  = trig(old)
    CMPLXmult(old, tmp, g_new);       // new  = old*trig(old)
    CMPLXsub(g_new, old, g_new);        // new  = trig(old) - old
    return floatbailout();
}

int
Sqr1overTrigFractal()
{
#if !defined(XFRACT)
    // z = sqr(1/trig(z))
    LCMPLXtrig0(g_l_old, g_l_old);
    LCMPLXrecip(g_l_old, g_l_old);
    LCMPLXsqr(g_l_old, g_l_new);
    return longbailout();
#else
    return 0;
#endif
}

int
Sqr1overTrigfpFractal()
{
    // z = sqr(1/trig(z))
    CMPLXtrig0(old, old);
    CMPLXrecip(old, old);
    CMPLXsqr(old, g_new);
    return floatbailout();
}

int
TrigPlusTrigFractal()
{
#if !defined(XFRACT)
    // z = trig(0,z)*p1+trig1(z)*p2
    LCMPLXtrig0(g_l_old, g_l_temp);
    LCMPLXmult(g_l_param, g_l_temp, g_l_temp);
    LCMPLXtrig1(g_l_old, ltmp2);
    LCMPLXmult(g_l_param2, ltmp2, g_l_old);
    LCMPLXadd(g_l_temp, g_l_old, g_l_new);
    return longbailout();
#else
    return 0;
#endif
}

int
TrigPlusTrigfpFractal()
{
    // z = trig0(z)*p1+trig1(z)*p2
    CMPLXtrig0(old, tmp);
    CMPLXmult(parm, tmp, tmp);
    CMPLXtrig1(old, old);
    CMPLXmult(parm2, old, old);
    CMPLXadd(tmp, old, g_new);
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
    if ((LCMPLXmod(g_l_old)) < g_l_param2.x)
    {
        LCMPLXtrig0(g_l_old, g_l_temp);
        LCMPLXmult(*g_long_param, g_l_temp, g_l_new);
    }
    else
    {
        LCMPLXtrig1(g_l_old, g_l_temp);
        LCMPLXmult(*g_long_param, g_l_temp, g_l_new);
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
    if (CMPLXmod(old) < parm2.x)
    {
        CMPLXtrig0(old, old);
        FPUcplxmul(g_float_param, &old, &g_new);
    }
    else
    {
        CMPLXtrig1(old, old);
        FPUcplxmul(g_float_param, &old, &g_new);
    }
    return floatbailout();
}

int
JuliaTrigOrTrigFractal()
{
#if !defined(XFRACT)
    /* z = trig0(z)+p1 if mod(old) < p2.x and
           trig1(z)+p1 if mod(old) >= p2.x */
    if (LCMPLXmod(g_l_old) < g_l_param2.x)
    {
        LCMPLXtrig0(g_l_old, g_l_temp);
        LCMPLXadd(*g_long_param, g_l_temp, g_l_new);
    }
    else
    {
        LCMPLXtrig1(g_l_old, g_l_temp);
        LCMPLXadd(*g_long_param, g_l_temp, g_l_new);
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
    if (CMPLXmod(old) < parm2.x)
    {
        CMPLXtrig0(old, old);
        CMPLXadd(*g_float_param, old, g_new);
    }
    else
    {
        CMPLXtrig1(old, old);
        CMPLXadd(*g_float_param, old, g_new);
    }
    return floatbailout();
}

int g_halley_a_plus_one, g_halley_a_plus_one_times_degree;
MP mpAplusOne, mpAp1deg;
MPC mpctmpparm;

int MPCHalleyFractal()
{
#if !defined(XFRACT)
    //  X(X^a - 1) = 0, Halley Map
    //  a = parm.x,  relaxation coeff. = parm.y,  epsilon = parm2.x

    MPC mpcXtoAlessOne, mpcXtoA;
    MPC mpcXtoAplusOne; // a-1, a, a+1
    MPC mpcFX, mpcF1prime, mpcF2prime, mpcHalnumer1;
    MPC mpcHalnumer2, mpcHaldenom, mpctmp;

    MPOverflow = 0;
    mpcXtoAlessOne.x = mpcold.x;
    mpcXtoAlessOne.y = mpcold.y;
    for (int ihal = 2; ihal < degree; ihal++)
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

    mpcF2prime.x = *pMPmul(mpAp1deg, mpcXtoAlessOne.x); // mpAp1deg in setup
    mpcF2prime.y = *pMPmul(mpAp1deg, mpcXtoAlessOne.y);        // F"

    mpcF1prime.x = *pMPsub(*pMPmul(mpAplusOne, mpcXtoA.x), mpone);
    mpcF1prime.y = *pMPmul(mpAplusOne, mpcXtoA.y);                   //  F'

    mpctmp.x = *pMPsub(*pMPmul(mpcF2prime.x, mpcFX.x), *pMPmul(mpcF2prime.y, mpcFX.y));
    mpctmp.y = *pMPadd(*pMPmul(mpcF2prime.x, mpcFX.y), *pMPmul(mpcF2prime.y, mpcFX.x));
    //  F * F"

    mpcHaldenom.x = *pMPadd(mpcF1prime.x, mpcF1prime.x);
    mpcHaldenom.y = *pMPadd(mpcF1prime.y, mpcF1prime.y);      //  2 * F'

    mpcHalnumer1 = MPCdiv(mpctmp, mpcHaldenom);        //  F"F/2F'
    mpctmp.x = *pMPsub(mpcF1prime.x, mpcHalnumer1.x);
    mpctmp.y = *pMPsub(mpcF1prime.y, mpcHalnumer1.y); //  F' - F"F/2F'
    mpcHalnumer2 = MPCdiv(mpcFX, mpctmp);

    mpctmp   =  MPCmul(mpctmpparm, mpcHalnumer2);  // mpctmpparm is
    // relaxation coef.
    mpcnew = MPCsub(mpcold, mpctmp);
    g_new    = MPC2cmplx(mpcnew);
    return MPCHalleybailout()||MPOverflow;
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

    XtoAlessOne = old;
    for (int ihal = 2; ihal < degree; ihal++)
    {
        FPUcplxmul(&old, &XtoAlessOne, &XtoAlessOne);
    }
    FPUcplxmul(&old, &XtoAlessOne, &XtoA);
    FPUcplxmul(&old, &XtoA, &XtoAplusOne);

    CMPLXsub(XtoAplusOne, old, FX);        // FX = X^(a+1) - X  = F
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
    relax.x = parm.y;
    relax.y = param[3];
    FPUcplxmul(&relax, &Halnumer2, &Halnumer2);
    g_new.x = old.x - Halnumer2.x;
    g_new.y = old.y - Halnumer2.y;
    return Halleybailout();
}

int
LongPhoenixFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    g_l_temp.x = multiply(g_l_old.x, g_l_old.y, bitshift);
    g_l_new.x = g_l_temp_sqr_x-g_l_temp_sqr_y+g_long_param->x+multiply(g_long_param->y, ltmp2.x, bitshift);
    g_l_new.y = (g_l_temp.x + g_l_temp.x) + multiply(g_long_param->y, ltmp2.y, bitshift);
    ltmp2 = g_l_old; // set ltmp2 to Y value
    return longbailout();
#else
    return 0;
#endif
}

int
PhoenixFractal()
{
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    tmp.x = old.x * old.y;
    g_new.x = tempsqrx - tempsqry + g_float_param->x + (g_float_param->y * tmp2.x);
    g_new.y = (tmp.x + tmp.x) + (g_float_param->y * tmp2.y);
    tmp2 = old; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixFractalcplx()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    g_l_temp.x = multiply(g_l_old.x, g_l_old.y, bitshift);
    g_l_new.x = g_l_temp_sqr_x-g_l_temp_sqr_y+g_long_param->x+multiply(g_l_param2.x, ltmp2.x, bitshift)-multiply(g_l_param2.y, ltmp2.y, bitshift);
    g_l_new.y = (g_l_temp.x + g_l_temp.x)+g_long_param->y+multiply(g_l_param2.x, ltmp2.y, bitshift)+multiply(g_l_param2.y, ltmp2.x, bitshift);
    ltmp2 = g_l_old; // set ltmp2 to Y value
    return longbailout();
#else
    return 0;
#endif
}

int
PhoenixFractalcplx()
{
    // z(n+1) = z(n)^2 + p1 + p2*y(n),  y(n+1) = z(n)
    tmp.x = old.x * old.y;
    g_new.x = tempsqrx - tempsqry + g_float_param->x + (parm2.x * tmp2.x) - (parm2.y * tmp2.y);
    g_new.y = (tmp.x + tmp.x) + g_float_param->y + (parm2.x * tmp2.y) + (parm2.y * tmp2.x);
    tmp2 = old; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixPlusFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    LComplex loldplus, lnewminus;
    loldplus = g_l_old;
    g_l_temp = g_l_old;
    for (int i = 1; i < degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        LCMPLXmult(g_l_old, g_l_temp, g_l_temp); // = old^(degree-1)
    }
    loldplus.x += g_long_param->x;
    LCMPLXmult(g_l_temp, loldplus, lnewminus);
    g_l_new.x = lnewminus.x + multiply(g_long_param->y, ltmp2.x, bitshift);
    g_l_new.y = lnewminus.y + multiply(g_long_param->y, ltmp2.y, bitshift);
    ltmp2 = g_l_old; // set ltmp2 to Y value
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
    oldplus = old;
    tmp = old;
    for (int i = 1; i < degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        FPUcplxmul(&old, &tmp, &tmp); // = old^(degree-1)
    }
    oldplus.x += g_float_param->x;
    FPUcplxmul(&tmp, &oldplus, &newminus);
    g_new.x = newminus.x + (g_float_param->y * tmp2.x);
    g_new.y = newminus.y + (g_float_param->y * tmp2.y);
    tmp2 = old; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixMinusFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    LComplex loldsqr, lnewminus;
    LCMPLXmult(g_l_old, g_l_old, loldsqr);
    g_l_temp = g_l_old;
    for (int i = 1; i < degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        LCMPLXmult(g_l_old, g_l_temp, g_l_temp); // = old^(degree-2)
    }
    loldsqr.x += g_long_param->x;
    LCMPLXmult(g_l_temp, loldsqr, lnewminus);
    g_l_new.x = lnewminus.x + multiply(g_long_param->y, ltmp2.x, bitshift);
    g_l_new.y = lnewminus.y + multiply(g_long_param->y, ltmp2.y, bitshift);
    ltmp2 = g_l_old; // set ltmp2 to Y value
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
    FPUcplxmul(&old, &old, &oldsqr);
    tmp = old;
    for (int i = 1; i < degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        FPUcplxmul(&old, &tmp, &tmp); // = old^(degree-2)
    }
    oldsqr.x += g_float_param->x;
    FPUcplxmul(&tmp, &oldsqr, &newminus);
    g_new.x = newminus.x + (g_float_param->y * tmp2.x);
    g_new.y = newminus.y + (g_float_param->y * tmp2.y);
    tmp2 = old; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixCplxPlusFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    LComplex loldplus, lnewminus;
    loldplus = g_l_old;
    g_l_temp = g_l_old;
    for (int i = 1; i < degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        LCMPLXmult(g_l_old, g_l_temp, g_l_temp); // = old^(degree-1)
    }
    loldplus.x += g_long_param->x;
    loldplus.y += g_long_param->y;
    LCMPLXmult(g_l_temp, loldplus, lnewminus);
    LCMPLXmult(g_l_param2, ltmp2, g_l_temp);
    g_l_new.x = lnewminus.x + g_l_temp.x;
    g_l_new.y = lnewminus.y + g_l_temp.y;
    ltmp2 = g_l_old; // set ltmp2 to Y value
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
    oldplus = old;
    tmp = old;
    for (int i = 1; i < degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        FPUcplxmul(&old, &tmp, &tmp); // = old^(degree-1)
    }
    oldplus.x += g_float_param->x;
    oldplus.y += g_float_param->y;
    FPUcplxmul(&tmp, &oldplus, &newminus);
    FPUcplxmul(&parm2, &tmp2, &tmp);
    g_new.x = newminus.x + tmp.x;
    g_new.y = newminus.y + tmp.y;
    tmp2 = old; // set tmp2 to Y value
    return floatbailout();
}

int
LongPhoenixCplxMinusFractal()
{
#if !defined(XFRACT)
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    LComplex loldsqr, lnewminus;
    LCMPLXmult(g_l_old, g_l_old, loldsqr);
    g_l_temp = g_l_old;
    for (int i = 1; i < degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        LCMPLXmult(g_l_old, g_l_temp, g_l_temp); // = old^(degree-2)
    }
    loldsqr.x += g_long_param->x;
    loldsqr.y += g_long_param->y;
    LCMPLXmult(g_l_temp, loldsqr, lnewminus);
    LCMPLXmult(g_l_param2, ltmp2, g_l_temp);
    g_l_new.x = lnewminus.x + g_l_temp.x;
    g_l_new.y = lnewminus.y + g_l_temp.y;
    ltmp2 = g_l_old; // set ltmp2 to Y value
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
    FPUcplxmul(&old, &old, &oldsqr);
    tmp = old;
    for (int i = 1; i < degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        FPUcplxmul(&old, &tmp, &tmp); // = old^(degree-2)
    }
    oldsqr.x += g_float_param->x;
    oldsqr.y += g_float_param->y;
    FPUcplxmul(&tmp, &oldsqr, &newminus);
    FPUcplxmul(&parm2, &tmp2, &tmp);
    g_new.x = newminus.x + tmp.x;
    g_new.y = newminus.y + tmp.y;
    tmp2 = old; // set tmp2 to Y value
    return floatbailout();
}

int
ScottTrigPlusTrigFractal()
{
#if !defined(XFRACT)
    // z = trig0(z)+trig1(z)
    LCMPLXtrig0(g_l_old, g_l_temp);
    LCMPLXtrig1(g_l_old, g_l_old);
    LCMPLXadd(g_l_temp, g_l_old, g_l_new);
    return longbailout();
#else
    return 0;
#endif
}

int
ScottTrigPlusTrigfpFractal()
{
    // z = trig0(z)+trig1(z)
    CMPLXtrig0(old, tmp);
    CMPLXtrig1(old, tmp2);
    CMPLXadd(tmp, tmp2, g_new);
    return floatbailout();
}

int
SkinnerTrigSubTrigFractal()
{
#if !defined(XFRACT)
    // z = trig(0, z)-trig1(z)
    LCMPLXtrig0(g_l_old, g_l_temp);
    LCMPLXtrig1(g_l_old, ltmp2);
    LCMPLXsub(g_l_temp, ltmp2, g_l_new);
    return longbailout();
#else
    return 0;
#endif
}

int
SkinnerTrigSubTrigfpFractal()
{
    // z = trig0(z)-trig1(z)
    CMPLXtrig0(old, tmp);
    CMPLXtrig1(old, tmp2);
    CMPLXsub(tmp, tmp2, g_new);
    return floatbailout();
}

int
TrigXTrigfpFractal()
{
    // z = trig0(z)*trig1(z)
    CMPLXtrig0(old, tmp);
    CMPLXtrig1(old, old);
    CMPLXmult(tmp, old, g_new);
    return floatbailout();
}

#if !defined(XFRACT)
// call float version of fractal if integer math overflow
static int TryFloatFractal(int (*fpFractal)())
{
    overflow = false;
    // lold had better not be changed!
    old.x = g_l_old.x;
    old.x /= g_fudge_factor;
    old.y = g_l_old.y;
    old.y /= g_fudge_factor;
    tempsqrx = sqr(old.x);
    tempsqry = sqr(old.y);
    fpFractal();
    if (save_release < 1900)
    {
        // for backwards compatibility
        g_l_new.x = (long)(g_new.x/g_fudge_factor); // this error has been here a long time
        g_l_new.y = (long)(g_new.y/g_fudge_factor);
    }
    else
    {
        g_l_new.x = (long)(g_new.x*g_fudge_factor);
        g_l_new.y = (long)(g_new.y*g_fudge_factor);
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
    LCMPLXtrig0(g_l_old, g_l_temp);
    LCMPLXtrig1(g_l_old, ltmp2);
    LCMPLXmult(g_l_temp, ltmp2, g_l_new);
    if (overflow)
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
    LCMPLXtrig0(g_l_old, g_l_temp);     // ltmp = trig(lold)
    LCMPLXmult(g_l_param, g_l_temp, g_l_new); // lnew = lparm*trig(lold)
    LCMPLXsqr_old(g_l_temp);         // ltmp = sqr(lold)
    LCMPLXmult(g_l_param2, g_l_temp, g_l_temp);// ltmp = lparm2*sqr(lold)
    LCMPLXadd(g_l_new, g_l_temp, g_l_new);   // lnew = lparm*trig(lold)+lparm2*sqr(lold)
    return longbailout();
#else
    return 0;
#endif
}

int
TrigPlusSqrfpFractal() // generalization of Scott and Skinner types
{
    // { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT }
    CMPLXtrig0(old, tmp);     // tmp = trig(old)
    CMPLXmult(parm, tmp, g_new); // new = parm*trig(old)
    CMPLXsqr_old(tmp);        // tmp = sqr(old)
    CMPLXmult(parm2, tmp, tmp2); // tmp = parm2*sqr(old)
    CMPLXadd(g_new, tmp2, g_new);    // new = parm*trig(old)+parm2*sqr(old)
    return floatbailout();
}

int
ScottTrigPlusSqrFractal()
{
#if !defined(XFRACT)
    //  { z=pixel: z=trig(z)+sqr(z), |z|<BAILOUT }
    LCMPLXtrig0(g_l_old, g_l_new);    // lnew = trig(lold)
    LCMPLXsqr_old(g_l_temp);        // lold = sqr(lold)
    LCMPLXadd(g_l_temp, g_l_new, g_l_new);  // lnew = trig(lold)+sqr(lold)
    return longbailout();
#else
    return 0;
#endif
}

int
ScottTrigPlusSqrfpFractal() // float version
{
    // { z=pixel: z=sin(z)+sqr(z), |z|<BAILOUT }
    CMPLXtrig0(old, g_new);       // new = trig(old)
    CMPLXsqr_old(tmp);          // tmp = sqr(old)
    CMPLXadd(g_new, tmp, g_new);      // new = trig(old)+sqr(old)
    return floatbailout();
}

int
SkinnerTrigSubSqrFractal()
{
#if !defined(XFRACT)
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    LCMPLXtrig0(g_l_old, g_l_new);    // lnew = trig(lold)
    LCMPLXsqr_old(g_l_temp);        // lold = sqr(lold)
    LCMPLXsub(g_l_new, g_l_temp, g_l_new);  // lnew = trig(lold)-sqr(lold)
    return longbailout();
#else
    return 0;
#endif
}

int
SkinnerTrigSubSqrfpFractal()
{
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    CMPLXtrig0(old, g_new);       // new = trig(old)
    CMPLXsqr_old(tmp);          // old = sqr(old)
    CMPLXsub(g_new, tmp, g_new);      // new = trig(old)-sqr(old)
    return floatbailout();
}

int
TrigZsqrdfpFractal()
{
    // { z=pixel: z=trig(z*z), |z|<TEST }
    CMPLXsqr_old(tmp);
    CMPLXtrig0(tmp, g_new);
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
            save_release > 1900)
    {
        overflow = true;
    }
    else
    {
        LCMPLXtrig0(g_l_temp, g_l_new);
    }
    if (overflow)
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
    LCMPLXtrig0(g_l_old, g_l_temp);
    LCMPLXsqr(g_l_temp, g_l_new);
    return longbailout();
#else
    return 0;
#endif
}

int
SqrTrigfpFractal()
{
    // SZSB(XYAXIS) { z=pixel, TEST=(p1+3): z=sin(z)*sin(z), |z|<TEST}
    CMPLXtrig0(old, tmp);
    CMPLXsqr(tmp, g_new);
    return floatbailout();
}

int
Magnet1Fractal()    //    Z = ((Z**2 + C - 1)/(2Z + C - 2))**2
{
    //  In "Beauty of Fractals", code by Kev Allen.
    DComplex top, bot, tmp;
    double div;

    top.x = tempsqrx - tempsqry + g_float_param->x - 1; // top = Z**2+C-1
    top.y = old.x * old.y;
    top.y = top.y + top.y + g_float_param->y;

    bot.x = old.x + old.x + g_float_param->x - 2;       // bot = 2*Z+C-2
    bot.y = old.y + old.y + g_float_param->y;

    div = bot.x*bot.x + bot.y*bot.y;                // tmp = top/bot
    if (div < FLT_MIN)
    {
        return 1;
    }
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    g_new.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      // Z = tmp**2
    g_new.y = tmp.x * tmp.y;
    g_new.y += g_new.y;

    return floatbailout();
}

int
Magnet2Fractal()  // Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2)  ) /
//       (3Z**2 + 3(C-2)Z + (C-1)(C-2)+1) )**2
{
    //   In "Beauty of Fractals", code by Kev Allen.
    DComplex top, bot, tmp;
    double div;

    top.x = old.x * (tempsqrx-tempsqry-tempsqry-tempsqry + T_Cm1.x)
            - old.y * T_Cm1.y + T_Cm1Cm2.x;
    top.y = old.y * (tempsqrx+tempsqrx+tempsqrx-tempsqry + T_Cm1.x)
            + old.x * T_Cm1.y + T_Cm1Cm2.y;

    bot.x = tempsqrx - tempsqry;
    bot.x = bot.x + bot.x + bot.x
            + old.x * T_Cm2.x - old.y * T_Cm2.y
            + T_Cm1Cm2.x + 1.0;
    bot.y = old.x * old.y;
    bot.y += bot.y;
    bot.y = bot.y + bot.y + bot.y
            + old.x * T_Cm2.y + old.y * T_Cm2.x
            + T_Cm1Cm2.y;

    div = bot.x*bot.x + bot.y*bot.y;                // tmp = top/bot
    if (div < FLT_MIN)
    {
        return 1;
    }
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    g_new.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      // Z = tmp**2
    g_new.y = tmp.x * tmp.y;
    g_new.y += g_new.y;

    return floatbailout();
}

int
LambdaTrigFractal()
{
#if !defined(XFRACT)
    LONGXYTRIGBAILOUT();
    LCMPLXtrig0(g_l_old, g_l_temp);           // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new);   // lnew = longparm*trig(lold)
    g_l_old = g_l_new;
    return 0;
#else
    return 0;
#endif
}

int
LambdaTrigfpFractal()
{
    FLOATXYTRIGBAILOUT();
    CMPLXtrig0(old, tmp);              // tmp = trig(old)
    CMPLXmult(*g_float_param, tmp, g_new);   // new = longparm*trig(old)
    old = g_new;
    return 0;
}

// bailouts are different for different trig functions
int
LambdaTrigFractal1()
{
#if !defined(XFRACT)
    LONGTRIGBAILOUT(); // sin,cos
    LCMPLXtrig0(g_l_old, g_l_temp);           // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new);   // lnew = longparm*trig(lold)
    g_l_old = g_l_new;
    return 0;
#else
    return 0;
#endif
}

int
LambdaTrigfpFractal1()
{
    FLOATTRIGBAILOUT(); // sin,cos
    CMPLXtrig0(old, tmp);              // tmp = trig(old)
    CMPLXmult(*g_float_param, tmp, g_new);   // new = longparm*trig(old)
    old = g_new;
    return 0;
}

int
LambdaTrigFractal2()
{
#if !defined(XFRACT)
    LONGHTRIGBAILOUT(); // sinh,cosh
    LCMPLXtrig0(g_l_old, g_l_temp);           // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new);   // lnew = longparm*trig(lold)
    g_l_old = g_l_new;
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
    CMPLXtrig0(old, tmp);              // tmp = trig(old)
    CMPLXmult(*g_float_param, tmp, g_new);   // new = longparm*trig(old)
    old = g_new;
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
    g_l_new.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_l_temp.x + g_long_param->x;
    g_l_new.y = multiply(g_l_old.x, g_l_old.y, bitshiftless1) + g_l_temp.y + g_long_param->y;
    g_l_temp = g_l_old;
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
    g_new.x = tempsqrx - tempsqry + tmp.x + g_float_param->x;
    g_new.y = 2.0 * old.x * old.y + tmp.y + g_float_param->y;
    tmp = old;
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
    CMPLXtrig0(old, g_new);
    CMPLXmult(tmp, g_new, g_new);
    g_new.x += g_float_param->x;
    g_new.y += g_float_param->y;
    return floatbailout();
}

int
MarksMandelPwrFractal()
{
#if !defined(XFRACT)
    LCMPLXtrig0(g_l_old, g_l_new);
    LCMPLXmult(g_l_temp, g_l_new, g_l_new);
    g_l_new.x += g_long_param->x;
    g_l_new.y += g_long_param->y;
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
    CMPLXtrig0(old, g_new);
    g_new.x = g_new.x * tmp.x - g_new.y * tmp.y;
    g_new.y = g_new.x * tmp.y - g_new.y * tmp.x;
    g_new.x += g_float_param->x;
    g_new.y += g_float_param->y;
    return floatbailout();
}

int
TimsErrorFractal()
{
#if !defined(XFRACT)
    LCMPLXtrig0(g_l_old, g_l_new);
    g_l_new.x = multiply(g_l_new.x, g_l_temp.x, bitshift)-multiply(g_l_new.y, g_l_temp.y, bitshift);
    g_l_new.y = multiply(g_l_new.x, g_l_temp.y, bitshift)-multiply(g_l_new.y, g_l_temp.x, bitshift);
    g_l_new.x += g_long_param->x;
    g_l_new.y += g_long_param->y;
    return longbailout();
#else
    return 0;
#endif
}

int
CirclefpFractal()
{
    long i;
    i = (long)(param[0]*(tempsqrx+tempsqry));
    g_color_iter = i%g_colors;
    return 1;
}
/*
CirclelongFractal()
{
   long i;
   i = multiply(lparm.x,(g_l_temp_sqr_x+g_l_temp_sqr_y),bitshift);
   i = i >> bitshift;
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
    z->x = dxpixel();
    z->y = dypixel();
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
        invertz2(&old);

        // watch out for overflow
        if (sqr(old.x)+sqr(old.y) >= 127)
        {
            old.x = 8;  // value to bail out in one iteration
            old.y = 8;
        }

        // convert to fudged longs
        g_l_old.x = (long)(old.x*g_fudge_factor);
        g_l_old.y = (long)(old.y*g_fudge_factor);
    }
    else
    {
        g_l_old.x = lxpixel();
        g_l_old.y = lypixel();
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
    g_l_init.x = lxpixel();
    if (save_release >= 2004)
    {
        g_l_init.y = lypixel();
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

    if (useinitorbit == 1)
    {
        g_l_old = g_l_init_orbit;
    }
    else
    {
        g_l_old = g_l_init;
    }

    g_l_old.x += g_l_param.x;    // initial pertubation of parameters set
    g_l_old.y += g_l_param.y;
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
        invertz2(&old);

        // watch out for overflow
        if (bitshift <= 24)
        {
            if (sqr(old.x)+sqr(old.y) >= 127)
            {
                old.x = 8;  // value to bail out in one iteration
                old.y = 8;
            }
        }
        if (bitshift >  24)
        {
            if (sqr(old.x)+sqr(old.y) >= 4.0)
            {
                old.x = 2;  // value to bail out in one iteration
                old.y = 2;
            }
        }

        // convert to fudged longs
        g_l_old.x = (long)(old.x*g_fudge_factor);
        g_l_old.y = (long)(old.y*g_fudge_factor);
    }
    else
    {
        g_l_old.x = lxpixel();
        g_l_old.y = lypixel();
    }

    g_l_temp_sqr_x = multiply(g_l_old.x, g_l_old.x, bitshift);
    g_l_temp_sqr_y = multiply(g_l_old.y, g_l_old.y, bitshift);
    g_l_temp = g_l_old;
    return 0;
}

int
marks_mandelpwr_per_pixel()
{
#if !defined(XFRACT)
    mandel_per_pixel();
    g_l_temp = g_l_old;
    g_l_temp.x -= g_fudge_factor;
    LCMPLXpwr(g_l_old, g_l_temp, g_l_temp);
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
        if (bitshift <= 24)
        {
            if (sqr(g_init.x)+sqr(g_init.y) >= 127)
            {
                g_init.x = 8;  // value to bail out in one iteration
                g_init.y = 8;
            }
        }
        if (bitshift >  24)
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
        g_l_init.x = lxpixel();
        if (save_release >= 2004)
        {
            g_l_init.y = lypixel();
        }
    }
    switch (fractype)
    {
    case fractal_type::MANDELLAMBDA:              // Critical Value 0.5 + 0.0i
        g_l_old.x = g_fudge_half;
        g_l_old.y = 0;
        break;
    default:
        g_l_old = g_l_init;
        break;
    }

    // alter init value
    if (useinitorbit == 1)
    {
        g_l_old = g_l_init_orbit;
    }
    else if (useinitorbit == 2)
    {
        g_l_old = g_l_init;
    }

    if ((g_inside == BOF60 || g_inside == BOF61) && !nobof)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        g_l_old.x = g_l_param.x; // initial pertubation of parameters set
        g_l_old.y = g_l_param.y;
        g_color_iter = -1;
    }
    else
    {
        g_l_old.x += g_l_param.x; // initial pertubation of parameters set
        g_l_old.y += g_l_param.y;
    }
    g_l_temp = g_l_init; // for spider
    g_l_temp_sqr_x = multiply(g_l_old.x, g_l_old.x, bitshift);
    g_l_temp_sqr_y = multiply(g_l_old.y, g_l_old.y, bitshift);
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
        g_l_init.x = lxpixel();
        if (save_release >= 2004)
        {
            g_l_init.y = lypixel();
        }
    }

    if (useinitorbit == 1)
    {
        g_l_old = g_l_init_orbit;
    }
    else
    {
        g_l_old = g_l_init;
    }

    g_l_old.x += g_l_param.x;    // initial pertubation of parameters set
    g_l_old.y += g_l_param.y;

    if (g_c_exponent > 3)
    {
        lcpower(&g_l_old, g_c_exponent-1, &g_l_coefficient, bitshift);
    }
    else if (g_c_exponent == 3)
    {
        g_l_coefficient.x = multiply(g_l_old.x, g_l_old.x, bitshift)
                         - multiply(g_l_old.y, g_l_old.y, bitshift);
        g_l_coefficient.y = multiply(g_l_old.x, g_l_old.y, bitshiftless1);
    }
    else if (g_c_exponent == 2)
    {
        g_l_coefficient = g_l_old;
    }
    else if (g_c_exponent < 2)
    {
        g_l_coefficient.x = 1L << bitshift;
        g_l_coefficient.y = 0L;
    }

    g_l_temp_sqr_x = multiply(g_l_old.x, g_l_old.x, bitshift);
    g_l_temp_sqr_y = multiply(g_l_old.y, g_l_old.y, bitshift);
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
        g_init.x = dxpixel();
        if (save_release >= 2004)
        {
            g_init.y = dypixel();
        }
    }

    if (useinitorbit == 1)
    {
        old = g_init_orbit;
    }
    else
    {
        old = g_init;
    }

    old.x += parm.x;      // initial pertubation of parameters set
    old.y += parm.y;

    tempsqrx = sqr(old.x);
    tempsqry = sqr(old.y);

    if (g_c_exponent > 3)
    {
        cpower(&old, g_c_exponent-1, &g_marks_coefficient);
    }
    else if (g_c_exponent == 3)
    {
        g_marks_coefficient.x = tempsqrx - tempsqry;
        g_marks_coefficient.y = old.x * old.y * 2;
    }
    else if (g_c_exponent == 2)
    {
        g_marks_coefficient = old;
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
    tmp = old;
    tmp.x -= 1;
    CMPLXpwr(old, tmp, tmp);
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
        g_init.x = dxpixel();
        if (save_release >= 2004)
        {
            g_init.y = dypixel();
        }
    }
    switch (fractype)
    {
    case fractal_type::MAGNET2M:
        FloatPreCalcMagnet2();
    case fractal_type::MAGNET1M:
        old.y = 0.0;        // Critical Val Zero both, but neither
        old.x = old.y;      // is of the form f(Z,C) = Z*g(Z)+C
        break;
    case fractal_type::MANDELLAMBDAFP:            // Critical Value 0.5 + 0.0i
        old.x = 0.5;
        old.y = 0.0;
        break;
    default:
        old = g_init;
        break;
    }

    // alter init value
    if (useinitorbit == 1)
    {
        old = g_init_orbit;
    }
    else if (useinitorbit == 2)
    {
        old = g_init;
    }

    if ((g_inside == BOF60 || g_inside == BOF61) && !nobof)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        old.x = parm.x; // initial pertubation of parameters set
        old.y = parm.y;
        g_color_iter = -1;
    }
    else
    {
        old.x += parm.x;
        old.y += parm.y;
    }
    tmp = g_init; // for spider
    tempsqrx = sqr(old.x);  // precalculated value for regular Mandelbrot
    tempsqry = sqr(old.y);
    return 1; // 1st iteration has been done
}

int juliafp_per_pixel()
{
    // floating point julia
    // juliafp
    if (g_invert != 0)
    {
        invertz2(&old);
    }
    else
    {
        old.x = dxpixel();
        old.y = dypixel();
    }
    tempsqrx = sqr(old.x);  // precalculated value for regular Julia
    tempsqry = sqr(old.y);
    tmp = old;
    return 0;
}

int MPCjulia_per_pixel()
{
#if !defined(XFRACT)
    // floating point julia
    // juliafp
    if (g_invert != 0)
    {
        invertz2(&old);
    }
    else
    {
        old.x = dxpixel();
        old.y = dypixel();
    }
    mpcold.x = *pd2MP(old.x);
    mpcold.y = *pd2MP(old.y);
    return 0;
#else
    return 0;
#endif
}

int
otherrichard8fp_per_pixel()
{
    othermandelfp_per_pixel();
    CMPLXtrig1(*g_float_param, tmp);
    CMPLXmult(tmp, parm2, tmp);
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
        g_init.x = dxpixel();
        if (save_release >= 2004)
        {
            g_init.y = dypixel();
        }
    }

    if (useinitorbit == 1)
    {
        old = g_init_orbit;
    }
    else
    {
        old = g_init;
    }

    old.x += parm.x;      // initial pertubation of parameters set
    old.y += parm.y;

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
        g_init.x = dxpixel();
        if (save_release >= 2004)
        {
            g_init.y = dypixel();
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
        g_init.x = dxpixel();
        if (save_release >= 2004)
        {
            g_init.y = dypixel();
        }
    }

    old = g_init;

    return 0; // 1st iteration is not done
}

int otherjuliafp_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&old);
    }
    else
    {
        old.x = dxpixel();
        old.y = dypixel();
    }
    return 0;
}

#define Q0 0
#define Q1 0

int quaternionjulfp_per_pixel()
{
    old.x = dxpixel();
    old.y = dypixel();
    g_float_param->x = param[4];
    g_float_param->y = param[5];
    qc  = param[0];
    qci = param[1];
    qcj = param[2];
    qck = param[3];
    return 0;
}

int quaternionfp_per_pixel()
{
    old.x = 0;
    old.y = 0;
    g_float_param->x = 0;
    g_float_param->y = 0;
    qc  = dxpixel();
    qci = dypixel();
    qcj = param[2];
    qck = param[3];
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
        g_init.x = dxpixel();
        if (save_release >= 2004)
        {
            g_init.y = dypixel();
        }
    }
    old.x = g_init.x + parm.x; // initial pertubation of parameters set
    old.y = g_init.y + parm.y;
    tempsqrx = sqr(old.x);  // precalculated value
    tempsqry = sqr(old.y);
    g_marks_coefficient = ComplexPower(g_init, pwr);
    return 1;
}

int long_phoenix_per_pixel()
{
#if !defined(XFRACT)
    if (g_invert != 0)
    {
        // invert
        invertz2(&old);

        // watch out for overflow
        if (sqr(old.x)+sqr(old.y) >= 127)
        {
            old.x = 8;  // value to bail out in one iteration
            old.y = 8;
        }

        // convert to fudged longs
        g_l_old.x = (long)(old.x*g_fudge_factor);
        g_l_old.y = (long)(old.y*g_fudge_factor);
    }
    else
    {
        g_l_old.x = lxpixel();
        g_l_old.y = lypixel();
    }
    g_l_temp_sqr_x = multiply(g_l_old.x, g_l_old.x, bitshift);
    g_l_temp_sqr_y = multiply(g_l_old.y, g_l_old.y, bitshift);
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
        invertz2(&old);
    }
    else
    {
        old.x = dxpixel();
        old.y = dypixel();
    }
    tempsqrx = sqr(old.x);  // precalculated value
    tempsqry = sqr(old.y);
    tmp2.x = 0; // use tmp2 as the complex Y value
    tmp2.y = 0;
    return 0;
}
int long_mandphoenix_per_pixel()
{
#if !defined(XFRACT)
    g_l_init.x = lxpixel();
    if (save_release >= 2004)
    {
        g_l_init.y = lypixel();
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

    if (useinitorbit == 1)
    {
        g_l_old = g_l_init_orbit;
    }
    else
    {
        g_l_old = g_l_init;
    }

    g_l_old.x += g_l_param.x;    // initial pertubation of parameters set
    g_l_old.y += g_l_param.y;
    g_l_temp_sqr_x = multiply(g_l_old.x, g_l_old.x, bitshift);
    g_l_temp_sqr_y = multiply(g_l_old.y, g_l_old.y, bitshift);
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
        g_init.x = dxpixel();
        if (save_release >= 2004)
        {
            g_init.y = dypixel();
        }
    }

    if (useinitorbit == 1)
    {
        old = g_init_orbit;
    }
    else
    {
        old = g_init;
    }

    old.x += parm.x;      // initial pertubation of parameters set
    old.y += parm.y;
    tempsqrx = sqr(old.x);  // precalculated value
    tempsqry = sqr(old.y);
    tmp2.x = 0;
    tmp2.y = 0;
    return 1; // 1st iteration has been done
}

int
QuaternionFPFractal()
{
    double a0, a1, a2, a3, n0, n1, n2, n3;
    a0 = old.x;
    a1 = old.y;
    a2 = g_float_param->x;
    a3 = g_float_param->y;

    n0 = a0*a0-a1*a1-a2*a2-a3*a3 + qc;
    n1 = 2*a0*a1 + qci;
    n2 = 2*a0*a2 + qcj;
    n3 = 2*a0*a3 + qck;
    // Check bailout
    magnitude = a0*a0+a1*a1+a2*a2+a3*a3;
    if (magnitude > rqlim)
    {
        return 1;
    }
    g_new.x = n0;
    old.x = g_new.x;
    g_new.y = n1;
    old.y = g_new.y;
    g_float_param->x = n2;
    g_float_param->y = n3;
    return 0;
}

int
HyperComplexFPFractal()
{
    DHyperComplex hold, hnew;
    hold.x = old.x;
    hold.y = old.y;
    hold.z = g_float_param->x;
    hold.t = g_float_param->y;

    HComplexTrig0(&hold, &hnew);

    hnew.x += qc;
    hnew.y += qci;
    hnew.z += qcj;
    hnew.t += qck;

    g_new.x = hnew.x;
    old.x = g_new.x;
    g_new.y = hnew.y;
    old.y = g_new.y;
    g_float_param->x = hnew.z;
    g_float_param->y = hnew.t;

    // Check bailout
    magnitude = sqr(old.x)+sqr(old.y)+sqr(g_float_param->x)+sqr(g_float_param->y);
    if (magnitude > rqlim)
    {
        return 1;
    }
    return 0;
}

int
VLfpFractal() // Beauty of Fractals pp. 125 - 127
{
    double a, b, ab, half, u, w, xy;

    half = param[0] / 2.0;
    xy = old.x * old.y;
    u = old.x - xy;
    w = -old.y + xy;
    a = old.x + param[1] * u;
    b = old.y + param[1] * w;
    ab = a * b;
    g_new.x = old.x + half * (u + (a - ab));
    g_new.y = old.y + half * (w + (-b + ab));
    return floatbailout();
}

int
EscherfpFractal() // Science of Fractal Images pp. 185, 187
{
    DComplex oldtest, newtest, testsqr;
    double testsize = 0.0;
    long testiter = 0;

    g_new.x = tempsqrx - tempsqry; // standard Julia with C == (0.0, 0.0i)
    g_new.y = 2.0 * old.x * old.y;
    oldtest.x = g_new.x * 15.0;    // scale it
    oldtest.y = g_new.y * 15.0;
    testsqr.x = sqr(oldtest.x);  // set up to test with user-specified ...
    testsqr.y = sqr(oldtest.y);  //    ... Julia as the target set
    while (testsize <= rqlim && testiter < maxit) // nested Julia loop
    {
        newtest.x = testsqr.x - testsqr.y + param[0];
        newtest.y = 2.0 * oldtest.x * oldtest.y + param[1];
        testsqr.x = sqr(newtest.x);
        testsqr.y = sqr(newtest.y);
        testsize = testsqr.x + testsqr.y;
        oldtest = newtest;
        testiter++;
    }
    if (testsize > rqlim)
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
    A.x = param[0];
    A.y = 0.0;    // a=real(p1),
    B.x = param[1];
    B.y = 0.0;    // b=imag(p1),
    D.x = param[2];
    D.y = 0.0;    // d=real(p2),
    F.x = param[3];
    F.y = 0.0;    // f=imag(p2),
    K.x = param[4]+1.0;
    K.y = 0.0;    // k=real(p3)+1,
    L.x = param[5]+100.0;
    L.y = 0.0;    // l=imag(p3)+100,
    CMPLXrecip(F, G);                // g=1/f,
    CMPLXrecip(D, H);                // h=1/d,
    CMPLXsub(F, B, tmp);              // tmp = f-b
    CMPLXrecip(tmp, J);              // j = 1/(f-b)
    CMPLXneg(A, tmp);
    CMPLXmult(tmp, B, tmp);           // z=(-a*b*g*h)^j,
    CMPLXmult(tmp, G, tmp);
    CMPLXmult(tmp, H, tmp);

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
    if (tmp.y == 0.0) // we know tmp.y IS zero but ...
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
            tmp.y = -tmp.y; // swap sign bit
        default: // do nothing - remaining cases already OK
            ;
        }
        // in case our kludge failed, let the user fix it
        if (g_debug_flag == debug_flags::mandelbrot_mix4_flip_sign)
        {
            tmp.y = -tmp.y;
        }
    }

    CMPLXpwr(tmp, J, tmp);   // note: z is old
    // in case our kludge failed, let the user fix it
    if (param[6] < 0.0)
    {
        tmp.y = -tmp.y;
    }

    if (g_bail_out == 0)
    {
        rqlim = L.x;
        rqlim2 = rqlim*rqlim;
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
        g_init.x = dxpixel();
        g_init.y = dypixel();
    }
    old = tmp;
    CMPLXtrig0(g_init, C);        // c=fn1(pixel):
    return 0; // 1st iteration has been NOT been done
}

int
MandelbrotMix4fpFractal() // from formula by Jim Muth
{
    // z=k*((a*(z^b))+(d*(z^f)))+c,
    DComplex z_b, z_f;
    CMPLXpwr(old, B, z_b);     // (z^b)
    CMPLXpwr(old, F, z_f);     // (z^f)
    g_new.x = K.x*A.x*z_b.x + K.x*D.x*z_f.x + C.x;
    g_new.y = K.x*A.x*z_b.y + K.x*D.x*z_f.y + C.y;
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
    return dx0[col]+dx1[row];
}

// Real component, calculation version - does not require arrays
static double dxpixel_calc()
{
    return (double)(xxmin + col*delxx + row*delxx2);
}

// Imaginary component, grid lookup version - requires dy0/dy1 arrays
static double dypixel_grid()
{
    return dy0[row]+dy1[col];
}

// Imaginary component, calculation version - does not require arrays
static double dypixel_calc()
{
    return (double)(yymax - row*delyy - col*delyy2);
}

// Real component, grid lookup version - requires lx0/lx1 arrays
static long lxpixel_grid()
{
    return lx0[col]+lx1[row];
}

// Real component, calculation version - does not require arrays
static long lxpixel_calc()
{
    return xmin + col*delx + row*delx2;
}

// Imaginary component, grid lookup version - requires ly0/ly1 arrays
static long lypixel_grid()
{
    return ly0[row]+ly1[col];
}

// Imaginary component, calculation version - does not require arrays
static long lypixel_calc()
{
    return ymax - row*dely - col*dely2;
}

double (*dxpixel)() = dxpixel_calc;
double (*dypixel)() = dypixel_calc;
long (*lxpixel)() = lxpixel_calc;
long (*lypixel)() = lypixel_calc;

void set_pixel_calc_functions()
{
    if (use_grid)
    {
        dxpixel = dxpixel_grid;
        dypixel = dypixel_grid;
        lxpixel = lxpixel_grid;
        lypixel = lypixel_grid;
    }
    else
    {
        dxpixel = dxpixel_calc;
        dypixel = dypixel_calc;
        lxpixel = lxpixel_calc;
        lypixel = lypixel_calc;
    }
}
