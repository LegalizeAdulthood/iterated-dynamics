#include <string>

#include "port.h"
#include "prototyp.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"
#include "fractype.h"
#include "fpu.h"
#include "fractals.h"
#include "Halley.h"
#include "mpmath.h"

static Halley s_halley;

static double modulus(const ComplexD &z)
{
	return sqr(z.real()) + sqr(z.imag());
}

bool halley_setup()
{
	return s_halley.setup();
}

int halley_per_pixel()
{
	return s_halley.per_pixel();
}

int halley_orbit_fp()
{
	return s_halley.orbit();
}

// TODO: delete this routine
int halley_per_pixel_mpc()
{
	return 0;
}

// TODO: delete this routine
int halley_orbit_mpc()
{
	return 0;
}

bool Halley::setup()
{
	g_periodicity_check = 0;

	g_fractal_type = g_user_float_flag ? FRACTYPE_HALLEY : FRACTYPE_HALLEY_MP;

	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];

	g_degree = int(g_parameter.real());
	if (g_degree < 2)
	{
		g_degree = 2;
	}
	g_parameters[P1_REAL] = double(g_degree);

	// precalculated values
	m_a_plus_1 = g_degree + 1; // a + 1
	m_a_plus_1_degree = m_a_plus_1*g_degree;

	g_symmetry = (g_degree % 2) ? SYMMETRY_X_AXIS : SYMMETRY_XY_AXIS;   // odd, even
	return true;
}

int Halley::bail_out()
{
	if (fabs(modulus(g_new_z)-modulus(g_old_z)) < g_parameter2.real())
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int Halley::orbit()
{
	// X(X^a - 1) = 0, Halley Map
	// a = g_parameter.real() = degree, relaxation coeff. = g_parameter.imag(), epsilon = g_parameter2.real()

	int ihal;
	ComplexD XtoAlessOne;
	ComplexD XtoA;
	ComplexD XtoAplusOne; // a-1, a, a + 1
	ComplexD FX;
	ComplexD F1prime;
	ComplexD F2prime;
	ComplexD Halnumer1;
	ComplexD Halnumer2;
	ComplexD Haldenom;
	ComplexD relax;

	XtoAlessOne = g_old_z;
	for (ihal = 2; ihal < g_degree; ihal++)
	{
		FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoAlessOne);
	}
	FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoA);
	FPUcplxmul(&g_old_z, &XtoA, &XtoAplusOne);

	CMPLXsub(XtoAplusOne, g_old_z, FX);        // FX = X^(a + 1) - X = F
	F2prime.real(m_a_plus_1_degree*XtoAlessOne.real()); // g_a_plus_1_degree in setup
	F2prime.imag(m_a_plus_1_degree*XtoAlessOne.imag());        // F"

	F1prime.real(m_a_plus_1*XtoA.real() - 1.0);
	F1prime.imag(m_a_plus_1*XtoA.imag());                             // F'

	FPUcplxmul(&F2prime, &FX, &Halnumer1);                  // F*F"
	Haldenom.real(F1prime.real() + F1prime.real());
	Haldenom.imag(F1prime.imag() + F1prime.imag());                     // 2*F'

	FPUcplxdiv(&Halnumer1, &Haldenom, &Halnumer1);         // F"F/2F'
	CMPLXsub(F1prime, Halnumer1, Halnumer2);          // F' - F"F/2F'
	FPUcplxdiv(&FX, &Halnumer2, &Halnumer2);
	// g_parameter.imag() is relaxation coef.
	// new.real(g_old_z.real() - (g_parameter.imag()*Halnumer2.real()));
	// new.imag(g_old_z.imag() - (g_parameter.imag()*Halnumer2.imag()));
	relax.real(g_parameter.imag());
	relax.imag(g_parameters[P2_IMAG]);
	FPUcplxmul(&relax, &Halnumer2, &Halnumer2);
	g_new_z.real(g_old_z.real() - Halnumer2.real());
	g_new_z.imag(g_old_z.imag() - Halnumer2.imag());
	return bail_out();
}

int Halley::per_pixel()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}

	g_old_z = g_initial_z;

	return 0; // 1st iteration is not done
}
