#include <string>

#include "port.h"
#include "cmplx.h"
#include "id.h"
#include "prototyp.h"
#include "externs.h"
#include "fractype.h"
#include "mpmath.h"
#include "fpu.h"

#include "calcfrac.h"
#include "Externals.h"
#include "fractals.h"
#include "MathUtil.h"
#include "Newton.h"

extern double TwoPi;
extern ComplexD temp;
extern ComplexD BaseLog;
extern ComplexD cdegree;
extern ComplexD croot;

static Newton s_newton;
static NewtonComplex s_newton_complex;
static ComplexD s_static_roots[16] = { { 0.0, 0.0 } }; // roots array for degree 16 or less 
static ComplexD *s_roots = s_static_roots;

static double distance(const ComplexD &z1, const ComplexD &z2)
{
	return sqr(z1.x - z2.x) + sqr(z1.y - z2.y);
}

static double distance_from_1(const ComplexD &z)
{
	return (((z).x-1.0)*((z).x-1.0) + ((z).y)*((z).y));
}

static int complex_multiply(ComplexD arg1, ComplexD arg2, ComplexD *pz)
{
	pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
	pz->y = arg1.x*arg2.y + arg1.y*arg2.x;
	return 0;
}

bool newton_setup()
{
	return s_newton.setup();
}

int newton2_orbit()
{
	return s_newton.orbit();
}

// TODO: delete this routine
int newton_orbit_mpc()
{
	return 0;
}

bool complex_newton_setup()
{
	return s_newton_complex.setup();
}

int complex_newton()
{
	return s_newton_complex.orbit();
}

bool Newton::setup()           // Newton/NewtBasin Routines 
{
	int i;
#if !defined(XFRACT)
	if (g_debug_mode != DEBUGMODE_FORCE_FP_NEWTON)
	{
		if (g_fractal_type == FRACTYPE_NEWTON_MP)
		{
			g_fractal_type = FRACTYPE_NEWTON;
		}
		else if (g_fractal_type == FRACTYPE_NEWTON_BASIN_MP)
		{
			g_fractal_type = FRACTYPE_NEWTON_BASIN;
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
	// set up table of roots of 1 along unit circle 
	g_degree = int(g_parameter.x);
	if (g_degree < 2)
	{
		g_degree = 3;   // defaults to 3, but 2 is possible 
	}

	// precalculated values 
	m_root_over_degree = 1.0/double(g_degree);
	m_degree_minus_1_over_degree = double(g_degree - 1)/double(g_degree);
	g_threshold = .3*MathUtil::Pi/g_degree; // less than half distance between roots 

	g_externs.SetBasin(0);
	if (s_roots != s_static_roots)
	{
		delete[] s_roots;
		s_roots = s_static_roots;
	}

	if (g_fractal_type == FRACTYPE_NEWTON_BASIN)
	{
		g_externs.SetBasin(g_parameter.y ? 2 : 1); // stripes
		if (g_degree > 16)
		{
			s_roots = new ComplexD[g_degree];
			if (s_roots == 0)
			{
				s_roots = s_static_roots;
				g_degree = 16;
			}
		}
		else
		{
			s_roots = s_static_roots;
		}

		// list of roots to discover where we converged for newtbasin 
		for (i = 0; i < g_degree; i++)
		{
			s_roots[i].x = cos(i*g_two_pi/double(g_degree));
			s_roots[i].y = sin(i*g_two_pi/double(g_degree));
		}
	}

	g_parameters[0] = double(g_degree);
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
			// this code determines which degree-th root of root the
			// Newton formula converges to. The roots of a 1 are
			// distributed on a circle of radius 1 about the origin.
			for (i = 0; i < g_degree; i++)
			{
				// color in alternating shades with iteration according to
				// which root of 1 it converged to
				if (distance(s_roots[i], g_old_z) < g_threshold)
				{
					tmpcolor = (g_externs.Basin() == 2) ?
						(1 + (i & 7) + ((g_color_iter & 1) << 3)) : (1 + i);
					break;
				}
			}
			g_color_iter = (tmpcolor == -1) ? 0 : tmpcolor;
		}
		return 1;
	}
	g_new_z.real(m_degree_minus_1_over_degree*g_new_z.x + m_root_over_degree);
	g_new_z.y *= m_degree_minus_1_over_degree;

	// Watch for divide underflow 
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

bool NewtonComplex::setup()
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
	return true;
}

int NewtonComplex::orbit()
{
	ComplexD cd1;

	// new = ((cdegree-1)*old**cdegree) + croot
	//		 ----------------------------------
	//       cdegree*old**(cdegree-1)

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
