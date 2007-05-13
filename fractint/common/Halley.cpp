#include "port.h"
#include "prototyp.h"
#include "cmplx.h"
#include "fractint.h"
#include "externs.h"
#include "fractype.h"
#include "Halley.h"

#if !defined(XFRACT)
extern struct MPC mpcold;
extern struct MPC mpcnew;
#endif

static Halley s_halley;
static HalleyMP s_halley_mp;

static double modulus(const DComplex &z)
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

int halley_per_pixel_mpc()
{
	return s_halley_mp.per_pixel();
}

int halley_orbit_mpc()
{
	return s_halley_mp.orbit();
}

int Halley::setup()
{
	/* Halley */
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

int HalleyMP::bail_out()
{
#if !defined(XFRACT)
	static struct MP mptmpbailout;
	mptmpbailout = *MPabs(*MPsub(MPCmod(mpcnew), MPCmod(mpcold)));
	if (MPcmp(mptmpbailout, g_parameter2_x_mp) < 0)
	{
		return 1;
	}
	mpcold = mpcnew;
#endif
	return 0;
}

int Halley::orbit()
{
	/*  X(X^a - 1) = 0, Halley Map */
	/*  a = g_parameter.x = degree, relaxation coeff. = g_parameter.y, epsilon = g_parameter2.x  */

	int ihal;
	DComplex XtoAlessOne, XtoA, XtoAplusOne; /* a-1, a, a + 1 */
	DComplex FX, F1prime, F2prime, Halnumer1, Halnumer2, Haldenom;
	DComplex relax;

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

int HalleyMP::setup()
{
	Halley::setup();

#if !defined(XFRACT)
	if (g_fractal_type == FRACTYPE_HALLEY_MP)
	{
		m_a_plus_1_mp = *d2MP((double) m_a_plus_1);
		m_a_plus_1_degree_mp = *d2MP((double) m_a_plus_1_degree);
		g_temp_parameter_mpc.x = *d2MP(g_parameter.y);
		g_temp_parameter_mpc.y = *d2MP(g_parameter2.y);
		g_parameter2_x_mp = *d2MP(g_parameter2.x);
		g_one_mp        = *d2MP(1.0);
	}
#endif

	return 1;
}

int HalleyMP::orbit()
{
#if !defined(XFRACT)
	/*  X(X^a - 1) = 0, Halley Map */
	/*  a = g_parameter.x,  relaxation coeff. = g_parameter.y,  epsilon = g_parameter2.x  */

	int ihal;
	struct MPC mpcXtoAlessOne, mpcXtoA;
	struct MPC mpcXtoAplusOne; /* a-1, a, a + 1 */
	struct MPC mpcFX, mpcF1prime, mpcF2prime, mpcHalnumer1;
	struct MPC mpcHalnumer2, mpcHaldenom, mpctmp;

	g_overflow_mp = 0;
	mpcXtoAlessOne.x = mpcold.x;
	mpcXtoAlessOne.y = mpcold.y;
	for (ihal = 2; ihal < g_degree; ihal++)
	{
		mpctmp.x = *MPsub(*MPmul(mpcXtoAlessOne.x, mpcold.x), *MPmul(mpcXtoAlessOne.y, mpcold.y));
		mpctmp.y = *MPadd(*MPmul(mpcXtoAlessOne.x, mpcold.y), *MPmul(mpcXtoAlessOne.y, mpcold.x));
		mpcXtoAlessOne.x = mpctmp.x;
		mpcXtoAlessOne.y = mpctmp.y;
	}
	mpcXtoA.x = *MPsub(*MPmul(mpcXtoAlessOne.x, mpcold.x), *MPmul(mpcXtoAlessOne.y, mpcold.y));
	mpcXtoA.y = *MPadd(*MPmul(mpcXtoAlessOne.x, mpcold.y), *MPmul(mpcXtoAlessOne.y, mpcold.x));
	mpcXtoAplusOne.x = *MPsub(*MPmul(mpcXtoA.x, mpcold.x), *MPmul(mpcXtoA.y, mpcold.y));
	mpcXtoAplusOne.y = *MPadd(*MPmul(mpcXtoA.x, mpcold.y), *MPmul(mpcXtoA.y, mpcold.x));

	mpcFX.x = *MPsub(mpcXtoAplusOne.x, mpcold.x);
	mpcFX.y = *MPsub(mpcXtoAplusOne.y, mpcold.y); /* FX = X^(a + 1) - X  = F */

	mpcF2prime.x = *MPmul(m_a_plus_1_degree_mp, mpcXtoAlessOne.x); /* g_a_plus_1_degree_mp in setup */
	mpcF2prime.y = *MPmul(m_a_plus_1_degree_mp, mpcXtoAlessOne.y);        /* F" */

	mpcF1prime.x = *MPsub(*MPmul(m_a_plus_1_mp, mpcXtoA.x), g_one_mp);
	mpcF1prime.y = *MPmul(m_a_plus_1_mp, mpcXtoA.y);                   /*  F'  */

	mpctmp.x = *MPsub(*MPmul(mpcF2prime.x, mpcFX.x), *MPmul(mpcF2prime.y, mpcFX.y));
	mpctmp.y = *MPadd(*MPmul(mpcF2prime.x, mpcFX.y), *MPmul(mpcF2prime.y, mpcFX.x));
	/*  F*F"  */

	mpcHaldenom.x = *MPadd(mpcF1prime.x, mpcF1prime.x);
	mpcHaldenom.y = *MPadd(mpcF1prime.y, mpcF1prime.y);      /*  2*F'  */

	mpcHalnumer1 = MPCdiv(mpctmp, mpcHaldenom);        /*  F"F/2F'  */
	mpctmp.x = *MPsub(mpcF1prime.x, mpcHalnumer1.x);
	mpctmp.y = *MPsub(mpcF1prime.y, mpcHalnumer1.y); /*  F' - F"F/2F'  */
	mpcHalnumer2 = MPCdiv(mpcFX, mpctmp);

	mpctmp   =  MPCmul(g_temp_parameter_mpc, mpcHalnumer2);  /* g_temp_parameter_mpc is relaxation coef. */
	mpcnew = MPCsub(mpcold, mpctmp);
	g_new_z    = MPC2cmplx(mpcnew);
	return bail_out() || g_overflow_mp;
#else
	return 0;
#endif
}

int HalleyMP::per_pixel()
{
#if !defined(XFRACT)
	/* MPC halley */
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

	mpcold.x = *d2MP(g_initial_z.x);
	mpcold.y = *d2MP(g_initial_z.y);

	return 0;
#else
	return 0;
#endif
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
