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
		longparm = &linit;
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
		longparm = &lparm;
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
	degree = (int)parm.x;
	if (degree < 2)
	{
		degree = 3;   /* defaults to 3, but 2 is possible */
	}
	root = 1;

	/* precalculated values */
	roverd       = (double)root / (double)degree;
	d1overd      = (double)(degree - 1) / (double)degree;
	maxcolor     = 0;
	threshold    = .3*PI/degree; /* less than half distance between roots */
#if !defined(XFRACT)
	if (fractype == MPNEWTON || fractype == MPNEWTBASIN)
	{
		mproverd     = *pd2MP(roverd);
		mpd1overd    = *pd2MP(d1overd);
		mpthreshold  = *pd2MP(threshold);
		mpone        = *pd2MP(1.0);
	}
#endif

	floatmin = FLT_MIN;
	floatmax = FLT_MAX;

	basin = 0;
	if (roots != staticroots)
	{
		free(roots);
		roots = staticroots;
	}

	if (fractype == NEWTBASIN)
	{
		basin = parm.y ? 2 : 1; /*stripes */
		if (degree > 16)
		{
			roots = (_CMPLX *) malloc(degree*sizeof(_CMPLX));
			if (roots == NULL)
			{
				roots = staticroots;
				degree = 16;
			}
		}
		else
		{
			roots = staticroots;
		}

		/* list of roots to discover where we converged for newtbasin */
		for (i = 0; i < degree; i++)
		{
			roots[i].x = cos(i*twopi/(double)degree);
			roots[i].y = sin(i*twopi/(double)degree);
		}
	}
#if !defined(XFRACT)
	else if (fractype == MPNEWTBASIN)
	{
		basin = parm.y ? 2 : 1; /*stripes */

		if (degree > 16)
		{
			MPCroots = (struct MPC *) malloc(degree*sizeof(struct MPC));
			if (MPCroots == NULL)
			{
				MPCroots = (struct MPC *)staticroots;
				degree = 16;
			}
		}
		else
		{
			MPCroots = (struct MPC *)staticroots;
		}

		/* list of roots to discover where we converged for newtbasin */
		for (i = 0; i < degree; i++)
		{
			MPCroots[i].x = *pd2MP(cos(i*twopi/(double)degree));
			MPCroots[i].y = *pd2MP(sin(i*twopi/(double)degree));
		}
	}
#endif

	param[0] = (double)degree; /* JCO 7/1/92 */
	g_symmetry = (degree % 4 == 0) ? XYAXIS : XAXIS;

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
	s_periodicity_check = 0;
	FgOne = (1L << bitshift);
	FgTwo = FgOne + FgOne;
	return 1;
}

int
MandelfpSetup(void)
{
	bf_math = 0;
	c_exp = (int)param[2];
	pwr.x = param[2] - 1.0;
	pwr.y = param[3];
	floatparm = &g_initial_z;
	switch (fractype)
	{
	case MARKSMANDELFP:
		if (c_exp < 1)
		{
			c_exp = 1;
			param[2] = 1;
		}
		if (!(c_exp & 1))
		{
			g_symmetry = XYAXIS_NOPARM;    /* odd exponents */
		}
		if (c_exp & 1)
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
		if ((double)c_exp == param[2] && (c_exp & 1)) /* odd exponents */
		{
			g_symmetry = XYAXIS_NOPARM;
		}
		if (param[3] != 0)
		{
			g_symmetry = NOSYM;
		}
		fractalspecific[fractype].orbitcalc = 
			(param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)c_exp == param[2]) ?
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
		if (s_periodicity_check == 1) /* if not user set */
		{
			s_periodicity_check = 4;
		}
		break;
	case MANDELEXP:
		g_symmetry = XAXIS_NOPARM;
		break;
/* Added to account for symmetry in manfn + exp and manfn + zsqrd */
/*     JCO 2/29/92 */
	case FPMANTRIGPLUSEXP:
	case FPMANTRIGPLUSZSQRD:
		g_symmetry = (parm.y == 0.0) ? XAXIS : NOSYM;
		if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
		{
			g_symmetry = NOSYM;
		}
		break;
	case QUATFP:
		floatparm = &g_temp_z;
		g_num_attractors = 0;
		s_periodicity_check = 0;
		break;
	case HYPERCMPLXFP:
		floatparm = &g_temp_z;
		g_num_attractors = 0;
		s_periodicity_check = 0;
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
	c_exp = (int)param[2];
	floatparm = &parm;
	if (fractype == COMPLEXMARKSJUL)
	{
		pwr.x = param[2] - 1.0;
		pwr.y = param[3];
		coefficient = ComplexPower(*floatparm, pwr);
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
		if ((c_exp & 1) || param[3] != 0.0 || (double)c_exp != param[2])
		{
			g_symmetry = NOSYM;
		}
		fractalspecific[fractype].orbitcalc = 
			(param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)c_exp == param[2])
			? floatZpowerFractal : floatCmplxZpowerFractal;
		get_julia_attractor (param[0], param[1]); /* another attractor? */
		break;
	case MAGNET2J:
		FloatPreCalcMagnet2();
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
		if (parm.y == 0.0)
		{
			g_symmetry = XAXIS;
		}
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		break;
	/* Added to account for symmetry in julfn + exp and julfn + zsqrd */
	/*     JCO 2/29/92 */
	case FPJULTRIGPLUSEXP:
	case FPJULTRIGPLUSZSQRD:
		g_symmetry = (parm.y == 0.0) ? XAXIS : NOSYM;
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
		s_periodicity_check = 0;
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
				fabs(parm2.x - 3.0) < .0001 &&
				parm2.y == 0 &&
				parm.y == 0)
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
	FgHalf = fudge >> 1;
	c_exp = (int)param[2];
	if (fractype == MARKSMANDEL && c_exp < 1)
	{
		c_exp = 1;
		param[2] = 1;
	}
	if ((fractype == MARKSMANDEL   && !(c_exp & 1)) ||
		(fractype == LMANDELZPOWER && (c_exp & 1)))
	{
		g_symmetry = XYAXIS_NOPARM;    /* odd exponents */
	}
	if ((fractype == MARKSMANDEL && (c_exp & 1)) || fractype == LMANDELEXP)
	{
		g_symmetry = XAXIS_NOPARM;
	}
	if (fractype == SPIDER && s_periodicity_check == 1)
	{
		s_periodicity_check = 4;
	}
	longparm = &linit;
	if (fractype == LMANDELZPOWER)
	{
		if (param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)c_exp == param[2])
		{
			fractalspecific[fractype].orbitcalc = longZpowerFractal;
		}
		else
		{
			fractalspecific[fractype].orbitcalc = longCmplxZpowerFractal;
		}
		if (param[3] != 0 || (double)c_exp != param[2])
		{
			g_symmetry = NOSYM;
		}
	}
/* Added to account for symmetry in manfn + exp and manfn + zsqrd */
/*     JCO 2/29/92 */
	if ((fractype == LMANTRIGPLUSEXP) || (fractype == LMANTRIGPLUSZSQRD))
	{
		g_symmetry = (parm.y == 0.0) ? XAXIS : NOSYM;
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
	c_exp = (int)param[2];
	longparm = &lparm;
	switch (fractype)
	{
	case LJULIAZPOWER:
		if ((c_exp & 1) || param[3] != 0.0 || (double)c_exp != param[2])
		{
			g_symmetry = NOSYM;
		}
		fractalspecific[fractype].orbitcalc = 
			(param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)c_exp == param[2])
			? longZpowerFractal : longCmplxZpowerFractal;
		break;
	case LAMBDA:
		get_julia_attractor (0.0, 0.0);   /* another attractor? */
		get_julia_attractor (0.5, 0.0);   /* another attractor? */
		break;
	case LLAMBDAEXP:
		if (lparm.y == 0)
		{
			g_symmetry = XAXIS;
		}
		break;
	/* Added to account for symmetry in julfn + exp and julfn + zsqrd */
	/*     JCO 2/29/92 */
	case LJULTRIGPLUSEXP:
	case LJULTRIGPLUSZSQRD:
		g_symmetry = (parm.y == 0.0) ? XAXIS : NOSYM;
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
				fabs(parm2.x - 3.0) < .0001 &&
				parm2.y == 0 &&
				parm.y == 0)
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
	if (lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L
		&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
	{
		if (lparm2.x == fudge)        /* Scott variant */
		{
			curfractalspecific->orbitcalc =  ScottTrigPlusSqrFractal;
		}
		else if (lparm2.x == -fudge)  /* Skinner variant */
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
	if (parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0
		&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
	{
		if (parm2.x == 1.0)        /* Scott variant */
		{
			curfractalspecific->orbitcalc =  ScottTrigPlusSqrfpFractal;
		}
		else if (parm2.x == -1.0)  /* Skinner variant */
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
	if (lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L
		&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
	{
		if (lparm2.x == fudge)        /* Scott variant */
		{
			curfractalspecific->orbitcalc =  ScottTrigPlusTrigFractal;
		}
		else if (lparm2.x == -fudge)  /* Skinner variant */
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
	if (parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0
		&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
	{
		if (parm2.x == 1.0)        /* Scott variant */
		{
			curfractalspecific->orbitcalc =  ScottTrigPlusTrigfpFractal;
		}
		else if (parm2.x == -1.0)  /* Skinner variant */
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
	if (parm.y == 0.0 && parm2.y == 0.0)
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
	longparm = &lparm; /* added to consolidate code 10/1/92 JCO */
	floatparm = &parm;
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
	longparm = &lparm; /* added to consolidate code 10/1/92 JCO */
	floatparm = &parm;
	if (parm.y != 0.0)
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
	longparm = &linit; /* added to consolidate code 10/1/92 JCO */
	floatparm = &g_initial_z;
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
	longparm = &linit; /* added to consolidate code 10/1/92 JCO */
	floatparm = &g_initial_z;
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
		if (lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L
			&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
		{
			if (lparm2.x == fudge)     /* Scott variant */
			{
				curfractalspecific->orbitcalc =  ScottZXTrigPlusZFractal;
			}
			else if (lparm2.x == -fudge)  /* Skinner variant */
			{
				curfractalspecific->orbitcalc =  SkinnerZXTrigSubZFractal;
			}
		}
		return JulialongSetup();
	}
	else
	{
		curfractalspecific->orbitcalc =  ZXTrigPlusZfpFractal;
		if (parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0
			&& debugflag != DEBUGFLAG_NO_ASM_MANDEL)
		{
			if (parm2.x == 1.0)     /* Scott variant */
			{
				curfractalspecific->orbitcalc =  ScottZXTrigPlusZfpFractal;
			}
			else if (parm2.x == -1.0)       /* Skinner variant */
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
	c_exp = (int)param[2];
	longparm = &lparm;
	lold = *longparm;
	if (c_exp > 3)
	{
		lcpower(&lold, c_exp-1, &lcoefficient, bitshift);
	}
	else if (c_exp == 3)
	{
		lcoefficient.x = multiply(lold.x, lold.x, bitshift) - multiply(lold.y, lold.y, bitshift);
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
	c_exp = (int)param[2];
	floatparm = &parm;
	g_old_z = *floatparm;
	if (c_exp > 3)
	{
		cpower(&g_old_z, c_exp-1, &coefficient);
	}
	else if (c_exp == 3)
	{
		coefficient.x = sqr(g_old_z.x) - sqr(g_old_z.y);
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
	get_julia_attractor (0.0, 0.0);      /* an attractor? */
	return 1;
}

int
SierpinskiSetup(void)
{
	/* sierpinski */
	s_periodicity_check = 0;                /* disable periodicity checks */
	ltmp.x = 1;
	ltmp.x = ltmp.x << bitshift; /* ltmp.x = 1 */
	ltmp.y = ltmp.x >> 1;                        /* ltmp.y = .5 */
	return 1;
}

int
SierpinskiFPSetup(void)
{
	/* sierpinski */
	s_periodicity_check = 0;                /* disable periodicity checks */
	g_temp_z.x = 1;
	g_temp_z.y = 0.5;
	return 1;
}

int
HalleySetup(void)
{
	/* Halley */
	s_periodicity_check = 0;

	fractype = usr_floatflag ? HALLEY : MPHALLEY;

	curfractalspecific = &fractalspecific[fractype];

	degree = (int)parm.x;
	if (degree < 2)
	{
		degree = 2;
	}
	param[0] = (double)degree;

	/*  precalculated values */
	AplusOne = degree + 1; /* a + 1 */
	Ap1deg = AplusOne*degree;

#if !defined(XFRACT)
	if (fractype == MPHALLEY)
	{
		setMPfunctions();
		mpAplusOne = *pd2MP((double)AplusOne);
		mpAp1deg = *pd2MP((double)Ap1deg);
		mpctmpparm.x = *pd2MP(parm.y);
		mpctmpparm.y = *pd2MP(parm2.y);
		mptmpparm2x = *pd2MP(parm2.x);
		mpone        = *pd2MP(1.0);
	}
#endif

	g_symmetry = (degree % 2) ? XAXIS : XYAXIS;   /* odd, even */
	return 1;
}

int
PhoenixSetup(void)
{
	longparm = &lparm; /* added to consolidate code 10/1/92 JCO */
	floatparm = &parm;
	degree = (int)parm2.x;
	if (degree < 2 && degree > -3)
	{
		degree = 0;
	}
	param[2] = (double)degree;
	if (degree == 0)
	{
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixFractal : LongPhoenixFractal;
	}
	if (degree >= 2)
	{
		degree = degree - 1;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixPlusFractal : LongPhoenixPlusFractal;
	}
	if (degree <= -3)
	{
		degree = abs(degree) - 2;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixMinusFractal : LongPhoenixMinusFractal;
	}

	return 1;
}

int
PhoenixCplxSetup(void)
{
	longparm = &lparm;
	floatparm = &parm;
	degree = (int)param[4];
	if (degree < 2 && degree > -3)
	{
		degree = 0;
	}
	param[4] = (double)degree;
	if (degree == 0)
	{
		g_symmetry = (parm2.x != 0 || parm2.y != 0) ? NOSYM : ORIGIN;
		if (parm.y == 0 && parm2.y == 0)
		{
			g_symmetry = XAXIS;
		}
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixFractalcplx : LongPhoenixFractalcplx;
	}
	if (degree >= 2)
	{
		degree = degree - 1;
		g_symmetry = (parm.y == 0 && parm2.y == 0) ? XAXIS : NOSYM;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixCplxPlusFractal : LongPhoenixCplxPlusFractal;
	}
	if (degree <= -3)
	{
		degree = abs(degree) - 2;
		g_symmetry = (parm.y == 0 && parm2.y == 0) ? XAXIS : NOSYM;
		curfractalspecific->orbitcalc = usr_floatflag ?
			PhoenixCplxMinusFractal : LongPhoenixCplxMinusFractal;
	}

	return 1;
}

int
MandPhoenixSetup(void)
{
	longparm = &linit; /* added to consolidate code 10/1/92 JCO */
	floatparm = &g_initial_z;
	degree = (int)parm2.x;
	if (degree < 2 && degree > -3)
	{
		degree = 0;
	}
	param[2] = (double)degree;
	if (degree == 0)
	{
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixFractal : LongPhoenixFractal;
	}
	if (degree >= 2)
	{
		degree = degree - 1;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixPlusFractal : LongPhoenixPlusFractal;
	}
	if (degree <= -3)
	{
		degree = abs(degree) - 2;
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixMinusFractal : LongPhoenixMinusFractal;
	}

	return 1;
}

int
MandPhoenixCplxSetup(void)
{
	longparm = &linit; /* added to consolidate code 10/1/92 JCO */
	floatparm = &g_initial_z;
	degree = (int)param[4];
	if (degree < 2 && degree > -3)
	{
		degree = 0;
	}
	param[4] = (double)degree;
	if (parm.y != 0 || parm2.y != 0)
	{
		g_symmetry = NOSYM;
	}
	if (degree == 0)
	{
		curfractalspecific->orbitcalc = 
			usr_floatflag ? PhoenixFractalcplx : LongPhoenixFractalcplx;
	}
	if (degree >= 2)
	{
		degree = degree - 1;
		curfractalspecific->orbitcalc =
			usr_floatflag ? PhoenixCplxPlusFractal : LongPhoenixCplxPlusFractal;
	}
	if (degree <= -3)
	{
		degree = abs(degree) - 2;
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
		s_periodicity_check = 0;
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
	floatparm = &parm;
	return 1;
}
