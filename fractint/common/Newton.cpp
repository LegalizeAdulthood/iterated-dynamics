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

static Newton s_newton;
static NewtonComplex s_newton_complex;
static ComplexD s_static_roots[16]; // roots array for degree 16 or less
static ComplexD *s_roots = s_static_roots;

static double distance(const ComplexD &z1, const ComplexD &z2)
{
	return sqr(z1.real() - z2.real()) + sqr(z1.imag() - z2.imag());
}

static double distance_from_1(const ComplexD &z)
{
	return (z.real() - 1.0)*(z.real() - 1.0) + z.imag()*z.imag();
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
#if !defined(NO_FIXED_POINT_MATH)
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
	g_degree = int(g_parameter.real());
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
		g_externs.SetBasin(g_parameter.imag() ? 2 : 1); // stripes
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
			s_roots[i].real(std::cos(i*g_two_pi/double(g_degree)));
			s_roots[i].imag(std::sin(i*g_two_pi/double(g_degree)));
		}
	}

	g_parameters[P1_REAL] = double(g_degree);
	g_symmetry = (g_degree % 4 == 0) ? SYMMETRY_XY_AXIS : SYMMETRY_X_AXIS;

	g_calculate_type = standard_fractal;
	return 1;
}

int Newton::orbit()
{
	pow(&g_old_z, g_degree-1, &g_temp_z);
	g_new_z = g_temp_z*g_old_z;

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
	g_new_z.real(m_degree_minus_1_over_degree*g_new_z.real() + m_root_over_degree);
	g_new_z.imag(g_new_z.imag()*m_degree_minus_1_over_degree);

	// Watch for divide underflow
	double s_t2 = g_temp_z.real()*g_temp_z.real() + g_temp_z.imag()*g_temp_z.imag();
	if (s_t2 < FLT_MIN)
	{
		return 1;
	}
	else
	{
		s_t2 = 1.0/s_t2;
		g_old_z.real(s_t2*(g_new_z.real()*g_temp_z.real() + g_new_z.imag()*g_temp_z.imag()));
		g_old_z.imag(s_t2*(g_new_z.imag()*g_temp_z.real() - g_new_z.real()*g_temp_z.imag()));
	}
	return 0;
}

bool NewtonComplex::setup()
{
	g_threshold = 0.001;
	g_periodicity_check = 0;
	if (g_parameters[P1_REAL] != 0.0 || g_parameters[P1_IMAG] != 0.0 || g_parameters[P2_REAL] != 0.0 ||
		g_parameters[P2_IMAG] != 0.0)
	{
		g_c_root.real(g_parameters[P2_REAL]);
		g_c_root.imag(g_parameters[P2_IMAG]);
		g_c_degree.real(g_parameters[P1_REAL]);
		g_c_degree.imag(g_parameters[P1_IMAG]);
		BaseLog = ComplexLog(g_c_root);
		TwoPi = std::asin(1.0)*4;
	}
	return true;
}

int NewtonComplex::orbit()
{
	ComplexD cd1;

	// new = ((g_c_degree-1)*old**g_c_degree) + g_c_root
	//		 ----------------------------------
	//       g_c_degree*old**(g_c_degree-1)

	cd1.real(g_c_degree.real() - 1.0);
	cd1.imag(g_c_degree.imag());

	temp = pow(g_old_z, cd1);
	g_new_z = temp*g_old_z;

	g_temp_z.real(g_new_z.real() - g_c_root.real());
	g_temp_z.imag(g_new_z.imag() - g_c_root.imag());
	if ((sqr(g_temp_z.real()) + sqr(g_temp_z.imag())) < g_threshold)
	{
		return 1;
	}

	g_temp_z = g_new_z*cd1;
	g_temp_z.real(g_temp_z.real() + g_c_root.real());
	g_temp_z.imag(g_temp_z.imag() + g_c_root.imag());

	cd1 = temp*g_c_degree;
	g_old_z = g_temp_z/cd1;
	if (g_overflow)
	{
		return 1;
	}
	g_new_z = g_old_z;
	return 0;
}
