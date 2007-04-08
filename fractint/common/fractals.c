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
		1 if we're done. Results for integer fractals are left in 'g_new_z_l.x' and
		'g_new_z_l.y', for floating point fractals in 'new.x' and 'new.y'.

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
		stored in fractalspecific[fractype].calculate_type.per_image.

	Adding a new fractal type should be simply a matter of adding an item
	to the 'fractalspecific' structure, writing (or re-using one of the existing)
	an appropriate setup, per_image, per_pixel, and orbit routines.

	--------------------------------------------------------------------
*/

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

#define modulus(z)			(sqr((z).x) + sqr((z).y))
#define conjugate(pz)		((pz)->y = - (pz)->y)
#define distance(z1, z2)	(sqr((z1).x-(z2).x) + sqr((z1).y-(z2).y))
#define pMPsqr(z)			(*pMPmul((z), (z)))
#define MPdistance(z1, z2)	(*pMPadd(pMPsqr(*pMPsub((z1).x, (z2).x)), pMPsqr(*pMPsub((z1).y, (z2).y))))
							/* Distance of complex z from unit circle */
#define DIST1(z)			(((z).x-1.0)*((z).x-1.0) + ((z).y)*((z).y))
#define LDIST1(z)			(lsqr((((z).x)-fudge)) + lsqr(((z).y)))

_LCMPLX g_coefficient_l = { 0, 0 };
_LCMPLX g_old_z_l = { 0, 0 };
_LCMPLX g_new_z_l = { 0, 0 };
_LCMPLX g_parameter_l = { 0, 0 };
_LCMPLX g_initial_z_l = { 0, 0 };
_LCMPLX g_tmp_z_l = { 0, 0 };
_LCMPLX g_tmp_z2_l = { 0, 0 };
_LCMPLX g_parameter2_l = { 0, 0 };
long g_temp_sqr_x_l = 0;
long g_temp_sqr_y_l = 0;
int g_max_color = 0;
int g_root = 0;
int g_degree = 0;
int g_basin = 0;
double g_root_over_degree = 0.0;
double g_degree_minus_1_over_degree = 0.0;
double g_threshold = 0.0;
_CMPLX g_coefficient = { 0.0, 0.0 };
_CMPLX  g_static_roots[16] = { { 0.0, 0.0 } }; /* roots array for degree 16 or less */
_CMPLX  *g_roots = g_static_roots;
struct MPC *g_roots_mpc = NULL;
_CMPLX g_power = { 0.0, 0.0};
int g_bit_shift_minus_1 = 0;                  /* bit shift less 1 */
double g_two_pi = PI*2.0;
int g_c_exp = 0;
/* These are local but I don't want to pass them as parameters */
_CMPLX g_parameter = { 0, 0 };
_CMPLX g_parameter2 = { 0, 0 };
_CMPLX *g_float_parameter = NULL;
_LCMPLX *g_long_parameter = NULL; /* used here and in jb.c */
double g_cos_x = 0.0;
double g_sin_x = 0.0;
double g_temp_sqr_x = 0.0;
double g_temp_sqr_y = 0.0;
/* These are for quaternions */
double g_quaternion_c = 0.0;
double g_quaternion_ci = 0.0;
double g_quaternion_cj = 0.0;
double g_quaternion_ck = 0.0;
int (*g_bail_out_fp)(void);
int (*g_bail_out_l)(void);
int (*g_bail_out_bn)(void);
int (*g_bail_out_bf)(void);
long g_one_fudge = 0;
long g_two_fudge = 0;
int g_a_plus_1 = 0;
int g_a_plus_1_degree = 0;
struct MP g_a_plus_1_mp;
struct MP g_a_plus_1_degree_mp;
struct MPC g_temp_parameter_mpc;

/*
**  pre-calculated values for fractal types Magnet2M & Magnet2J
*/
static _CMPLX  s_3_c_minus_1 = { 0.0, 0.0 };		/* 3*(g_float_parameter - 1)                */
static _CMPLX  s_3_c_minus_2 = { 0.0, 0.0 };        /* 3*(g_float_parameter - 2)                */
static _CMPLX  s_c_minus_1_c_minus_2 = { 0.0, 0.0 }; /* (g_float_parameter - 1)*(g_float_parameter - 2) */
static _CMPLX s_temp2 = { 0.0, 0.0 };
static double s_cos_y = 0.0;
static double s_sin_y = 0.0;
static double s_temp_exp = 0.0;
static double s_old_x_init_x_fp = 0.0;
static double s_old_y_init_y_fp = 0.0;
static double s_old_x_init_y_fp = 0.0;
static double s_old_y_init_x_fp = 0.0;
static long s_old_x_init_x = 0;
static long s_old_y_init_y = 0;
static long s_old_x_init_y = 0;
static long s_old_y_init_x = 0;
/* temporary variables for trig use */
static long s_cos_x_l = 0;
static long s_sin_x_l = 0;
static long s_cos_y_l = 0;
static long s_sin_y_l = 0;
static double s_xt;
static double s_yt;
static double s_t2;

void magnet2_precalculate_fp(void) /* precalculation for Magnet2 (M & J) for speed */
{
	s_3_c_minus_1.x = g_float_parameter->x - 1.0;
	s_3_c_minus_1.y = g_float_parameter->y;
	s_3_c_minus_2.x = g_float_parameter->x - 2.0;
	s_3_c_minus_2.y = g_float_parameter->y;
	s_c_minus_1_c_minus_2.x = (s_3_c_minus_1.x*s_3_c_minus_2.x) - (s_3_c_minus_1.y*s_3_c_minus_2.y);
	s_c_minus_1_c_minus_2.y = (s_3_c_minus_1.x*s_3_c_minus_2.y) + (s_3_c_minus_1.y*s_3_c_minus_2.x);
	s_3_c_minus_1.x += s_3_c_minus_1.x + s_3_c_minus_1.x;
	s_3_c_minus_1.y += s_3_c_minus_1.y + s_3_c_minus_1.y;
	s_3_c_minus_2.x += s_3_c_minus_2.x + s_3_c_minus_2.x;
	s_3_c_minus_2.y += s_3_c_minus_2.y + s_3_c_minus_2.y;
}

/* -------------------------------------------------------------------- */
/*              Bailout Routines Macros                                                                                                 */
/* -------------------------------------------------------------------- */
int bail_out_mod_fp(void)
{
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	if (g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_real_fp(void)
{
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	if (g_temp_sqr_x >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_imag_fp(void)
{
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	if (g_temp_sqr_y >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_or_fp(void)
{
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	if (g_temp_sqr_x >= g_rq_limit || g_temp_sqr_y >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_and_fp(void)
{
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	if (g_temp_sqr_x >= g_rq_limit && g_temp_sqr_y >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_manhattan_fp(void)
{
	double manhmag;
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	manhmag = fabs(g_new_z.x) + fabs(g_new_z.y);
	if ((manhmag*manhmag) >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_manhattan_r_fp(void)
{
	double manrmag;
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	manrmag = g_new_z.x + g_new_z.y; /* don't need abs() since we square it next */
	if ((manrmag*manrmag) >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

#define BAIL_OUT_TRIG_FP()		\
	if (fabs(g_old_z.y) >= g_rq_limit2)	\
	{							\
		return 1;				\
	}

#define BAIL_OUT_TRIG_LONG()			\
	if (labs(g_old_z_l.y) >= g_limit2_l)	\
	{								\
		return 1;					\
	}

#define BAIL_OUT_TRIG_XY_LONG()									\
	if (labs(g_old_z_l.x) >= g_limit2_l || labs(g_old_z_l.y) >= g_limit2_l)	\
	{														\
		return 1;											\
	}

#define BAIL_OUT_TRIG_XY_FP()							\
	if (fabs(g_old_z.x) >= g_rq_limit2 || fabs(g_old_z.y) >= g_rq_limit2)	\
	{													\
		return 1;										\
	}

#define BAIL_OUT_TRIG_H_FP()		\
	if (fabs(g_old_z.x) >= g_rq_limit2)	\
	{							\
		return 1;				\
	}

#define BAIL_OUT_TRIG_H_LONG()  \
	if (labs(g_old_z_l.x) >= g_limit2_l) \
	{ \
		return 1; \
	}

#define BAIL_OUT_TRIG16(_x)  \
	if (labs(_x) > TRIG_LIMIT_16) \
	{ \
		return 1; \
	}

#define BAIL_OUT_EXP_OLD_FP()	\
	if (fabs(g_old_z.y) >= 1.0e8)	\
	{							\
		return 1;				\
	}							\
	if (fabs(g_old_z.x) >= 6.4e2)	\
	{							\
		return 1;				\
	}

#define BAIL_OUT_EXP_FP()		\
	if (fabs(g_old_z.y) >= 1.0e3)	\
	{							\
		return 1;				\
	}							\
	if (fabs(g_old_z.x) >= 8)		\
	{							\
		return 1;				\
	}

#define BAIL_OUT_EXP_LONG()					\
	if (labs(g_old_z_l.y) >= (1000L << bitshift))\
	{										\
		return 1;							\
	}										\
	if (labs(g_old_z_l.x) >=    (8L << bitshift))\
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

#define TRIG_ARG_L(_x)    \
	if (labs(_x) > TRIG_LIMIT_16)\
	{\
		double tmp = (_x); \
		tmp /= fudge; \
		tmp = fmod(tmp, g_two_pi); \
		tmp *= fudge; \
		(_x) = (long) tmp; \
	}\

static int bail_out_halley(void)
{
	if (fabs(modulus(g_new_z)-modulus(g_old_z)) < g_parameter2.x)
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

static int bail_out_halley_mpc(void)
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

/* Raise complex number (base) to the (exp) power, storing the result
** in complex (result).
*/
void complex_power(_CMPLX *base, int exp, _CMPLX *result)
{
	if (exp < 0)
	{
		complex_power(base, -exp, result);
		CMPLXrecip(*result, *result);
		return;
	}

	s_xt = base->x;
	s_yt = base->y;

	if (exp & 1)
	{
		result->x = s_xt;
		result->y = s_yt;
	}
	else
	{
		result->x = 1.0;
		result->y = 0.0;
	}

	exp >>= 1;
	while (exp)
	{
		s_t2 = s_xt*s_xt - s_yt*s_yt;
		s_yt = 2*s_xt*s_yt;
		s_xt = s_t2;

		if (exp & 1)
		{
				s_t2 = s_xt*result->x - s_yt*result->y;
				result->y = result->y*s_xt + s_yt*result->x;
				result->x = s_t2;
		}
		exp >>= 1;
	}
}

#if !defined(XFRACT)
/* long version */
static long lxt, lyt, lt2;
int
complex_power_l(_LCMPLX *base, int exp, _LCMPLX *result, int bitshift)
{
	static long maxarg;
	maxarg = 64L << bitshift;

	if (exp < 0)
	{
		overflow = complex_power_l(base, -exp, result, bitshift);
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
		lyt = multiply(lxt, lyt, g_bit_shift_minus_1);
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

	s_temp_exp = exp(tmp2.x);

	FPUsincos(&tmp2.y, &s_sin_y, &s_cos_y);
	out->x = s_temp_exp*s_cos_y;
	out->y = s_temp_exp*s_sin_y;
	return errno_xxx;
}
#endif
#endif

static int complex_multiply(_CMPLX arg1, _CMPLX arg2, _CMPLX *pz)
{
	pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
	pz->y = arg1.x*arg2.y + arg1.y*arg2.x;
	return 0;
}

int newton2_orbit(void)
{
	static char start = 1;
	if (start)
	{
		start = 0;
	}
	complex_power(&g_old_z, g_degree-1, &g_temp_z);
	complex_multiply(g_temp_z, g_old_z, &g_new_z);

	if (DIST1(g_new_z) < g_threshold)
	{
		if (fractype == NEWTBASIN || fractype == MPNEWTBASIN)
		{
			long tmpcolor;
			int i;
			tmpcolor = -1;
			/* this code determines which degree-th root of root the
				Newton formula converges to. The roots of a 1 are
				distributed on a circle of radius 1 about the origin. */
			for (i = 0; i < g_degree; i++)
			{
				/* color in alternating shades with iteration according to
					which root of 1 it converged to */
				if (distance(g_roots[i], g_old_z) < g_threshold)
				{
					tmpcolor = (g_basin == 2) ?
						(1 + (i&7) + ((g_color_iter&1) << 3)) : (1 + i);
					break;
				}
			}
			g_color_iter = (tmpcolor == -1) ? g_max_color : tmpcolor;
		}
		return 1;
	}
	g_new_z.x = g_degree_minus_1_over_degree*g_new_z.x + g_root_over_degree;
	g_new_z.y *= g_degree_minus_1_over_degree;

	/* Watch for divide underflow */
	s_t2 = g_temp_z.x*g_temp_z.x + g_temp_z.y*g_temp_z.y;
	if (s_t2 < FLT_MIN)
	{
		return 1;
	}
	else
	{
		s_t2 = 1.0 / s_t2;
		g_old_z.x = s_t2*(g_new_z.x*g_temp_z.x + g_new_z.y*g_temp_z.y);
		g_old_z.y = s_t2*(g_new_z.y*g_temp_z.x - g_new_z.x*g_temp_z.y);
	}
	return 0;
}

#if !defined(XFRACT)
struct MP mproverd, mpd1overd, mpthreshold;
struct MP mpt2;
struct MP mpone;
#endif

int newton_orbit_mpc(void)
{
#if !defined(XFRACT)
	MPOverflow = 0;
	mpctmp   = MPCpow(mpcold, g_degree-1);

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
			for (i = 0; i < g_degree; i++)
			{
				if (pMPcmp(MPdistance(g_roots_mpc[i], mpcold), mpthreshold) < 0)
				{
					tmpcolor = (g_basin == 2) ?
						(1 + (i&7) + ((g_color_iter&1) << 3)) : (1 + i);
					break;
				}
			}
			g_color_iter = (tmpcolor == -1) ? g_max_color : tmpcolor;
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

int barnsley1_orbit(void)
{
#if !defined(XFRACT)
	/* Barnsley's Mandelbrot type M1 from "Fractals
	Everywhere" by Michael Barnsley, p. 322 */

	/* calculate intermediate products */
	s_old_x_init_x   = multiply(g_old_z_l.x, g_long_parameter->x, bitshift);
	s_old_y_init_y   = multiply(g_old_z_l.y, g_long_parameter->y, bitshift);
	s_old_x_init_y   = multiply(g_old_z_l.x, g_long_parameter->y, bitshift);
	s_old_y_init_x   = multiply(g_old_z_l.y, g_long_parameter->x, bitshift);
	/* orbit calculation */
	if (g_old_z_l.x >= 0)
	{
		g_new_z_l.x = (s_old_x_init_x - g_long_parameter->x - s_old_y_init_y);
		g_new_z_l.y = (s_old_y_init_x - g_long_parameter->y + s_old_x_init_y);
	}
	else
	{
		g_new_z_l.x = (s_old_x_init_x + g_long_parameter->x - s_old_y_init_y);
		g_new_z_l.y = (s_old_y_init_x + g_long_parameter->y + s_old_x_init_y);
	}
	return g_bail_out_l();
#else
	return 0;
#endif
}

int barnsley1_orbit_fp(void)
{
	/* Barnsley's Mandelbrot type M1 from "Fractals
	Everywhere" by Michael Barnsley, p. 322 */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

	/* calculate intermediate products */
	s_old_x_init_x_fp = g_old_z.x*g_float_parameter->x;
	s_old_y_init_y_fp = g_old_z.y*g_float_parameter->y;
	s_old_x_init_y_fp = g_old_z.x*g_float_parameter->y;
	s_old_y_init_x_fp = g_old_z.y*g_float_parameter->x;
	/* orbit calculation */
	if (g_old_z.x >= 0)
	{
		g_new_z.x = (s_old_x_init_x_fp - g_float_parameter->x - s_old_y_init_y_fp);
		g_new_z.y = (s_old_y_init_x_fp - g_float_parameter->y + s_old_x_init_y_fp);
	}
	else
	{
		g_new_z.x = (s_old_x_init_x_fp + g_float_parameter->x - s_old_y_init_y_fp);
		g_new_z.y = (s_old_y_init_x_fp + g_float_parameter->y + s_old_x_init_y_fp);
	}
	return g_bail_out_fp();
}

int barnsley2_orbit(void)
{
#if !defined(XFRACT)
	/* An unnamed Mandelbrot/Julia function from "Fractals
	Everywhere" by Michael Barnsley, p. 331, example 4.2 */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

	/* calculate intermediate products */
	s_old_x_init_x   = multiply(g_old_z_l.x, g_long_parameter->x, bitshift);
	s_old_y_init_y   = multiply(g_old_z_l.y, g_long_parameter->y, bitshift);
	s_old_x_init_y   = multiply(g_old_z_l.x, g_long_parameter->y, bitshift);
	s_old_y_init_x   = multiply(g_old_z_l.y, g_long_parameter->x, bitshift);

	/* orbit calculation */
	if (s_old_x_init_y + s_old_y_init_x >= 0)
	{
		g_new_z_l.x = s_old_x_init_x - g_long_parameter->x - s_old_y_init_y;
		g_new_z_l.y = s_old_y_init_x - g_long_parameter->y + s_old_x_init_y;
	}
	else
	{
		g_new_z_l.x = s_old_x_init_x + g_long_parameter->x - s_old_y_init_y;
		g_new_z_l.y = s_old_y_init_x + g_long_parameter->y + s_old_x_init_y;
	}
	return g_bail_out_l();
#else
	return 0;
#endif
}

int barnsley2_orbit_fp(void)
{
	/* An unnamed Mandelbrot/Julia function from "Fractals
	Everywhere" by Michael Barnsley, p. 331, example 4.2 */

	/* calculate intermediate products */
	s_old_x_init_x_fp = g_old_z.x*g_float_parameter->x;
	s_old_y_init_y_fp = g_old_z.y*g_float_parameter->y;
	s_old_x_init_y_fp = g_old_z.x*g_float_parameter->y;
	s_old_y_init_x_fp = g_old_z.y*g_float_parameter->x;

	/* orbit calculation */
	if (s_old_x_init_y_fp + s_old_y_init_x_fp >= 0)
	{
		g_new_z.x = s_old_x_init_x_fp - g_float_parameter->x - s_old_y_init_y_fp;
		g_new_z.y = s_old_y_init_x_fp - g_float_parameter->y + s_old_x_init_y_fp;
	}
	else
	{
		g_new_z.x = s_old_x_init_x_fp + g_float_parameter->x - s_old_y_init_y_fp;
		g_new_z.y = s_old_y_init_x_fp + g_float_parameter->y + s_old_x_init_y_fp;
	}
	return g_bail_out_fp();
}

int julia_orbit(void)
{
	/* used for C prototype of fast integer math routines for classic
		Mandelbrot and Julia */
	g_new_z_l.x  = g_temp_sqr_x_l - g_temp_sqr_y_l + g_long_parameter->x;
	g_new_z_l.y = multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift_minus_1) + g_long_parameter->y;
	return g_bail_out_l();
}

int julia_orbit_fp(void)
{
	/* floating point version of classical Mandelbrot/Julia */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */
	g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_float_parameter->x;
	g_new_z.y = 2.0*g_old_z.x*g_old_z.y + g_float_parameter->y;
	return g_bail_out_fp();
}

int lambda_orbit_fp(void)
{
	/* variation of classical Mandelbrot/Julia */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

	g_temp_sqr_x = g_old_z.x - g_temp_sqr_x + g_temp_sqr_y;
	g_temp_sqr_y = -(g_old_z.y*g_old_z.x);
	g_temp_sqr_y += g_temp_sqr_y + g_old_z.y;

	g_new_z.x = g_float_parameter->x*g_temp_sqr_x - g_float_parameter->y*g_temp_sqr_y;
	g_new_z.y = g_float_parameter->x*g_temp_sqr_y + g_float_parameter->y*g_temp_sqr_x;
	return g_bail_out_fp();
}

int lambda_orbit(void)
{
#if !defined(XFRACT)
	/* variation of classical Mandelbrot/Julia */

	/* in complex math) temp = Z*(1-Z) */
	g_temp_sqr_x_l = g_old_z_l.x - g_temp_sqr_x_l + g_temp_sqr_y_l;
	g_temp_sqr_y_l = g_old_z_l.y - multiply(g_old_z_l.y, g_old_z_l.x, g_bit_shift_minus_1);
	/* (in complex math) Z = Lambda*Z */
	g_new_z_l.x = multiply(g_long_parameter->x, g_temp_sqr_x_l, bitshift)
		- multiply(g_long_parameter->y, g_temp_sqr_y_l, bitshift);
	g_new_z_l.y = multiply(g_long_parameter->x, g_temp_sqr_y_l, bitshift)
		+ multiply(g_long_parameter->y, g_temp_sqr_x_l, bitshift);
	return g_bail_out_l();
#else
	return 0;
#endif
}

int sierpinski_orbit(void)
{
#if !defined(XFRACT)
	/* following code translated from basic - see "Fractals
	Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */
	g_new_z_l.x = (g_old_z_l.x << 1);              /* new.x = 2*old.x  */
	g_new_z_l.y = (g_old_z_l.y << 1);              /* new.y = 2*old.y  */
	if (g_old_z_l.y > g_tmp_z_l.y)  /* if old.y > .5 */
	{
		g_new_z_l.y = g_new_z_l.y - g_tmp_z_l.x; /* new.y = 2*old.y - 1 */
	}
	else if (g_old_z_l.x > g_tmp_z_l.y)     /* if old.x > .5 */
	{
		g_new_z_l.x = g_new_z_l.x - g_tmp_z_l.x; /* new.x = 2*old.x - 1 */
	}
	/* end barnsley code */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int sierpinski_orbit_fp(void)
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
	return g_bail_out_fp();
}

int lambda_exponent_orbit_fp(void)
{
	/* found this in  "Science of Fractal Images" */
	if (save_release > 2002)  /* need braces since these are macros */
	{
		BAIL_OUT_EXP_FP();
	}
	else
	{
		BAIL_OUT_EXP_OLD_FP();
	}
	FPUsincos  (&g_old_z.y, &s_sin_y, &s_cos_y);

	if (g_old_z.x >= g_rq_limit && s_cos_y >= 0.0)
	{
		return 1;
	}
	s_temp_exp = exp(g_old_z.x);
	g_temp_z.x = s_temp_exp*s_cos_y;
	g_temp_z.y = s_temp_exp*s_sin_y;

	/*multiply by lamda */
	g_new_z.x = g_float_parameter->x*g_temp_z.x - g_float_parameter->y*g_temp_z.y;
	g_new_z.y = g_float_parameter->y*g_temp_z.x + g_float_parameter->x*g_temp_z.y;
	g_old_z = g_new_z;
	return 0;
}

int lambda_exponent_orbit(void)
{
#if !defined(XFRACT)
	long tmp;
	/* found this in  "Science of Fractal Images" */
	BAIL_OUT_EXP_LONG();

	SinCos086  (g_old_z_l.y, &s_sin_y_l,  &s_cos_y_l);

	if (g_old_z_l.x >= g_limit_l && s_cos_y_l >= 0L)
	{
		return 1;
	}
	tmp = Exp086(g_old_z_l.x);

	g_tmp_z_l.x = multiply(tmp,      s_cos_y_l,   bitshift);
	g_tmp_z_l.y = multiply(tmp,      s_sin_y_l,   bitshift);

	g_new_z_l.x  = multiply(g_long_parameter->x, g_tmp_z_l.x, bitshift)
			- multiply(g_long_parameter->y, g_tmp_z_l.y, bitshift);
	g_new_z_l.y  = multiply(g_long_parameter->x, g_tmp_z_l.y, bitshift)
			+ multiply(g_long_parameter->y, g_tmp_z_l.x, bitshift);
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int trig_plus_exponent_orbit_fp(void)
{
	/* another Scientific American biomorph type */
	/* z(n + 1) = e**z(n) + trig(z(n)) + C */

	if (fabs(g_old_z.x) >= 6.4e2) /* DOMAIN errors  */
	{
		return 1;
	}
	s_temp_exp = exp(g_old_z.x);
	FPUsincos  (&g_old_z.y, &s_sin_y, &s_cos_y);
	CMPLXtrig0(g_old_z, g_new_z);

	/*new =   trig(old) + e**old + C  */
	g_new_z.x += s_temp_exp*s_cos_y + g_float_parameter->x;
	g_new_z.y += s_temp_exp*s_sin_y + g_float_parameter->y;
	return g_bail_out_fp();
}

int trig_plus_exponent_orbit(void)
{
#if !defined(XFRACT)
	/* calculate exp(z) */
	long tmp;

	/* domain check for fast transcendental functions */
	BAIL_OUT_TRIG16(g_old_z_l.x);
	BAIL_OUT_TRIG16(g_old_z_l.y);

	tmp = Exp086(g_old_z_l.x);
	SinCos086  (g_old_z_l.y, &s_sin_y_l,  &s_cos_y_l);
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	g_new_z_l.x += multiply(tmp,    s_cos_y_l,   bitshift) + g_long_parameter->x;
	g_new_z_l.y += multiply(tmp,    s_sin_y_l,   bitshift) + g_long_parameter->y;
	return g_bail_out_l();
#else
	return 0;
#endif
}

int marks_lambda_orbit(void)
{
	/* Mark Peterson's variation of "lambda" function */

	/* Z1 = (C^(exp-1)*Z**2) + C */
#if !defined(XFRACT)
	g_tmp_z_l.x = g_temp_sqr_x_l - g_temp_sqr_y_l;
	g_tmp_z_l.y = multiply(g_old_z_l.x , g_old_z_l.y , g_bit_shift_minus_1);

	g_new_z_l.x = multiply(g_coefficient_l.x, g_tmp_z_l.x, bitshift)
		- multiply(g_coefficient_l.y, g_tmp_z_l.y, bitshift) + g_long_parameter->x;
	g_new_z_l.y = multiply(g_coefficient_l.x, g_tmp_z_l.y, bitshift)
		+ multiply(g_coefficient_l.y, g_tmp_z_l.x, bitshift) + g_long_parameter->y;

	return g_bail_out_l();
#else
	return 0;
#endif
}

int marks_lambda_orbit_fp(void)
{
	/* Mark Peterson's variation of "lambda" function */

	/* Z1 = (C^(exp-1)*Z**2) + C */
	g_temp_z.x = g_temp_sqr_x - g_temp_sqr_y;
	g_temp_z.y = g_old_z.x*g_old_z.y *2;

	g_new_z.x = g_coefficient.x*g_temp_z.x - g_coefficient.y*g_temp_z.y + g_float_parameter->x;
	g_new_z.y = g_coefficient.x*g_temp_z.y + g_coefficient.y*g_temp_z.x + g_float_parameter->y;

	return g_bail_out_fp();
}

int unity_orbit(void)
{
#if !defined(XFRACT)
	/* brought to you by Mark Peterson - you won't find this in any fractal
		books unless they saw it here first - Mark invented it! */
	long XXOne = multiply(g_old_z_l.x, g_old_z_l.x, bitshift) + multiply(g_old_z_l.y, g_old_z_l.y, bitshift);
	if ((XXOne > g_two_fudge) || (labs(XXOne - g_one_fudge) < delmin))
	{
		return 1;
	}
	g_old_z_l.y = multiply(g_two_fudge - XXOne, g_old_z_l.x, bitshift);
	g_old_z_l.x = multiply(g_two_fudge - XXOne, g_old_z_l.y, bitshift);
	g_new_z_l = g_old_z_l;  /* TW added this line */
	return 0;
#else
	return 0;
#endif
}

int unity_orbit_fp(void)
{
	/* brought to you by Mark Peterson - you won't find this in any fractal
		books unless they saw it here first - Mark invented it! */
	double XXOne = sqr(g_old_z.x) + sqr(g_old_z.y);
	if ((XXOne > 2.0) || (fabs(XXOne - 1.0) < ddelmin))
	{
		return 1;
	}
	g_old_z.y = (2.0 - XXOne)* g_old_z.x;
	g_old_z.x = (2.0 - XXOne)* g_old_z.y;
	g_new_z = g_old_z;  /* TW added this line */
	return 0;
}

int mandel4_orbit(void)
{
	/* By writing this code, Bert has left behind the excuse "don't
		know what a fractal is, just know how to make'em go fast".
		Bert is hereby declared a bonafide fractal expert! Supposedly
		this routine calculates the Mandelbrot/Julia set based on the
		polynomial z**4 + lambda, but I wouldn't know -- can't follow
		all that integer math speedup stuff - Tim */

	/* first, compute (x + iy)**2 */
#if !defined(XFRACT)
	g_new_z_l.x  = g_temp_sqr_x_l - g_temp_sqr_y_l;
	g_new_z_l.y = multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift_minus_1);
	if (g_bail_out_l())
	{
		return 1;
	}

	/* then, compute ((x + iy)**2)**2 + lambda */
	g_new_z_l.x  = g_temp_sqr_x_l - g_temp_sqr_y_l + g_long_parameter->x;
	g_new_z_l.y = multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift_minus_1) + g_long_parameter->y;
	return g_bail_out_l();
#else
	return 0;
#endif
}

int mandel4_orbit_fp(void)
{
	/* first, compute (x + iy)**2 */
	g_new_z.x  = g_temp_sqr_x - g_temp_sqr_y;
	g_new_z.y = g_old_z.x*g_old_z.y*2;
	if (g_bail_out_fp())
	{
		return 1;
	}

	/* then, compute ((x + iy)**2)**2 + lambda */
	g_new_z.x  = g_temp_sqr_x - g_temp_sqr_y + g_float_parameter->x;
	g_new_z.y =  g_old_z.x*g_old_z.y*2 + g_float_parameter->y;
	return g_bail_out_fp();
}

int z_to_z_plus_z_orbit_fp(void)
{
	complex_power(&g_old_z, (int)param[2], &g_new_z);
	g_old_z = ComplexPower(g_old_z, g_old_z);
	g_new_z.x = g_new_z.x + g_old_z.x +g_float_parameter->x;
	g_new_z.y = g_new_z.y + g_old_z.y +g_float_parameter->y;
	return g_bail_out_fp();
}

int z_power_orbit(void)
{
#if !defined(XFRACT)
	if (complex_power_l(&g_old_z_l, g_c_exp, &g_new_z_l, bitshift))
	{
		g_new_z_l.x = g_new_z_l.y = 8L << bitshift;
	}
	g_new_z_l.x += g_long_parameter->x;
	g_new_z_l.y += g_long_parameter->y;
	return g_bail_out_l();
#else
	return 0;
#endif
}

int complex_z_power_orbit(void)
{
#if !defined(XFRACT)
	_CMPLX x, y;

	x.x = (double)g_old_z_l.x / fudge;
	x.y = (double)g_old_z_l.y / fudge;
	y.x = (double)g_parameter2_l.x / fudge;
	y.y = (double)g_parameter2_l.y / fudge;
	x = ComplexPower(x, y);
	if (fabs(x.x) < fgLimit && fabs(x.y) < fgLimit)
	{
		g_new_z_l.x = (long)(x.x*fudge);
		g_new_z_l.y = (long)(x.y*fudge);
	}
	else
	{
		overflow = 1;
	}
	g_new_z_l.x += g_long_parameter->x;
	g_new_z_l.y += g_long_parameter->y;
	return g_bail_out_l();
#else
	return 0;
#endif
}

int z_power_orbit_fp(void)
{
	complex_power(&g_old_z, g_c_exp, &g_new_z);
	g_new_z.x += g_float_parameter->x;
	g_new_z.y += g_float_parameter->y;
	return g_bail_out_fp();
}

int complex_z_power_orbit_fp(void)
{
	g_new_z = ComplexPower(g_old_z, g_parameter2);
	g_new_z.x += g_float_parameter->x;
	g_new_z.y += g_float_parameter->y;
	return g_bail_out_fp();
}

int barnsley3_orbit(void)
{
	/* An unnamed Mandelbrot/Julia function from "Fractals
	Everywhere" by Michael Barnsley, p. 292, example 4.1 */

	/* calculate intermediate products */
#if !defined(XFRACT)
	s_old_x_init_x   = multiply(g_old_z_l.x, g_old_z_l.x, bitshift);
	s_old_y_init_y   = multiply(g_old_z_l.y, g_old_z_l.y, bitshift);
	s_old_x_init_y   = multiply(g_old_z_l.x, g_old_z_l.y, bitshift);

	/* orbit calculation */
	if (g_old_z_l.x > 0)
	{
		g_new_z_l.x = s_old_x_init_x   - s_old_y_init_y - fudge;
		g_new_z_l.y = s_old_x_init_y << 1;
	}
	else
	{
		g_new_z_l.x = s_old_x_init_x - s_old_y_init_y - fudge
			+ multiply(g_long_parameter->x, g_old_z_l.x, bitshift);
		g_new_z_l.y = s_old_x_init_y <<1;

		/* This term added by Tim Wegner to make dependent on the
			imaginary part of the parameter. (Otherwise Mandelbrot
			is uninteresting. */
		g_new_z_l.y += multiply(g_long_parameter->y, g_old_z_l.x, bitshift);
	}
	return g_bail_out_l();
#else
	return 0;
#endif
}

/*
	An unnamed Mandelbrot/Julia function from "Fractals
	Everywhere" by Michael Barnsley, p. 292, example 4.1
*/
int barnsley3_orbit_fp(void)
{
	/* calculate intermediate products */
	s_old_x_init_x_fp  = g_old_z.x*g_old_z.x;
	s_old_y_init_y_fp  = g_old_z.y*g_old_z.y;
	s_old_x_init_y_fp  = g_old_z.x*g_old_z.y;

	/* orbit calculation */
	if (g_old_z.x > 0)
	{
		g_new_z.x = s_old_x_init_x_fp - s_old_y_init_y_fp - 1.0;
		g_new_z.y = s_old_x_init_y_fp*2;
	}
	else
	{
		g_new_z.x = s_old_x_init_x_fp - s_old_y_init_y_fp -1.0 + g_float_parameter->x*g_old_z.x;
		g_new_z.y = s_old_x_init_y_fp*2;

		/* This term added by Tim Wegner to make dependent on the
			imaginary part of the parameter. (Otherwise Mandelbrot
			is uninteresting. */
		g_new_z.y += g_float_parameter->y*g_old_z.x;
	}
	return g_bail_out_fp();
}

/* From Scientific American, July 1989 */
/* A Biomorph                          */
/* z(n + 1) = trig(z(n)) + z(n)**2 + C       */
int trig_plus_z_squared_orbit(void)
{
#if !defined(XFRACT)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	g_new_z_l.x += g_temp_sqr_x_l - g_temp_sqr_y_l + g_long_parameter->x;
	g_new_z_l.y += multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift_minus_1) + g_long_parameter->y;
	return g_bail_out_l();
#else
	return 0;
#endif
}

/* From Scientific American, July 1989 */
/* A Biomorph                          */
/* z(n + 1) = trig(z(n)) + z(n)**2 + C       */
int trig_plus_z_squared_orbit_fp(void)
{
	CMPLXtrig0(g_old_z, g_new_z);
	g_new_z.x += g_temp_sqr_x - g_temp_sqr_y + g_float_parameter->x;
	g_new_z.y += 2.0*g_old_z.x*g_old_z.y + g_float_parameter->y;
	return g_bail_out_fp();
}

/*  Richard8 {c = z = pixel: z = sin(z) + sin(pixel), |z| <= 50} */
int richard8_orbit_fp(void)
{
	CMPLXtrig0(g_old_z, g_new_z);
	/* CMPLXtrig1(*g_float_parameter, g_temp_z); */
	g_new_z.x += g_temp_z.x;
	g_new_z.y += g_temp_z.y;
	return g_bail_out_fp();
}

/*  Richard8 {c = z = pixel: z = sin(z) + sin(pixel), |z| <= 50} */
int richard8_orbit(void)
{
#if !defined(XFRACT)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	/* LCMPLXtrig1(*g_long_parameter, g_tmp_z_l); */
	g_new_z_l.x += g_tmp_z_l.x;
	g_new_z_l.y += g_tmp_z_l.y;
	return g_bail_out_l();
#else
	return 0;
#endif
}

int popcorn_old_orbit_fp(void)
{
	g_temp_z = g_old_z;
	g_temp_z.x *= 3.0;
	g_temp_z.y *= 3.0;
	FPUsincos(&g_temp_z.x, &g_sin_x, &g_cos_x);
	FPUsincos(&g_temp_z.y, &s_sin_y, &s_cos_y);
	g_temp_z.x = g_sin_x/g_cos_x + g_old_z.x;
	g_temp_z.y = s_sin_y/s_cos_y + g_old_z.y;
	FPUsincos(&g_temp_z.x, &g_sin_x, &g_cos_x);
	FPUsincos(&g_temp_z.y, &s_sin_y, &s_cos_y);
	g_new_z.x = g_old_z.x - g_parameter.x*s_sin_y;
	g_new_z.y = g_old_z.y - g_parameter.x*g_sin_x;
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
		g_temp_sqr_x = sqr(g_new_z.x);
	}
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	if (g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int popcorn_orbit_fp(void)
{
	g_temp_z = g_old_z;
	g_temp_z.x *= 3.0;
	g_temp_z.y *= 3.0;
	FPUsincos(&g_temp_z.x, &g_sin_x, &g_cos_x);
	FPUsincos(&g_temp_z.y, &s_sin_y, &s_cos_y);
	g_temp_z.x = g_sin_x/g_cos_x + g_old_z.x;
	g_temp_z.y = s_sin_y/s_cos_y + g_old_z.y;
	FPUsincos(&g_temp_z.x, &g_sin_x, &g_cos_x);
	FPUsincos(&g_temp_z.y, &s_sin_y, &s_cos_y);
	g_new_z.x = g_old_z.x - g_parameter.x*s_sin_y;
	g_new_z.y = g_old_z.y - g_parameter.x*g_sin_x;
	/*
	g_new_z.x = g_old_z.x - g_parameter.x*sin(g_old_z.y + tan(3*g_old_z.y));
	g_new_z.y = g_old_z.y - g_parameter.x*sin(g_old_z.x + tan(3*g_old_z.x));
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
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	if (g_magnitude >= g_rq_limit || fabs(g_new_z.x) > g_rq_limit2 || fabs(g_new_z.y) > g_rq_limit2)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int popcorn_old_orbit(void)
{
#if !defined(XFRACT)
	g_tmp_z_l = g_old_z_l;
	g_tmp_z_l.x *= 3L;
	g_tmp_z_l.y *= 3L;
	TRIG_ARG_L(g_tmp_z_l.x);
	TRIG_ARG_L(g_tmp_z_l.y);
	SinCos086(g_tmp_z_l.x, &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_tmp_z_l.y, &s_sin_y_l, &s_cos_y_l);
	g_tmp_z_l.x = divide(s_sin_x_l, s_cos_x_l, bitshift) + g_old_z_l.x;
	g_tmp_z_l.y = divide(s_sin_y_l, s_cos_y_l, bitshift) + g_old_z_l.y;
	TRIG_ARG_L(g_tmp_z_l.x);
	TRIG_ARG_L(g_tmp_z_l.y);
	SinCos086(g_tmp_z_l.x, &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_tmp_z_l.y, &s_sin_y_l, &s_cos_y_l);
	g_new_z_l.x = g_old_z_l.x - multiply(g_parameter_l.x, s_sin_y_l, bitshift);
	g_new_z_l.y = g_old_z_l.y - multiply(g_parameter_l.x, s_sin_x_l, bitshift);
	if (g_plot_color == noplot)
	{
		plot_orbit_i(g_new_z_l.x, g_new_z_l.y, 1 + g_row % colors);
		g_old_z_l = g_new_z_l;
	}
	else
	{
		/* LONGBAILOUT(); */
		/* PB above still the old way, is weird, see notes in FP popcorn case */
		g_temp_sqr_x_l = lsqr(g_new_z_l.x);
		g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	}
	g_magnitude_l = g_temp_sqr_x_l + g_temp_sqr_y_l;
	if (g_magnitude_l >= g_limit_l || g_magnitude_l < 0 || labs(g_new_z_l.x) > g_limit2_l
			|| labs(g_new_z_l.y) > g_limit2_l)
					return 1;
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int popcorn_orbit(void)
{
#if !defined(XFRACT)
	g_tmp_z_l = g_old_z_l;
	g_tmp_z_l.x *= 3L;
	g_tmp_z_l.y *= 3L;
	TRIG_ARG_L(g_tmp_z_l.x);
	TRIG_ARG_L(g_tmp_z_l.y);
	SinCos086(g_tmp_z_l.x, &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_tmp_z_l.y, &s_sin_y_l, &s_cos_y_l);
	g_tmp_z_l.x = divide(s_sin_x_l, s_cos_x_l, bitshift) + g_old_z_l.x;
	g_tmp_z_l.y = divide(s_sin_y_l, s_cos_y_l, bitshift) + g_old_z_l.y;
	TRIG_ARG_L(g_tmp_z_l.x);
	TRIG_ARG_L(g_tmp_z_l.y);
	SinCos086(g_tmp_z_l.x, &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_tmp_z_l.y, &s_sin_y_l, &s_cos_y_l);
	g_new_z_l.x = g_old_z_l.x - multiply(g_parameter_l.x, s_sin_y_l, bitshift);
	g_new_z_l.y = g_old_z_l.y - multiply(g_parameter_l.x, s_sin_x_l, bitshift);
	if (g_plot_color == noplot)
	{
		plot_orbit_i(g_new_z_l.x, g_new_z_l.y, 1 + g_row % colors);
		g_old_z_l = g_new_z_l;
	}
	/* else */
	/* JCO: sqr's should always be done, else magnitude could be wrong */
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude_l = g_temp_sqr_x_l + g_temp_sqr_y_l;
	if (g_magnitude_l >= g_limit_l || g_magnitude_l < 0
		|| labs(g_new_z_l.x) > g_limit2_l
			|| labs(g_new_z_l.y) > g_limit2_l)
					return 1;
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

/* Popcorn generalization proposed by HB  */

int popcorn_fn_orbit_fp(void)
{
	_CMPLX tmpx;
	_CMPLX tmpy;

	/* tmpx contains the generalized value of the old real "x" equation */
	CMPLXtimesreal(g_parameter2, g_old_z.y, g_temp_z);  /* tmp = (C*old.y)         */
	CMPLXtrig1(g_temp_z, tmpx);             /* tmpx = trig1(tmp)         */
	tmpx.x += g_old_z.y;                  /* tmpx = old.y + trig1(tmp) */
	CMPLXtrig0(tmpx, g_temp_z);             /* tmp = trig0(tmpx)         */
	CMPLXmult(g_temp_z, g_parameter, tmpx);         /* tmpx = tmp*h            */

	/* tmpy contains the generalized value of the old real "y" equation */
	CMPLXtimesreal(g_parameter2, g_old_z.x, g_temp_z);  /* tmp = (C*old.x)         */
	CMPLXtrig3(g_temp_z, tmpy);             /* tmpy = trig3(tmp)         */
	tmpy.x += g_old_z.x;                  /* tmpy = old.x + trig1(tmp) */
	CMPLXtrig2(tmpy, g_temp_z);             /* tmp = trig2(tmpy)         */

	CMPLXmult(g_temp_z, g_parameter, tmpy);         /* tmpy = tmp*h            */

	g_new_z.x = g_old_z.x - tmpx.x - tmpy.y;
	g_new_z.y = g_old_z.y - tmpy.x - tmpx.y;

	if (g_plot_color == noplot)
	{
		plot_orbit(g_new_z.x, g_new_z.y, 1 + g_row % colors);
		g_old_z = g_new_z;
	}

	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	if (g_magnitude >= g_rq_limit
		|| fabs(g_new_z.x) > g_rq_limit2 || fabs(g_new_z.y) > g_rq_limit2)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

#define FIX_OVERFLOW(arg_) \
	if (overflow)  \
	{ \
		(arg_).x = fudge; \
		(arg_).y = 0; \
		overflow = 0; \
	}

int popcorn_fn_orbit(void)
{
#if !defined(XFRACT)
	_LCMPLX ltmpx, ltmpy;

	overflow = 0;

	/* ltmpx contains the generalized value of the old real "x" equation */
	LCMPLXtimesreal(g_parameter2_l, g_old_z_l.y, g_tmp_z_l); /* tmp = (C*old.y)         */
	LCMPLXtrig1(g_tmp_z_l, ltmpx);             /* tmpx = trig1(tmp)         */
	FIX_OVERFLOW(ltmpx);
	ltmpx.x += g_old_z_l.y;                   /* tmpx = old.y + trig1(tmp) */
	LCMPLXtrig0(ltmpx, g_tmp_z_l);             /* tmp = trig0(tmpx)         */
	FIX_OVERFLOW(g_tmp_z_l);
	LCMPLXmult(g_tmp_z_l, g_parameter_l, ltmpx);        /* tmpx = tmp*h            */

	/* ltmpy contains the generalized value of the old real "y" equation */
	LCMPLXtimesreal(g_parameter2_l, g_old_z_l.x, g_tmp_z_l); /* tmp = (C*old.x)         */
	LCMPLXtrig3(g_tmp_z_l, ltmpy);             /* tmpy = trig3(tmp)         */
	FIX_OVERFLOW(ltmpy);
	ltmpy.x += g_old_z_l.x;                   /* tmpy = old.x + trig1(tmp) */
	LCMPLXtrig2(ltmpy, g_tmp_z_l);             /* tmp = trig2(tmpy)         */
	FIX_OVERFLOW(g_tmp_z_l);
	LCMPLXmult(g_tmp_z_l, g_parameter_l, ltmpy);        /* tmpy = tmp*h            */

	g_new_z_l.x = g_old_z_l.x - ltmpx.x - ltmpy.y;
	g_new_z_l.y = g_old_z_l.y - ltmpy.x - ltmpx.y;

	if (g_plot_color == noplot)
	{
		plot_orbit_i(g_new_z_l.x, g_new_z_l.y, 1 + g_row % colors);
		g_old_z_l = g_new_z_l;
	}
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude_l = g_temp_sqr_x_l + g_temp_sqr_y_l;
	if (g_magnitude_l >= g_limit_l || g_magnitude_l < 0
		|| labs(g_new_z_l.x) > g_limit2_l
		|| labs(g_new_z_l.y) > g_limit2_l)
		return 1;
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int marks_complex_mandelbrot_orbit(void)
{
	g_temp_z.x = g_temp_sqr_x - g_temp_sqr_y;
	g_temp_z.y = 2*g_old_z.x*g_old_z.y;
	FPUcplxmul(&g_temp_z, &g_coefficient, &g_new_z);
	g_new_z.x += g_float_parameter->x;
	g_new_z.y += g_float_parameter->y;
	return g_bail_out_fp();
}

int spider_orbit_fp(void)
{
	/* Spider(XAXIS) { c = z=pixel: z = z*z + c; c = c/2 + z, |z| <= 4 } */
	g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_temp_z.x;
	g_new_z.y = 2*g_old_z.x*g_old_z.y + g_temp_z.y;
	g_temp_z.x = g_temp_z.x/2 + g_new_z.x;
	g_temp_z.y = g_temp_z.y/2 + g_new_z.y;
	return g_bail_out_fp();
}

int spider_orbit(void)
{
#if !defined(XFRACT)
	/* Spider(XAXIS) { c = z=pixel: z = z*z + c; c = c/2 + z, |z| <= 4 } */
	g_new_z_l.x  = g_temp_sqr_x_l - g_temp_sqr_y_l + g_tmp_z_l.x;
	g_new_z_l.y = multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift_minus_1) + g_tmp_z_l.y;
	g_tmp_z_l.x = (g_tmp_z_l.x >> 1) + g_new_z_l.x;
	g_tmp_z_l.y = (g_tmp_z_l.y >> 1) + g_new_z_l.y;
	return g_bail_out_l();
#else
	return 0;
#endif
}

int tetrate_orbit_fp(void)
{
	/* Tetrate(XAXIS) { c = z=pixel: z = c^z, |z| <= (P1 + 3) } */
	g_new_z = ComplexPower(*g_float_parameter, g_old_z);
	return g_bail_out_fp();
}

int z_trig_z_plus_z_orbit(void)
{
#if !defined(XFRACT)
	/* z = (p1*z*trig(z)) + p2*z */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);          /* g_tmp_z_l  = trig(old)             */
	LCMPLXmult(g_parameter_l, g_tmp_z_l, g_tmp_z_l);      /* g_tmp_z_l  = p1*trig(old)          */
	LCMPLXmult(g_old_z_l, g_tmp_z_l, g_tmp_z2_l);      /* g_tmp_z2_l = p1*old*trig(old)      */
	LCMPLXmult(g_parameter2_l, g_old_z_l, g_tmp_z_l);     /* g_tmp_z_l  = p2*old                */
	LCMPLXadd(g_tmp_z2_l, g_tmp_z_l, g_new_z_l);       /* g_new_z_l  = p1*trig(old) + p2*old */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int scott_z_trig_z_plus_z_orbit(void)
{
#if !defined(XFRACT)
	/* z = (z*trig(z)) + z */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);          /* g_tmp_z_l  = trig(old)       */
	LCMPLXmult(g_old_z_l, g_tmp_z_l, g_new_z_l);       /* g_new_z_l  = old*trig(old)   */
	LCMPLXadd(g_new_z_l, g_old_z_l, g_new_z_l);        /* g_new_z_l  = trig(old) + old */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int skinner_z_trig_z_minus_z_orbit(void)
{
#if !defined(XFRACT)
	/* z = (z*trig(z))-z */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);          /* g_tmp_z_l  = trig(old)       */
	LCMPLXmult(g_old_z_l, g_tmp_z_l, g_new_z_l);       /* g_new_z_l  = old*trig(old)   */
	LCMPLXsub(g_new_z_l, g_old_z_l, g_new_z_l);        /* g_new_z_l  = trig(old) - old */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int z_trig_z_plus_z_orbit_fp(void)
{
	/* z = (p1*z*trig(z)) + p2*z */
	CMPLXtrig0(g_old_z, g_temp_z);          /* tmp  = trig(old)             */
	CMPLXmult(g_parameter, g_temp_z, g_temp_z);      /* tmp  = p1*trig(old)          */
	CMPLXmult(g_old_z, g_temp_z, s_temp2);      /* s_temp2 = p1*old*trig(old)      */
	CMPLXmult(g_parameter2, g_old_z, g_temp_z);     /* tmp  = p2*old                */
	CMPLXadd(s_temp2, g_temp_z, g_new_z);       /* new  = p1*trig(old) + p2*old */
	return g_bail_out_fp();
}

int scott_z_trig_z_plus_z_orbit_fp(void)
{
	/* z = (z*trig(z)) + z */
	CMPLXtrig0(g_old_z, g_temp_z);         /* tmp  = trig(old)       */
	CMPLXmult(g_old_z, g_temp_z, g_new_z);       /* new  = old*trig(old)   */
	CMPLXadd(g_new_z, g_old_z, g_new_z);        /* new  = trig(old) + old */
	return g_bail_out_fp();
}

int skinner_z_trig_z_minus_z_orbit_fp(void)
{
	/* z = (z*trig(z))-z */
	CMPLXtrig0(g_old_z, g_temp_z);         /* tmp  = trig(old)       */
	CMPLXmult(g_old_z, g_temp_z, g_new_z);       /* new  = old*trig(old)   */
	CMPLXsub(g_new_z, g_old_z, g_new_z);        /* new  = trig(old) - old */
	return g_bail_out_fp();
}

int sqr_1_over_trig_z_orbit(void)
{
#if !defined(XFRACT)
	/* z = sqr(1/trig(z)) */
	LCMPLXtrig0(g_old_z_l, g_old_z_l);
	LCMPLXrecip(g_old_z_l, g_old_z_l);
	LCMPLXsqr(g_old_z_l, g_new_z_l);
	return g_bail_out_l();
#else
	return 0;
#endif
}

int sqr_1_over_trig_z_orbit_fp(void)
{
	/* z = sqr(1/trig(z)) */
	CMPLXtrig0(g_old_z, g_old_z);
	CMPLXrecip(g_old_z, g_old_z);
	CMPLXsqr(g_old_z, g_new_z);
	return g_bail_out_fp();
}

int trig_plus_trig_orbit(void)
{
#if !defined(XFRACT)
	/* z = trig(0, z)*p1 + trig1(z)*p2 */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);
	LCMPLXmult(g_parameter_l, g_tmp_z_l, g_tmp_z_l);
	LCMPLXtrig1(g_old_z_l, g_tmp_z2_l);
	LCMPLXmult(g_parameter2_l, g_tmp_z2_l, g_old_z_l);
	LCMPLXadd(g_tmp_z_l, g_old_z_l, g_new_z_l);
	return g_bail_out_l();
#else
	return 0;
#endif
}

int trig_plus_trig_orbit_fp(void)
{
	/* z = trig0(z)*p1 + trig1(z)*p2 */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXmult(g_parameter, g_temp_z, g_temp_z);
	CMPLXtrig1(g_old_z, g_old_z);
	CMPLXmult(g_parameter2, g_old_z, g_old_z);
	CMPLXadd(g_temp_z, g_old_z, g_new_z);
	return g_bail_out_fp();
}

/* The following four fractals are based on the idea of parallel
	or alternate calculations.  The shift is made when the mod
	reaches a given value.  JCO  5/6/92 */

int lambda_trig_or_trig_orbit(void)
{
#if !defined(XFRACT)
	/* z = trig0(z)*p1 if mod(old) < p2.x and
			trig1(z)*p1 if mod(old) >= p2.x */
	if ((LCMPLXmod(g_old_z_l)) < g_parameter2_l.x)
	{
		LCMPLXtrig0(g_old_z_l, g_tmp_z_l);
		LCMPLXmult(*g_long_parameter, g_tmp_z_l, g_new_z_l);
	}
	else
	{
		LCMPLXtrig1(g_old_z_l, g_tmp_z_l);
		LCMPLXmult(*g_long_parameter, g_tmp_z_l, g_new_z_l);
	}
	return g_bail_out_l();
#else
	return 0;
#endif
}

int lambda_trig_or_trig_orbit_fp(void)
{
	/* z = trig0(z)*p1 if mod(old) < p2.x and
			trig1(z)*p1 if mod(old) >= p2.x */
	if (CMPLXmod(g_old_z) < g_parameter2.x)
	{
		CMPLXtrig0(g_old_z, g_old_z);
		FPUcplxmul(g_float_parameter, &g_old_z, &g_new_z);
	}
	else
	{
		CMPLXtrig1(g_old_z, g_old_z);
		FPUcplxmul(g_float_parameter, &g_old_z, &g_new_z);
	}
	return g_bail_out_fp();
}

int julia_trig_or_trig_orbit(void)
{
#if !defined(XFRACT)
	/* z = trig0(z) + p1 if mod(old) < p2.x and
			trig1(z) + p1 if mod(old) >= p2.x */
	if (LCMPLXmod(g_old_z_l) < g_parameter2_l.x)
	{
		LCMPLXtrig0(g_old_z_l, g_tmp_z_l);
		LCMPLXadd(*g_long_parameter, g_tmp_z_l, g_new_z_l);
	}
	else
	{
		LCMPLXtrig1(g_old_z_l, g_tmp_z_l);
		LCMPLXadd(*g_long_parameter, g_tmp_z_l, g_new_z_l);
	}
	return g_bail_out_l();
#else
	return 0;
#endif
}

int julia_trig_or_trig_orbit_fp(void)
{
	/* z = trig0(z) + p1 if mod(old) < p2.x and
			trig1(z) + p1 if mod(old) >= p2.x */
	if (CMPLXmod(g_old_z) < g_parameter2.x)
	{
		CMPLXtrig0(g_old_z, g_old_z);
		CMPLXadd(*g_float_parameter, g_old_z, g_new_z);
	}
	else
	{
		CMPLXtrig1(g_old_z, g_old_z);
		CMPLXadd(*g_float_parameter, g_old_z, g_new_z);
	}
	return g_bail_out_fp();
}

int halley_orbit_mpc(void)
{
#if !defined(XFRACT)
	/*  X(X^a - 1) = 0, Halley Map */
	/*  a = g_parameter.x,  relaxation coeff. = g_parameter.y,  epsilon = g_parameter2.x  */

	int ihal;
	struct MPC mpcXtoAlessOne, mpcXtoA;
	struct MPC mpcXtoAplusOne; /* a-1, a, a + 1 */
	struct MPC mpcFX, mpcF1prime, mpcF2prime, mpcHalnumer1;
	struct MPC mpcHalnumer2, mpcHaldenom, mpctmp;

	MPOverflow = 0;
	mpcXtoAlessOne.x = mpcold.x;
	mpcXtoAlessOne.y = mpcold.y;
	for (ihal = 2; ihal < g_degree; ihal++)
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

	mpcF2prime.x = *pMPmul(g_a_plus_1_degree_mp, mpcXtoAlessOne.x); /* g_a_plus_1_degree_mp in setup */
	mpcF2prime.y = *pMPmul(g_a_plus_1_degree_mp, mpcXtoAlessOne.y);        /* F" */

	mpcF1prime.x = *pMPsub(*pMPmul(g_a_plus_1_mp, mpcXtoA.x), mpone);
	mpcF1prime.y = *pMPmul(g_a_plus_1_mp, mpcXtoA.y);                   /*  F'  */

	mpctmp.x = *pMPsub(*pMPmul(mpcF2prime.x, mpcFX.x), *pMPmul(mpcF2prime.y, mpcFX.y));
	mpctmp.y = *pMPadd(*pMPmul(mpcF2prime.x, mpcFX.y), *pMPmul(mpcF2prime.y, mpcFX.x));
	/*  F*F"  */

	mpcHaldenom.x = *pMPadd(mpcF1prime.x, mpcF1prime.x);
	mpcHaldenom.y = *pMPadd(mpcF1prime.y, mpcF1prime.y);      /*  2*F'  */

	mpcHalnumer1 = MPCdiv(mpctmp, mpcHaldenom);        /*  F"F/2F'  */
	mpctmp.x = *pMPsub(mpcF1prime.x, mpcHalnumer1.x);
	mpctmp.y = *pMPsub(mpcF1prime.y, mpcHalnumer1.y); /*  F' - F"F/2F'  */
	mpcHalnumer2 = MPCdiv(mpcFX, mpctmp);

	mpctmp   =  MPCmul(g_temp_parameter_mpc, mpcHalnumer2);  /* g_temp_parameter_mpc is relaxation coef. */
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
	return bail_out_halley_mpc() || MPOverflow;
#else
	return 0;
#endif
}

int halley_orbit_fp(void)
{
	/*  X(X^a - 1) = 0, Halley Map */
	/*  a = g_parameter.x = degree, relaxation coeff. = g_parameter.y, epsilon = g_parameter2.x  */

	int ihal;
	_CMPLX XtoAlessOne, XtoA, XtoAplusOne; /* a-1, a, a + 1 */
	_CMPLX FX, F1prime, F2prime, Halnumer1, Halnumer2, Haldenom;
	_CMPLX relax;

	XtoAlessOne = g_old_z;
	for (ihal = 2; ihal < g_degree; ihal++)
	{
		FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoAlessOne);
	}
	FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoA);
	FPUcplxmul(&g_old_z, &XtoA, &XtoAplusOne);

	CMPLXsub(XtoAplusOne, g_old_z, FX);        /* FX = X^(a + 1) - X  = F */
	F2prime.x = g_a_plus_1_degree*XtoAlessOne.x; /* g_a_plus_1_degree in setup */
	F2prime.y = g_a_plus_1_degree*XtoAlessOne.y;        /* F" */

	F1prime.x = g_a_plus_1*XtoA.x - 1.0;
	F1prime.y = g_a_plus_1*XtoA.y;                             /*  F'  */

	FPUcplxmul(&F2prime, &FX, &Halnumer1);                  /*  F*F"  */
	Haldenom.x = F1prime.x + F1prime.x;
	Haldenom.y = F1prime.y + F1prime.y;                     /*  2*F'  */

	FPUcplxdiv(&Halnumer1, &Haldenom, &Halnumer1);         /*  F"F/2F'  */
	CMPLXsub(F1prime, Halnumer1, Halnumer2);          /*  F' - F"F/2F'  */
	FPUcplxdiv(&FX, &Halnumer2, &Halnumer2);
	/* g_parameter.y is relaxation coef. */
	/* new.x = g_old_z.x - (g_parameter.y*Halnumer2.x);
	new.y = g_old_z.y - (g_parameter.y*Halnumer2.y); */
	relax.x = g_parameter.y;
	relax.y = param[3];
	FPUcplxmul(&relax, &Halnumer2, &Halnumer2);
	g_new_z.x = g_old_z.x - Halnumer2.x;
	g_new_z.y = g_old_z.y - Halnumer2.y;
	return bail_out_halley();
}

int phoenix_orbit(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n) */
	g_tmp_z_l.x = multiply(g_old_z_l.x, g_old_z_l.y, bitshift);
	g_new_z_l.x = g_temp_sqr_x_l-g_temp_sqr_y_l + g_long_parameter->x + multiply(g_long_parameter->y, g_tmp_z2_l.x, bitshift);
	g_new_z_l.y = (g_tmp_z_l.x + g_tmp_z_l.x) + multiply(g_long_parameter->y, g_tmp_z2_l.y, bitshift);
	g_tmp_z2_l = g_old_z_l; /* set g_tmp_z2_l to Y value */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int phoenix_orbit_fp(void)
{
	/* z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n) */
	g_temp_z.x = g_old_z.x*g_old_z.y;
	g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_float_parameter->x + (g_float_parameter->y*s_temp2.x);
	g_new_z.y = (g_temp_z.x + g_temp_z.x) + (g_float_parameter->y*s_temp2.y);
	s_temp2 = g_old_z; /* set s_temp2 to Y value */
	return g_bail_out_fp();
}

int phoenix_complex_orbit(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n) */
	g_tmp_z_l.x = multiply(g_old_z_l.x, g_old_z_l.y, bitshift);
	g_new_z_l.x = g_temp_sqr_x_l-g_temp_sqr_y_l + g_long_parameter->x + multiply(g_parameter2_l.x, g_tmp_z2_l.x, bitshift)-multiply(g_parameter2_l.y, g_tmp_z2_l.y, bitshift);
	g_new_z_l.y = (g_tmp_z_l.x + g_tmp_z_l.x) + g_long_parameter->y + multiply(g_parameter2_l.x, g_tmp_z2_l.y, bitshift) + multiply(g_parameter2_l.y, g_tmp_z2_l.x, bitshift);
	g_tmp_z2_l = g_old_z_l; /* set g_tmp_z2_l to Y value */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int
phoenix_complex_orbit_fp(void)
{
	/* z(n + 1) = z(n)^2 + p1 + p2*y(n),  y(n + 1) = z(n) */
	g_temp_z.x = g_old_z.x*g_old_z.y;
	g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_float_parameter->x + (g_parameter2.x*s_temp2.x) - (g_parameter2.y*s_temp2.y);
	g_new_z.y = (g_temp_z.x + g_temp_z.x) + g_float_parameter->y + (g_parameter2.x*s_temp2.y) + (g_parameter2.y*s_temp2.x);
	s_temp2 = g_old_z; /* set s_temp2 to Y value */
	return g_bail_out_fp();
}

int phoenix_plus_orbit(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_LCMPLX loldplus, lnewminus;
	loldplus = g_old_z_l;
	g_tmp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  /* degree >= 2, degree = degree-1 in setup */
	{
		LCMPLXmult(g_old_z_l, g_tmp_z_l, g_tmp_z_l); /* = old^(degree-1) */
	}
	loldplus.x += g_long_parameter->x;
	LCMPLXmult(g_tmp_z_l, loldplus, lnewminus);
	g_new_z_l.x = lnewminus.x + multiply(g_long_parameter->y, g_tmp_z2_l.x, bitshift);
	g_new_z_l.y = lnewminus.y + multiply(g_long_parameter->y, g_tmp_z2_l.y, bitshift);
	g_tmp_z2_l = g_old_z_l; /* set g_tmp_z2_l to Y value */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int phoenix_plus_orbit_fp(void)
{
	/* z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_CMPLX oldplus, newminus;
	oldplus = g_old_z;
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  /* degree >= 2, degree = degree-1 in setup */
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); /* = old^(degree-1) */
	}
	oldplus.x += g_float_parameter->x;
	FPUcplxmul(&g_temp_z, &oldplus, &newminus);
	g_new_z.x = newminus.x + (g_float_parameter->y*s_temp2.x);
	g_new_z.y = newminus.y + (g_float_parameter->y*s_temp2.y);
	s_temp2 = g_old_z; /* set s_temp2 to Y value */
	return g_bail_out_fp();
}

int phoenix_minus_orbit(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_LCMPLX loldsqr, lnewminus;
	LCMPLXmult(g_old_z_l, g_old_z_l, loldsqr);
	g_tmp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  /* degree >= 3, degree = degree-2 in setup */
	{
		LCMPLXmult(g_old_z_l, g_tmp_z_l, g_tmp_z_l); /* = old^(degree-2) */
	}
	loldsqr.x += g_long_parameter->x;
	LCMPLXmult(g_tmp_z_l, loldsqr, lnewminus);
	g_new_z_l.x = lnewminus.x + multiply(g_long_parameter->y, g_tmp_z2_l.x, bitshift);
	g_new_z_l.y = lnewminus.y + multiply(g_long_parameter->y, g_tmp_z2_l.y, bitshift);
	g_tmp_z2_l = g_old_z_l; /* set g_tmp_z2_l to Y value */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int phoenix_minus_orbit_fp(void)
{
	/* z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_CMPLX oldsqr, newminus;
	FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  /* degree >= 3, degree = degree-2 in setup */
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); /* = old^(degree-2) */
	}
	oldsqr.x += g_float_parameter->x;
	FPUcplxmul(&g_temp_z, &oldsqr, &newminus);
	g_new_z.x = newminus.x + (g_float_parameter->y*s_temp2.x);
	g_new_z.y = newminus.y + (g_float_parameter->y*s_temp2.y);
	s_temp2 = g_old_z; /* set s_temp2 to Y value */
	return g_bail_out_fp();
}

int phoenix_complex_plus_orbit(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_LCMPLX loldplus, lnewminus;
	loldplus = g_old_z_l;
	g_tmp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  /* degree >= 2, degree = degree-1 in setup */
	{
		LCMPLXmult(g_old_z_l, g_tmp_z_l, g_tmp_z_l); /* = old^(degree-1) */
	}
	loldplus.x += g_long_parameter->x;
	loldplus.y += g_long_parameter->y;
	LCMPLXmult(g_tmp_z_l, loldplus, lnewminus);
	LCMPLXmult(g_parameter2_l, g_tmp_z2_l, g_tmp_z_l);
	g_new_z_l.x = lnewminus.x + g_tmp_z_l.x;
	g_new_z_l.y = lnewminus.y + g_tmp_z_l.y;
	g_tmp_z2_l = g_old_z_l; /* set g_tmp_z2_l to Y value */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int phoenix_complex_plus_orbit_fp(void)
{
	/* z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_CMPLX oldplus, newminus;
	oldplus = g_old_z;
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  /* degree >= 2, degree = degree-1 in setup */
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); /* = old^(degree-1) */
	}
	oldplus.x += g_float_parameter->x;
	oldplus.y += g_float_parameter->y;
	FPUcplxmul(&g_temp_z, &oldplus, &newminus);
	FPUcplxmul(&g_parameter2, &s_temp2, &g_temp_z);
	g_new_z.x = newminus.x + g_temp_z.x;
	g_new_z.y = newminus.y + g_temp_z.y;
	s_temp2 = g_old_z; /* set s_temp2 to Y value */
	return g_bail_out_fp();
}

int phoenix_complex_minus_orbit(void)
{
#if !defined(XFRACT)
	/* z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_LCMPLX loldsqr, lnewminus;
	LCMPLXmult(g_old_z_l, g_old_z_l, loldsqr);
	g_tmp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  /* degree >= 3, degree = degree-2 in setup */
	{
		LCMPLXmult(g_old_z_l, g_tmp_z_l, g_tmp_z_l); /* = old^(degree-2) */
	}
	loldsqr.x += g_long_parameter->x;
	loldsqr.y += g_long_parameter->y;
	LCMPLXmult(g_tmp_z_l, loldsqr, lnewminus);
	LCMPLXmult(g_parameter2_l, g_tmp_z2_l, g_tmp_z_l);
	g_new_z_l.x = lnewminus.x + g_tmp_z_l.x;
	g_new_z_l.y = lnewminus.y + g_tmp_z_l.y;
	g_tmp_z2_l = g_old_z_l; /* set g_tmp_z2_l to Y value */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int phoenix_complex_minus_orbit_fp(void)
{
	/* z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n) */
	int i;
	_CMPLX oldsqr, newminus;
	FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  /* degree >= 3, degree = degree-2 in setup */
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); /* = old^(degree-2) */
	}
	oldsqr.x += g_float_parameter->x;
	oldsqr.y += g_float_parameter->y;
	FPUcplxmul(&g_temp_z, &oldsqr, &newminus);
	FPUcplxmul(&g_parameter2, &s_temp2, &g_temp_z);
	g_new_z.x = newminus.x + g_temp_z.x;
	g_new_z.y = newminus.y + g_temp_z.y;
	s_temp2 = g_old_z; /* set s_temp2 to Y value */
	return g_bail_out_fp();
}

int scott_trig_plus_trig_orbit(void)
{
#if !defined(XFRACT)
	/* z = trig0(z) + trig1(z) */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);
	LCMPLXtrig1(g_old_z_l, g_old_z_l);
	LCMPLXadd(g_tmp_z_l, g_old_z_l, g_new_z_l);
	return g_bail_out_l();
#else
	return 0;
#endif
}

int scott_trig_plus_trig_orbit_fp(void)
{
	/* z = trig0(z) + trig1(z) */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXtrig1(g_old_z, s_temp2);
	CMPLXadd(g_temp_z, s_temp2, g_new_z);
	return g_bail_out_fp();
}

int skinner_trig_sub_trig_orbit(void)
{
#if !defined(XFRACT)
	/* z = trig(0, z)-trig1(z) */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);
	LCMPLXtrig1(g_old_z_l, g_tmp_z2_l);
	LCMPLXsub(g_tmp_z_l, g_tmp_z2_l, g_new_z_l);
	return g_bail_out_l();
#else
	return 0;
#endif
}

int skinner_trig_sub_trig_orbit_fp(void)
{
	/* z = trig0(z)-trig1(z) */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXtrig1(g_old_z, s_temp2);
	CMPLXsub(g_temp_z, s_temp2, g_new_z);
	return g_bail_out_fp();
}

int trig_trig_orbit_fp(void)
{
	/* z = trig0(z)*trig1(z) */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXtrig1(g_old_z, g_old_z);
	CMPLXmult(g_temp_z, g_old_z, g_new_z);
	return g_bail_out_fp();
}

#if !defined(XFRACT)
/* call float version of fractal if integer math overflow */
static int try_float_fractal(int (*fpFractal)(void))
{
	overflow = 0;
	/* g_old_z_l had better not be changed! */
	g_old_z.x = g_old_z_l.x; g_old_z.x /= fudge;
	g_old_z.y = g_old_z_l.y; g_old_z.y /= fudge;
	g_temp_sqr_x = sqr(g_old_z.x);
	g_temp_sqr_y = sqr(g_old_z.y);
	fpFractal();
	if (save_release < 1900)  /* for backwards compatibility */
	{
		g_new_z_l.x = (long)(g_new_z.x/fudge); /* this error has been here a long time */
		g_new_z_l.y = (long)(g_new_z.y/fudge);
	}
	else
	{
		g_new_z_l.x = (long)(g_new_z.x*fudge);
		g_new_z_l.y = (long)(g_new_z.y*fudge);
	}
	return 0;
}
#endif

int trig_trig_orbit(void)
{
#if !defined(XFRACT)
	_LCMPLX g_tmp_z2_l;
	/* z = trig0(z)*trig1(z) */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);
	LCMPLXtrig1(g_old_z_l, g_tmp_z2_l);
	LCMPLXmult(g_tmp_z_l, g_tmp_z2_l, g_new_z_l);
	if (overflow)
	{
		try_float_fractal(trig_trig_orbit_fp);
	}
	return g_bail_out_l();
#else
	return 0;
#endif
}

/********************************************************************/
/*  Next six orbit functions are one type - extra functions are     */
/*    special cases written for speed.                              */
/********************************************************************/

int trig_plus_sqr_orbit(void) /* generalization of Scott and Skinner types */
{
#if !defined(XFRACT)
	/* { z = pixel: z = (p1, p2)*trig(z) + (p3, p4)*sqr(z), |z|<BAILOUT } */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);     /* g_tmp_z_l = trig(g_old_z_l)                        */
	LCMPLXmult(g_parameter_l, g_tmp_z_l, g_new_z_l); /* g_new_z_l = g_parameter_l*trig(g_old_z_l)                  */
	LCMPLXsqr_old(g_tmp_z_l);         /* g_tmp_z_l = sqr(g_old_z_l)                         */
	LCMPLXmult(g_parameter2_l, g_tmp_z_l, g_tmp_z_l); /* g_tmp_z_l = g_parameter2_l*sqr(g_old_z_l)                  */
	LCMPLXadd(g_new_z_l, g_tmp_z_l, g_new_z_l);   /* g_new_z_l = g_parameter_l*trig(g_old_z_l) + g_parameter2_l*sqr(g_old_z_l) */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int trig_plus_sqr_orbit_fp(void) /* generalization of Scott and Skinner types */
{
	/* { z = pixel: z = (p1, p2)*trig(z) + (p3, p4)*sqr(z), |z|<BAILOUT } */
	CMPLXtrig0(g_old_z, g_temp_z);     /* tmp = trig(old)                     */
	CMPLXmult(g_parameter, g_temp_z, g_new_z); /* new = g_parameter*trig(old)                */

	CMPLXsqr_old(g_temp_z);        /* tmp = sqr(old)                      */
	CMPLXmult(g_parameter2, g_temp_z, s_temp2); /* tmp = g_parameter2*sqr(old)                */
	CMPLXadd(g_new_z, s_temp2, g_new_z);    /* new = g_parameter*trig(old) + g_parameter2*sqr(old) */
	return g_bail_out_fp();
}

int scott_trig_plus_sqr_orbit(void)
{
#if !defined(XFRACT)
	/*  { z = pixel: z = trig(z) + sqr(z), |z|<BAILOUT } */
	LCMPLXtrig0(g_old_z_l, g_new_z_l);    /* g_new_z_l = trig(g_old_z_l)           */
	LCMPLXsqr_old(g_tmp_z_l);        /* g_old_z_l = sqr(g_old_z_l)            */
	LCMPLXadd(g_tmp_z_l, g_new_z_l, g_new_z_l);  /* g_new_z_l = trig(g_old_z_l) + sqr(g_old_z_l) */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int scott_trig_plus_sqr_orbit_fp(void) /* float version */
{
	/* { z = pixel: z = sin(z) + sqr(z), |z|<BAILOUT } */
	CMPLXtrig0(g_old_z, g_new_z);       /* new = trig(old)          */
	CMPLXsqr_old(g_temp_z);          /* tmp = sqr(old)           */
	CMPLXadd(g_new_z, g_temp_z, g_new_z);      /* new = trig(old) + sqr(old) */
	return g_bail_out_fp();
}

int skinner_trig_sub_sqr_orbit(void)
{
#if !defined(XFRACT)
	/* { z = pixel: z = sin(z)-sqr(z), |z|<BAILOUT }               */
	LCMPLXtrig0(g_old_z_l, g_new_z_l);    /* g_new_z_l = trig(g_old_z_l)           */
	LCMPLXsqr_old(g_tmp_z_l);        /* g_old_z_l = sqr(g_old_z_l)            */
	LCMPLXsub(g_new_z_l, g_tmp_z_l, g_new_z_l);  /* g_new_z_l = trig(g_old_z_l)-sqr(g_old_z_l) */
	return g_bail_out_l();
#else
	return 0;
#endif
}

int skinner_trig_sub_sqr_orbit_fp(void)
{
	/* { z = pixel: z = sin(z)-sqr(z), |z|<BAILOUT } */
	CMPLXtrig0(g_old_z, g_new_z);       /* new = trig(old) */
	CMPLXsqr_old(g_temp_z);          /* old = sqr(old)  */
	CMPLXsub(g_new_z, g_temp_z, g_new_z);      /* new = trig(old)-sqr(old) */
	return g_bail_out_fp();
}

int trig_z_squared_orbit_fp(void)
{
	/* { z = pixel: z = trig(z*z), |z|<TEST } */
	CMPLXsqr_old(g_temp_z);
	CMPLXtrig0(g_temp_z, g_new_z);
	return g_bail_out_fp();
}

int trig_z_squared_orbit(void) /* this doesn't work very well */
{
#if !defined(XFRACT)
	/* { z = pixel: z = trig(z*z), |z|<TEST } */
	long l16triglim_2 = 8L << 15;
	LCMPLXsqr_old(g_tmp_z_l);
	if ((labs(g_tmp_z_l.x) > l16triglim_2 || labs(g_tmp_z_l.y) > l16triglim_2) &&
		save_release > 1900)
	{
		overflow = 1;
	}
	else
	{
		LCMPLXtrig0(g_tmp_z_l, g_new_z_l);
	}
	if (overflow)
	{
		try_float_fractal(trig_z_squared_orbit_fp);
	}
	return g_bail_out_l();
#else
	return 0;
#endif
}

int sqr_trig_orbit(void)
{
#if !defined(XFRACT)
	/* { z = pixel: z = sqr(trig(z)), |z|<TEST} */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);
	LCMPLXsqr(g_tmp_z_l, g_new_z_l);
	return g_bail_out_l();
#else
	return 0;
#endif
}

int sqr_trig_orbit_fp(void)
{
	/* SZSB(XYAXIS) { z = pixel, TEST = (p1 + 3): z = sin(z)*sin(z), |z|<TEST} */
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXsqr(g_temp_z, g_new_z);
	return g_bail_out_fp();
}

int magnet1_orbit_fp(void)    /*    Z = ((Z**2 + C - 1)/(2Z + C - 2))**2    */
{                   /*  In "Beauty of Fractals", code by Kev Allen. */
	_CMPLX top, bot, tmp;
	double div;

	top.x = g_temp_sqr_x - g_temp_sqr_y + g_float_parameter->x - 1; /* top = Z**2 + C-1 */
	top.y = g_old_z.x*g_old_z.y;
	top.y = top.y + top.y + g_float_parameter->y;

	bot.x = g_old_z.x + g_old_z.x + g_float_parameter->x - 2;       /* bot = 2*Z + C-2  */
	bot.y = g_old_z.y + g_old_z.y + g_float_parameter->y;

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

	return g_bail_out_fp();
}

/* Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2)) /      */
/*       (3Z**2 + 3(C-2)Z + (C-1)(C-2) + 1))**2  */
int magnet2_orbit_fp(void)
{
	/*   In "Beauty of Fractals", code by Kev Allen.  */
	_CMPLX top, bot, tmp;
	double div;

	top.x = g_old_z.x*(g_temp_sqr_x-g_temp_sqr_y-g_temp_sqr_y-g_temp_sqr_y + s_3_c_minus_1.x)
			- g_old_z.y*s_3_c_minus_1.y + s_c_minus_1_c_minus_2.x;
	top.y = g_old_z.y*(g_temp_sqr_x + g_temp_sqr_x + g_temp_sqr_x-g_temp_sqr_y + s_3_c_minus_1.x)
			+ g_old_z.x*s_3_c_minus_1.y + s_c_minus_1_c_minus_2.y;

	bot.x = g_temp_sqr_x - g_temp_sqr_y;
	bot.x = bot.x + bot.x + bot.x
			+ g_old_z.x*s_3_c_minus_2.x - g_old_z.y*s_3_c_minus_2.y
			+ s_c_minus_1_c_minus_2.x + 1.0;
	bot.y = g_old_z.x*g_old_z.y;
	bot.y += bot.y;
	bot.y = bot.y + bot.y + bot.y
			+ g_old_z.x*s_3_c_minus_2.y + g_old_z.y*s_3_c_minus_2.x
			+ s_c_minus_1_c_minus_2.y;

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

	return g_bail_out_fp();
}

int lambda_trig_orbit(void)
{
#if !defined(XFRACT)
	BAIL_OUT_TRIG_XY_LONG();
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);           /* g_tmp_z_l = trig(g_old_z_l)           */
	LCMPLXmult(*g_long_parameter, g_tmp_z_l, g_new_z_l);   /* g_new_z_l = g_long_parameter*trig(g_old_z_l)  */
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int lambda_trig_orbit_fp(void)
{
	BAIL_OUT_TRIG_XY_FP();
	CMPLXtrig0(g_old_z, g_temp_z);              /* tmp = trig(old)           */
	CMPLXmult(*g_float_parameter, g_temp_z, g_new_z);   /* new = g_long_parameter*trig(old)  */
	g_old_z = g_new_z;
	return 0;
}

/* bailouts are different for different trig functions */
int lambda_trig1_orbit(void)
{
#if !defined(XFRACT)
	BAIL_OUT_TRIG_LONG(); /* sin, cos */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);           /* g_tmp_z_l = trig(g_old_z_l)           */
	LCMPLXmult(*g_long_parameter, g_tmp_z_l, g_new_z_l);   /* g_new_z_l = g_long_parameter*trig(g_old_z_l)  */
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int lambda_trig1_orbit_fp(void)
{
	BAIL_OUT_TRIG_FP(); /* sin, cos */
	CMPLXtrig0(g_old_z, g_temp_z);              /* tmp = trig(old)           */
	CMPLXmult(*g_float_parameter, g_temp_z, g_new_z);   /* new = g_long_parameter*trig(old)  */
	g_old_z = g_new_z;
	return 0;
}

int lambda_trig2_orbit(void)
{
#if !defined(XFRACT)
	BAIL_OUT_TRIG_H_LONG(); /* sinh, cosh */
	LCMPLXtrig0(g_old_z_l, g_tmp_z_l);           /* g_tmp_z_l = trig(g_old_z_l)           */
	LCMPLXmult(*g_long_parameter, g_tmp_z_l, g_new_z_l);   /* g_new_z_l = g_long_parameter*trig(g_old_z_l)  */
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int lambda_trig2_orbit_fp(void)
{
#if !defined(XFRACT)
	BAIL_OUT_TRIG_H_FP(); /* sinh, cosh */
	CMPLXtrig0(g_old_z, g_temp_z);              /* tmp = trig(old)           */
	CMPLXmult(*g_float_parameter, g_temp_z, g_new_z);   /* new = g_long_parameter*trig(old)  */
	g_old_z = g_new_z;
	return 0;
#else
	return 0;
#endif
}

int man_o_war_orbit(void)
{
#if !defined(XFRACT)
	/* From Art Matrix via Lee Skinner */
	g_new_z_l.x  = g_temp_sqr_x_l - g_temp_sqr_y_l + g_tmp_z_l.x + g_long_parameter->x;
	g_new_z_l.y = multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift_minus_1) + g_tmp_z_l.y + g_long_parameter->y;
	g_tmp_z_l = g_old_z_l;
	return g_bail_out_l();
#else
	return 0;
#endif
}

int man_o_war_orbit_fp(void)
{
	/* From Art Matrix via Lee Skinner */
	/* note that fast >= 287 equiv in fracsuba.asm must be kept in step */
	g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_temp_z.x + g_float_parameter->x;
	g_new_z.y = 2.0*g_old_z.x*g_old_z.y + g_temp_z.y + g_float_parameter->y;
	g_temp_z = g_old_z;
	return g_bail_out_fp();
}

/*
	MarksMandelPwr (XAXIS)
	{
		z = pixel, c = z ^ (z - 1):
			z = c*sqr(z) + pixel,
		|z| <= 4
	}
*/
int marks_mandel_power_orbit_fp(void)
{
	CMPLXtrig0(g_old_z, g_new_z);
	CMPLXmult(g_temp_z, g_new_z, g_new_z);
	g_new_z.x += g_float_parameter->x;
	g_new_z.y += g_float_parameter->y;
	return g_bail_out_fp();
}

int marks_mandel_power_orbit(void)
{
#if !defined(XFRACT)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	LCMPLXmult(g_tmp_z_l, g_new_z_l, g_new_z_l);
	g_new_z_l.x += g_long_parameter->x;
	g_new_z_l.y += g_long_parameter->y;
	return g_bail_out_l();
#else
	return 0;
#endif
}

/*
	I was coding Marksmandelpower and failed to use some temporary
	variables. The result was nice, and since my name is not on any fractal,
	I thought I would immortalize myself with this error!
	Tim Wegner
*/
int tims_error_orbit_fp(void)
{
	CMPLXtrig0(g_old_z, g_new_z);
	g_new_z.x = g_new_z.x*g_temp_z.x - g_new_z.y*g_temp_z.y;
	g_new_z.y = g_new_z.x*g_temp_z.y - g_new_z.y*g_temp_z.x;
	g_new_z.x += g_float_parameter->x;
	g_new_z.y += g_float_parameter->y;
	return g_bail_out_fp();
}

int tims_error_orbit(void)
{
#if !defined(XFRACT)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	g_new_z_l.x = multiply(g_new_z_l.x, g_tmp_z_l.x, bitshift)-multiply(g_new_z_l.y, g_tmp_z_l.y, bitshift);
	g_new_z_l.y = multiply(g_new_z_l.x, g_tmp_z_l.y, bitshift)-multiply(g_new_z_l.y, g_tmp_z_l.x, bitshift);
	g_new_z_l.x += g_long_parameter->x;
	g_new_z_l.y += g_long_parameter->y;
	return g_bail_out_l();
#else
	return 0;
#endif
}

int circle_orbit_fp(void)
{
	long i;
	i = (long)(param[0]*(g_temp_sqr_x + g_temp_sqr_y));
	g_color_iter = i % colors;
	return 1;
}

/*
int circle_orbit(void)
{
	long i;
	i = multiply(g_parameter_l.x, (g_temp_sqr_x_l + g_temp_sqr_y_l), bitshift);
	i = i >> bitshift;
	g_color_iter = i % colors);
	return 1;
}
*/

/* -------------------------------------------------------------------- */
/*              Initialization (once per pixel) routines                                                */
/* -------------------------------------------------------------------- */

/* transform points with reciprocal function */
void invert_z(_CMPLX *z)
{
	z->x = dxpixel();
	z->y = dypixel();
	z->x -= g_f_x_center; z->y -= g_f_y_center;  /* Normalize values to center of circle */

	g_temp_sqr_x = sqr(z->x) + sqr(z->y);  /* Get old radius */
	g_temp_sqr_x = (fabs(g_temp_sqr_x) > FLT_MIN) ? (g_f_radius / g_temp_sqr_x) : FLT_MAX;
	z->x *= g_temp_sqr_x;
	z->y *= g_temp_sqr_x;      /* Perform inversion */
	z->x += g_f_x_center;
	z->y += g_f_y_center; /* Renormalize */
}

int julia_per_pixel_l(void)
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
		invert_z(&g_old_z);

		/* watch out for overflow */
		if (sqr(g_old_z.x) + sqr(g_old_z.y) >= 127)
		{
			g_old_z.x = 8;  /* value to bail out in one iteration */
			g_old_z.y = 8;
		}

		/* convert to fudged longs */
		g_old_z_l.x = (long)(g_old_z.x*fudge);
		g_old_z_l.y = (long)(g_old_z.y*fudge);
	}
	else
	{
		g_old_z_l.x = lxpixel();
		g_old_z_l.y = lypixel();
	}
	return 0;
#else
	return 0;
#endif
}

int richard8_per_pixel(void)
{
#if !defined(XFRACT)
	mandelbrot_per_pixel_l();
	LCMPLXtrig1(*g_long_parameter, g_tmp_z_l);
	LCMPLXmult(g_tmp_z_l, g_parameter2_l, g_tmp_z_l);
	return 1;
#else
	return 0;
#endif
}

int mandelbrot_per_pixel_l(void)
{
#if !defined(XFRACT)
	/* integer mandel types */
	/* barnsleym1 */
	/* barnsleym2 */
	g_initial_z_l.x = lxpixel();
	if (save_release >= 2004)
	{
		g_initial_z_l.y = lypixel();
	}

	if (g_invert)
	{
		/* invert */
		invert_z(&g_initial_z);

		/* watch out for overflow */
		if (sqr(g_initial_z.x) + sqr(g_initial_z.y) >= 127)
		{
			g_initial_z.x = 8;  /* value to bail out in one iteration */
			g_initial_z.y = 8;
		}

		/* convert to fudged longs */
		g_initial_z_l.x = (long)(g_initial_z.x*fudge);
		g_initial_z_l.y = (long)(g_initial_z.y*fudge);
	}

	g_old_z_l = (useinitorbit == 1) ? g_init_orbit_l : g_initial_z_l;

	g_old_z_l.x += g_parameter_l.x;    /* initial pertubation of parameters set */
	g_old_z_l.y += g_parameter_l.y;
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
		invert_z(&g_old_z);

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
		g_old_z_l.x = (long)(g_old_z.x*fudge);
		g_old_z_l.y = (long)(g_old_z.y*fudge);
	}
	else
	{
		g_old_z_l.x = lxpixel();
		g_old_z_l.y = lypixel();
	}

	g_temp_sqr_x_l = multiply(g_old_z_l.x, g_old_z_l.x, bitshift);
	g_temp_sqr_y_l = multiply(g_old_z_l.y, g_old_z_l.y, bitshift);
	g_tmp_z_l = g_old_z_l;
	return 0;
}

int marks_mandelbrot_power_per_pixel(void)
{
#if !defined(XFRACT)
	mandelbrot_per_pixel();
	g_tmp_z_l = g_old_z_l;
	g_tmp_z_l.x -= fudge;
	LCMPLXpwr(g_old_z_l, g_tmp_z_l, g_tmp_z_l);
	return 1;
#else
	return 0;
#endif
}

int mandelbrot_per_pixel(void)
{
	if (g_invert)
	{
		invert_z(&g_initial_z);

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
		g_initial_z_l.x = (long)(g_initial_z.x*fudge);
		g_initial_z_l.y = (long)(g_initial_z.y*fudge);
	}
	else
	{
		g_initial_z_l.x = lxpixel();
		if (save_release >= 2004)
		{
			g_initial_z_l.y = lypixel();
		}
	}
	switch (fractype)
	{
	case MANDELLAMBDA:              /* Critical Value 0.5 + 0.0i  */
		g_old_z_l.x = fudge >> 1;
		g_old_z_l.y = 0;
		break;
	default:
		g_old_z_l = g_initial_z_l;
		break;
	}

	/* alter g_initial_z value */
	if (useinitorbit == 1)
	{
		g_old_z_l = g_init_orbit_l;
	}
	else if (useinitorbit == 2)
	{
		g_old_z_l = g_initial_z_l;
	}

	if ((inside == BOF60 || inside == BOF61) && !nobof)
	{
		/* kludge to match "Beauty of Fractals" picture since we start
			Mandelbrot iteration with g_initial_z rather than 0 */
		g_old_z_l.x = g_parameter_l.x; /* initial pertubation of parameters set */
		g_old_z_l.y = g_parameter_l.y;
		g_color_iter = -1;
	}
	else
	{
		g_old_z_l.x += g_parameter_l.x; /* initial pertubation of parameters set */
		g_old_z_l.y += g_parameter_l.y;
	}
	g_tmp_z_l = g_initial_z_l; /* for spider */
	g_temp_sqr_x_l = multiply(g_old_z_l.x, g_old_z_l.x, bitshift);
	g_temp_sqr_y_l = multiply(g_old_z_l.y, g_old_z_l.y, bitshift);
	return 1; /* 1st iteration has been done */
}

int marks_mandelbrot_per_pixel()
{
#if !defined(XFRACT)
	/* marksmandel */
	if (g_invert)
	{
		invert_z(&g_initial_z);

		/* watch out for overflow */
		if (sqr(g_initial_z.x) + sqr(g_initial_z.y) >= 127)
		{
			g_initial_z.x = 8;  /* value to bail out in one iteration */
			g_initial_z.y = 8;
		}

		/* convert to fudged longs */
		g_initial_z_l.x = (long)(g_initial_z.x*fudge);
		g_initial_z_l.y = (long)(g_initial_z.y*fudge);
	}
	else
	{
		g_initial_z_l.x = lxpixel();
		if (save_release >= 2004)
		{
			g_initial_z_l.y = lypixel();
		}
	}

	g_old_z_l = (useinitorbit == 1) ? g_init_orbit_l : g_initial_z_l;

	g_old_z_l.x += g_parameter_l.x;    /* initial pertubation of parameters set */
	g_old_z_l.y += g_parameter_l.y;

	if (g_c_exp > 3)
	{
		complex_power_l(&g_old_z_l, g_c_exp-1, &g_coefficient_l, bitshift);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient_l.x = multiply(g_old_z_l.x, g_old_z_l.x, bitshift)
			- multiply(g_old_z_l.y, g_old_z_l.y, bitshift);
		g_coefficient_l.y = multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift_minus_1);
	}
	else if (g_c_exp == 2)
	{
		g_coefficient_l = g_old_z_l;
	}
	else if (g_c_exp < 2)
	{
		g_coefficient_l.x = 1L << bitshift;
		g_coefficient_l.y = 0L;
	}

	g_temp_sqr_x_l = multiply(g_old_z_l.x, g_old_z_l.x, bitshift);
	g_temp_sqr_y_l = multiply(g_old_z_l.y, g_old_z_l.y, bitshift);
#endif
	return 1; /* 1st iteration has been done */
}

int marks_mandelbrot_per_pixel_fp()
{
	/* marksmandel */

	if (g_invert)
	{
		invert_z(&g_initial_z);
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

	g_old_z.x += g_parameter.x;      /* initial pertubation of parameters set */
	g_old_z.y += g_parameter.y;

	g_temp_sqr_x = sqr(g_old_z.x);
	g_temp_sqr_y = sqr(g_old_z.y);

	if (g_c_exp > 3)
	{
		complex_power(&g_old_z, g_c_exp-1, &g_coefficient);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient.x = g_temp_sqr_x - g_temp_sqr_y;
		g_coefficient.y = g_old_z.x*g_old_z.y*2;
	}
	else if (g_c_exp == 2)
	{
		g_coefficient = g_old_z;
	}
	else if (g_c_exp < 2)
	{
		g_coefficient.x = 1.0;
		g_coefficient.y = 0.0;
	}

	return 1; /* 1st iteration has been done */
}

int marks_mandelbrot_power_per_pixel_fp(void)
{
	mandelbrot_per_pixel_fp();
	g_temp_z = g_old_z;
	g_temp_z.x -= 1;
	CMPLXpwr(g_old_z, g_temp_z, g_temp_z);
	return 1;
}

int mandelbrot_per_pixel_fp(void)
{
	/* floating point mandelbrot */
	/* mandelfp */

	if (g_invert)
	{
		invert_z(&g_initial_z);
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
		magnet2_precalculate_fp();
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
		g_old_z.x = g_parameter.x; /* initial pertubation of parameters set */
		g_old_z.y = g_parameter.y;
		g_color_iter = -1;
	}
	else
	{
		g_old_z.x += g_parameter.x;
		g_old_z.y += g_parameter.y;
	}
	g_temp_z = g_initial_z; /* for spider */
	g_temp_sqr_x = sqr(g_old_z.x);  /* precalculated value for regular Mandelbrot */
	g_temp_sqr_y = sqr(g_old_z.y);
	return 1; /* 1st iteration has been done */
}

int julia_per_pixel_fp(void)
{
	/* floating point julia */
	/* juliafp */
	if (g_invert)
	{
		invert_z(&g_old_z);
	}
	else
	{
		g_old_z.x = dxpixel();
		g_old_z.y = dypixel();
	}
	g_temp_sqr_x = sqr(g_old_z.x);  /* precalculated value for regular Julia */
	g_temp_sqr_y = sqr(g_old_z.y);
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
		invert_z(&g_old_z);
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
	CMPLXtrig1(*g_float_parameter, g_temp_z);
	CMPLXmult(g_temp_z, g_parameter2, g_temp_z);
	return 1;
}

int othermandelfp_per_pixel(void)
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
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

	g_old_z.x += g_parameter.x;      /* initial pertubation of parameters set */
	g_old_z.y += g_parameter.y;

	return 1; /* 1st iteration has been done */
}

int MPCHalley_per_pixel(void)
{
#if !defined(XFRACT)
	/* MPC halley */
	if (g_invert)
	{
		invert_z(&g_initial_z);
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
		invert_z(&g_initial_z);
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
		invert_z(&g_old_z);
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
	g_float_parameter->x = param[4];
	g_float_parameter->y = param[5];
	g_quaternion_c  = param[0];
	g_quaternion_ci = param[1];
	g_quaternion_cj = param[2];
	g_quaternion_ck = param[3];
	return 0;
}

int quaternionfp_per_pixel(void)
{
	g_old_z.x = 0;
	g_old_z.y = 0;
	g_float_parameter->x = 0;
	g_float_parameter->y = 0;
	g_quaternion_c  = dxpixel();
	g_quaternion_ci = dypixel();
	g_quaternion_cj = param[2];
	g_quaternion_ck = param[3];
	return 0;
}

int MarksCplxMandperp(void)
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		if (save_release >= 2004)
		{
			g_initial_z.y = dypixel();
		}
	}
	g_old_z.x = g_initial_z.x + g_parameter.x; /* initial pertubation of parameters set */
	g_old_z.y = g_initial_z.y + g_parameter.y;
	g_temp_sqr_x = sqr(g_old_z.x);  /* precalculated value */
	g_temp_sqr_y = sqr(g_old_z.y);
	g_coefficient = ComplexPower(g_initial_z, g_power);
	return 1;
}

int long_phoenix_per_pixel(void)
{
#if !defined(XFRACT)
	if (g_invert)
	{
		/* invert */
		invert_z(&g_old_z);

		/* watch out for overflow */
		if (sqr(g_old_z.x) + sqr(g_old_z.y) >= 127)
		{
			g_old_z.x = 8;  /* value to bail out in one iteration */
			g_old_z.y = 8;
		}

		/* convert to fudged longs */
		g_old_z_l.x = (long)(g_old_z.x*fudge);
		g_old_z_l.y = (long)(g_old_z.y*fudge);
	}
	else
	{
		g_old_z_l.x = lxpixel();
		g_old_z_l.y = lypixel();
	}
	g_temp_sqr_x_l = multiply(g_old_z_l.x, g_old_z_l.x, bitshift);
	g_temp_sqr_y_l = multiply(g_old_z_l.y, g_old_z_l.y, bitshift);
	g_tmp_z2_l.x = 0; /* use g_tmp_z2_l as the complex Y value */
	g_tmp_z2_l.y = 0;
	return 0;
#else
	return 0;
#endif
}

int phoenix_per_pixel(void)
{
	if (g_invert)
	{
		invert_z(&g_old_z);
	}
	else
	{
		g_old_z.x = dxpixel();
		g_old_z.y = dypixel();
	}
	g_temp_sqr_x = sqr(g_old_z.x);  /* precalculated value */
	g_temp_sqr_y = sqr(g_old_z.y);
	s_temp2.x = 0; /* use s_temp2 as the complex Y value */
	s_temp2.y = 0;
	return 0;
}
int long_mandphoenix_per_pixel(void)
{
#if !defined(XFRACT)
	g_initial_z_l.x = lxpixel();
	if (save_release >= 2004)
	{
		g_initial_z_l.y = lypixel();
	}

	if (g_invert)
	{
		/* invert */
		invert_z(&g_initial_z);

		/* watch out for overflow */
		if (sqr(g_initial_z.x) + sqr(g_initial_z.y) >= 127)
		{
			g_initial_z.x = 8;  /* value to bail out in one iteration */
			g_initial_z.y = 8;
		}

		/* convert to fudged longs */
		g_initial_z_l.x = (long)(g_initial_z.x*fudge);
		g_initial_z_l.y = (long)(g_initial_z.y*fudge);
	}

	g_old_z_l = (useinitorbit == 1) ? g_init_orbit_l : g_initial_z_l;

	g_old_z_l.x += g_parameter_l.x;    /* initial pertubation of parameters set */
	g_old_z_l.y += g_parameter_l.y;
	g_temp_sqr_x_l = multiply(g_old_z_l.x, g_old_z_l.x, bitshift);
	g_temp_sqr_y_l = multiply(g_old_z_l.y, g_old_z_l.y, bitshift);
	g_tmp_z2_l.x = 0;
	g_tmp_z2_l.y = 0;
	return 1; /* 1st iteration has been done */
#else
	return 0;
#endif
}
int mandphoenix_per_pixel(void)
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
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

	g_old_z.x += g_parameter.x;      /* initial pertubation of parameters set */
	g_old_z.y += g_parameter.y;
	g_temp_sqr_x = sqr(g_old_z.x);  /* precalculated value */
	g_temp_sqr_y = sqr(g_old_z.y);
	s_temp2.x = 0;
	s_temp2.y = 0;
	return 1; /* 1st iteration has been done */
}

int
QuaternionFPFractal(void)
{
	double a0, a1, a2, a3, n0, n1, n2, n3;
	a0 = g_old_z.x;
	a1 = g_old_z.y;
	a2 = g_float_parameter->x;
	a3 = g_float_parameter->y;

	n0 = a0*a0-a1*a1-a2*a2-a3*a3 + g_quaternion_c;
	n1 = 2*a0*a1 + g_quaternion_ci;
	n2 = 2*a0*a2 + g_quaternion_cj;
	n3 = 2*a0*a3 + g_quaternion_ck;
	/* Check bailout */
	g_magnitude = a0*a0 + a1*a1 + a2*a2 + a3*a3;
	if (g_magnitude > g_rq_limit)
	{
		return 1;
	}
	g_old_z.x = g_new_z.x = n0;
	g_old_z.y = g_new_z.y = n1;
	g_float_parameter->x = n2;
	g_float_parameter->y = n3;
	return 0;
}

int
HyperComplexFPFractal(void)
{
	_HCMPLX hold, hnew;
	hold.x = g_old_z.x;
	hold.y = g_old_z.y;
	hold.z = g_float_parameter->x;
	hold.t = g_float_parameter->y;

/*   HComplexSqr(&hold, &hnew); */
	HComplexTrig0(&hold, &hnew);

	hnew.x += g_quaternion_c;
	hnew.y += g_quaternion_ci;
	hnew.z += g_quaternion_cj;
	hnew.t += g_quaternion_ck;

	g_old_z.x = g_new_z.x = hnew.x;
	g_old_z.y = g_new_z.y = hnew.y;
	g_float_parameter->x = hnew.z;
	g_float_parameter->y = hnew.t;

	/* Check bailout */
	g_magnitude = sqr(g_old_z.x) + sqr(g_old_z.y) + sqr(g_float_parameter->x) + sqr(g_float_parameter->y);
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
	return g_bail_out_fp();
}

int
EscherfpFractal(void) /* Science of Fractal Images pp. 185, 187 */
{
	_CMPLX oldtest, newtest, testsqr;
	double testsize = 0.0;
	long testiter = 0;

	g_new_z.x = g_temp_sqr_x - g_temp_sqr_y; /* standard Julia with C == (0.0, 0.0i) */
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
		return g_bail_out_fp();
	}
	else /* make distinct level sets if point stayed in target set */
	{
		g_color_iter = ((3L*g_color_iter) % 255L) + 1L;
		return 1;
	}
}

/* re-use static roots variable
	memory for mandelmix4 */

#define A g_static_roots[ 0]
#define B g_static_roots[ 1]
#define C g_static_roots[ 2]
#define D g_static_roots[ 3]
#define F g_static_roots[ 4]
#define G g_static_roots[ 5]
#define H g_static_roots[ 6]
#define J g_static_roots[ 7]
#define K g_static_roots[ 8]
#define L g_static_roots[ 9]
#define Z g_static_roots[10]

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
		invert_z(&g_initial_z);
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
	return g_bail_out_fp();
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
	if (g_use_grid)
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
