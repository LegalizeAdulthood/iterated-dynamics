#include "port.h"
#include "cmplx.h"
#include "fractint.h"
#include "prototyp.h"
#include "externs.h"
#include "fractype.h"
#include "mpmath.h"
#include "Newton.h"
#include "MathUtil.h"

#if !defined(XFRACT)
static struct MPC mpctmp, mpctmp1;
extern struct MPC mpcold, mpcnew;
#endif

extern double TwoPi;
extern ComplexD temp;
extern ComplexD BaseLog;
extern ComplexD cdegree;
extern ComplexD croot;

static Newton s_newton;
static NewtonMPC s_newton_mpc;
static NewtonComplex s_newton_complex;

static double distance(const ComplexD &z1, const ComplexD &z2)
{
	return sqr(z1.x - z2.x) + sqr(z1.y - z2.y);
}

static double distance_from_1(const ComplexD &z)
{
	return (((z).x-1.0)*((z).x-1.0) + ((z).y)*((z).y));
}

#if !defined(XFRACT)
static struct MP pMPsqr(struct MP &z)
{
	return *MPmul(z, z);
}

static struct MP operator-(const struct MP &left, const struct MP &right)
{
	return *MPsub(left, right);
}

static struct MP operator+(const struct MP &left, const struct MP &right)
{
	return *MPadd(left, right);
}

static struct MP MPCdistance(const struct MPC &z1, const struct MPC &z2)
{
	 return pMPsqr(z1.x - z2.x) + pMPsqr(z1.y - z2.y);
							/* Distance of complex z from unit circle */
}
#endif

static int complex_multiply(ComplexD arg1, ComplexD arg2, ComplexD *pz)
{
	pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
	pz->y = arg1.x*arg2.y + arg1.y*arg2.x;
	return 0;
}

int newton_setup()
{
	return s_newton.setup();
}

int newton2_orbit()
{
	return s_newton.orbit();
}

int newton_orbit_mpc()
{
	return s_newton_mpc.orbit();
}

int complex_newton_setup()
{
	return s_newton_complex.setup();
}

int complex_newton()
{
	return s_newton_complex.orbit();
}

int Newton::setup()           /* Newton/NewtBasin Routines */
{
	int i;
#if !defined(XFRACT)
	if (g_debug_flag != DEBUGFLAG_FORCE_FP_NEWTON)
	{
		if (g_fpu != 0)
		{
			if (g_fractal_type == FRACTYPE_NEWTON_MP)
			{
				g_fractal_type = FRACTYPE_NEWTON;
			}
			else if (g_fractal_type == FRACTYPE_NEWTON_BASIN_MP)
			{
				g_fractal_type = FRACTYPE_NEWTON_BASIN;
			}
		}
		else
		{
			if (g_fractal_type == FRACTYPE_NEWTON)
			{
				g_fractal_type = FRACTYPE_NEWTON_MP;
			}
			else if (g_fractal_type == FRACTYPE_NEWTON_BASIN)
			{
				g_fractal_type = FRACTYPE_NEWTON_BASIN_MP;
			}
		}
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	}
#else
	if (g_fractal_type == FRACTYPE_NEWTON_MP)
	{
		g_fractal_type = FRACTYPE_NEWTON;
	}
	else if (g_fractal_type == FRACTYPE_NEWTON_BASIN_MP)
	{
		g_fractal_type = FRACTYPE_NEWTON_BASIN;
	}

	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
#endif
	/* set up table of roots of 1 along unit circle */
	g_degree = (int)g_parameter.x;
	if (g_degree < 2)
	{
		g_degree = 3;   /* defaults to 3, but 2 is possible */
	}
	g_root = 1;

	/* precalculated values */
	m_root_over_degree       = (double)g_root / (double)g_degree;
	m_degree_minus_1_over_degree      = (double)(g_degree - 1) / (double)g_degree;
	g_max_color     = 0;
	g_threshold    = .3*MathUtil::Pi/g_degree; /* less than half distance between roots */
#if !defined(XFRACT)
	if (g_fractal_type == FRACTYPE_NEWTON_MP || g_fractal_type == FRACTYPE_NEWTON_BASIN_MP)
	{
		m_root_over_degree_mp			= *d2MP(m_root_over_degree);
		m_degree_minus_1_over_degree_mp	= *d2MP(m_degree_minus_1_over_degree);
		g_threshold_mp					= *d2MP(g_threshold);
		g_one_mp						= *d2MP(1.0);
	}
#endif

	g_basin = 0;
	if (g_roots != g_static_roots)
	{
		free(g_roots);
		g_roots = g_static_roots;
	}

	if (g_fractal_type == FRACTYPE_NEWTON_BASIN)
	{
		g_basin = g_parameter.y ? 2 : 1; /*stripes */
		if (g_degree > 16)
		{
			g_roots = (ComplexD *) malloc(g_degree*sizeof(ComplexD));
			if (g_roots == NULL)
			{
				g_roots = g_static_roots;
				g_degree = 16;
			}
		}
		else
		{
			g_roots = g_static_roots;
		}

		/* list of roots to discover where we converged for newtbasin */
		for (i = 0; i < g_degree; i++)
		{
			g_roots[i].x = cos(i*g_two_pi/(double)g_degree);
			g_roots[i].y = sin(i*g_two_pi/(double)g_degree);
		}
	}
#if !defined(XFRACT)
	else if (g_fractal_type == FRACTYPE_NEWTON_BASIN_MP)
	{
		g_basin = g_parameter.y ? 2 : 1; /*stripes */

		if (g_degree > 16)
		{
			g_roots_mpc = (struct MPC *) malloc(g_degree*sizeof(struct MPC));
			if (g_roots_mpc == NULL)
			{
				g_roots_mpc = (struct MPC *)g_static_roots;
				g_degree = 16;
			}
		}
		else
		{
			g_roots_mpc = (struct MPC *)g_static_roots;
		}

		/* list of roots to discover where we converged for newtbasin */
		for (i = 0; i < g_degree; i++)
		{
			g_roots_mpc[i].x = *d2MP(cos(i*g_two_pi/(double)g_degree));
			g_roots_mpc[i].y = *d2MP(sin(i*g_two_pi/(double)g_degree));
		}
	}
#endif

	g_parameters[0] = (double)g_degree;
	g_symmetry = (g_degree % 4 == 0) ? SYMMETRY_XY_AXIS : SYMMETRY_X_AXIS;

	g_calculate_type = standard_fractal;
	return 1;
}

int Newton::orbit()
{
	complex_power(&g_old_z, g_degree-1, &g_temp_z);
	complex_multiply(g_temp_z, g_old_z, &g_new_z);

	if (distance_from_1(g_new_z) < g_threshold)
	{
		if (g_fractal_type == FRACTYPE_NEWTON_BASIN || g_fractal_type == FRACTYPE_NEWTON_BASIN_MP)
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
						(1 + (i & 7) + ((g_color_iter & 1) << 3)) : (1 + i);
					break;
				}
			}
			g_color_iter = (tmpcolor == -1) ? g_max_color : tmpcolor;
		}
		return 1;
	}
	g_new_z.x = m_degree_minus_1_over_degree*g_new_z.x + m_root_over_degree;
	g_new_z.y *= m_degree_minus_1_over_degree;

	/* Watch for divide underflow */
	double s_t2 = g_temp_z.x*g_temp_z.x + g_temp_z.y*g_temp_z.y;
	if (s_t2 < FLT_MIN)
	{
		return 1;
	}
	else
	{
		s_t2 = 1.0/s_t2;
		g_old_z.x = s_t2*(g_new_z.x*g_temp_z.x + g_new_z.y*g_temp_z.y);
		g_old_z.y = s_t2*(g_new_z.y*g_temp_z.x - g_new_z.x*g_temp_z.y);
	}
	return 0;
}

int NewtonMPC::orbit()
{
#if !defined(XFRACT)
	g_overflow_mp = 0;
	mpctmp   = MPCpow(mpcold, g_degree-1);

	mpcnew.x = *MPsub(*MPmul(mpctmp.x, mpcold.x), *MPmul(mpctmp.y, mpcold.y));
	mpcnew.y = *MPadd(*MPmul(mpctmp.x, mpcold.y), *MPmul(mpctmp.y, mpcold.x));
	mpctmp1.x = *MPsub(mpcnew.x, g_one_mpc.x);
	mpctmp1.y = *MPsub(mpcnew.y, g_one_mpc.y);
	if (MPcmp(MPCmod(mpctmp1), g_threshold_mp)< 0)
	{
		if (g_fractal_type == FRACTYPE_NEWTON_BASIN_MP)
		{
			long tmpcolor;
			int i;
			tmpcolor = -1;
			for (i = 0; i < g_degree; i++)
			{
				if (MPcmp(MPCdistance(g_roots_mpc[i], mpcold), g_threshold_mp) < 0)
				{
					tmpcolor = (g_basin == 2) ?
						(1 + (i & 7) + ((g_color_iter & 1) << 3)) : (1 + i);
					break;
				}
			}
			g_color_iter = (tmpcolor == -1) ? g_max_color : tmpcolor;
		}
		return 1;
	}

	{
		struct MP mpt2;
		mpcnew.x = *MPadd(*MPmul(m_degree_minus_1_over_degree_mp, mpcnew.x), m_root_over_degree_mp);
		mpcnew.y = *MPmul(mpcnew.y, m_degree_minus_1_over_degree_mp);
		mpt2 = MPCmod(mpctmp);
		mpt2 = *MPdiv(g_one_mp, mpt2);
		mpcold.x = *MPmul(mpt2, (*MPadd(*MPmul(mpcnew.x, mpctmp.x), *MPmul(mpcnew.y, mpctmp.y))));
		mpcold.y = *MPmul(mpt2, (*MPsub(*MPmul(mpcnew.y, mpctmp.x), *MPmul(mpcnew.x, mpctmp.y))));
	}
	g_new_z.x = *MP2d(mpcold.x);
	g_new_z.y = *MP2d(mpcold.y);
	return g_overflow_mp;
#else
	return 0;
#endif
}

int NewtonComplex::setup()
{
	g_threshold = 0.001;
	g_periodicity_check = 0;
	if (g_parameters[0] != 0.0 || g_parameters[1] != 0.0 || g_parameters[2] != 0.0 ||
		g_parameters[3] != 0.0)
	{
		croot.x = g_parameters[2];
		croot.y = g_parameters[3];
		cdegree.x = g_parameters[0];
		cdegree.y = g_parameters[1];
		FPUcplxlog(&croot, &BaseLog);
		TwoPi = asin(1.0)*4;
	}
	return 1;
}

int NewtonComplex::orbit()
{
	ComplexD cd1;

	/* new = ((cdegree-1)*old**cdegree) + croot
				----------------------------------
                 cdegree*old**(cdegree-1)         */

	cd1.x = cdegree.x - 1.0;
	cd1.y = cdegree.y;

	temp = ComplexPower(g_old_z, cd1);
	FPUcplxmul(&temp, &g_old_z, &g_new_z);

	g_temp_z.x = g_new_z.x - croot.x;
	g_temp_z.y = g_new_z.y - croot.y;
	if ((sqr(g_temp_z.x) + sqr(g_temp_z.y)) < g_threshold)
	{
		return 1;
	}

	FPUcplxmul(&g_new_z, &cd1, &g_temp_z);
	g_temp_z.x += croot.x;
	g_temp_z.y += croot.y;

	FPUcplxmul(&temp, &cdegree, &cd1);
	FPUcplxdiv(&g_temp_z, &cd1, &g_old_z);
	if (g_overflow)
	{
		return 1;
	}
	g_new_z = g_old_z;
	return 0;
}
