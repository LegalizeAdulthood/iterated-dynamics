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

#if !defined(XFRACT)
#define MPCmod(m) (*pMPadd(*pMPmul((m).x, (m).x), *pMPmul((m).y, (m).y)))
#endif

extern long calcmandfpasm_c(void);

/* -------------------------------------------------------------------- */
/*              Setup (once per fractal image) routines                 */
/* -------------------------------------------------------------------- */

int
MandelSetup(void)           /* Mandelbrot Routine */
{
	if (debugflag != DEBUGFLAG_NO_ASM_MANDEL
		&& !g_invert && decomp[0] == 0 && g_rq_limit == 4.0
		&& bitshift == 29 && potflag == 0
		&& biomorph == -1 && inside > -59 && outside >= -1
		&& useinitorbit != 1 && using_jiim == 0 && g_bail_out_test == Mod
		&& (orbitsave & ORBITSAVE_SOUND) == 0)
	{
		g_calculate_type = calculate_mandelbrot; /* the normal case - use CALCMAND */
	}
	else
	{
		/* special case: use the main processing loop */
		g_calculate_type = standard_fractal;
		g_long_parameter = &g_initial_z_l;
	}
	return 1;
}

int
JuliaSetup(void)            /* Julia Routine */
{
	if (debugflag != DEBUGFLAG_NO_ASM_MANDEL
		&& !g_invert && decomp[0] == 0 && g_rq_limit == 4.0
		&& bitshift == 29 && potflag == 0
		&& biomorph == -1 && inside > -59 && outside >= -1
		&& !finattract && using_jiim == 0 && g_bail_out_test == Mod
		&& (orbitsave & ORBITSAVE_SOUND) == 0)
	{
		g_calculate_type = calculate_mandelbrot; /* the normal case - use CALCMAND */
	}
	else
	{
		/* special case: use the main processing loop */
		g_calculate_type = standard_fractal;
		g_long_parameter = &g_parameter_l;
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
	}
	return 1;
}

int
NewtonSetup(void)           /* Newton/NewtBasin Routines */
{
	int i;
#if !defined(XFRACT)
	if (debugflag != DEBUGFLAG_FORCE_FP_NEWTON)
	{
		if (fpu != 0)
		{
			if (fractype == MPNEWTON)
			{
				fractype = NEWTON;
			}
			else if (fractype == MPNEWTBASIN)
			{
				fractype = NEWTBASIN;
			}
		}
		else
		{
			if (fractype == NEWTON)
			{
					fractype = MPNEWTON;
			}
			else if (fractype == NEWTBASIN)
			{
					fractype = MPNEWTBASIN;
			}
		}
		curfractalspecific = &fractalspecific[fractype];
	}
#else
	if (fractype == MPNEWTON)
	{
		fractype = NEWTON;
	}
	else if (fractype == MPNEWTBASIN)
	{
		fractype = NEWTBASIN;
	}

	curfractalspecific = &fractalspecific[fractype];
#endif
	/* set up table of roots of 1 along unit circle */
	g_degree = (int)g_parameter.x;
	if (g_degree < 2)
	{
		g_degree = 3;   /* defaults to 3, but 2 is possible */
	}
	g_root = 1;

	/* precalculated values */
	g_root_over_degree       = (double)g_root / (double)g_degree;
	g_degree_minus_1_over_degree      = (double)(g_degree - 1) / (double)g_degree;
	g_max_color     = 0;
	g_threshold    = .3*PI/g_degree; /* less than half distance between roots */
#if !defined(XFRACT)
	if (fractype == MPNEWTON || fractype == MPNEWTBASIN)
	{
		mproverd     = *pd2MP(g_root_over_degree);
		mpd1overd    = *pd2MP(g_degree_minus_1_over_degree);
		mpthreshold  = *pd2MP(g_threshold);
		mpone        = *pd2MP(1.0);
	}
#endif

	g_basin = 0;
	if (g_roots != g_static_roots)
	{
		free(g_roots);
		g_roots = g_static_roots;
	}

	if (fractype == NEWTBASIN)
	{
		g_basin = g_parameter.y ? 2 : 1; /*stripes */
		if (g_degree > 16)
		{
			g_roots = (_CMPLX *) malloc(g_degree*sizeof(_CMPLX));
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
	else if (fractype == MPNEWTBASIN)
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
			g_roots_mpc[i].x = *pd2MP(cos(i*g_two_pi/(double)g_degree));
			g_roots_mpc[i].y = *pd2MP(sin(i*g_two_pi/(double)g_degree));
		}
	}
#endif

	param[0] = (double)g_degree; /* JCO 7/1/92 */
	g_symmetry = (g_degree % 4 == 0) ? XYAXIS : XAXIS;

	g_calculate_type = standard_fractal;
#if !defined(XFRACT)
	if (fractype == MPNEWTON || fractype == MPNEWTBASIN)
	{
		setMPfunctions();
	}
#endif
	return 1;
}


int
StandaloneSetup(void)
{
	timer(TIMER_ENGINE, curfractalspecific->calculate_type);
	return 0;           /* effectively disable solid-guessing */
}

int
UnitySetup(void)
{
	g_periodicity_check = 0;
	FgOne = (1L << bitshift);
	FgTwo = FgOne + FgOne;
	return 1;
}

int
MandelfpSetup(void)
{
	bf_math = 0;
	g_c_exp = (int)param[2];
	g_power.x = param[2] - 1.0;
	g_power.y = param[3];
	g_float_parameter = &g_initial_z;
	switch (fractype)
	{
	case MARKSMANDELFP:
		if (g_c_exp < 1)
		{
			g_c_exp = 1;
			param[2] = 1;
		}
		if (!(g_c_exp & 1))
		{
			g_symmetry = XYAXIS_NOPARM;    /* odd exponents */
		}
		if (g_c_exp & 1)
		{
			g_symmetry = XAXIS_NOPARM;
		}
		break;
	case MANDELFP:
		/*
		floating point code could probably be altered to handle many of
		the situations that otherwise are using standard_fractal().
		calculate_mandelbrot_fp() can currently handle invert, any g_rq_limit, potflag
		zmag, epsilon cross, and all the current outside options
													Wes Loewer 11/03/91
		Took out support for inside= options, for speed. 7/13/97
		*/
		if (debugflag != DEBUGFLAG_NO_ASM_MANDEL
			&& !distest
			&& decomp[0] == 0
			&& biomorph == -1
			&& (inside >= -1)
			/* uncomment this next line if more outside options are added */
			&& outside >= -6
			&& useinitorbit != 1
			&& (soundflag & SOUNDFLAG_ORBITMASK) < SOUNDFLAG_X
			&& using_jiim == 0 && g_bail_out_test == Mod
			&& (orbitsave & ORBITSAVE_SOUND) == 0)
		{
			g_calculate_type = calculate_mandelbrot_fp; /* the normal case - use calculate_mandelbrot_fp */
#if !defined(XFRACT)
			if (cpu >= 386 && fpu >= 387)
			{
				calcmandfpasmstart_p5();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_p5;
			}
			else if (cpu == 286 && fpu >= 287)
			{
				calcmandfpasmstart();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_287;
			}
			else
			{
				calcmandfpasmstart();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_87;
			}
#else
#ifdef NASM
			if (fpu == -1)
			{
				calcmandfpasmstart_p5();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_p5;
			}
			else
#endif
			{
				calcmandfpasmstart();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_c;
			}
#endif
		}
		else
		{
			/* special case: use the main processing loop */
			g_calculate_type = standard_fractal;
		}
		break;
	case FPMANDELZPOWER:
		if ((double)g_c_exp == param[2] && (g_c_exp & 1)) /* odd exponents */
		{
			g_symmetry = XYAXIS_NOPARM;
		}
		if (param[3] != 0)
		{
			g_symmetry = NOSYM;
		}
		fractalspecific[fractype].orbitcalc = 
			(param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)g_c_exp == param[2]) ?
			floatZpowerFractal : floatCmplxZpowerFractal;
		break;
	case MAGNET1M:
	case MAGNET2M:
		g_attractors[0].x = 1.0;      /* 1.0 + 0.0i always attracts */
		g_attractors[0].y = 0.0;      /* - both MAGNET1 and MAGNET2 */
		g_attractor_period[0] = 1;
		g_num_attractors = 1;
		break;
	case SPIDERFP:
		if (g_periodicity_check == 1) /* if not user set */
		{
			g_periodicity_check = 4;
		}
		break;
	case MANDELEXP:
		g_symmetry = XAXIS_NOPARM;
		break;
/* Added to account for symmetry in manfn + exp and manfn + zsqrd */
/*     JCO 2/29/92 */
	case FPMANTRIGPLUSEXP:
	case FPMANTRIGPLUSZSQRD:
		g_symmetry = (g_parameter.y == 0.0) ? XAXIS : NOSYM;
		if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
		{
			g_symmetry = NOSYM;
		}
		break;
	case QUATFP:
		g_float_parameter = &g_temp_z;
		g_num_attractors = 0;
		g_periodicity_check = 0;
		break;
	case HYPERCMPLXFP:
		g_float_parameter = &g_temp_z;
		g_num_attractors = 0;
		g_periodicity_check = 0;
		if (param[2] != 0)
		{
			g_symmetry = NOSYM;
		}
		if (trigndx[0] == 14) /* FLIP */
		{
			g_symmetry = NOSYM;
		}
		break;
	case TIMSERRORFP:
		if (trigndx[0] == 14) /* FLIP */
		{
			g_symmetry = NOSYM;
		}
		break;
	case MARKSMANDELPWRFP:
		if (trigndx[0] == 14) /* FLIP */
		{
			g_symmetry = NOSYM;
		}
		break;
	default:
		break;
	}
	return 1;
}

int
JuliafpSetup(void)
{
	g_c_exp = (int)param[2];
	g_float_parameter = &g_parameter;
	if (fractype == COMPLEXMARKSJUL)
	{
		g_power.x = param[2] - 1.0;
		g_power.y = param[3];
		g_coefficient = ComplexPower(*g_float_parameter, g_power);
	}
	switch (fractype)
	{
	case JULIAFP:
		/*
		floating point code could probably be altered to handle many of
		the situations that otherwise are using standard_fractal().
		calculate_mandelbrot_fp() can currently handle invert, any g_rq_limit, potflag
		zmag, epsilon cross, and all the current outside options
													Wes Loewer 11/03/91
		Took out support for inside= options, for speed. 7/13/97
		*/
		if (debugflag != DEBUGFLAG_NO_ASM_MANDEL
				&& !distest
				&& decomp[0] == 0
				&& biomorph == -1
				&& (inside >= -1)
				/* uncomment this next line if more outside options are added */
				&& outside >= -6
				&& useinitorbit != 1
				&& (soundflag & SOUNDFLAG_ORBITMASK) < SOUNDFLAG_X
				&& !finattract
				&& using_jiim == 0 && g_bail_out_test == Mod
				&& (orbitsave & ORBITSAVE_SOUND) == 0)
		{
			g_calculate_type = calculate_mandelbrot_fp; /* the normal case - use calculate_mandelbrot_fp */
#if !defined(XFRACT)
			if (cpu >= 386 && fpu >= 387)
			{
				calcmandfpasmstart_p5();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_p5;
			}
			else if (cpu == 286 && fpu >= 287)
			{
				calcmandfpasmstart();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_287;
			}
			else
			{
				calcmandfpasmstart();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_87;
			}
#else
#ifdef NASM
			if (fpu == -1)
			{
				calcmandfpasmstart_p5();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_p5;
			}
			else
#endif
			{
				calcmandfpasmstart();
				g_calculate_mandelbrot_asm_fp = (long (*)(void))calcmandfpasm_c;
			}
#endif
		}
		else
		{
			/* special case: use the main processing loop */
			g_calculate_type = standard_fractal;
			get_julia_attractor (0.0, 0.0);   /* another attractor? */
		}
		break;
	case FPJULIAZPOWER:
		if ((g_c_exp & 1) || param[3] != 0.0 || (double)g_c_exp != param[2])
		{
			g_symmetry = NOSYM;
		}
		fractalspecific[fractype].orbitcalc = 
			(param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)g_c_exp == param[2])
			? floatZpowerFractal : floatCmplxZpowerFractal;
		get_julia_attractor (param[0], param[1]); /* another attractor? */
		break;
	case MAGNET2J:
		magnet2_precalculate_fp();
	case MAGNET1J:
		g_attractors[0].x = 1.0;      /* 1.0 + 0.0i always attracts */
		g_attractors[0].y = 0.0;      /* - both MAGNET1 and MAGNET2 */
		g_attractor_period[0] = 1;
		g_num_attractors = 1;
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		break;
	case LAMBDAFP:
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		get_julia_attractor (0.5, 0.0);   /* another attractor? */
		break;
	case LAMBDAEXP:
		if (g_parameter.y == 0.0)
		{
			g_symmetry = XAXIS;
		}
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		break;
	/* Added to account for symmetry in julfn + exp and julfn + zsqrd */
	/*     JCO 2/29/92 */
	case FPJULTRIGPLUSEXP:
	case FPJULTRIGPLUSZSQRD:
		g_symmetry = (g_parameter.y == 0.0) ? XAXIS : NOSYM;
		if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
		{
			g_symmetry = NOSYM;
		}
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		break;
	case HYPERCMPLXJFP:
		if (param[2] != 0)
		{
			g_symmetry = NOSYM;
		}
		if (trigndx[0] != SQR)
		{
			g_symmetry = NOSYM;
		}
	case QUATJULFP:
		g_num_attractors = 0;   /* attractors broken since code checks r, i not j, k */
		g_periodicity_check = 0;
		if (param[4] != 0.0 || param[5] != 0)
		{
			g_symmetry = NOSYM;
		}
		break;
	case FPPOPCORN:
	case FPPOPCORNJUL:
		{
			int default_functions = 0;
			if (trigndx[0] == SIN &&
				trigndx[1] == TAN &&
				trigndx[2] == SIN &&
				trigndx[3] == TAN &&
				fabs(g_parameter2.x - 3.0) < .0001 &&
				g_parameter2.y == 0 &&
				g_parameter.y == 0)
			{
				default_functions = 1;
				if (fractype == FPPOPCORNJUL)
				{
					g_symmetry = ORIGIN;
				}
			}
			if (save_release <= 1960)
			{
				curfractalspecific->orbitcalc = PopcornFractal_Old;
			}
			else if (default_functions && DEBUGFLAG_REAL_POPCORN == debugflag)
			{
				curfractalspecific->orbitcalc = PopcornFractal;
			}
			else
			{
				curfractalspecific->orbitcalc = PopcornFractalFn;
			}
			get_julia_attractor (0.0, 0.0);   /* another attractor? */
		}
		break;
	case FPCIRCLE:
		if (inside == STARTRAIL) /* FPCIRCLE locks up when used with STARTRAIL */
		{
			inside = 0; /* arbitrarily set inside = NUMB */
		}
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		break;
	default:
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		break;
	}
	return 1;
}

int
MandellongSetup(void)
{
	g_c_exp = (int)param[2];
	if (fractype == MARKSMANDEL && g_c_exp < 1)
	{
		g_c_exp = 1;
		param[2] = 1;
	}
	if ((fractype == MARKSMANDEL   && !(g_c_exp & 1)) ||
		(fractype == LMANDELZPOWER && (g_c_exp & 1)))
	{
		g_symmetry = XYAXIS_NOPARM;    /* odd exponents */
	}
	if ((fractype == MARKSMANDEL && (g_c_exp & 1)) || fractype == LMANDELEXP)
	{
		g_symmetry = XAXIS_NOPARM;
	}
	if (fractype == SPIDER && g_periodicity_check == 1)
	{
		g_periodicity_check = 4;
	}
	g_long_parameter = &g_initial_z_l;
	if (fractype == LMANDELZPOWER)
	{
		if (param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)g_c_exp == param[2])
		{
			fractalspecific[fractype].orbitcalc = longZpowerFractal;
		}
		else
		{
			fractalspecific[fractype].orbitcalc = longCmplxZpowerFractal;
		}
		if (param[3] != 0 || (double)g_c_exp != param[2])
		{
			g_symmetry = NOSYM;
		}
	}
/* Added to account for symmetry in manfn + exp and manfn + zsqrd */
/*     JCO 2/29/92 */
	if ((fractype == LMANTRIGPLUSEXP) || (fractype == LMANTRIGPLUSZSQRD))
	{
		g_symmetry = (g_parameter.y == 0.0) ? XAXIS : NOSYM;
		if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
		{
			g_symmetry = NOSYM;
		}
	}
	if (fractype == TIMSERROR)
	{
		if (trigndx[0] == 14) /* FLIP */
		{
			g_symmetry = NOSYM;
		}
	}
	if (fractype == MARKSMANDELPWR)
	{
		if (trigndx[0] == 14) /* FLIP */
		{
			g_symmetry = NOSYM;
		}
	}
	return 1;
}

int
JulialongSetup(void)
{
	g_c_exp = (int)param[2];
	g_long_parameter = &g_parameter_l;
	switch (fractype)
	{
	case LJULIAZPOWER:
		if ((g_c_exp & 1) || param[3] != 0.0 || (double)g_c_exp != param[2])
		{
			g_symmetry = NOSYM;
		}
		fractalspecific[fractype].orbitcalc = 
			(param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)g_c_exp == param[2])
			? longZpowerFractal : longCmplxZpowerFractal;
		break;
	case LAMBDA:
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		get_julia_attractor (0.5, 0.0);   /* another attractor? */
		break;
	case LLAMBDAEXP:
		if (g_parameter_l.y == 0)
		{
			g_symmetry = XAXIS;
		}
		break;
	/* Added to account for symmetry in julfn + exp and julfn + zsqrd */
	/*     JCO 2/29/92 */
	case LJULTRIGPLUSEXP:
	case LJULTRIGPLUSZSQRD:
		g_symmetry = (g_parameter.y == 0.0) ? XAXIS : NOSYM;
		if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
		{
			g_symmetry = NOSYM;
		}
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		break;
	case LPOPCORN:
	case LPOPCORNJUL:
		{
			int default_functions = 0;
			if (trigndx[0] == SIN &&
				trigndx[1] == TAN &&
				trigndx[2] == SIN &&
				trigndx[3] == TAN &&
				fabs(g_parameter2.x - 3.0) < .0001 &&
				g_parameter2.y == 0 &&
				g_parameter.y == 0)
			{
				default_functions = 1;
				if (fractype == LPOPCORNJUL)
				{
					g_symmetry = ORIGIN;
				}
			}
			if (save_release <= 1960)
			{
				curfractalspecific->orbitcalc = LPopcornFractal_Old;
			}
			else if (default_functions && DEBUGFLAG_REAL_POPCORN == debugflag)
			{
				curfractalspecific->orbitcalc = LPopcornFractal;
			}
			else
			{
				curfractalspecific->orbitcalc = LPopcornFractalFn;
			}
			get_julia_attractor (0.0, 0.0);   /* another attractor? */
		}
		break;
	default:
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		break;
	}
	return 1;
}

int
TrigPlusSqrlongSetup(void)
{
	curfractalspecific->per_pixel =  julia_per_pixel;
	curfractalspecific->orbitcalc =  TrigPlusSqrFractal;
	if (g_parameter_l.x == fudge && g_parameter_l.y == 0L && g_parameter2_l.y == 0L
		&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
	{
		if (g_parameter2_l.x == fudge)        /* Scott variant */
		{
			curfractalspecific->orbitcalc =  ScottTrigPlusSqrFractal;
		}
		else if (g_parameter2_l.x == -fudge)  /* Skinner variant */
		{
			curfractalspecific->orbitcalc =  SkinnerTrigSubSqrFractal;
		}
	}
	return JulialongSetup();
}

int
TrigPlusSqrfpSetup(void)
{
	curfractalspecific->per_pixel =  juliafp_per_pixel;
	curfractalspecific->orbitcalc =  TrigPlusSqrfpFractal;
	if (g_parameter.x == 1.0 && g_parameter.y == 0.0 && g_parameter2.y == 0.0
		&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
	{
		if (g_parameter2.x == 1.0)        /* Scott variant */
		{
			curfractalspecific->orbitcalc =  ScottTrigPlusSqrfpFractal;
		}
		else if (g_parameter2.x == -1.0)  /* Skinner variant */
		{
			curfractalspecific->orbitcalc =  SkinnerTrigSubSqrfpFractal;
		}
	}
	return JuliafpSetup();
}

int
TrigPlusTriglongSetup(void)
{
	FnPlusFnSym();
	if (trigndx[1] == SQR)
	{
		return TrigPlusSqrlongSetup();
	}
	curfractalspecific->per_pixel =  long_julia_per_pixel;
	curfractalspecific->orbitcalc =  TrigPlusTrigFractal;
	if (g_parameter_l.x == fudge && g_parameter_l.y == 0L && g_parameter2_l.y == 0L
		&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
	{
		if (g_parameter2_l.x == fudge)        /* Scott variant */
		{
			curfractalspecific->orbitcalc =  ScottTrigPlusTrigFractal;
		}
		else if (g_parameter2_l.x == -fudge)  /* Skinner variant */
		{
			curfractalspecific->orbitcalc =  SkinnerTrigSubTrigFractal;
		}
	}
	return JulialongSetup();
}

int
TrigPlusTrigfpSetup(void)
{
	FnPlusFnSym();
	if (trigndx[1] == SQR)
	{
		return TrigPlusSqrfpSetup();
	}
	curfractalspecific->per_pixel =  otherjuliafp_per_pixel;
	curfractalspecific->orbitcalc =  TrigPlusTrigfpFractal;
	if (g_parameter.x == 1.0 && g_parameter.y == 0.0 && g_parameter2.y == 0.0
		&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
	{
		if (g_parameter2.x == 1.0)        /* Scott variant */
		{
			curfractalspecific->orbitcalc =  ScottTrigPlusTrigfpFractal;
		}
		else if (g_parameter2.x == -1.0)  /* Skinner variant */
		{
			curfractalspecific->orbitcalc =  SkinnerTrigSubTrigfpFractal;
		}
	}
	return JuliafpSetup();
}

int
FnPlusFnSym(void) /* set symmetry matrix for fn + fn type */
{
	static char fnplusfn[7][7] =
	{/* fn2 ->sin     cos    sinh    cosh   exp    log    sqr  */
	/* fn1 */
	/* sin */ {PI_SYM, XAXIS, XYAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
	/* cos */ {XAXIS, PI_SYM, XAXIS,  XYAXIS, XAXIS, XAXIS, XAXIS},
	/* sinh*/ {XYAXIS, XAXIS, XYAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
	/* cosh*/ {XAXIS, XYAXIS, XAXIS,  XYAXIS, XAXIS, XAXIS, XAXIS},
	/* exp */ {XAXIS, XYAXIS, XAXIS,  XAXIS, XYAXIS, XAXIS, XAXIS},
	/* log */ {XAXIS, XAXIS, XAXIS,  XAXIS, XAXIS, XAXIS, XAXIS},
	/* sqr */ {XAXIS, XAXIS, XAXIS,  XAXIS, XAXIS, XAXIS, XYAXIS}
	};
	if (g_parameter.y == 0.0 && g_parameter2.y == 0.0)
	{
		if (trigndx[0] < 7 && trigndx[1] < 7)  /* bounds of array JCO 5/6/92*/
		{
			g_symmetry = fnplusfn[trigndx[0]][trigndx[1]];  /* JCO 5/6/92 */
		}
		if (trigndx[0] == 14 || trigndx[1] == 14) /* FLIP */
		{
			g_symmetry = NOSYM;
		}
	}                 /* defaults to XAXIS symmetry JCO 5/6/92 */
	else
	{
		g_symmetry = NOSYM;
	}
	return 0;
}

int
LambdaTrigOrTrigSetup(void)
{
	/* default symmetry is ORIGIN  JCO 2/29/92 (changed from PI_SYM) */
	g_long_parameter = &g_parameter_l; /* added to consolidate code 10/1/92 JCO */
	g_float_parameter = &g_parameter;
	if ((trigndx[0] == EXP) || (trigndx[1] == EXP))
	{
		g_symmetry = NOSYM; /* JCO 1/9/93 */
	}
	if ((trigndx[0] == LOG) || (trigndx[1] == LOG))
	{
		g_symmetry = XAXIS;
	}
	get_julia_attractor (0.0, 0.0);      /* an attractor? */
	return 1;
}

int
JuliaTrigOrTrigSetup(void)
{
	/* default symmetry is XAXIS */
	g_long_parameter = &g_parameter_l; /* added to consolidate code 10/1/92 JCO */
	g_float_parameter = &g_parameter;
	if (g_parameter.y != 0.0)
	{
		g_symmetry = NOSYM;
	}
	if (trigndx[0] == 14 || trigndx[1] == 14) /* FLIP */
	{
		g_symmetry = NOSYM;
	}
	get_julia_attractor (0.0, 0.0);      /* an attractor? */
	return 1;
}

int
ManlamTrigOrTrigSetup(void)
{ /* psuedo */
	/* default symmetry is XAXIS */
	g_long_parameter = &g_initial_z_l; /* added to consolidate code 10/1/92 JCO */
	g_float_parameter = &g_initial_z;
	if (trigndx[0] == SQR)
	{
		g_symmetry = NOSYM;
	}
	if ((trigndx[0] == LOG) || (trigndx[1] == LOG))
	{
		g_symmetry = NOSYM;
	}
	return 1;
}

int
MandelTrigOrTrigSetup(void)
{
/* default symmetry is XAXIS_NOPARM */
	g_long_parameter = &g_initial_z_l; /* added to consolidate code 10/1/92 JCO */
	g_float_parameter = &g_initial_z;
	if ((trigndx[0] == 14) || (trigndx[1] == 14)) /* FLIP  JCO 5/28/92 */
	{
		g_symmetry = NOSYM;
	}
	return 1;
}


int
ZXTrigPlusZSetup(void)
{
	/*   static char ZXTrigPlusZSym1[] = */
	/* fn1 ->  sin   cos    sinh  cosh exp   log   sqr */
	/*           {XAXIS, XYAXIS, XAXIS, XYAXIS, XAXIS, NOSYM, XYAXIS}; */
	/*   static char ZXTrigPlusZSym2[] = */
	/* fn1 ->  sin   cos    sinh  cosh exp   log   sqr */
	/*           {NOSYM, ORIGIN, NOSYM, ORIGIN, NOSYM, NOSYM, ORIGIN}; */

	if (param[1] == 0.0 && param[3] == 0.0)
	{
		/*      symmetry = ZXTrigPlusZSym1[trigndx[0]]; */
		switch (trigndx[0])
		{
		case COS:   /* changed to two case statments and made any added */
		case COSH:  /* functions default to XAXIS symmetry. JCO 5/25/92 */
		case SQR:
		case 9:   /* 'real' cos */
			g_symmetry = XYAXIS;
			break;
		case 14:   /* FLIP  JCO 2/29/92 */
			g_symmetry = YAXIS;
			break;
		case LOG:
			g_symmetry = NOSYM;
			break;
		default:
			g_symmetry = XAXIS;
			break;
		}
	}
	else
	{
		/*      symmetry = ZXTrigPlusZSym2[trigndx[0]]; */
		switch (trigndx[0])
		{
		case COS:
		case COSH:
		case SQR:
		case 9:   /* 'real' cos */
			g_symmetry = ORIGIN;
			break;
		case 14:   /* FLIP  JCO 2/29/92 */
			g_symmetry = NOSYM;
			break;
		default:
			g_symmetry = NOSYM;
			break;
		}
	}
	if (curfractalspecific->isinteger)
	{
		curfractalspecific->orbitcalc =  ZXTrigPlusZFractal;
		if (g_parameter_l.x == fudge && g_parameter_l.y == 0L && g_parameter2_l.y == 0L
			&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
		{
			if (g_parameter2_l.x == fudge)     /* Scott variant */
			{
				curfractalspecific->orbitcalc =  ScottZXTrigPlusZFractal;
			}
			else if (g_parameter2_l.x == -fudge)  /* Skinner variant */
			{
				curfractalspecific->orbitcalc =  SkinnerZXTrigSubZFractal;
			}
		}
		return JulialongSetup();
	}
	else
	{
		curfractalspecific->orbitcalc =  ZXTrigPlusZfpFractal;
		if (g_parameter.x == 1.0 && g_parameter.y == 0.0 && g_parameter2.y == 0.0
			&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
		{
			if (g_parameter2.x == 1.0)     /* Scott variant */
			{
				curfractalspecific->orbitcalc =  ScottZXTrigPlusZfpFractal;
			}
			else if (g_parameter2.x == -1.0)       /* Skinner variant */
			{
				curfractalspecific->orbitcalc =  SkinnerZXTrigSubZfpFractal;
			}
		}
	}
	return JuliafpSetup();
}

int
LambdaTrigSetup(void)
{
	int isinteger = curfractalspecific->isinteger;
	curfractalspecific->orbitcalc =  (isinteger != 0)
		? LambdaTrigFractal : LambdaTrigfpFractal;
	switch (trigndx[0])
	{
	case SIN:
	case COS:
	case 9:   /* 'real' cos, added this and default for additional functions */
		g_symmetry = PI_SYM;
		curfractalspecific->orbitcalc = 
			isinteger ? LambdaTrigFractal1 : LambdaTrigfpFractal1;
		break;
	case SINH:
	case COSH:
		g_symmetry = ORIGIN;
		curfractalspecific->orbitcalc = 
			isinteger ? LambdaTrigFractal2 : LambdaTrigfpFractal2;
		break;
	case SQR:
		g_symmetry = ORIGIN;
		break;
	case EXP:
		curfractalspecific->orbitcalc = 
			isinteger ? LongLambdaexponentFractal : LambdaexponentFractal;
		g_symmetry = NOSYM; /* JCO 1/9/93 */
		break;
	case LOG:
		g_symmetry = NOSYM;
		break;
	default:   /* default for additional functions */
		g_symmetry = ORIGIN;  /* JCO 5/8/92 */
		break;
	}
	get_julia_attractor (0.0, 0.0);      /* an attractor? */
	return isinteger ? JulialongSetup() : JuliafpSetup();
}

int
JuliafnPlusZsqrdSetup(void)
{
	/*   static char fnpluszsqrd[] = */
	/* fn1 ->  sin   cos    sinh  cosh   sqr    exp   log  */
	/* sin    {NOSYM, ORIGIN, NOSYM, ORIGIN, ORIGIN, NOSYM, NOSYM}; */

	/*   symmetry = fnpluszsqrd[trigndx[0]];   JCO  5/8/92 */
	switch (trigndx[0]) /* fix sqr symmetry & add additional functions */
	{
	case COS: /* cosxx */
	case COSH:
	case SQR:
	case 9:   /* 'real' cos */
	case 10:  /* tan */
	case 11:  /* tanh */
		g_symmetry = ORIGIN;
		/* default is for NOSYM symmetry */
	}
	return curfractalspecific->isinteger ? JulialongSetup() : JuliafpSetup();
}

int
SqrTrigSetup(void)
{
	/*   static char SqrTrigSym[] = */
	/* fn1 ->  sin    cos    sinh   cosh   sqr    exp   log  */
	/*           {PI_SYM, PI_SYM, XYAXIS, XYAXIS, XYAXIS, XAXIS, XAXIS}; */
	/*   symmetry = SqrTrigSym[trigndx[0]];      JCO  5/9/92 */
	switch (trigndx[0]) /* fix sqr symmetry & add additional functions */
	{
	case SIN:
	case COS: /* cosxx */
	case 9:   /* 'real' cos */
		g_symmetry = PI_SYM;
		/* default is for XAXIS symmetry */
	}
	return curfractalspecific->isinteger ? JulialongSetup() : JuliafpSetup();
}

int
FnXFnSetup(void)
{
	static char fnxfn[7][7] =
	{/* fn2 ->sin     cos    sinh    cosh  exp    log    sqr */
	/* fn1 */
	/* sin */ {PI_SYM, YAXIS, XYAXIS, XYAXIS, XAXIS, NOSYM, XYAXIS},
	/* cos */ {YAXIS, PI_SYM, XYAXIS, XYAXIS, XAXIS, NOSYM, XYAXIS},
	/* sinh*/ {XYAXIS, XYAXIS, XYAXIS, XYAXIS, XAXIS, NOSYM, XYAXIS},
	/* cosh*/ {XYAXIS, XYAXIS, XYAXIS, XYAXIS, XAXIS, NOSYM, XYAXIS},
	/* exp */ {XAXIS, XAXIS, XAXIS, XAXIS, XAXIS, NOSYM, XYAXIS},
	/* log */ {NOSYM, NOSYM, NOSYM, NOSYM, NOSYM, XAXIS, NOSYM},
	/* sqr */ {XYAXIS, XYAXIS, XYAXIS, XYAXIS, XYAXIS, NOSYM, XYAXIS},
	};
	/*
	if (trigndx[0] == EXP || trigndx[0] == LOG || trigndx[1] == EXP || trigndx[1] == LOG)
		g_symmetry = XAXIS;
	else if ((trigndx[0] == SIN && trigndx[1] == SIN) || (trigndx[0] == COS && trigndx[1] == COS))
		g_symmetry = PI_SYM;
	else if ((trigndx[0] == SIN && trigndx[1] == COS) || (trigndx[0] == COS && trigndx[1] == SIN))
		g_symmetry = YAXIS;
	else
		g_symmetry = XYAXIS;
	*/
	if (trigndx[0] < 7 && trigndx[1] < 7)  /* bounds of array JCO 5/22/92*/
	{
		g_symmetry = fnxfn[trigndx[0]][trigndx[1]];  /* JCO 5/22/92 */
	}
	/* defaults to XAXIS symmetry JCO 5/22/92 */
	else  /* added to complete the symmetry JCO 5/22/92 */
	{
		if (trigndx[0] == LOG || trigndx[1] == LOG)
		{
			g_symmetry = NOSYM;
		}
		if (trigndx[0] == 9 || trigndx[1] == 9)  /* 'real' cos */
		{
			if (trigndx[0] == SIN || trigndx[1] == SIN)
			{
				g_symmetry = PI_SYM;
			}
			if (trigndx[0] == COS || trigndx[1] == COS)
			{
				g_symmetry = PI_SYM;
			}
		}
		if (trigndx[0] == 9 && trigndx[1] == 9)
		{
			g_symmetry = PI_SYM;
		}
	}
	return curfractalspecific->isinteger ? JulialongSetup() : JuliafpSetup();
}

int
MandelTrigSetup(void)
{
	int isinteger = curfractalspecific->isinteger;
	curfractalspecific->orbitcalc = 
		isinteger ? LambdaTrigFractal : LambdaTrigfpFractal;
	g_symmetry = XYAXIS_NOPARM;
	switch (trigndx[0])
	{
	case SIN:
	case COS:
		curfractalspecific->orbitcalc = 
			isinteger ? LambdaTrigFractal1 : LambdaTrigfpFractal1;
		break;
	case SINH:
	case COSH:
		curfractalspecific->orbitcalc = 
			isinteger ? LambdaTrigFractal2 : LambdaTrigfpFractal2;
		break;
	case EXP:
		g_symmetry = XAXIS_NOPARM;
		curfractalspecific->orbitcalc = 
			isinteger ? LongLambdaexponentFractal : LambdaexponentFractal;
		break;
	case LOG:
		g_symmetry = XAXIS_NOPARM;
		break;
	default:   /* added for additional functions, JCO 5/25/92 */
		g_symmetry = XYAXIS_NOPARM;
		break;
	}
	return isinteger ? MandellongSetup() : MandelfpSetup();
}

int
MarksJuliaSetup(void)
{
#if !defined(XFRACT)
	if (param[2] < 1)
	{
		param[2] = 1;
	}
	g_c_exp = (int)param[2];
	g_long_parameter = &g_parameter_l;
	g_old_z_l = *g_long_parameter;
	if (g_c_exp > 3)
	{
		lcpower(&g_old_z_l, g_c_exp-1, &g_coefficient_l, bitshift);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient_l.x = multiply(g_old_z_l.x, g_old_z_l.x, bitshift) - multiply(g_old_z_l.y, g_old_z_l.y, bitshift);
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
	get_julia_attractor (0.0, 0.0);      /* an attractor? */
#endif
	return 1;
}

int
MarksJuliafpSetup(void)
{
	if (param[2] < 1)
	{
		param[2] = 1;
	}
	g_c_exp = (int)param[2];
	g_float_parameter = &g_parameter;
	g_old_z = *g_float_parameter;
	if (g_c_exp > 3)
	{
		complex_power(&g_old_z, g_c_exp-1, &g_coefficient);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient.x = sqr(g_old_z.x) - sqr(g_old_z.y);
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
	get_julia_attractor (0.0, 0.0);      /* an attractor? */
	return 1;
}

int
SierpinskiSetup(void)
{
	/* sierpinski */
	g_periodicity_check = 0;                /* disable periodicity checks */
	g_tmp_z_l.x = 1;
	g_tmp_z_l.x = g_tmp_z_l.x << bitshift; /* g_tmp_z_l.x = 1 */
	g_tmp_z_l.y = g_tmp_z_l.x >> 1;                        /* g_tmp_z_l.y = .5 */
	return 1;
}

int
SierpinskiFPSetup(void)
{
	/* sierpinski */
	g_periodicity_check = 0;                /* disable periodicity checks */
	g_temp_z.x = 1;
	g_temp_z.y = 0.5;
	return 1;
}

int
HalleySetup(void)
{
	/* Halley */
	g_periodicity_check = 0;

	fractype = usr_floatflag ? HALLEY : MPHALLEY;

	curfractalspecific = &fractalspecific[fractype];

	g_degree = (int)g_parameter.x;
	if (g_degree < 2)
	{
		g_degree = 2;
	}
	param[0] = (double)g_degree;

	/*  precalculated values */
	AplusOne = g_degree + 1; /* a + 1 */
	Ap1deg = AplusOne*g_degree;

#if !defined(XFRACT)
	if (fractype == MPHALLEY)
	{
		setMPfunctions();
		mpAplusOne = *pd2MP((double)AplusOne);
		mpAp1deg = *pd2MP((double)Ap1deg);
		mpctmpparm.x = *pd2MP(g_parameter.y);
		mpctmpparm.y = *pd2MP(g_parameter2.y);
		mptmpparm2x = *pd2MP(g_parameter2.x);
		mpone        = *pd2MP(1.0);
	}
#endif

	g_symmetry = (g_degree % 2) ? XAXIS : XYAXIS;   /* odd, even */
	return 1;
}

int
PhoenixSetup(void)
{
	g_long_parameter = &g_parameter_l; /* added to consolidate code 10/1/92 JCO */
	g_float_parameter = &g_parameter;
	g_degree = (int)g_parameter2.x;
	if (g_degree < 2 && g_degree > -3)
	{
		g_degree = 0;
	}
	param[2] = (double)g_degree;
	if (g_degree == 0)
	{
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixFractal : LongPhoenixFractal;
	}
	if (g_degree >= 2)
	{
		g_degree = g_degree - 1;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixPlusFractal : LongPhoenixPlusFractal;
	}
	if (g_degree <= -3)
	{
		g_degree = abs(g_degree) - 2;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixMinusFractal : LongPhoenixMinusFractal;
	}

	return 1;
}

int
PhoenixCplxSetup(void)
{
	g_long_parameter = &g_parameter_l;
	g_float_parameter = &g_parameter;
	g_degree = (int)param[4];
	if (g_degree < 2 && g_degree > -3)
	{
		g_degree = 0;
	}
	param[4] = (double)g_degree;
	if (g_degree == 0)
	{
		g_symmetry = (g_parameter2.x != 0 || g_parameter2.y != 0) ? NOSYM : ORIGIN;
		if (g_parameter.y == 0 && g_parameter2.y == 0)
		{
			g_symmetry = XAXIS;
		}
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixFractalcplx : LongPhoenixFractalcplx;
	}
	if (g_degree >= 2)
	{
		g_degree = g_degree - 1;
		g_symmetry = (g_parameter.y == 0 && g_parameter2.y == 0) ? XAXIS : NOSYM;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixCplxPlusFractal : LongPhoenixCplxPlusFractal;
	}
	if (g_degree <= -3)
	{
		g_degree = abs(g_degree) - 2;
		g_symmetry = (g_parameter.y == 0 && g_parameter2.y == 0) ? XAXIS : NOSYM;
		curfractalspecific->orbitcalc = usr_floatflag ?
			PhoenixCplxMinusFractal : LongPhoenixCplxMinusFractal;
	}

	return 1;
}

int
MandPhoenixSetup(void)
{
	g_long_parameter = &g_initial_z_l; /* added to consolidate code 10/1/92 JCO */
	g_float_parameter = &g_initial_z;
	g_degree = (int)g_parameter2.x;
	if (g_degree < 2 && g_degree > -3)
	{
		g_degree = 0;
	}
	param[2] = (double)g_degree;
	if (g_degree == 0)
	{
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixFractal : LongPhoenixFractal;
	}
	if (g_degree >= 2)
	{
		g_degree = g_degree - 1;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixPlusFractal : LongPhoenixPlusFractal;
	}
	if (g_degree <= -3)
	{
		g_degree = abs(g_degree) - 2;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixMinusFractal : LongPhoenixMinusFractal;
	}

	return 1;
}

int
MandPhoenixCplxSetup(void)
{
	g_long_parameter = &g_initial_z_l; /* added to consolidate code 10/1/92 JCO */
	g_float_parameter = &g_initial_z;
	g_degree = (int)param[4];
	if (g_degree < 2 && g_degree > -3)
	{
		g_degree = 0;
	}
	param[4] = (double)g_degree;
	if (g_parameter.y != 0 || g_parameter2.y != 0)
	{
		g_symmetry = NOSYM;
	}
	if (g_degree == 0)
	{
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixFractalcplx : LongPhoenixFractalcplx;
	}
	if (g_degree >= 2)
	{
		g_degree = g_degree - 1;
		curfractalspecific->orbitcalc =
			usr_floatflag ? PhoenixCplxPlusFractal : LongPhoenixCplxPlusFractal;
	}
	if (g_degree <= -3)
	{
		g_degree = abs(g_degree) - 2;
		curfractalspecific->orbitcalc =
			usr_floatflag ? PhoenixCplxMinusFractal : LongPhoenixCplxMinusFractal;
	}

	return 1;
}

int
StandardSetup(void)
{
	if (fractype == UNITYFP)
	{
		g_periodicity_check = 0;
	}
	return 1;
}

int
VLSetup(void)
{
	if (param[0] < 0.0)
	{
		param[0] = 0.0;
	}
	if (param[1] < 0.0)
	{
		param[1] = 0.0;
	}
	if (param[0] > 1.0)
	{
		param[0] = 1.0;
	}
	if (param[1] > 1.0)
	{
		param[1] = 1.0;
	}
	g_float_parameter = &g_parameter;
	return 1;
}
