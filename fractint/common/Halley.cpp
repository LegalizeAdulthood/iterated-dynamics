#include "port.h"
#include "prototyp.h"
#include "cmplx.h"
#include "fractint.h"
#include "externs.h"
#include "fractype.h"
#include "fpu.h"
#include "fractals.h"

#include "Halley.h"

static Halley s_halley;

static double modulus(const ComplexD &z)
{
	return sqr(z.x) + sqr(z.y);
}

int halley_setup()
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

int Halley::setup()
{
	g_periodicity_check = 0;

	g_fractal_type = g_user_float_flag ? FRACTYPE_HALLEY : FRACTYPE_HALLEY_MP;

	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];

	g_degree = (int) g_parameter.x;
	if (g_degree < 2)
	{
		g_degree = 2;
	}
	g_parameters[0] = (double) g_degree;

	/*  precalculated values */
	m_a_plus_1 = g_degree + 1; /* a + 1 */
	m_a_plus_1_degree = m_a_plus_1*g_degree;

	g_symmetry = (g_degree % 2) ? SYMMETRY_X_AXIS : SYMMETRY_XY_AXIS;   /* odd, even */
	return 1;
}

int Halley::bail_out()
{
	if (fabs(modulus(g_new_z)-modulus(g_old_z)) < g_parameter2.x)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int Halley::orbit()
{
	/*  X(X^a - 1) = 0, Halley Map */
	/*  a = g_parameter.x = degree, relaxation coeff. = g_parameter.y, epsilon = g_parameter2.x  */

	int ihal;
	ComplexD XtoAlessOne;
	ComplexD XtoA;
	ComplexD XtoAplusOne; /* a-1, a, a + 1 */
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

	CMPLXsub(XtoAplusOne, g_old_z, FX);        /* FX = X^(a + 1) - X  = F */
	F2prime.x = m_a_plus_1_degree*XtoAlessOne.x; /* g_a_plus_1_degree in setup */
	F2prime.y = m_a_plus_1_degree*XtoAlessOne.y;        /* F" */

	F1prime.x = m_a_plus_1*XtoA.x - 1.0;
	F1prime.y = m_a_plus_1*XtoA.y;                             /*  F'  */

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
	relax.y = g_parameters[3];
	FPUcplxmul(&relax, &Halnumer2, &Halnumer2);
	g_new_z.x = g_old_z.x - Halnumer2.x;
	g_new_z.y = g_old_z.y - Halnumer2.y;
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
		g_initial_z.x = g_dx_pixel();
		if (g_save_release >= 2004)
		{
			g_initial_z.y = g_dy_pixel();
		}
	}

	g_old_z = g_initial_z;

	return 0; /* 1st iteration is not done */
}
