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

4. The main fractal routine. Usually this will be StandardFractal(),
	but if you have written a stand-alone fractal routine independent
	of the StandardFractal mechanisms, your routine name goes here,
	stored in fractalspecific[fractype].calculate_type.per_image.

Adding a new fractal type should be simply a matter of adding an item
to the 'fractalspecific' structure, writing (or re-using one of the existing)
an appropriate setup, per_image, per_pixel, and orbit routines.

--------------------------------------------------------------------   */

#include <limits.h>
#include <string.h>
#if !defined(__386BSD__)
#if !defined(_WIN32)
#include <malloc.h>
#endif
#endif
/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "externs.h"


#define NEWTONDEGREELIMIT  100

_LCMPLX lcoefficient, lold, lnew, lparm, linit, ltmp, ltmp2, lparm2;
long ltempsqrx, ltempsqry;
int maxcolor;
int root, degree, basin;
double floatmin, floatmax;
double roverd, d1overd, threshold;
_CMPLX tmp2;
_CMPLX coefficient;
_CMPLX  staticroots[16]; /* roots array for degree 16 or less */
_CMPLX  *roots = staticroots;
struct MPC      *MPCroots;
long FgHalf;
_CMPLX pwr;
int     bitshiftless1;                  /* bit shift less 1 */

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x), (x), bitshift))
#endif

#define modulus(z)       (sqr((z).x) + sqr((z).y))
#define conjugate(pz)   ((pz)->y = - (pz)->y)
#define distance(z1, z2)  (sqr((z1).x-(z2).x) + sqr((z1).y-(z2).y))
#define pMPsqr(z) (*pMPmul((z), (z)))
#define MPdistance(z1, z2)  (*pMPadd(pMPsqr(*pMPsub((z1).x, (z2).x)), pMPsqr(*pMPsub((z1).y, (z2).y))))

double twopi = PI*2.0;
int c_exp;


/* These are local but I don't want to pass them as parameters */
_CMPLX parm, parm2;
_CMPLX *floatparm;
_LCMPLX *longparm; /* used here and in jb.c */

/* -------------------------------------------------------------------- */
/*              These variables are external for speed's sake only      */
/* -------------------------------------------------------------------- */

double sinx, cosx;
double siny, cosy;
double tmpexp;
double tempsqrx, tempsqry;

double foldxinitx, foldyinity, foldxinity, foldyinitx;
long oldxinitx, oldyinity, oldxinity, oldyinitx;
long longtmp;

/* These are for quaternions */
double qc, qci, qcj, qck;

/* temporary variables for trig use */
long lcosx, lsinx;
long lcosy, lsiny;

/*
**  details of finite attractors (required for Magnet Fractals)
**  (can also be used in "coloring in" the lakes of Julia types)
*/

/*
**  pre-calculated values for fractal types Magnet2M & Magnet2J
*/
_CMPLX  T_Cm1;        /* 3*(floatparm - 1)                */
_CMPLX  T_Cm2;        /* 3*(floatparm - 2)                */
_CMPLX  T_Cm1Cm2;     /* (floatparm - 1)*(floatparm - 2) */

void FloatPreCalcMagnet2(void) /* precalculation for Magnet2 (M & J) for speed */
{
	T_Cm1.x = floatparm->x - 1.0;
	T_Cm1.y = floatparm->y;
	T_Cm2.x = floatparm->x - 2.0;
	T_Cm2.y = floatparm->y;
	T_Cm1Cm2.x = (T_Cm1.x*T_Cm2.x) - (T_Cm1.y*T_Cm2.y);
	T_Cm1Cm2.y = (T_Cm1.x*T_Cm2.y) + (T_Cm1.y*T_Cm2.x);
	T_Cm1.x += T_Cm1.x + T_Cm1.x;
	T_Cm1.y += T_Cm1.y + T_Cm1.y;
	T_Cm2.x += T_Cm2.x + T_Cm2.x;
	T_Cm2.y += T_Cm2.y + T_Cm2.y;
}

/* -------------------------------------------------------------------- */
/*              Bailout Routines Macros                                                                                                 */
/* -------------------------------------------------------------------- */

int (*floatbailout)(void);
int (*longbailout)(void);
int (*bignumbailout)(void);
int (*bigfltbailout)(void);

int  fpMODbailout(void)
{
	tempsqrx = sqr(g_new_z.x);
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	if (g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int  fpREALbailout(void)
{
	tempsqrx = sqr(g_new_z.x);
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	if (tempsqrx >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int  fpIMAGbailout(void)
{
	tempsqrx = sqr(g_new_z.x);
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	if (tempsqry >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int  fpORbailout(void)
{
	tempsqrx = sqr(g_new_z.x);
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	if (tempsqrx >= g_rq_limit || tempsqry >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int  fpANDbailout(void)
{
	tempsqrx = sqr(g_new_z.x);
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	if (tempsqrx >= g_rq_limit && tempsqry >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int  fpMANHbailout(void)
{
	double manhmag;
	tempsqrx = sqr(g_new_z.x);
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	manhmag = fabs(g_new_z.x) + fabs(g_new_z.y);
	if ((manhmag*manhmag) >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int  fpMANRbailout(void)
{
	double manrmag;
	tempsqrx = sqr(g_new_z.x);
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	manrmag = g_new_z.x + g_new_z.y; /* don't need abs() since we square it next */
	if ((manrmag*manrmag) >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

#define FLOATTRIGBAILOUT()		\
	if (fabs(g_old_z.y) >= g_rq_limit2)	\
	{							\
		return 1;				\
	}

#define LONGTRIGBAILOUT()			\
	if (labs(lold.y) >= g_limit2_l)	\
	{								\
		return 1;					\
	}

#define LONGXYTRIGBAILOUT()									\
	if (labs(lold.x) >= g_limit2_l || labs(lold.y) >= g_limit2_l)	\
	{														\
		return 1;											\
	}

#define FLOATXYTRIGBAILOUT()							\
	if (fabs(g_old_z.x) >= g_rq_limit2 || fabs(g_old_z.y) >= g_rq_limit2)	\
	{													\
		return 1;										\
	}

#define FLOATHTRIGBAILOUT()		\
	if (fabs(g_old_z.x) >= g_rq_limit2)	\
	{							\
		return 1;				\
	}

#define LONGHTRIGBAILOUT()  \
	if (labs(lold.x) >= g_limit2_l) \
	{ \
		return 1; \
	}

#define TRIG16CHECK(X)  \
	if (labs((X)) > TRIG_LIMIT_16) \
	{ \
		return 1; \
	}

#define OLD_FLOATEXPBAILOUT()	\
	if (fabs(g_old_z.y) >= 1.0e8)	\
	{							\
		return 1;				\
	}							\
	if (fabs(g_old_z.x) >= 6.4e2)	\
	{							\
		return 1;				\
	}

#define FLOATEXPBAILOUT()		\
	if (fabs(g_old_z.y) >= 1.0e3)	\
	{							\
		return 1;				\
	}							\
	if (fabs(g_old_z.x) >= 8)		\
	{							\
		return 1;				\
	}

#define LONGEXPBAILOUT()					\
	if (labs(lold.y) >= (1000L << bitshift))\
	{										\
		return 1;							\
	}										\
	if (labs(lold.x) >=    (8L << bitshift))\
	{										\
		return 1;							\
	}

#if 0
/* this define uses usual trig instead of fast trig */
#define FPUsincos(px, psinx, pcosx) \
	*(psinx) = sin(*(px)); \
	*(pcosx) = cos(*(px));

#define FPUsinhcosh(px, psinhx, pcoshx) \
	*(psinhx) = sinh(*(px)); \
	*(pcoshx) = cosh(*(px));
#endif

#define LTRIGARG(X)    \
	if (labs((X)) > TRIG_LIMIT_16)\
	{\
		double tmp; \
		tmp = (X); \
		tmp /= fudge; \
		tmp = fmod(tmp, twopi); \
		tmp *= fudge; \
		(X) = (long)tmp; \
	}\

static int  Halleybailout(void)
{
	if (fabs(modulus(g_new_z)-modulus(g_old_z)) < parm2.x)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

#if !defined(XFRACT)
#define MPCmod(m) (*pMPadd(*pMPmul((m).x, (m).x), *pMPmul((m).y, (m).y)))
struct MPC mpcold, mpcnew, mpctmp, mpctmp1;
struct MP mptmpparm2x;

static int  MPCHalleybailout(void)
{
	static struct MP mptmpbailout;
	mptmpbailout = *MPabs(*pMPsub(MPCmod(mpcnew), MPCmod(mpcold)));
	if (pMPcmp(mptmpbailout, mptmpparm2x) < 0)
	{
		return 1;
	}
	mpcold = mpcnew;
	return 0;
}
#endif

#if defined(XFRACT)
int asmlMODbailout(void) { return 0; }
int asmlREALbailout(void) { return 0; }
int asmlIMAGbailout(void) { return 0; }
int asmlORbailout(void) { return 0; }
int asmlANDbailout(void) { return 0; }
int asmlMANHbailout(void) { return 0; }
int asmlMANRbailout(void) { return 0; }
int asm386lMODbailout(void) { return 0; }
int asm386lREALbailout(void) { return 0; }
int asm386lIMAGbailout(void) { return 0; }
int asm386lORbailout(void) { return 0; }
int asm386lANDbailout(void) { return 0; }
int asm386lMANHbailout(void) { return 0; }
int asm386lMANRbailout(void) { return 0; }
int asmfpMODbailout(void) { return 0; }
int asmfpREALbailout(void) { return 0; }
int asmfpIMAGbailout(void) { return 0; }
int asmfpORbailout(void) { return 0; }
int asmfpANDbailout(void) { return 0; }
int asmfpMANHbailout(void) { return 0; }
int asmfpMANRbailout(void) { return 0; }
#endif

/* -------------------------------------------------------------------- */
/*              Fractal (once per iteration) routines                   */
/* -------------------------------------------------------------------- */
static double xt, yt, t2;

/* Raise complex number (base) to the (exp) power, storing the result
** in complex (result).
*/
void cpower(_CMPLX *base, int exp, _CMPLX *result)
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
		t2 = xt*xt - yt*yt;
		yt = 2*xt*yt;
		xt = t2;

		if (exp & 1)
		{
				t2 = xt*result->x - yt*result->y;
				result->y = result->y*xt + yt*result->x;
				result->x = t2;
		}
		exp >>= 1;
	}
}

#if !defined(XFRACT)
/* long version */
static long lxt, lyt, lt2;
int
lcpower(_LCMPLX *base, int exp, _LCMPLX *result, int bitshift)
{
	static long maxarg;
	maxarg = 64L << bitshift;

	if (exp < 0)
	{
		overflow = lcpower(base, -exp, result, bitshift);
		LCMPLXrecip(*result, *result);
		return overflow;
	}

	overflow = 0;
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
		/*
		if (labs(lxt) >= maxarg || labs(lyt) >= maxarg)
			return -1;
		*/
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
		overflow = 1;
	}
	return overflow;
}
#if 0
int
z_to_the_z(_CMPLX *z, _CMPLX *out)
{
	static _CMPLX tmp1, tmp2;
	/* raises complex z to the z power */
	int errno_xxx;
	errno_xxx = 0;

	if (fabs(z->x) < DBL_EPSILON)
	{
		return -1;
	}

	/* log(x + iy) = 1/2(log(x*x + y*y) + i(arc_tan(y/x)) */
	tmp1.x = .5*log(sqr(z->x) + sqr(z->y));

	/* the fabs in next line added to prevent discontinuity in image */
	tmp1.y = atan(fabs(z->y/z->x));

	/* log(z)*z */
	tmp2.x = tmp1.x*z->x - tmp1.y*z->y;
	tmp2.y = tmp1.x*z->y + tmp1.y*z->x;

	/* z*z = e**(log(z)*z) */
	/* e**(x + iy) =  e**x*(cos(y) + isin(y)) */

	tmpexp = exp(tmp2.x);

	FPUsincos(&tmp2.y, &siny, &cosy);
	out->x = tmpexp*cosy;
	out->y = tmpexp*siny;
	return errno_xxx;
}
#endif
#endif

int complex_div(_CMPLX arg1, _CMPLX arg2, _CMPLX *pz);
int complex_mult(_CMPLX arg1, _CMPLX arg2, _CMPLX *pz);

/* Distance of complex z from unit circle */
#define DIST1(z) (((z).x-1.0)*((z).x-1.0) + ((z).y)*((z).y))
#define LDIST1(z) (lsqr((((z).x)-fudge)) + lsqr(((z).y)))


int NewtonFractal2(void)
{
	static char start = 1;
	if (start)
	{
		start = 0;
	}
	cpower(&g_old_z, degree-1, &g_temp_z);
	complex_mult(g_temp_z, g_old_z, &g_new_z);

	if (DIST1(g_new_z) < threshold)
	{
		if (fractype == NEWTBASIN || fractype == MPNEWTBASIN)
		{
			long tmpcolor;
			int i;
			tmpcolor = -1;
			/* this code determines which degree-th root of root the
				Newton formula converges to. The roots of a 1 are
				distributed on a circle of radius 1 about the origin. */
			for (i = 0; i < degree; i++)
			{
				/* color in alternating shades with iteration according to
					which root of 1 it converged to */
				if (distance(roots[i], g_old_z) < threshold)
				{
					tmpcolor = (basin == 2) ?
						(1 + (i&7) + ((g_color_iter&1) << 3)) : (1 + i);
					break;
				}
			}
			g_color_iter = (tmpcolor == -1) ? maxcolor : tmpcolor;
		}
		return 1;
	}
	g_new_z.x = d1overd*g_new_z.x + roverd;
	g_new_z.y *= d1overd;

	/* Watch for divide underflow */
	t2 = g_temp_z.x*g_temp_z.x + g_temp_z.y*g_temp_z.y;
	if (t2 < FLT_MIN)
	{
		return 1;
	}
	else
	{
		t2 = 1.0 / t2;
		g_old_z.x = t2*(g_new_z.x*g_temp_z.x + g_new_z.y*g_temp_z.y);
		g_old_z.y = t2*(g_new_z.y*g_temp_z.x - g_new_z.x*g_temp_z.y);
	}
	return 0;
}

int
complex_mult(_CMPLX arg1, _CMPLX arg2, _CMPLX *pz)
{
	pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
	pz->y = arg1.x*arg2.y + arg1.y*arg2.x;
	return 0;
}

int
complex_div(_CMPLX numerator, _CMPLX denominator, _CMPLX *pout)
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
struct MP mproverd, mpd1overd, mpthreshold;
struct MP mpt2;
struct MP mpone;
#endif

int MPCNewtonFractal(void)
{
#if !defined(XFRACT)
	MPOverflow = 0;
	mpctmp   = MPCpow(mpcold, degree-1);

	mpcnew.x = *pMPsub(*pMPmul(mpctmp.x, mpcold.x), *pMPmul(mpctmp.y, mpcold.y));
	mpcnew.y = *pMPadd(*pMPmul(mpctmp.x, mpcold.y), *pMPmul(mpctmp.y, mpcold.x));
	mpctmp1.x = *pMPsub(mpcnew.x, MPCone.x);
	mpctmp1.y = *pMPsub(mpcnew.y, MPCone.y);
	if (pMPcmp(MPCmod(mpctmp1), mpthreshold)< 0)
	{
		if (fractype == MPNEWTBASIN)
		{
			long tmpcolor;
			int i;
			tmpcolor = -1;
			for (i = 0; i < degree; i++)
			{
				if (pMPcmp(MPdistance(MPCroots[i], mpcold), mpthreshold) < 0)
				{
					tmpcolor = (basin == 2) ?
						(1 + (i&7) + ((g_color_iter&1) << 3)) : (1 + i);
					break;
				}
			}
			g_color_iter = (tmpcolor == -1) ? maxcolor : tmpcolor;
		}
		return 1;
	}

	mpcnew.x = *pMPadd(*pMPmul(mpd1overd, mpcnew.x), mproverd);
	mpcnew.y = *pMPmul(mpcnew.y, mpd1overd);
	mpt2 = MPCmod(mpctmp);
	mpt2 = *pMPdiv(mpone, mpt2);
	mpcold.x = *pMPmul(mpt2, (*pMPadd(*pMPmul(mpcnew.x, mpctmp.x), *pMPmul(mpcnew.y, mpctmp.y))));
	mpcold.y = *pMPmul(mpt2, (*pMPsub(*pMPmul(mpcnew.y, mpctmp.x), *pMPmul(mpcnew.x, mpctmp.y))));
	g_new_z.x = *pMP2d(mpcold.x);
	g_new_z.y = *pMP2d(mpcold.y);
	return MPOverflow;
#else
	return 0;
#endif
}

int
Barnsley1Fractal(void)
{
#if !defined(XFRACT)
	/* Barnsley's Mandelbrot type M1 from "Fractals
	Everywhere" by Michael Barnsley, p. 322 */

	/* calculate intermediate products */
	oldxinitx   = multiply(lold.x, longparm->x, bitshift);
	oldyinity   = multiply(lold.y, longparm->y, bitshift);
	oldxinity   = multiply(lold.x, longparm->y, bitshift);
	oldyinitx   = multiply(lold.y, longparm->x, bitshift);
	/* orbit calculation */
	if (lold.x >= 0)
	{
		lnew.x = (oldxinitx - longparm->x - oldyinity);
		lnew.y = (oldyinitx - longparm->y + oldxinity);
	}
	else
	{
		lnew.x = (oldxinitx + longparm->x - oldyinity);
		lnew.y = (oldyinitx + longparm->y + oldxinity);
	}
	return longbailout();
#else
	return 0;
#endif
}

int
Barnsley1FPFractal(void)
{
	/* Barnsley's Mandelbrot type M1 from "Fractals
	Everywhere" by Michael Barnsley, p. 322 */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

	/* calculate intermediate products */
	foldxinitx = g_old_z.x*floatparm->x;
	foldyinity = g_old_z.y*floatparm->y;
	foldxinity = g_old_z.x*floatparm->y;
	foldyinitx = g_old_z.y*floatparm->x;
	/* orbit calculation */
	if (g_old_z.x >= 0)
	{
		g_new_z.x = (foldxinitx - floatparm->x - foldyinity);
		g_new_z.y = (foldyinitx - floatparm->y + foldxinity);
	}
	else
	{
		g_new_z.x = (foldxinitx + floatparm->x - foldyinity);
		g_new_z.y = (foldyinitx + floatparm->y + foldxinity);
	}
	return floatbailout();
}

int
Barnsley2Fractal(void)
{
#if !defined(XFRACT)
	/* An unnamed Mandelbrot/Julia function from "Fractals
	Everywhere" by Michael Barnsley, p. 331, example 4.2 */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

	/* calculate intermediate products */
	oldxinitx   = multiply(lold.x, longparm->x, bitshift);
	oldyinity   = multiply(lold.y, longparm->y, bitshift);
	oldxinity   = multiply(lold.x, longparm->y, bitshift);
	oldyinitx   = multiply(lold.y, longparm->x, bitshift);

	/* orbit calculation */
	if (oldxinity + oldyinitx >= 0)
	{
		lnew.x = oldxinitx - longparm->x - oldyinity;
		lnew.y = oldyinitx - longparm->y + oldxinity;
	}
	else
	{
		lnew.x = oldxinitx + longparm->x - oldyinity;
		lnew.y = oldyinitx + longparm->y + oldxinity;
	}
	return longbailout();
#else
	return 0;
#endif
}

int
Barnsley2FPFractal(void)
{
	/* An unnamed Mandelbrot/Julia function from "Fractals
	Everywhere" by Michael Barnsley, p. 331, example 4.2 */

	/* calculate intermediate products */
	foldxinitx = g_old_z.x*floatparm->x;
	foldyinity = g_old_z.y*floatparm->y;
	foldxinity = g_old_z.x*floatparm->y;
	foldyinitx = g_old_z.y*floatparm->x;

	/* orbit calculation */
	if (foldxinity + foldyinitx >= 0)
	{
		g_new_z.x = foldxinitx - floatparm->x - foldyinity;
		g_new_z.y = foldyinitx - floatparm->y + foldxinity;
	}
	else
	{
		g_new_z.x = foldxinitx + floatparm->x - foldyinity;
		g_new_z.y = foldyinitx + floatparm->y + foldxinity;
	}
	return floatbailout();
}

int
JuliaFractal(void)
{
	/* used for C prototype of fast integer math routines for classic
		Mandelbrot and Julia */
	lnew.x  = ltempsqrx - ltempsqry + longparm->x;
	lnew.y = multiply(lold.x, lold.y, bitshiftless1) + longparm->y;
	return longbailout();
}

int
JuliafpFractal(void)
{
	/* floating point version of classical Mandelbrot/Julia */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */
	g_new_z.x = tempsqrx - tempsqry + floatparm->x;
	g_new_z.y = 2.0*g_old_z.x*g_old_z.y + floatparm->y;
	return floatbailout();
}

int
LambdaFPFractal(void)
{
	/* variation of classical Mandelbrot/Julia */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

	tempsqrx = g_old_z.x - tempsqrx + tempsqry;
	tempsqry = -(g_old_z.y*g_old_z.x);
	tempsqry += tempsqry + g_old_z.y;

	g_new_z.x = floatparm->x*tempsqrx - floatparm->y*tempsqry;
	g_new_z.y = floatparm->x*tempsqry + floatparm->y*tempsqrx;
	return floatbailout();
}

int
LambdaFractal(void)
{
#if !defined(XFRACT)
	/* variation of classical Mandelbrot/Julia */

	/* in complex math) temp = Z*(1-Z) */
	ltempsqrx = lold.x - ltempsqrx + ltempsqry;
	ltempsqry = lold.y - multiply(lold.y, lold.x, bitshiftless1);
	/* (in complex math) Z = Lambda*Z */
	lnew.x = multiply(longparm->x, ltempsqrx, bitshift)
		- multiply(longparm->y, ltempsqry, bitshift);
	lnew.y = multiply(longparm->x, ltempsqry, bitshift)
		+ multiply(longparm->y, ltempsqrx, bitshift);
	return longbailout();
#else
	return 0;
#endif
}

int
SierpinskiFractal(void)
{
#if !defined(XFRACT)
	/* following code translated from basic - see "Fractals
	Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */
	lnew.x = (lold.x << 1);              /* new.x = 2*old.x  */
	lnew.y = (lold.y << 1);              /* new.y = 2*old.y  */
	if (lold.y > ltmp.y)  /* if old.y > .5 */
	{
		lnew.y = lnew.y - ltmp.x; /* new.y = 2*old.y - 1 */
	}
	else if (lold.x > ltmp.y)     /* if old.x > .5 */
	{
		lnew.x = lnew.x - ltmp.x; /* new.x = 2*old.x - 1 */
	}
	/* end barnsley code */
	return longbailout();
#else
	return 0;
#endif
}

int
SierpinskiFPFractal(void)
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

	/* end barnsley code */
	return floatbailout();
}

int
LambdaexponentFractal(void)
{
	/* found this in  "Science of Fractal Images" */
	if (save_release > 2002)  /* need braces since these are macros */
	{
		FLOATEXPBAILOUT();
	}
	else
	{
		OLD_FLOATEXPBAILOUT();
	}
	FPUsincos  (&g_old_z.y, &siny, &cosy);

	if (g_old_z.x >= g_rq_limit && cosy >= 0.0)
	{
		return 1;
	}
	tmpexp = exp(g_old_z.x);
	g_temp_z.x = tmpexp*cosy;
	g_temp_z.y = tmpexp*siny;

	/*multiply by lamda */
	g_new_z.x = floatparm->x*g_temp_z.x - floatparm->y*g_temp_z.y;
	g_new_z.y = floatparm->y*g_temp_z.x + floatparm->x*g_temp_z.y;
	g_old_z = g_new_z;
	return 0;
}

int
LongLambdaexponentFractal(void)
{
#if !defined(XFRACT)
	/* found this in  "Science of Fractal Images" */
	LONGEXPBAILOUT();

	SinCos086  (lold.y, &lsiny,  &lcosy);

	if (lold.x >= g_limit_l && lcosy >= 0L)
	{
		return 1;
	}
	longtmp = Exp086(lold.x);

	ltmp.x = multiply(longtmp,      lcosy,   bitshift);
	ltmp.y = multiply(longtmp,      lsiny,   bitshift);

	lnew.x  = multiply(longparm->x, ltmp.x, bitshift)
			- multiply(longparm->y, ltmp.y, bitshift);
	lnew.y  = multiply(longparm->x, ltmp.y, bitshift)
			+ multiply(longparm->y, ltmp.x, bitshift);
	lold = lnew;
	return 0;
#else
	return 0;
#endif
}

int
FloatTrigPlusExponentFractal(void)
{
	/* another Scientific American biomorph type */
	/* z(n + 1) = e**z(n) + trig(z(n)) + C */

	if (fabs(g_old_z.x) >= 6.4e2) /* DOMAIN errors  */
	{
		return 1;
	}
	tmpexp = exp(g_old_z.x);
	FPUsincos  (&g_old_z.y, &siny, &cosy);
	CMPLXtrig0(g_old_z, g_new_z);

	/*new =   trig(old) + e**old + C  */
	g_new_z.x += tmpexp*cosy + floatparm->x;
	g_new_z.y += tmpexp*siny + floatparm->y;
	return floatbailout();
}

int
LongTrigPlusExponentFractal(void)
{
#if !defined(XFRACT)
	/* calculate exp(z) */

	/* domain check for fast transcendental functions */
	TRIG16CHECK(lold.x);
	TRIG16CHECK(lold.y);

	longtmp = Exp086(lold.x);
	SinCos086  (lold.y, &lsiny,  &lcosy);
	LCMPLXtrig0(lold, lnew);
	lnew.x += multiply(longtmp,    lcosy,   bitshift) + longparm->x;
	lnew.y += multiply(longtmp,    lsiny,   bitshift) + longparm->y;
	return longbailout();
#else
	return 0;
#endif
}

int
MarksLambdaFractal(void)
{
	/* Mark Peterson's variation of "lambda" function */

	/* Z1 = (C^(exp-1)*Z**2) + C */
#if !defined(XFRACT)
	ltmp.x = ltempsqrx - ltempsqry;
	ltmp.y = multiply(lold.x , lold.y , bitshiftless1);

	lnew.x = multiply(lcoefficient.x, ltmp.x, bitshift)
		- multiply(lcoefficient.y, ltmp.y, bitshift) + longparm->x;
	lnew.y = multiply(lcoefficient.x, ltmp.y, bitshift)
		+ multiply(lcoefficient.y, ltmp.x, bitshift) + longparm->y;

	return longbailout();
#else
	return 0;
#endif
}

int
MarksLambdafpFractal(void)
{
	/* Mark Peterson's variation of "lambda" function */

	/* Z1 = (C^(exp-1)*Z**2) + C */
	g_temp_z.x = tempsqrx - tempsqry;
	g_temp_z.y = g_old_z.x*g_old_z.y *2;

	g_new_z.x = coefficient.x*g_temp_z.x - coefficient.y*g_temp_z.y + floatparm->x;
	g_new_z.y = coefficient.x*g_temp_z.y + coefficient.y*g_temp_z.x + floatparm->y;

	return floatbailout();
}


long XXOne, FgOne, FgTwo;

int
UnityFractal(void)
{
#if !defined(XFRACT)
	/* brought to you by Mark Peterson - you won't find this in any fractal
		books unless they saw it here first - Mark invented it! */
	XXOne = multiply(lold.x, lold.x, bitshift) + multiply(lold.y, lold.y, bitshift);
	if ((XXOne > FgTwo) || (labs(XXOne - FgOne) < delmin))
	{
		return 1;
	}
	lold.y = multiply(FgTwo - XXOne, lold.x, bitshift);
	lold.x = multiply(FgTwo - XXOne, lold.y, bitshift);
	lnew = lold;  /* TW added this line */
	return 0;
#else
	return 0;
#endif
}

int
UnityfpFractal(void)
{
double XXOne;
	/* brought to you by Mark Peterson - you won't find this in any fractal
		books unless they saw it here first - Mark invented it! */

	XXOne = sqr(g_old_z.x) + sqr(g_old_z.y);
	if ((XXOne > 2.0) || (fabs(XXOne - 1.0) < ddelmin))
	{
		return 1;
	}
	g_old_z.y = (2.0 - XXOne)* g_old_z.x;
	g_old_z.x = (2.0 - XXOne)* g_old_z.y;
	g_new_z = g_old_z;  /* TW added this line */
	return 0;
}

int
Mandel4Fractal(void)
{
	/* By writing this code, Bert has left behind the excuse "don't
		know what a fractal is, just know how to make'em go fast".
		Bert is hereby declared a bonafide fractal expert! Supposedly
		this routine calculates the Mandelbrot/Julia set based on the
		polynomial z**4 + lambda, but I wouldn't know -- can't follow
		all that integer math speedup stuff - Tim */

	/* first, compute (x + iy)**2 */
#if !defined(XFRACT)
	lnew.x  = ltempsqrx - ltempsqry;
	lnew.y = multiply(lold.x, lold.y, bitshiftless1);
	if (longbailout())
	{
		return 1;
	}

	/* then, compute ((x + iy)**2)**2 + lambda */
	lnew.x  = ltempsqrx - ltempsqry + longparm->x;
	lnew.y = multiply(lold.x, lold.y, bitshiftless1) + longparm->y;
	return longbailout();
#else
	return 0;
#endif
}

int
Mandel4fpFractal(void)
{
	/* first, compute (x + iy)**2 */
	g_new_z.x  = tempsqrx - tempsqry;
	g_new_z.y = g_old_z.x*g_old_z.y*2;
	if (floatbailout())
	{
		return 1;
	}

	/* then, compute ((x + iy)**2)**2 + lambda */
	g_new_z.x  = tempsqrx - tempsqry + floatparm->x;
	g_new_z.y =  g_old_z.x*g_old_z.y*2 + floatparm->y;
	return floatbailout();
}

int
floatZtozPluszpwrFractal(void)
{
	cpower(&g_old_z, (int)param[2], &g_new_z);
	g_old_z = ComplexPower(g_old_z, g_old_z);
	g_new_z.x = g_new_z.x + g_old_z.x +floatparm->x;
	g_new_z.y = g_new_z.y + g_old_z.y +floatparm->y;
	return floatbailout();
}

int
longZpowerFractal(void)
{
#if !defined(XFRACT)
	if (lcpower(&lold, c_exp, &lnew, bitshift))
	{
		lnew.x = lnew.y = 8L << bitshift;
	}
	lnew.x += longparm->x;
	lnew.y += longparm->y;
	return longbailout();
#else
	return 0;
#endif
}

int
longCmplxZpowerFractal(void)
{
#if !defined(XFRACT)
	_CMPLX x, y;

	x.x = (double)lold.x / fudge;
	x.y = (double)lold.y / fudge;
	y.x = (double)lparm2.x / fudge;
	y.y = (double)lparm2.y / fudge;
	x = ComplexPower(x, y);
	if (fabs(x.x) < fgLimit && fabs(x.y) < fgLimit)
	{
		lnew.x = (long)(x.x*fudge);
		lnew.y = (long)(x.y*fudge);
	}
	else
	{
		overflow = 1;
	}
	lnew.x += longparm->x;
	lnew.y += longparm->y;
	return longbailout();
#else
	return 0;
#endif
}

int
floatZpowerFractal(void)
{
	cpower(&g_old_z, c_exp, &g_new_z);
	g_new_z.x += floatparm->x;
	g_new_z.y += floatparm->y;
	return floatbailout();
}

int
floatCmplxZpowerFractal(void)
{
	g_new_z = ComplexPower(g_old_z, parm2);
	g_new_z.x += floatparm->x;
	g_new_z.y += floatparm->y;
	return floatbailout();
}

int
Barnsley3Fractal(void)
{
	/* An unnamed Mandelbrot/Julia function from "Fractals
	Everywhere" by Michael Barnsley, p. 292, example 4.1 */

	/* calculate intermediate products */
#if !defined(XFRACT)
	oldxinitx   = multiply(lold.x, lold.x, bitshift);
	oldyinity   = multiply(lold.y, lold.y, bitshift);
	oldxinity   = multiply(lold.x, lold.y, bitshift);

	/* orbit calculation */
	if (lold.x > 0)
	{
		lnew.x = oldxinitx   - oldyinity - fudge;
		lnew.y = oldxinity << 1;
	}
	else
	{
		lnew.x = oldxinitx - oldyinity - fudge
			+ multiply(longparm->x, lold.x, bitshift);
		lnew.y = oldxinity <<1;

		/* This term added by Tim Wegner to make dependent on the
			imaginary part of the parameter. (Otherwise Mandelbrot
			is uninteresting. */
		lnew.y += multiply(longparm->y, lold.x, bitshift);
	}
	return longbailout();
#else
	return 0;
#endif
}

int
Barnsley3FPFractal(void)
{
	/* An unnamed Mandelbrot/Julia function from "Fractals
	Everywhere" by Michael Barnsley, p. 292, example 4.1 */


	/* calculate intermediate products */
	foldxinitx  = g_old_z.x*g_old_z.x;
	foldyinity  = g_old_z.y*g_old_z.y;
	foldxinity  = g_old_z.x*g_old_z.y;

	/* orbit calculation */
	if (g_old_z.x > 0)
	{
		g_new_z.x = foldxinitx - foldyinity - 1.0;
		g_new_z.y = foldxinity*2;
	}
	else
	{
		g_new_z.x = foldxinitx - foldyinity -1.0 + floatparm->x*g_old_z.x;
		g_new_z.y = foldxinity*2;

		/* This term added by Tim Wegner to make dependent on the
			imaginary part of the parameter. (Otherwise Mandelbrot
			is uninteresting. */
		g_new_z.y += floatparm->y*g_old_z.x;
	}
	return floatbailout();
}

int
TrigPlusZsquaredFractal(void)
{
#if !defined(XFRACT)
	/* From Scientific American, July 1989 */
	/* A Biomorph                          */
	/* z(n + 1) = trig(z(n)) + z(n)**2 + C       */
	LCMPLXtrig0(lold, lnew);
	lnew.x += ltempsqrx - ltempsqry + longparm->x;
	lnew.y += multiply(lold.x, lold.y, bitshiftless1) + longparm->y;
	return longbailout();
#else
	return 0;
#endif
}

int
TrigPlusZsquaredfpFractal(void)
{
	/* From Scientific American, July 1989 */
	/* A Biomorph                          */
	/* z(n + 1) = trig(z(n)) + z(n)**2 + C       */

	CMPLXtrig0(g_old_z, g_new_z);
	g_new_z.x += tempsqrx - tempsqry + floatparm->x;
	g_new_z.y += 2.0*g_old_z.x*g_old_z.y + floatparm->y;
	return floatbailout();
}

int
Richard8fpFractal(void)
{
	/*  Richard8 {c = z = pixel: z = sin(z) + sin(pixel), |z| <= 50} */
	CMPLXtrig0(g_old_z, g_new_z);
/*   CMPLXtrig1(*floatparm, g_temp_z); */
	g_new_z.x += g_temp_z.x;
	g_new_z.y += g_temp_z.y;
	return floatbailout();
}

int
Richard8Fractal(void)
{
#if !defined(XFRACT)
	/*  Richard8 {c = z = pixel: z = sin(z) + sin(pixel), |z| <= 50} */
	LCMPLXtrig0(lold, lnew);
/*   LCMPLXtrig1(*longparm, ltmp); */
	lnew.x += ltmp.x;
	lnew.y += ltmp.y;
	return longbailout();
#else
	return 0;
#endif
}

int
PopcornFractal_Old(void)
{
	g_temp_z = g_old_z;
	g_temp_z.x *= 3.0;
	g_temp_z.y *= 3.0;
	FPUsincos(&g_temp_z.x, &sinx, &cosx);
	FPUsincos(&g_temp_z.y, &siny, &cosy);
	g_temp_z.x = sinx/cosx + g_old_z.x;
	g_temp_z.y = siny/cosy + g_old_z.y;
	FPUsincos(&g_temp_z.x, &sinx, &cosx);
	FPUsincos(&g_temp_z.y, &siny, &cosy);
	g_new_z.x = g_old_z.x - parm.x*siny;
	g_new_z.y = g_old_z.y - parm.x*sinx;
	if (g_plot_color == noplot)
	{
		plot_orbit(g_new_z.x, g_new_z.y, 1 + g_row % colors);
		g_old_z = g_new_z;
	}
	else
	{
		/* FLOATBAILOUT(); */
		/* PB The above line was weird, not what it seems to be!  But, bracketing
			it or always doing it (either of which seem more likely to be what
			was intended) changes the image for the worse, so I'm not touching it.
			Same applies to int form in next routine. */
		/* PB later: recoded inline, still leaving it weird */
		tempsqrx = sqr(g_new_z.x);
	}
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	if (g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int
PopcornFractal(void)
{
	g_temp_z = g_old_z;
	g_temp_z.x *= 3.0;
	g_temp_z.y *= 3.0;
	FPUsincos(&g_temp_z.x, &sinx, &cosx);
	FPUsincos(&g_temp_z.y, &siny, &cosy);
	g_temp_z.x = sinx/cosx + g_old_z.x;
	g_temp_z.y = siny/cosy + g_old_z.y;
	FPUsincos(&g_temp_z.x, &sinx, &cosx);
	FPUsincos(&g_temp_z.y, &siny, &cosy);
	g_new_z.x = g_old_z.x - parm.x*siny;
	g_new_z.y = g_old_z.y - parm.x*sinx;
	/*
	g_new_z.x = g_old_z.x - parm.x*sin(g_old_z.y + tan(3*g_old_z.y));
	g_new_z.y = g_old_z.y - parm.x*sin(g_old_z.x + tan(3*g_old_z.x));
	*/
	if (g_plot_color == noplot)
	{
		plot_orbit(g_new_z.x, g_new_z.y, 1 + g_row % colors);
		g_old_z = g_new_z;
	}
	/* else */
	/* FLOATBAILOUT(); */
	/* PB The above line was weird, not what it seems to be!  But, bracketing
			it or always doing it (either of which seem more likely to be what
			was intended) changes the image for the worse, so I'm not touching it.
			Same applies to int form in next routine. */
	/* PB later: recoded inline, still leaving it weird */
	/* JCO: sqr's should always be done, else magnitude could be wrong */
	tempsqrx = sqr(g_new_z.x);
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	if (g_magnitude >= g_rq_limit || fabs(g_new_z.x) > g_rq_limit2 || fabs(g_new_z.y) > g_rq_limit2)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int
LPopcornFractal_Old(void)
{
#if !defined(XFRACT)
	ltmp = lold;
	ltmp.x *= 3L;
	ltmp.y *= 3L;
	LTRIGARG(ltmp.x);
	LTRIGARG(ltmp.y);
	SinCos086(ltmp.x, &lsinx, &lcosx);
	SinCos086(ltmp.y, &lsiny, &lcosy);
	ltmp.x = divide(lsinx, lcosx, bitshift) + lold.x;
	ltmp.y = divide(lsiny, lcosy, bitshift) + lold.y;
	LTRIGARG(ltmp.x);
	LTRIGARG(ltmp.y);
	SinCos086(ltmp.x, &lsinx, &lcosx);
	SinCos086(ltmp.y, &lsiny, &lcosy);
	lnew.x = lold.x - multiply(lparm.x, lsiny, bitshift);
	lnew.y = lold.y - multiply(lparm.x, lsinx, bitshift);
	if (g_plot_color == noplot)
	{
		iplot_orbit(lnew.x, lnew.y, 1 + g_row % colors);
		lold = lnew;
	}
	else
	{
		/* LONGBAILOUT(); */
		/* PB above still the old way, is weird, see notes in FP popcorn case */
		ltempsqrx = lsqr(lnew.x);
		ltempsqry = lsqr(lnew.y);
	}
	g_magnitude_l = ltempsqrx + ltempsqry;
	if (g_magnitude_l >= g_limit_l || g_magnitude_l < 0 || labs(lnew.x) > g_limit2_l
			|| labs(lnew.y) > g_limit2_l)
					return 1;
	lold = lnew;
	return 0;
#else
	return 0;
#endif
}

int
LPopcornFractal(void)
{
#if !defined(XFRACT)
	ltmp = lold;
	ltmp.x *= 3L;
	ltmp.y *= 3L;
	LTRIGARG(ltmp.x);
	LTRIGARG(ltmp.y);
	SinCos086(ltmp.x, &lsinx, &lcosx);
	SinCos086(ltmp.y, &lsiny, &lcosy);
	ltmp.x = divide(lsinx, lcosx, bitshift) + lold.x;
	ltmp.y = divide(lsiny, lcosy, bitshift) + lold.y;
	LTRIGARG(ltmp.x);
	LTRIGARG(ltmp.y);
	SinCos086(ltmp.x, &lsinx, &lcosx);
	SinCos086(ltmp.y, &lsiny, &lcosy);
	lnew.x = lold.x - multiply(lparm.x, lsiny, bitshift);
	lnew.y = lold.y - multiply(lparm.x, lsinx, bitshift);
	if (g_plot_color == noplot)
	{
		iplot_orbit(lnew.x, lnew.y, 1 + g_row % colors);
		lold = lnew;
	}
	/* else */
	/* JCO: sqr's should always be done, else magnitude could be wrong */
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	g_magnitude_l = ltempsqrx + ltempsqry;
	if (g_magnitude_l >= g_limit_l || g_magnitude_l < 0
		|| labs(lnew.x) > g_limit2_l
			|| labs(lnew.y) > g_limit2_l)
					return 1;
	lold = lnew;
	return 0;
#else
	return 0;
#endif
}

/* Popcorn generalization proposed by HB  */

int
PopcornFractalFn(void)
{
	_CMPLX tmpx;
	_CMPLX tmpy;

	/* tmpx contains the generalized value of the old real "x" equation */
	CMPLXtimesreal(parm2, g_old_z.y, g_temp_z);  /* tmp = (C*old.y)         */
	CMPLXtrig1(g_temp_z, tmpx);             /* tmpx = trig1(tmp)         */
	tmpx.x += g_old_z.y;                  /* tmpx = old.y + trig1(tmp) */
	CMPLXtrig0(tmpx, g_temp_z);             /* tmp = trig0(tmpx)         */
	CMPLXmult(g_temp_z, parm, tmpx);         /* tmpx = tmp*h            */

	/* tmpy contains the generalized value of the old real "y" equation */
	CMPLXtimesreal(parm2, g_old_z.x, g_temp_z);  /* tmp = (C*old.x)         */
	CMPLXtrig3(g_temp_z, tmpy);             /* tmpy = trig3(tmp)         */
	tmpy.x += g_old_z.x;                  /* tmpy = old.x + trig1(tmp) */
	CMPLXtrig2(tmpy, g_temp_z);             /* tmp = trig2(tmpy)         */

	CMPLXmult(g_temp_z, parm, tmpy);         /* tmpy = tmp*h            */

	g_new_z.x = g_old_z.x - tmpx.x - tmpy.y;
	g_new_z.y = g_old_z.y - tmpy.x - tmpx.y;

	if (g_plot_color == noplot)
	{
		plot_orbit(g_new_z.x, g_new_z.y, 1 + g_row % colors);
		g_old_z = g_new_z;
	}

	tempsqrx = sqr(g_new_z.x);
	tempsqry = sqr(g_new_z.y);
	g_magnitude = tempsqrx + tempsqry;
	if (g_magnitude >= g_rq_limit
		|| fabs(g_new_z.x) > g_rq_limit2 || fabs(g_new_z.y) > g_rq_limit2)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

#define FIX_OVERFLOW(arg) if (overflow)  \
	{ \
		(arg).x = fudge; \
		(arg).y = 0; \
		overflow = 0; \
	}

int
LPopcornFractalFn(void)
{
#if !defined(XFRACT)
	_LCMPLX ltmpx, ltmpy;

	overflow = 0;

	/* ltmpx contains the generalized value of the old real "x" equation */
	LCMPLXtimesreal(lparm2, lold.y, ltmp); /* tmp = (C*old.y)         */
	LCMPLXtrig1(ltmp, ltmpx);             /* tmpx = trig1(tmp)         */
	FIX_OVERFLOW(ltmpx);
	ltmpx.x += lold.y;                   /* tmpx = old.y + trig1(tmp) */
	LCMPLXtrig0(ltmpx, ltmp);             /* tmp = trig0(tmpx)         */
	FIX_OVERFLOW(ltmp);
	LCMPLXmult(ltmp, lparm, ltmpx);        /* tmpx = tmp*h            */

	/* ltmpy contains the generalized value of the old real "y" equation */
	LCMPLXtimesreal(lparm2, lold.x, ltmp); /* tmp = (C*old.x)         */
	LCMPLXtrig3(ltmp, ltmpy);             /* tmpy = trig3(tmp)         */
	FIX_OVERFLOW(ltmpy);
	ltmpy.x += lold.x;                   /* tmpy = old.x + trig1(tmp) */
	LCMPLXtrig2(ltmpy, ltmp);             /* tmp = trig2(tmpy)         */
	FIX_OVERFLOW(ltmp);
	LCMPLXmult(ltmp, lparm, ltmpy);        /* tmpy = tmp*h            */

	lnew.x = lold.x - ltmpx.x - ltmpy.y;
	lnew.y = lold.y - ltmpy.x - ltmpx.y;

	if (g_plot_color == noplot)
	{
		iplot_orbit(lnew.x, lnew.y, 1 + g_row % colors);
		lold = lnew;
	}
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	g_magnitude_l = ltempsqrx + ltempsqry;
	if (g_magnitude_l >= g_limit_l || g_magnitude_l < 0
		|| labs(lnew.x) > g_limit2_l
		|| labs(lnew.y) > g_limit2_l)
		return 1;
	lold = lnew;
	return 0;
#else
	return 0;
#endif
}

int MarksCplxMand(void)
{
	g_temp_z.x = tempsqrx - tempsqry;
	g_temp_z.y = 2*g_old_z.x*g_old_z.y;
	FPUcplxmul(&g_temp_z, &coefficient, &g_new_z);
	g_new_z.x += floatparm->x;
	g_new_z.y += floatparm->y;
	return floatbailout();
}

int SpiderfpFractal(void)
{
	/* Spider(XAXIS) { c = z=pixel: z = z*z + c; c = c/2 + z, |z| <= 4 } */
	g_new_z.x = tempsqrx - tempsqry + g_temp_z.x;
	g_new_z.y = 2*g_old_z.x*g_old_z.y + g_temp_z.y;
	g_temp_z.x = g_temp_z.x/2 + g_new_z.x;
	g_temp_z.y = g_temp_z.y/2 + g_new_z.y;
	return floatbailout();
}

int
SpiderFractal(void)
{
#if !defined(XFRACT)
	/* Spider(XAXIS) { c = z=pixel: z = z*z + c; c = c/2 + z, |z| <= 4 } */
	lnew.x  = ltempsqrx - ltempsqry + ltmp.x;
	lnew.y = multiply(lold.x, lold.y, bitshiftless1) + ltmp.y;
	ltmp.x = (ltmp.x >> 1) + lnew.x;
	ltmp.y = (ltmp.y >> 1) + lnew.y;
	return longbailout();
#else
	return 0;
#endif
}

int
TetratefpFractal(void)
{
	/* Tetrate(XAXIS) { c = z=pixel: z = c^z, |z| <= (P1 + 3) } */
	g_new_z = ComplexPower(*floatparm, g_old_z);
	return floatbailout();
}

int
ZXTrigPlusZFractal(void)
{
#if !defined(XFRACT)
	/* z = (p1*z*trig(z)) + p2*z */
	LCMPLXtrig0(lold, ltmp);          /* ltmp  = trig(old)             */
	LCMPLXmult(lparm, ltmp, ltmp);      /* ltmp  = p1*trig(old)          */
	LCMPLXmult(lold, ltmp, ltmp2);      /* ltmp2 = p1*old*trig(old)      */
	LCMPLXmult(lparm2, lold, ltmp);     /* ltmp  = p2*old                */
	LCMPLXadd(ltmp2, ltmp, lnew);       /* lnew  = p1*trig(old) + p2*old */
	return longbailout();
#else
	return 0;
#endif
}

int
ScottZXTrigPlusZFractal(void)
{
#if !defined(XFRACT)
	/* z = (z*trig(z)) + z */
	LCMPLXtrig0(lold, ltmp);          /* ltmp  = trig(old)       */
	LCMPLXmult(lold, ltmp, lnew);       /* lnew  = old*trig(old)   */
	LCMPLXadd(lnew, lold, lnew);        /* lnew  = trig(old) + old */
	return longbailout();
#else
	return 0;
#endif
}

int
SkinnerZXTrigSubZFractal(void)
{
#if !defined(XFRACT)
	/* z = (z*trig(z))-z */
	LCMPLXtrig0(lold, ltmp);          /* ltmp  = trig(old)       */
	LCMPLXmult(lold, ltmp, lnew);       /* lnew  = old*trig(old)   */
	LCMPLXsub(lnew, lold, lnew);        /* lnew  = trig(old) - old */
	return longbailout();
#else
	return 0;
#endif
}

int
ZXTrigPlusZfpFractal(void)
{
	/* z = (p1*z*trig(z)) + p2*z */
	CMPLXtrig0(g_old_z, g_temp_z);          /* tmp  = trig(old)             */
	CMPLXmult(parm, g_temp_z, g_temp_z);      /* tmp  = p1*trig(old)          */
	CMPLXmult(g_old_z, g_temp_z, tmp2);      /* tmp2 = p1*old*trig(old)      */
	CMPLXmult(parm2, g_old_z, g_temp_z);     /* tmp  = p2*old                */
	CMPLXadd(tmp2, g_temp_z, g_new_z);       /* new  = p1*trig(old) + p2*old */
	return floatbailout();
}

int
ScottZXTrigPlusZfpFractal(void)
{
	/* z = (z*trig(z)) + z */
	CMPLXtrig0(g_old_z, g_temp_z);         /* tmp  = trig(old)       */
	CMPLXmult(g_old_z, g_temp_z, g_new_z);       /* new  = old*trig(old)   */
	CMPLXadd(g_new_z, g_old_z, g_new_z);        /* new  = trig(old) + old */
	return floatbailout();
}

int
SkinnerZXTrigSubZfpFractal(void)
{
	/* z = (z*trig(z))-z */
	CMPLXtrig0(g_old_z, g_temp_z);         /* tmp  = trig(old)       */
	CMPLXmult(g_old_z, g_temp_z, g_new_z);       /* new  = old*trig(old)   */
	CMPLXsub(g_new_z, g_old_z, g_new_z);        /* new  = trig(old) - old */
	return floatbailout();
}

int
Sqr1overTrigFractal(void)
{
#if !defined(XFRACT)
	/* z = sqr(1/trig(z)) */
	LCMPLXtrig0(lold, lold);
	LCMPLXrecip(lold, lold);
	LCMPLXsqr(lold, lnew);
	return longbailout();
#else
	return 0;
#endif
}

int
Sqr1overTrigfpFractal(void)
{
	/* z = sqr(1/trig(z)) */
	CMPLXtrig0(g_old_z, g_old_z);
	CMPLXrecip(g_old_z, g_old_z);
	CMPLXsqr(g_old_z, g_new_z);
	return floatbailout();
}

int
TrigPlusTrigFractal(void)
{
#if !defined(XFRACT)
	/* z = trig(0, z)*p1 + trig1(z)*p2 */
	LCMPLXtrig0(lold, ltmp);
	LCMPLXmult(lparm, ltmp, ltmp);
	LCMPLXtrig1(lold, ltmp2);
	LCMPLXmult(lparm2, ltmp2, lold);
	LCMPLXadd(ltmp, lold, lnew);
	return longbailout();
#else
	return 0;
#endif
}

int
TrigPlusTrigfpFractal(void)
{
	/* z = trig0(z)*p1 + trig1(z)*p2 */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXmult(parm, g_temp_z, g_temp_z);
	CMPLXtrig1(g_old_z, g_old_z);
	CMPLXmult(parm2, g_old_z, g_old_z);
	CMPLXadd(g_temp_z, g_old_z, g_new_z);
	return floatbailout();
}

/* The following four fractals are based on the idea of parallel
	or alternate calculations.  The shift is made when the mod
	reaches a given value.  JCO  5/6/92 */

int
LambdaTrigOrTrigFractal(void)
{
#if !defined(XFRACT)
	/* z = trig0(z)*p1 if mod(old) < p2.x and
			trig1(z)*p1 if mod(old) >= p2.x */
	if ((LCMPLXmod(lold)) < lparm2.x)
	{
		LCMPLXtrig0(lold, ltmp);
		LCMPLXmult(*longparm, ltmp, lnew);
	}
	else
	{
		LCMPLXtrig1(lold, ltmp);
		LCMPLXmult(*longparm, ltmp, lnew);
	}
	return longbailout();
#else
	return 0;
#endif
}

int
LambdaTrigOrTrigfpFractal(void)
{
	/* z = trig0(z)*p1 if mod(old) < p2.x and
			trig1(z)*p1 if mod(old) >= p2.x */
	if (CMPLXmod(g_old_z) < parm2.x)
	{
		CMPLXtrig0(g_old_z, g_old_z);
		FPUcplxmul(floatparm, &g_old_z, &g_new_z);
	}
	else
	{
		CMPLXtrig1(g_old_z, g_old_z);
		FPUcplxmul(floatparm, &g_old_z, &g_new_z);
	}
	return floatbailout();
}

int
JuliaTrigOrTrigFractal(void)
{
#if !defined(XFRACT)
	/* z = trig0(z) + p1 if mod(old) < p2.x and
			trig1(z) + p1 if mod(old) >= p2.x */
	if (LCMPLXmod(lold) < lparm2.x)
	{
		LCMPLXtrig0(lold, ltmp);
		LCMPLXadd(*longparm, ltmp, lnew);
	}
	else
	{
		LCMPLXtrig1(lold, ltmp);
		LCMPLXadd(*longparm, ltmp, lnew);
	}
	return longbailout();
#else
	return 0;
#endif
}

int
JuliaTrigOrTrigfpFractal(void)
{
	/* z = trig0(z) + p1 if mod(old) < p2.x and
			trig1(z) + p1 if mod(old) >= p2.x */
	if (CMPLXmod(g_old_z) < parm2.x)
	{
		CMPLXtrig0(g_old_z, g_old_z);
		CMPLXadd(*floatparm, g_old_z, g_new_z);
	}
	else
	{
		CMPLXtrig1(g_old_z, g_old_z);
		CMPLXadd(*floatparm, g_old_z, g_new_z);
	}
	return floatbailout();
}

int AplusOne, Ap1deg;
struct MP mpAplusOne, mpAp1deg;
struct MPC mpctmpparm;

int MPCHalleyFractal(void)
{
#if !defined(XFRACT)
	/*  X(X^a - 1) = 0, Halley Map */
	/*  a = parm.x,  relaxation coeff. = parm.y,  epsilon = parm2.x  */

	int ihal;
	struct MPC mpcXtoAlessOne, mpcXtoA;
	struct MPC mpcXtoAplusOne; /* a-1, a, a + 1 */
	struct MPC mpcFX, mpcF1prime, mpcF2prime, mpcHalnumer1;
	struct MPC mpcHalnumer2, mpcHaldenom, mpctmp;

	MPOverflow = 0;
	mpcXtoAlessOne.x = mpcold.x;
	mpcXtoAlessOne.y = mpcold.y;
	for (ihal = 2; ihal < degree; ihal++)
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
	mpcFX.y = *pMPsub(mpcXtoAplusOne.y, mpcold.y); /* FX = X^(a + 1) - X  = F */

	mpcF2prime.x = *pMPmul(mpAp1deg, mpcXtoAlessOne.x); /* mpAp1deg in setup */
	mpcF2prime.y = *pMPmul(mpAp1deg, mpcXtoAlessOne.y);        /* F" */

	mpcF1prime.x = *pMPsub(*pMPmul(mpAplusOne, mpcXtoA.x), mpone);
	mpcF1prime.y = *pMPmul(mpAplusOne, mpcXtoA.y);                   /*  F'  */

	mpctmp.x = *pMPsub(*pMPmul(mpcF2prime.x, mpcFX.x), *pMPmul(mpcF2prime.y, mpcFX.y));
	mpctmp.y = *pMPadd(*pMPmul(mpcF2prime.x, mpcFX.y), *pMPmul(mpcF2prime.y, mpcFX.x));
	/*  F*F"  */

	mpcHaldenom.x = *pMPadd(mpcF1prime.x, mpcF1prime.x);
	mpcHaldenom.y = *pMPadd(mpcF1prime.y, mpcF1prime.y);      /*  2*F'  */

	mpcHalnumer1 = MPCdiv(mpctmp, mpcHaldenom);        /*  F"F/2F'  */
	mpctmp.x = *pMPsub(mpcF1prime.x, mpcHalnumer1.x);
	mpctmp.y = *pMPsub(mpcF1prime.y, mpcHalnumer1.y); /*  F' - F"F/2F'  */
	mpcHalnumer2 = MPCdiv(mpcFX, mpctmp);

	mpctmp   =  MPCmul(mpctmpparm, mpcHalnumer2);  /* mpctmpparm is */
													/* relaxation coef. */
#if 0
	mpctmp.x = *pMPmul(mptmpparmy, mpcHalnumer2.x); /* mptmpparmy is */
	mpctmp.y = *pMPmul(mptmpparmy, mpcHalnumer2.y); /* relaxation coef. */

	mpcnew.x = *pMPsub(mpcold.x, mpctmp.x);
	mpcnew.y = *pMPsub(mpcold.y, mpctmp.y);

	g_new_z.x = *pMP2d(mpcnew.x);
	g_new_z.y = *pMP2d(mpcnew.y);
#endif
	mpcnew = MPCsub(mpcold, mpctmp);
	g_new_z    = MPC2cmplx(mpcnew);
	return MPCHalleybailout() || MPOverflow;
#else
	return 0;
#endif
}

int
HalleyFractal(void)
{
	/*  X(X^a - 1) = 0, Halley Map */
	/*  a = parm.x = degree, relaxation coeff. = parm.y, epsilon = parm2.x  */

	int ihal;
	_CMPLX XtoAlessOne, XtoA, XtoAplusOne; /* a-1, a, a + 1 */
	_CMPLX FX, F1prime, F2prime, Halnumer1, Halnumer2, Haldenom;
	_CMPLX relax;

	XtoAlessOne = g_old_z;
	for (ihal = 2; ihal < degree; ihal++)
	{
		FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoAlessOne);
	}
	FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoA);
	FPUcplxmul(&g_old_z, &XtoA, &XtoAplusOne);

	CMPLXsub(XtoAplusOne, g_old_z, FX);        /* FX = X^(a + 1) - X  = F */
	F2prime.x = Ap1deg*XtoAlessOne.x; /* Ap1deg in setup */
	F2prime.y = Ap1deg*XtoAlessOne.y;        /* F" */

	F1prime.x = AplusOne*XtoA.x - 1.0;
	F1prime.y = AplusOne*XtoA.y;                             /*  F'  */

	FPUcplxmul(&F2prime, &FX, &Halnumer1);                  /*  F*F"  */
	Haldenom.x = F1prime.x + F1prime.x;
	Haldenom.y = F1prime.y + F1prime.y;                     /*  2*F'  */

	FPUcplxdiv(&Halnumer1, &Haldenom, &Halnumer1);         /*  F"F/2F'  */
	CMPLXsub(F1prime, Halnumer1, Halnumer2);          /*  F' - F"F/2F'  */
	FPUcplxdiv(&FX, &Halnumer2, &Halnumer2);
	/* parm.y is relaxation coef. */
	/* new.x = g_old_z.x - (parm.y*Halnumer2.x);
	new.y = g_old_z.y - (parm.y*Halnumer2.y); */
	relax.x = parm.y;
	relax.y = param[3];
	FPUcplxmul(&relax, &Halnumer2, &Halnumer2);
	g_new_z.x = g_old_z.x - Halnumer2.x;
	g_new_z.y = g_old_z.y - Halnumer2.y;
	return Halleybailout();
}

int
LongPhoenixFractal(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n) */
	ltmp.x = multiply(lold.x, lold.y, bitshift);
	lnew.x = ltempsqrx-ltempsqry + longparm->x + multiply(longparm->y, ltmp2.x, bitshift);
	lnew.y = (ltmp.x + ltmp.x) + multiply(longparm->y, ltmp2.y, bitshift);
	ltmp2 = lold; /* set ltmp2 to Y value */
	return longbailout();
#else
	return 0;
#endif
}

int
PhoenixFractal(void)
{
	/* z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n) */
	g_temp_z.x = g_old_z.x*g_old_z.y;
	g_new_z.x = tempsqrx - tempsqry + floatparm->x + (floatparm->y*tmp2.x);
	g_new_z.y = (g_temp_z.x + g_temp_z.x) + (floatparm->y*tmp2.y);
	tmp2 = g_old_z; /* set tmp2 to Y value */
	return floatbailout();
}

int
LongPhoenixFractalcplx(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n) */
	ltmp.x = multiply(lold.x, lold.y, bitshift);
	lnew.x = ltempsqrx-ltempsqry + longparm->x + multiply(lparm2.x, ltmp2.x, bitshift)-multiply(lparm2.y, ltmp2.y, bitshift);
	lnew.y = (ltmp.x + ltmp.x) + longparm->y + multiply(lparm2.x, ltmp2.y, bitshift) + multiply(lparm2.y, ltmp2.x, bitshift);
	ltmp2 = lold; /* set ltmp2 to Y value */
	return longbailout();
#else
	return 0;
#endif
}

int
PhoenixFractalcplx(void)
{
	/* z(n + 1) = z(n)^2 + p1 + p2*y(n),  y(n + 1) = z(n) */
	g_temp_z.x = g_old_z.x*g_old_z.y;
	g_new_z.x = tempsqrx - tempsqry + floatparm->x + (parm2.x*tmp2.x) - (parm2.y*tmp2.y);
	g_new_z.y = (g_temp_z.x + g_temp_z.x) + floatparm->y + (parm2.x*tmp2.y) + (parm2.y*tmp2.x);
	tmp2 = g_old_z; /* set tmp2 to Y value */
	return floatbailout();
}

int
LongPhoenixPlusFractal(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_LCMPLX loldplus, lnewminus;
	loldplus = lold;
	ltmp = lold;
	for (i = 1; i < degree; i++)  /* degree >= 2, degree = degree-1 in setup */
	{
		LCMPLXmult(lold, ltmp, ltmp); /* = old^(degree-1) */
	}
	loldplus.x += longparm->x;
	LCMPLXmult(ltmp, loldplus, lnewminus);
	lnew.x = lnewminus.x + multiply(longparm->y, ltmp2.x, bitshift);
	lnew.y = lnewminus.y + multiply(longparm->y, ltmp2.y, bitshift);
	ltmp2 = lold; /* set ltmp2 to Y value */
	return longbailout();
#else
	return 0;
#endif
}

int
PhoenixPlusFractal(void)
{
	/* z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_CMPLX oldplus, newminus;
	oldplus = g_old_z;
	g_temp_z = g_old_z;
	for (i = 1; i < degree; i++)  /* degree >= 2, degree = degree-1 in setup */
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); /* = old^(degree-1) */
	}
	oldplus.x += floatparm->x;
	FPUcplxmul(&g_temp_z, &oldplus, &newminus);
	g_new_z.x = newminus.x + (floatparm->y*tmp2.x);
	g_new_z.y = newminus.y + (floatparm->y*tmp2.y);
	tmp2 = g_old_z; /* set tmp2 to Y value */
	return floatbailout();
}

int
LongPhoenixMinusFractal(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_LCMPLX loldsqr, lnewminus;
	LCMPLXmult(lold, lold, loldsqr);
	ltmp = lold;
	for (i = 1; i < degree; i++)  /* degree >= 3, degree = degree-2 in setup */
	{
		LCMPLXmult(lold, ltmp, ltmp); /* = old^(degree-2) */
	}
	loldsqr.x += longparm->x;
	LCMPLXmult(ltmp, loldsqr, lnewminus);
	lnew.x = lnewminus.x + multiply(longparm->y, ltmp2.x, bitshift);
	lnew.y = lnewminus.y + multiply(longparm->y, ltmp2.y, bitshift);
	ltmp2 = lold; /* set ltmp2 to Y value */
	return longbailout();
#else
	return 0;
#endif
}

int
PhoenixMinusFractal(void)
{
	/* z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_CMPLX oldsqr, newminus;
	FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
	g_temp_z = g_old_z;
	for (i = 1; i < degree; i++)  /* degree >= 3, degree = degree-2 in setup */
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); /* = old^(degree-2) */
	}
	oldsqr.x += floatparm->x;
	FPUcplxmul(&g_temp_z, &oldsqr, &newminus);
	g_new_z.x = newminus.x + (floatparm->y*tmp2.x);
	g_new_z.y = newminus.y + (floatparm->y*tmp2.y);
	tmp2 = g_old_z; /* set tmp2 to Y value */
	return floatbailout();
}

int
LongPhoenixCplxPlusFractal(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_LCMPLX loldplus, lnewminus;
	loldplus = lold;
	ltmp = lold;
	for (i = 1; i < degree; i++)  /* degree >= 2, degree = degree-1 in setup */
	{
		LCMPLXmult(lold, ltmp, ltmp); /* = old^(degree-1) */
	}
	loldplus.x += longparm->x;
	loldplus.y += longparm->y;
	LCMPLXmult(ltmp, loldplus, lnewminus);
	LCMPLXmult(lparm2, ltmp2, ltmp);
	lnew.x = lnewminus.x + ltmp.x;
	lnew.y = lnewminus.y + ltmp.y;
	ltmp2 = lold; /* set ltmp2 to Y value */
	return longbailout();
#else
	return 0;
#endif
}

int
PhoenixCplxPlusFractal(void)
{
	/* z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_CMPLX oldplus, newminus;
	oldplus = g_old_z;
	g_temp_z = g_old_z;
	for (i = 1; i < degree; i++)  /* degree >= 2, degree = degree-1 in setup */
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); /* = old^(degree-1) */
	}
	oldplus.x += floatparm->x;
	oldplus.y += floatparm->y;
	FPUcplxmul(&g_temp_z, &oldplus, &newminus);
	FPUcplxmul(&parm2, &tmp2, &g_temp_z);
	g_new_z.x = newminus.x + g_temp_z.x;
	g_new_z.y = newminus.y + g_temp_z.y;
	tmp2 = g_old_z; /* set tmp2 to Y value */
	return floatbailout();
}

int
LongPhoenixCplxMinusFractal(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_LCMPLX loldsqr, lnewminus;
	LCMPLXmult(lold, lold, loldsqr);
	ltmp = lold;
	for (i = 1; i < degree; i++)  /* degree >= 3, degree = degree-2 in setup */
	{
		LCMPLXmult(lold, ltmp, ltmp); /* = old^(degree-2) */
	}
	loldsqr.x += longparm->x;
	loldsqr.y += longparm->y;
	LCMPLXmult(ltmp, loldsqr, lnewminus);
	LCMPLXmult(lparm2, ltmp2, ltmp);
	lnew.x = lnewminus.x + ltmp.x;
	lnew.y = lnewminus.y + ltmp.y;
	ltmp2 = lold; /* set ltmp2 to Y value */
	return longbailout();
#else
	return 0;
#endif
}

int
PhoenixCplxMinusFractal(void)
{
	/* z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_CMPLX oldsqr, newminus;
	FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
	g_temp_z = g_old_z;
	for (i = 1; i < degree; i++)  /* degree >= 3, degree = degree-2 in setup */
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); /* = old^(degree-2) */
	}
	oldsqr.x += floatparm->x;
	oldsqr.y += floatparm->y;
	FPUcplxmul(&g_temp_z, &oldsqr, &newminus);
	FPUcplxmul(&parm2, &tmp2, &g_temp_z);
	g_new_z.x = newminus.x + g_temp_z.x;
	g_new_z.y = newminus.y + g_temp_z.y;
	tmp2 = g_old_z; /* set tmp2 to Y value */
	return floatbailout();
}

int
ScottTrigPlusTrigFractal(void)
{
#if !defined(XFRACT)
	/* z = trig0(z) + trig1(z) */
	LCMPLXtrig0(lold, ltmp);
	LCMPLXtrig1(lold, lold);
	LCMPLXadd(ltmp, lold, lnew);
	return longbailout();
#else
	return 0;
#endif
}

int
ScottTrigPlusTrigfpFractal(void)
{
	/* z = trig0(z) + trig1(z) */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXtrig1(g_old_z, tmp2);
	CMPLXadd(g_temp_z, tmp2, g_new_z);
	return floatbailout();
}

int
SkinnerTrigSubTrigFractal(void)
{
#if !defined(XFRACT)
	/* z = trig(0, z)-trig1(z) */
	LCMPLXtrig0(lold, ltmp);
	LCMPLXtrig1(lold, ltmp2);
	LCMPLXsub(ltmp, ltmp2, lnew);
	return longbailout();
#else
	return 0;
#endif
}

int
SkinnerTrigSubTrigfpFractal(void)
{
	/* z = trig0(z)-trig1(z) */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXtrig1(g_old_z, tmp2);
	CMPLXsub(g_temp_z, tmp2, g_new_z);
	return floatbailout();
}

int
TrigXTrigfpFractal(void)
{
	/* z = trig0(z)*trig1(z) */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXtrig1(g_old_z, g_old_z);
	CMPLXmult(g_temp_z, g_old_z, g_new_z);
	return floatbailout();
}

#if !defined(XFRACT)
/* call float version of fractal if integer math overflow */
static int TryFloatFractal(int (*fpFractal)(void))
{
	overflow = 0;
	/* lold had better not be changed! */
	g_old_z.x = lold.x; g_old_z.x /= fudge;
	g_old_z.y = lold.y; g_old_z.y /= fudge;
	tempsqrx = sqr(g_old_z.x);
	tempsqry = sqr(g_old_z.y);
	fpFractal();
	if (save_release < 1900)  /* for backwards compatibility */
	{
		lnew.x = (long)(g_new_z.x/fudge); /* this error has been here a long time */
		lnew.y = (long)(g_new_z.y/fudge);
	}
	else
	{
		lnew.x = (long)(g_new_z.x*fudge);
		lnew.y = (long)(g_new_z.y*fudge);
	}
	return 0;
}
#endif

int
TrigXTrigFractal(void)
{
#if !defined(XFRACT)
	_LCMPLX ltmp2;
	/* z = trig0(z)*trig1(z) */
	LCMPLXtrig0(lold, ltmp);
	LCMPLXtrig1(lold, ltmp2);
	LCMPLXmult(ltmp, ltmp2, lnew);
	if (overflow)
	{
		TryFloatFractal(TrigXTrigfpFractal);
	}
	return longbailout();
#else
	return 0;
#endif
}

/********************************************************************/
/*  Next six orbit functions are one type - extra functions are     */
/*    special cases written for speed.                              */
/********************************************************************/

int
TrigPlusSqrFractal(void) /* generalization of Scott and Skinner types */
{
#if !defined(XFRACT)
	/* { z = pixel: z = (p1, p2)*trig(z) + (p3, p4)*sqr(z), |z|<BAILOUT } */
	LCMPLXtrig0(lold, ltmp);     /* ltmp = trig(lold)                        */
	LCMPLXmult(lparm, ltmp, lnew); /* lnew = lparm*trig(lold)                  */
	LCMPLXsqr_old(ltmp);         /* ltmp = sqr(lold)                         */
	LCMPLXmult(lparm2, ltmp, ltmp); /* ltmp = lparm2*sqr(lold)                  */
	LCMPLXadd(lnew, ltmp, lnew);   /* lnew = lparm*trig(lold) + lparm2*sqr(lold) */
	return longbailout();
#else
	return 0;
#endif
}

int
TrigPlusSqrfpFractal(void) /* generalization of Scott and Skinner types */
{
	/* { z = pixel: z = (p1, p2)*trig(z) + (p3, p4)*sqr(z), |z|<BAILOUT } */
	CMPLXtrig0(g_old_z, g_temp_z);     /* tmp = trig(old)                     */
	CMPLXmult(parm, g_temp_z, g_new_z); /* new = parm*trig(old)                */
	CMPLXsqr_old(g_temp_z);        /* tmp = sqr(old)                      */
	CMPLXmult(parm2, g_temp_z, tmp2); /* tmp = parm2*sqr(old)                */
	CMPLXadd(g_new_z, tmp2, g_new_z);    /* new = parm*trig(old) + parm2*sqr(old) */
	return floatbailout();
}

int
ScottTrigPlusSqrFractal(void)
{
#if !defined(XFRACT)
	/*  { z = pixel: z = trig(z) + sqr(z), |z|<BAILOUT } */
	LCMPLXtrig0(lold, lnew);    /* lnew = trig(lold)           */
	LCMPLXsqr_old(ltmp);        /* lold = sqr(lold)            */
	LCMPLXadd(ltmp, lnew, lnew);  /* lnew = trig(lold) + sqr(lold) */
	return longbailout();
#else
	return 0;
#endif
}

int
ScottTrigPlusSqrfpFractal(void) /* float version */
{
	/* { z = pixel: z = sin(z) + sqr(z), |z|<BAILOUT } */
	CMPLXtrig0(g_old_z, g_new_z);       /* new = trig(old)          */
	CMPLXsqr_old(g_temp_z);          /* tmp = sqr(old)           */
	CMPLXadd(g_new_z, g_temp_z, g_new_z);      /* new = trig(old) + sqr(old) */
	return floatbailout();
}

int
SkinnerTrigSubSqrFractal(void)
{
#if !defined(XFRACT)
	/* { z = pixel: z = sin(z)-sqr(z), |z|<BAILOUT }               */
	LCMPLXtrig0(lold, lnew);    /* lnew = trig(lold)           */
	LCMPLXsqr_old(ltmp);        /* lold = sqr(lold)            */
	LCMPLXsub(lnew, ltmp, lnew);  /* lnew = trig(lold)-sqr(lold) */
	return longbailout();
#else
	return 0;
#endif
}

int
SkinnerTrigSubSqrfpFractal(void)
{
	/* { z = pixel: z = sin(z)-sqr(z), |z|<BAILOUT } */
	CMPLXtrig0(g_old_z, g_new_z);       /* new = trig(old) */
	CMPLXsqr_old(g_temp_z);          /* old = sqr(old)  */
	CMPLXsub(g_new_z, g_temp_z, g_new_z);      /* new = trig(old)-sqr(old) */
	return floatbailout();
}

int
TrigZsqrdfpFractal(void)
{
	/* { z = pixel: z = trig(z*z), |z|<TEST } */
	CMPLXsqr_old(g_temp_z);
	CMPLXtrig0(g_temp_z, g_new_z);
	return floatbailout();
}

int
TrigZsqrdFractal(void) /* this doesn't work very well */
{
#if !defined(XFRACT)
	/* { z = pixel: z = trig(z*z), |z|<TEST } */
	long l16triglim_2 = 8L << 15;
	LCMPLXsqr_old(ltmp);
	if ((labs(ltmp.x) > l16triglim_2 || labs(ltmp.y) > l16triglim_2) &&
		save_release > 1900)
	{
		overflow = 1;
	}
	else
	{
		LCMPLXtrig0(ltmp, lnew);
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
SqrTrigFractal(void)
{
#if !defined(XFRACT)
	/* { z = pixel: z = sqr(trig(z)), |z|<TEST} */
	LCMPLXtrig0(lold, ltmp);
	LCMPLXsqr(ltmp, lnew);
	return longbailout();
#else
	return 0;
#endif
}

int
SqrTrigfpFractal(void)
{
	/* SZSB(XYAXIS) { z = pixel, TEST = (p1 + 3): z = sin(z)*sin(z), |z|<TEST} */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXsqr(g_temp_z, g_new_z);
	return floatbailout();
}

int
Magnet1Fractal(void)    /*    Z = ((Z**2 + C - 1)/(2Z + C - 2))**2    */
{                   /*  In "Beauty of Fractals", code by Kev Allen. */
	_CMPLX top, bot, tmp;
	double div;

	top.x = tempsqrx - tempsqry + floatparm->x - 1; /* top = Z**2 + C-1 */
	top.y = g_old_z.x*g_old_z.y;
	top.y = top.y + top.y + floatparm->y;

	bot.x = g_old_z.x + g_old_z.x + floatparm->x - 2;       /* bot = 2*Z + C-2  */
	bot.y = g_old_z.y + g_old_z.y + floatparm->y;

	div = bot.x*bot.x + bot.y*bot.y;                /* tmp = top/bot  */
	if (div < FLT_MIN)
	{
		return 1;
	}
	tmp.x = (top.x*bot.x + top.y*bot.y)/div;
	tmp.y = (top.y*bot.x - top.x*bot.y)/div;

	g_new_z.x = (tmp.x + tmp.y)*(tmp.x - tmp.y);      /* Z = tmp**2     */
	g_new_z.y = tmp.x*tmp.y;
	g_new_z.y += g_new_z.y;

	return floatbailout();
}

/* Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2)) /      */
/*       (3Z**2 + 3(C-2)Z + (C-1)(C-2) + 1))**2  */
int Magnet2Fractal(void)
{
	/*   In "Beauty of Fractals", code by Kev Allen.  */
	_CMPLX top, bot, tmp;
	double div;

	top.x = g_old_z.x*(tempsqrx-tempsqry-tempsqry-tempsqry + T_Cm1.x)
			- g_old_z.y*T_Cm1.y + T_Cm1Cm2.x;
	top.y = g_old_z.y*(tempsqrx + tempsqrx + tempsqrx-tempsqry + T_Cm1.x)
			+ g_old_z.x*T_Cm1.y + T_Cm1Cm2.y;

	bot.x = tempsqrx - tempsqry;
	bot.x = bot.x + bot.x + bot.x
			+ g_old_z.x*T_Cm2.x - g_old_z.y*T_Cm2.y
			+ T_Cm1Cm2.x + 1.0;
	bot.y = g_old_z.x*g_old_z.y;
	bot.y += bot.y;
	bot.y = bot.y + bot.y + bot.y
			+ g_old_z.x*T_Cm2.y + g_old_z.y*T_Cm2.x
			+ T_Cm1Cm2.y;

	div = bot.x*bot.x + bot.y*bot.y;                /* tmp = top/bot  */
	if (div < FLT_MIN)
	{
		return 1;
	}
	tmp.x = (top.x*bot.x + top.y*bot.y)/div;
	tmp.y = (top.y*bot.x - top.x*bot.y)/div;

	g_new_z.x = (tmp.x + tmp.y)*(tmp.x - tmp.y);      /* Z = tmp**2     */
	g_new_z.y = tmp.x*tmp.y;
	g_new_z.y += g_new_z.y;

	return floatbailout();
}

int
LambdaTrigFractal(void)
{
#if !defined(XFRACT)
	LONGXYTRIGBAILOUT();
	LCMPLXtrig0(lold, ltmp);           /* ltmp = trig(lold)           */
	LCMPLXmult(*longparm, ltmp, lnew);   /* lnew = longparm*trig(lold)  */
	lold = lnew;
	return 0;
#else
	return 0;
#endif
}

int
LambdaTrigfpFractal(void)
{
	FLOATXYTRIGBAILOUT();
	CMPLXtrig0(g_old_z, g_temp_z);              /* tmp = trig(old)           */
	CMPLXmult(*floatparm, g_temp_z, g_new_z);   /* new = longparm*trig(old)  */
	g_old_z = g_new_z;
	return 0;
}

/* bailouts are different for different trig functions */
int
LambdaTrigFractal1(void)
{
#if !defined(XFRACT)
	LONGTRIGBAILOUT(); /* sin, cos */
	LCMPLXtrig0(lold, ltmp);           /* ltmp = trig(lold)           */
	LCMPLXmult(*longparm, ltmp, lnew);   /* lnew = longparm*trig(lold)  */
	lold = lnew;
	return 0;
#else
	return 0;
#endif
}

int
LambdaTrigfpFractal1(void)
{
	FLOATTRIGBAILOUT(); /* sin, cos */
	CMPLXtrig0(g_old_z, g_temp_z);              /* tmp = trig(old)           */
	CMPLXmult(*floatparm, g_temp_z, g_new_z);   /* new = longparm*trig(old)  */
	g_old_z = g_new_z;
	return 0;
}

int
LambdaTrigFractal2(void)
{
#if !defined(XFRACT)
	LONGHTRIGBAILOUT(); /* sinh, cosh */
	LCMPLXtrig0(lold, ltmp);           /* ltmp = trig(lold)           */
	LCMPLXmult(*longparm, ltmp, lnew);   /* lnew = longparm*trig(lold)  */
	lold = lnew;
	return 0;
#else
	return 0;
#endif
}

int
LambdaTrigfpFractal2(void)
{
#if !defined(XFRACT)
	FLOATHTRIGBAILOUT(); /* sinh, cosh */
	CMPLXtrig0(g_old_z, g_temp_z);              /* tmp = trig(old)           */
	CMPLXmult(*floatparm, g_temp_z, g_new_z);   /* new = longparm*trig(old)  */
	g_old_z = g_new_z;
	return 0;
#else
	return 0;
#endif
}

int
ManOWarFractal(void)
{
#if !defined(XFRACT)
	/* From Art Matrix via Lee Skinner */
	lnew.x  = ltempsqrx - ltempsqry + ltmp.x + longparm->x;
	lnew.y = multiply(lold.x, lold.y, bitshiftless1) + ltmp.y + longparm->y;
	ltmp = lold;
	return longbailout();
#else
	return 0;
#endif
}

int
ManOWarfpFractal(void)
{
	/* From Art Matrix via Lee Skinner */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */
	g_new_z.x = tempsqrx - tempsqry + g_temp_z.x + floatparm->x;
	g_new_z.y = 2.0*g_old_z.x*g_old_z.y + g_temp_z.y + floatparm->y;
	g_temp_z = g_old_z;
	return floatbailout();
}

/*
	MarksMandelPwr (XAXIS)
	{
		z = pixel, c = z ^ (z - 1):
			z = c*sqr(z) + pixel,
		|z| <= 4
	}
*/

int
MarksMandelPwrfpFractal(void)
{
	CMPLXtrig0(g_old_z, g_new_z);
	CMPLXmult(g_temp_z, g_new_z, g_new_z);
	g_new_z.x += floatparm->x;
	g_new_z.y += floatparm->y;
	return floatbailout();
}

int
MarksMandelPwrFractal(void)
{
#if !defined(XFRACT)
	LCMPLXtrig0(lold, lnew);
	LCMPLXmult(ltmp, lnew, lnew);
	lnew.x += longparm->x;
	lnew.y += longparm->y;
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
TimsErrorfpFractal(void)
{
	CMPLXtrig0(g_old_z, g_new_z);
	g_new_z.x = g_new_z.x*g_temp_z.x - g_new_z.y*g_temp_z.y;
	g_new_z.y = g_new_z.x*g_temp_z.y - g_new_z.y*g_temp_z.x;
	g_new_z.x += floatparm->x;
	g_new_z.y += floatparm->y;
	return floatbailout();
}

int
TimsErrorFractal(void)
{
#if !defined(XFRACT)
	LCMPLXtrig0(lold, lnew);
	lnew.x = multiply(lnew.x, ltmp.x, bitshift)-multiply(lnew.y, ltmp.y, bitshift);
	lnew.y = multiply(lnew.x, ltmp.y, bitshift)-multiply(lnew.y, ltmp.x, bitshift);
	lnew.x += longparm->x;
	lnew.y += longparm->y;
	return longbailout();
#else
	return 0;
#endif
}

int
CirclefpFractal(void)
{
	long i;
	i = (long)(param[0]*(tempsqrx + tempsqry));
	g_color_iter = i % colors;
	return 1;
}
/*
CirclelongFractal()
{
	long i;
	i = multiply(lparm.x, (ltempsqrx + ltempsqry), bitshift);
	i = i >> bitshift;
	g_color_iter = i % colors);
	return 1;
}
*/

/* -------------------------------------------------------------------- */
/*              Initialization (once per pixel) routines                                                */
/* -------------------------------------------------------------------- */

/* transform points with reciprocal function */
void invertz2(_CMPLX *z)
{
	z->x = dxpixel();
	z->y = dypixel();
	z->x -= g_f_x_center; z->y -= g_f_y_center;  /* Normalize values to center of circle */

	tempsqrx = sqr(z->x) + sqr(z->y);  /* Get old radius */
	tempsqrx = (fabs(tempsqrx) > FLT_MIN) ? (g_f_radius / tempsqrx) : FLT_MAX;
	z->x *= tempsqrx;
	z->y *= tempsqrx;      /* Perform inversion */
	z->x += g_f_x_center;
	z->y += g_f_y_center; /* Renormalize */
}

int long_julia_per_pixel(void)
{
#if !defined(XFRACT)
	/* integer julia types */
	/* lambda */
	/* barnsleyj1 */
	/* barnsleyj2 */
	/* sierpinski */
	if (g_invert)
	{
		/* invert */
		invertz2(&g_old_z);

		/* watch out for overflow */
		if (sqr(g_old_z.x) + sqr(g_old_z.y) >= 127)
		{
			g_old_z.x = 8;  /* value to bail out in one iteration */
			g_old_z.y = 8;
		}

		/* convert to fudged longs */
		lold.x = (long)(g_old_z.x*fudge);
		lold.y = (long)(g_old_z.y*fudge);
	}
	else
	{
		lold.x = lxpixel();
		lold.y = lypixel();
	}
	return 0;
#else
	return 0;
#endif
}

int long_richard8_per_pixel(void)
{
#if !defined(XFRACT)
	long_mandel_per_pixel();
	LCMPLXtrig1(*longparm, ltmp);
	LCMPLXmult(ltmp, lparm2, ltmp);
	return 1;
#else
	return 0;
#endif
}

int long_mandel_per_pixel(void)
{
#if !defined(XFRACT)
	/* integer mandel types */
	/* barnsleym1 */
	/* barnsleym2 */
	linit.x = lxpixel();
	if (save_release >= 2004)
	{
		linit.y = lypixel();
	}

	if (g_invert)
	{
		/* invert */
		invertz2(&g_initial_z);

		/* watch out for overflow */
		if (sqr(g_initial_z.x) + sqr(g_initial_z.y) >= 127)
		{
			g_initial_z.x = 8;  /* value to bail out in one iteration */
			g_initial_z.y = 8;
		}

		/* convert to fudged longs */
		linit.x = (long)(g_initial_z.x*fudge);
		linit.y = (long)(g_initial_z.y*fudge);
	}

	lold = (useinitorbit == 1) ? g_init_orbit_l : linit;

	lold.x += lparm.x;    /* initial pertubation of parameters set */
	lold.y += lparm.y;
	return 1; /* 1st iteration has been done */
#else
	return 0;
#endif
}

int julia_per_pixel(void)
{
	/* julia */

	if (g_invert)
	{
		/* invert */
		invertz2(&g_old_z);

		/* watch out for overflow */
		if (bitshift <= 24)
		{
			if (sqr(g_old_z.x) + sqr(g_old_z.y) >= 127)
			{
				g_old_z.x = 8;  /* value to bail out in one iteration */
				g_old_z.y = 8;
			}
		}
		else if (sqr(g_old_z.x) + sqr(g_old_z.y) >= 4.0)
		{
			g_old_z.x = 2;  /* value to bail out in one iteration */
			g_old_z.y = 2;
		}

		/* convert to fudged longs */
		lold.x = (long)(g_old_z.x*fudge);
		lold.y = (long)(g_old_z.y*fudge);
	}
	else
	{
		lold.x = lxpixel();
		lold.y = lypixel();
	}

	ltempsqrx = multiply(lold.x, lold.x, bitshift);
	ltempsqry = multiply(lold.y, lold.y, bitshift);
	ltmp = lold;
	return 0;
}

int
marks_mandelpwr_per_pixel(void)
{
#if !defined(XFRACT)
	mandel_per_pixel();
	ltmp = lold;
	ltmp.x -= fudge;
	LCMPLXpwr(lold, ltmp, ltmp);
	return 1;
#else
	return 0;
#endif
}

int mandel_per_pixel(void)
{
	/* mandel */

	if (g_invert)
	{
		invertz2(&g_initial_z);

		/* watch out for overflow */
		if (bitshift <= 24)
		{
			if (sqr(g_initial_z.x) + sqr(g_initial_z.y) >= 127)
			{
				g_initial_z.x = 8;  /* value to bail out in one iteration */
				g_initial_z.y = 8;
			}
		}
		else if (sqr(g_initial_z.x) + sqr(g_initial_z.y) >= 4)
		{
			g_initial_z.x = 2;  /* value to bail out in one iteration */
			g_initial_z.y = 2;
		}

		/* convert to fudged longs */
		linit.x = (long)(g_initial_z.x*fudge);
		linit.y = (long)(g_initial_z.y*fudge);
	}
	else
	{
		linit.x = lxpixel();
		if (save_release >= 2004)
		{
			linit.y = lypixel();
		}
	}
	switch (fractype)
	{
	case MANDELLAMBDA:              /* Critical Value 0.5 + 0.0i  */
		lold.x = FgHalf;
		lold.y = 0;
		break;
	default:
		lold = linit;
		break;
	}

	/* alter g_initial_z value */
	if (useinitorbit == 1)
	{
		lold = g_init_orbit_l;
	}
	else if (useinitorbit == 2)
	{
		lold = linit;
	}

	if ((inside == BOF60 || inside == BOF61) && !nobof)
	{
		/* kludge to match "Beauty of Fractals" picture since we start
			Mandelbrot iteration with g_initial_z rather than 0 */
		lold.x = lparm.x; /* initial pertubation of parameters set */
		lold.y = lparm.y;
		g_color_iter = -1;
	}
	else
	{
		lold.x += lparm.x; /* initial pertubation of parameters set */
		lold.y += lparm.y;
	}
	ltmp = linit; /* for spider */
	ltempsqrx = multiply(lold.x, lold.x, bitshift);
	ltempsqry = multiply(lold.y, lold.y, bitshift);
	return 1; /* 1st iteration has been done */
}

int marksmandel_per_pixel()
{
#if !defined(XFRACT)
	/* marksmandel */
	if (g_invert)
	{
		invertz2(&g_initial_z);

		/* watch out for overflow */
		if (sqr(g_initial_z.x) + sqr(g_initial_z.y) >= 127)
		{
			g_initial_z.x = 8;  /* value to bail out in one iteration */
			g_initial_z.y = 8;
		}

		/* convert to fudged longs */
		linit.x = (long)(g_initial_z.x*fudge);
		linit.y = (long)(g_initial_z.y*fudge);
	}
	else
	{
		linit.x = lxpixel();
		if (save_release >= 2004)
		{
			linit.y = lypixel();
		}
	}

	lold = (useinitorbit == 1) ? g_init_orbit_l : linit;

	lold.x += lparm.x;    /* initial pertubation of parameters set */
	lold.y += lparm.y;

	if (c_exp > 3)
	{
		lcpower(&lold, c_exp-1, &lcoefficient, bitshift);
	}
	else if (c_exp == 3)
	{
		lcoefficient.x = multiply(lold.x, lold.x, bitshift)
			- multiply(lold.y, lold.y, bitshift);
		lcoefficient.y = multiply(lold.x, lold.y, bitshiftless1);
	}
	else if (c_exp == 2)
	{
		lcoefficient = lold;
	}
	else if (c_exp < 2)
	{
		lcoefficient.x = 1L << bitshift;
		lcoefficient.y = 0L;
	}

	ltempsqrx = multiply(lold.x, lold.x, bitshift);
	ltempsqry = multiply(lold.y, lold.y, bitshift);
#endif
	return 1; /* 1st iteration has been done */
}

int marksmandelfp_per_pixel()
{
	/* marksmandel */

	if (g_invert)
	{
		invertz2(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		if (save_release >= 2004)
		{
			g_initial_z.y = dypixel();
		}
	}

	g_old_z = (useinitorbit == 1) ? initorbit : g_initial_z;

	g_old_z.x += parm.x;      /* initial pertubation of parameters set */
	g_old_z.y += parm.y;

	tempsqrx = sqr(g_old_z.x);
	tempsqry = sqr(g_old_z.y);

	if (c_exp > 3)
	{
		cpower(&g_old_z, c_exp-1, &coefficient);
	}
	else if (c_exp == 3)
	{
		coefficient.x = tempsqrx - tempsqry;
		coefficient.y = g_old_z.x*g_old_z.y*2;
	}
	else if (c_exp == 2)
	{
		coefficient = g_old_z;
	}
	else if (c_exp < 2)
	{
		coefficient.x = 1.0;
		coefficient.y = 0.0;
	}

	return 1; /* 1st iteration has been done */
}

int
marks_mandelpwrfp_per_pixel(void)
{
	mandelfp_per_pixel();
	g_temp_z = g_old_z;
	g_temp_z.x -= 1;
	CMPLXpwr(g_old_z, g_temp_z, g_temp_z);
	return 1;
}

int mandelfp_per_pixel(void)
{
	/* floating point mandelbrot */
	/* mandelfp */

	if (g_invert)
	{
		invertz2(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		if (save_release >= 2004)
		{
			g_initial_z.y = dypixel();
		}
	}
	switch (fractype)
	{
	case MAGNET2M:
		FloatPreCalcMagnet2();
	case MAGNET1M:           /* Crit Val Zero both, but neither   */
		g_old_z.x = g_old_z.y = 0.0; /* is of the form f(Z, C) = Z*g(Z) + C  */
		break;
	case MANDELLAMBDAFP:            /* Critical Value 0.5 + 0.0i  */
		g_old_z.x = 0.5;
		g_old_z.y = 0.0;
		break;
	default:
		g_old_z = g_initial_z;
		break;
	}

	/* alter g_initial_z value */
	if (useinitorbit == 1)
	{
		g_old_z = initorbit;
	}
	else if (useinitorbit == 2)
	{
		g_old_z = g_initial_z;
	}

	if ((inside == BOF60 || inside == BOF61) && !nobof)
	{
		/* kludge to match "Beauty of Fractals" picture since we start
			Mandelbrot iteration with g_initial_z rather than 0 */
		g_old_z.x = parm.x; /* initial pertubation of parameters set */
		g_old_z.y = parm.y;
		g_color_iter = -1;
	}
	else
	{
		g_old_z.x += parm.x;
		g_old_z.y += parm.y;
	}
	g_temp_z = g_initial_z; /* for spider */
	tempsqrx = sqr(g_old_z.x);  /* precalculated value for regular Mandelbrot */
	tempsqry = sqr(g_old_z.y);
	return 1; /* 1st iteration has been done */
}

int juliafp_per_pixel(void)
{
	/* floating point julia */
	/* juliafp */
	if (g_invert)
	{
		invertz2(&g_old_z);
	}
	else
	{
		g_old_z.x = dxpixel();
		g_old_z.y = dypixel();
	}
	tempsqrx = sqr(g_old_z.x);  /* precalculated value for regular Julia */
	tempsqry = sqr(g_old_z.y);
	g_temp_z = g_old_z;
	return 0;
}

int MPCjulia_per_pixel(void)
{
#if !defined(XFRACT)
	/* floating point julia */
	/* juliafp */
	if (g_invert)
	{
		invertz2(&g_old_z);
	}
	else
	{
		g_old_z.x = dxpixel();
		g_old_z.y = dypixel();
	}
	mpcold.x = *pd2MP(g_old_z.x);
	mpcold.y = *pd2MP(g_old_z.y);
	return 0;
#else
	return 0;
#endif
}

int
otherrichard8fp_per_pixel(void)
{
	othermandelfp_per_pixel();
	CMPLXtrig1(*floatparm, g_temp_z);
	CMPLXmult(g_temp_z, parm2, g_temp_z);
	return 1;
}

int othermandelfp_per_pixel(void)
{
	if (g_invert)
	{
		invertz2(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		if (save_release >= 2004)
		{
			g_initial_z.y = dypixel();
		}
	}

	g_old_z = (useinitorbit == 1) ? initorbit : g_initial_z;

	g_old_z.x += parm.x;      /* initial pertubation of parameters set */
	g_old_z.y += parm.y;

	return 1; /* 1st iteration has been done */
}

int MPCHalley_per_pixel(void)
{
#if !defined(XFRACT)
	/* MPC halley */
	if (g_invert)
	{
		invertz2(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		if (save_release >= 2004)
		{
			g_initial_z.y = dypixel();
		}
	}

	mpcold.x = *pd2MP(g_initial_z.x);
	mpcold.y = *pd2MP(g_initial_z.y);

	return 0;
#else
	return 0;
#endif
}

int Halley_per_pixel(void)
{
	if (g_invert)
	{
		invertz2(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		if (save_release >= 2004)
		{
			g_initial_z.y = dypixel();
		}
	}

	g_old_z = g_initial_z;

	return 0; /* 1st iteration is not done */
}

int otherjuliafp_per_pixel(void)
{
	if (g_invert)
	{
		invertz2(&g_old_z);
	}
	else
	{
		g_old_z.x = dxpixel();
		g_old_z.y = dypixel();
	}
	return 0;
}

#if 0
#define Q0 .113
#define Q1 .01
#else
#define Q0 0
#define Q1 0
#endif

int quaternionjulfp_per_pixel(void)
{
	g_old_z.x = dxpixel();
	g_old_z.y = dypixel();
	floatparm->x = param[4];
	floatparm->y = param[5];
	qc  = param[0];
	qci = param[1];
	qcj = param[2];
	qck = param[3];
	return 0;
}

int quaternionfp_per_pixel(void)
{
	g_old_z.x = 0;
	g_old_z.y = 0;
	floatparm->x = 0;
	floatparm->y = 0;
	qc  = dxpixel();
	qci = dypixel();
	qcj = param[2];
	qck = param[3];
	return 0;
}

int MarksCplxMandperp(void)
{
	if (g_invert)
	{
		invertz2(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		if (save_release >= 2004)
		{
			g_initial_z.y = dypixel();
		}
	}
	g_old_z.x = g_initial_z.x + parm.x; /* initial pertubation of parameters set */
	g_old_z.y = g_initial_z.y + parm.y;
	tempsqrx = sqr(g_old_z.x);  /* precalculated value */
	tempsqry = sqr(g_old_z.y);
	coefficient = ComplexPower(g_initial_z, pwr);
	return 1;
}

int long_phoenix_per_pixel(void)
{
#if !defined(XFRACT)
	if (g_invert)
	{
		/* invert */
		invertz2(&g_old_z);

		/* watch out for overflow */
		if (sqr(g_old_z.x) + sqr(g_old_z.y) >= 127)
		{
			g_old_z.x = 8;  /* value to bail out in one iteration */
			g_old_z.y = 8;
		}

		/* convert to fudged longs */
		lold.x = (long)(g_old_z.x*fudge);
		lold.y = (long)(g_old_z.y*fudge);
	}
	else
	{
		lold.x = lxpixel();
		lold.y = lypixel();
	}
	ltempsqrx = multiply(lold.x, lold.x, bitshift);
	ltempsqry = multiply(lold.y, lold.y, bitshift);
	ltmp2.x = 0; /* use ltmp2 as the complex Y value */
	ltmp2.y = 0;
	return 0;
#else
	return 0;
#endif
}

int phoenix_per_pixel(void)
{
	if (g_invert)
	{
		invertz2(&g_old_z);
	}
	else
	{
		g_old_z.x = dxpixel();
		g_old_z.y = dypixel();
	}
	tempsqrx = sqr(g_old_z.x);  /* precalculated value */
	tempsqry = sqr(g_old_z.y);
	tmp2.x = 0; /* use tmp2 as the complex Y value */
	tmp2.y = 0;
	return 0;
}
int long_mandphoenix_per_pixel(void)
{
#if !defined(XFRACT)
	linit.x = lxpixel();
	if (save_release >= 2004)
	{
		linit.y = lypixel();
	}

	if (g_invert)
	{
		/* invert */
		invertz2(&g_initial_z);

		/* watch out for overflow */
		if (sqr(g_initial_z.x) + sqr(g_initial_z.y) >= 127)
		{
			g_initial_z.x = 8;  /* value to bail out in one iteration */
			g_initial_z.y = 8;
		}

		/* convert to fudged longs */
		linit.x = (long)(g_initial_z.x*fudge);
		linit.y = (long)(g_initial_z.y*fudge);
	}

	lold = (useinitorbit == 1) ? g_init_orbit_l : linit;

	lold.x += lparm.x;    /* initial pertubation of parameters set */
	lold.y += lparm.y;
	ltempsqrx = multiply(lold.x, lold.x, bitshift);
	ltempsqry = multiply(lold.y, lold.y, bitshift);
	ltmp2.x = 0;
	ltmp2.y = 0;
	return 1; /* 1st iteration has been done */
#else
	return 0;
#endif
}
int mandphoenix_per_pixel(void)
{
	if (g_invert)
	{
		invertz2(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		if (save_release >= 2004)
		{
			g_initial_z.y = dypixel();
		}
	}

	g_old_z = (useinitorbit == 1) ? initorbit : g_initial_z;

	g_old_z.x += parm.x;      /* initial pertubation of parameters set */
	g_old_z.y += parm.y;
	tempsqrx = sqr(g_old_z.x);  /* precalculated value */
	tempsqry = sqr(g_old_z.y);
	tmp2.x = 0;
	tmp2.y = 0;
	return 1; /* 1st iteration has been done */
}

int
QuaternionFPFractal(void)
{
	double a0, a1, a2, a3, n0, n1, n2, n3;
	a0 = g_old_z.x;
	a1 = g_old_z.y;
	a2 = floatparm->x;
	a3 = floatparm->y;

	n0 = a0*a0-a1*a1-a2*a2-a3*a3 + qc;
	n1 = 2*a0*a1 + qci;
	n2 = 2*a0*a2 + qcj;
	n3 = 2*a0*a3 + qck;
	/* Check bailout */
	g_magnitude = a0*a0 + a1*a1 + a2*a2 + a3*a3;
	if (g_magnitude > g_rq_limit)
	{
		return 1;
	}
	g_old_z.x = g_new_z.x = n0;
	g_old_z.y = g_new_z.y = n1;
	floatparm->x = n2;
	floatparm->y = n3;
	return 0;
}

int
HyperComplexFPFractal(void)
{
	_HCMPLX hold, hnew;
	hold.x = g_old_z.x;
	hold.y = g_old_z.y;
	hold.z = floatparm->x;
	hold.t = floatparm->y;

/*   HComplexSqr(&hold, &hnew); */
	HComplexTrig0(&hold, &hnew);

	hnew.x += qc;
	hnew.y += qci;
	hnew.z += qcj;
	hnew.t += qck;

	g_old_z.x = g_new_z.x = hnew.x;
	g_old_z.y = g_new_z.y = hnew.y;
	floatparm->x = hnew.z;
	floatparm->y = hnew.t;

	/* Check bailout */
	g_magnitude = sqr(g_old_z.x) + sqr(g_old_z.y) + sqr(floatparm->x) + sqr(floatparm->y);
	if (g_magnitude > g_rq_limit)
	{
		return 1;
	}
	return 0;
}

int
VLfpFractal(void) /* Beauty of Fractals pp. 125 - 127 */
{
	double a, b, ab, half, u, w, xy;

	half = param[0] / 2.0;
	xy = g_old_z.x*g_old_z.y;
	u = g_old_z.x - xy;
	w = -g_old_z.y + xy;
	a = g_old_z.x + param[1]*u;
	b = g_old_z.y + param[1]*w;
	ab = a*b;
	g_new_z.x = g_old_z.x + half*(u + (a - ab));
	g_new_z.y = g_old_z.y + half*(w + (-b + ab));
	return floatbailout();
}

int
EscherfpFractal(void) /* Science of Fractal Images pp. 185, 187 */
{
	_CMPLX oldtest, newtest, testsqr;
	double testsize = 0.0;
	long testiter = 0;

	g_new_z.x = tempsqrx - tempsqry; /* standard Julia with C == (0.0, 0.0i) */
	g_new_z.y = 2.0*g_old_z.x*g_old_z.y;
	oldtest.x = g_new_z.x*15.0;    /* scale it */
	oldtest.y = g_new_z.y*15.0;
	testsqr.x = sqr(oldtest.x);  /* set up to test with user-specified ... */
	testsqr.y = sqr(oldtest.y);  /*    ... Julia as the target set */
	while (testsize <= g_rq_limit && testiter < maxit) /* nested Julia loop */
	{
		newtest.x = testsqr.x - testsqr.y + param[0];
		newtest.y = 2.0*oldtest.x*oldtest.y + param[1];
		testsqr.x = sqr(newtest.x);
		testsqr.y = sqr(newtest.y);
		testsize = testsqr.x + testsqr.y;
		oldtest = newtest;
		testiter++;
	}
	if (testsize > g_rq_limit) /* point not in target set  */
	{
		return floatbailout();
	}
	else /* make distinct level sets if point stayed in target set */
	{
		g_color_iter = ((3L*g_color_iter) % 255L) + 1L;
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

int MandelbrotMix4Setup(void)
{
	int sign_array = 0;
	A.x = param[0];
	A.y = 0.0;						/* a = real(p1),     */
	B.x = param[1];
	B.y = 0.0;						/* b = imag(p1),     */
	D.x = param[2];
	D.y = 0.0;						/* d = real(p2),     */
	F.x = param[3];
	F.y = 0.0;						/* f = imag(p2),     */
	K.x = param[4] + 1.0;
	K.y = 0.0;						/* k = real(p3) + 1,   */
	L.x = param[5] + 100.0;
	L.y = 0.0;						/* l = imag(p3) + 100, */
	CMPLXrecip(F, G);				/* g = 1/f,          */
	CMPLXrecip(D, H);				/* h = 1/d,          */
	CMPLXsub(F, B, g_temp_z);			/* tmp = f-b       */
	CMPLXrecip(g_temp_z, J);				/* j = 1/(f-b)     */
	CMPLXneg(A, g_temp_z);
	CMPLXmult(g_temp_z, B, g_temp_z);			/* z = (-a*b*g*h)^j, */
	CMPLXmult(g_temp_z, G, g_temp_z);
	CMPLXmult(g_temp_z, H, g_temp_z);

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
	if (g_temp_z.y == 0.0) /* we know tmp.y IS zero but ... */
	{
		switch (sign_array)
		{
		/*
			Add to this list the magic numbers of any cases
			in which the fractal does not match the formula version
		*/
		case 15: /* 1111 */
		case 10: /* 1010 */
		case  6: /* 0110 */
		case  5: /* 0101 */
		case  3: /* 0011 */
		case  0: /* 0000 */
			g_temp_z.y = -g_temp_z.y; /* swap sign bit */
		default: /* do nothing - remaining cases already OK */
			break;
		}
		/* in case our kludge failed, let the user fix it */
		if (DEBUGFLAG_SWAP_SIGN == debugflag)
		{
			g_temp_z.y = -g_temp_z.y;
		}
	}

	CMPLXpwr(g_temp_z, J, g_temp_z);   /* note: z is old */
	/* in case our kludge failed, let the user fix it */
	if (param[6] < 0.0)
	{
		g_temp_z.y = -g_temp_z.y;
	}

	if (bailout == 0)
	{
		g_rq_limit = L.x;
		g_rq_limit2 = g_rq_limit*g_rq_limit;
	}
	return 1;
}

int MandelbrotMix4fp_per_pixel(void)
{
	if (g_invert)
	{
		invertz2(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		g_initial_z.y = dypixel();
	}
	g_old_z = g_temp_z;
	CMPLXtrig0(g_initial_z, C);        /* c = fn1(pixel): */
	return 0; /* 1st iteration has been NOT been done */
}

int
MandelbrotMix4fpFractal(void) /* from formula by Jim Muth */
{
	/* z = k*((a*(z^b)) + (d*(z^f))) + c, */
	_CMPLX z_b, z_f;
	CMPLXpwr(g_old_z, B, z_b);     /* (z^b)     */
	CMPLXpwr(g_old_z, F, z_f);     /* (z^f)     */
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
 * the screen coordinates (col, row) at the current zoom corners
 * settings. The functions come in two flavors. One looks up the pixel
 * values using the precalculated grid arrays dx0, dx1, dy0, and dy1,
 * which has a speed advantage but is limited to MAXPIXELS image
 * dimensions. The other calculates the complex coordinates at a
 * cost of two additions and two multiplications for each component,
 * but works at any resolution.
 *
 * With Microsoft C's _fastcall keyword, the function call overhead
 * appears to be negligible. It also appears that the speed advantage
 * of the lookup vs the calculation is negligible on machines with
 * coprocessors. Bert Tyler's original implementation was designed for
 * machines with no coprocessor; on those machines the saving was
 * significant. For the time being, the table lookup capability will
 * be maintained.
 */

/* Real component, grid lookup version - requires dx0/dx1 arrays */
static double _fastcall dxpixel_grid(void)
{
	return dx0[g_col] + dx1[g_row];
}

/* Real component, calculation version - does not require arrays */
static double _fastcall dxpixel_calc(void)
{
	return (double) (xxmin + g_col*delxx + g_row*delxx2);
}

/* Imaginary component, grid lookup version - requires dy0/dy1 arrays */
static double _fastcall dypixel_grid(void)
{
	return dy0[g_row] + dy1[g_col];
}

/* Imaginary component, calculation version - does not require arrays */
static double _fastcall dypixel_calc(void)
{
	return (double)(yymax - g_row*delyy - g_col*delyy2);
}

/* Real component, grid lookup version - requires lx0/lx1 arrays */
static long _fastcall lxpixel_grid(void)
{
	return lx0[g_col] + lx1[g_row];
}

/* Real component, calculation version - does not require arrays */
static long _fastcall lxpixel_calc(void)
{
	return xmin + g_col*delx + g_row*delx2;
}

/* Imaginary component, grid lookup version - requires ly0/ly1 arrays */
static long _fastcall lypixel_grid(void)
{
	return ly0[g_row] + ly1[g_col];
}

/* Imaginary component, calculation version - does not require arrays */
static long _fastcall lypixel_calc(void)
{
	return ymax - g_row*dely - g_col*dely2;
}

double (_fastcall *dxpixel)(void) = dxpixel_calc;
double (_fastcall *dypixel)(void) = dypixel_calc;
long   (_fastcall *lxpixel)(void) = lxpixel_calc;
long   (_fastcall *lypixel)(void) = lypixel_calc;

void set_pixel_calc_functions(void)
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
