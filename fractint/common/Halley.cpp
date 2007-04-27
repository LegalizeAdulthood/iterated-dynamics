#include "port.h"
#include "cmplx.h"
#include "fractint.h"
#include "externs.h"
#include "prototyp.h"
#include "fractype.h"
#include "halley.h"

#define modulus(z)			(sqr((z).x) + sqr((z).y))

static Halley s_halley;

int halley_setup()
{
	return s_halley.setup();
}

int bail_out_halley()
{
	return s_halley.bail_out();
}

int bail_out_halley_mpc()
{
	return s_halley.bail_out_mpc();
}

int halley_per_pixel()
{
	return s_halley.per_pixel();
}

int halley_orbit_fp()
{
	return s_halley.orbit_fp();
}

int halley_per_pixel_mpc()
{
	return s_halley.per_pixel_mpc();
}

int halley_orbit_mpc()
{
	return s_halley.orbit_mpc();
}

int Halley::setup()
{
	/* Halley */
	g_periodicity_check = 0;

	g_fractal_type = g_user_float_flag ? HALLEY : MPHALLEY;

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

#if !defined(XFRACT)
	if (g_fractal_type == MPHALLEY)
	{
		setMPfunctions();
		m_a_plus_1_mp = *pd2MP((double) m_a_plus_1);
		m_a_plus_1_degree_mp = *pd2MP((double) m_a_plus_1_degree);
		g_temp_parameter_mpc.x = *pd2MP(g_parameter.y);
		g_temp_parameter_mpc.y = *pd2MP(g_parameter2.y);
		g_parameter2_x_mp = *pd2MP(g_parameter2.x);
		g_one_mp        = *pd2MP(1.0);
	}
#endif

	g_symmetry = (g_degree % 2) ? XAXIS : XYAXIS;   /* odd, even */
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

#if !defined(XFRACT)
#define MPCmod(m) (*pMPadd(*pMPmul((m).x, (m).x), *pMPmul((m).y, (m).y)))

int Halley::bail_out_mpc()
{
	static struct MP mptmpbailout;
	mptmpbailout = *MPabs(*pMPsub(MPCmod(m_new_mpc), MPCmod(m_old_mpc)));
	if (pMPcmp(mptmpbailout, g_parameter2_x_mp) < 0)
	{
		return 1;
	}
	m_old_mpc = m_new_mpc;
	return 0;
}
#endif

int Halley::orbit_fp()
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
	return bail_out_halley();
}

int Halley::orbit_mpc()
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
	mpcXtoAlessOne.x = m_old_mpc.x;
	mpcXtoAlessOne.y = m_old_mpc.y;
	for (ihal = 2; ihal < g_degree; ihal++)
	{
		mpctmp.x = *pMPsub(*pMPmul(mpcXtoAlessOne.x, m_old_mpc.x), *pMPmul(mpcXtoAlessOne.y, m_old_mpc.y));
		mpctmp.y = *pMPadd(*pMPmul(mpcXtoAlessOne.x, m_old_mpc.y), *pMPmul(mpcXtoAlessOne.y, m_old_mpc.x));
		mpcXtoAlessOne.x = mpctmp.x;
		mpcXtoAlessOne.y = mpctmp.y;
	}
	mpcXtoA.x = *pMPsub(*pMPmul(mpcXtoAlessOne.x, m_old_mpc.x), *pMPmul(mpcXtoAlessOne.y, m_old_mpc.y));
	mpcXtoA.y = *pMPadd(*pMPmul(mpcXtoAlessOne.x, m_old_mpc.y), *pMPmul(mpcXtoAlessOne.y, m_old_mpc.x));
	mpcXtoAplusOne.x = *pMPsub(*pMPmul(mpcXtoA.x, m_old_mpc.x), *pMPmul(mpcXtoA.y, m_old_mpc.y));
	mpcXtoAplusOne.y = *pMPadd(*pMPmul(mpcXtoA.x, m_old_mpc.y), *pMPmul(mpcXtoA.y, m_old_mpc.x));

	mpcFX.x = *pMPsub(mpcXtoAplusOne.x, m_old_mpc.x);
	mpcFX.y = *pMPsub(mpcXtoAplusOne.y, m_old_mpc.y); /* FX = X^(a + 1) - X  = F */

	mpcF2prime.x = *pMPmul(m_a_plus_1_degree_mp, mpcXtoAlessOne.x); /* g_a_plus_1_degree_mp in setup */
	mpcF2prime.y = *pMPmul(m_a_plus_1_degree_mp, mpcXtoAlessOne.y);        /* F" */

	mpcF1prime.x = *pMPsub(*pMPmul(m_a_plus_1_mp, mpcXtoA.x), g_one_mp);
	mpcF1prime.y = *pMPmul(m_a_plus_1_mp, mpcXtoA.y);                   /*  F'  */

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

	s_halley.new_mpc.x = *pMPsub(s_halley.old_mpc.x, mpctmp.x);
	s_halley.new_mpc.y = *pMPsub(s_halley.old_mpc.y, mpctmp.y);

	g_new_z.x = *pMP2d(s_halley.new_mpc.x);
	g_new_z.y = *pMP2d(s_halley.new_mpc.y);
#endif
	m_new_mpc = MPCsub(m_old_mpc, mpctmp);
	g_new_z    = MPC2cmplx(m_new_mpc);
	return bail_out_halley_mpc() || g_overflow_mp;
#else
	return 0;
#endif
}

int Halley::per_pixel_mpc()
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

	m_old_mpc.x = *pd2MP(g_initial_z.x);
	m_old_mpc.y = *pd2MP(g_initial_z.y);

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
