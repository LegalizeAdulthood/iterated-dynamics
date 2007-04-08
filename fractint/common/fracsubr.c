/*
	FRACSUBR.C contains subroutines which belong primarily to CALCFRAC.C and
	FRACTALS.C, i.e. which are non-fractal-specific fractal engine subroutines.
*/
#include <assert.h>
#include <memory.h>

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifndef XFRACT
#include <sys/timeb.h>
#endif
#include <sys/types.h>
#include <time.h>

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "drivers.h"

#if defined(_WIN32)
#define ftimex ftime
#define timebx timeb
#endif

/* fudge all values up by 2 << FUDGE_FACTOR{,2} */
#define FUDGE_FACTOR     29
#define FUDGE_FACTOR2    24

#define MAX_Y_BLOCK 7    /* must match calcfrac.c */
#define MAX_X_BLOCK 202  /* must match calcfrac.c */

int g_resume_length = 0;				/* length of resume info */
int g_use_grid = FALSE;

static int s_tab_or_help = 0;			/* kludge for sound and tab or help key press */
static int s_resume_offset;				/* offset in resume info gets */
static int s_resume_info_length = 0;
static int s_save_orbit[1500] = { 0 };	/* array to save orbit values */

/* routines in this module      */
static long   _fastcall fudge_to_long(double d);
static double _fastcall fudge_to_double(long l);
static void   _fastcall adjust_to_limits(double);
static void   _fastcall smallest_add(double *);
static int    _fastcall ratio_bad(double, double);
static void   _fastcall plot_orbit_d(double, double, int);
static int    _fastcall work_list_combine(void);
static void   _fastcall adjust_to_limits_bf(double);
static void   _fastcall smallest_add_bf(bf_t);
static int sound_open(void);

void free_grid_pointers()
{
	if (dx0)
	{
		free(dx0);
		dx0 = NULL;
	}
	if (lx0)
	{
		free(lx0);
		lx0 = NULL;
	}
}

void set_grid_pointers()
{
	free_grid_pointers();
	dx0 = (double *) malloc(sizeof(double)*(2*xdots + 2*ydots));
	dy1 = dx0 + xdots;
	dy0 = dy1 + xdots;
	dx1 = dy0 + ydots;
	lx0 = (long *) malloc(sizeof(long)*(2*xdots + 2*ydots));
	ly1 = lx0 + xdots;
	ly0 = ly1 + xdots;
	lx1 = ly0 + ydots;
	set_pixel_calc_functions();
}

void fill_dx_array(void)
{
	int i;
	if (g_use_grid)
	{
		dx0[0] = xxmin;              /* fill up the x, y grids */
		dy0[0] = yymax;
		dx1[0] = dy1[0] = 0;
		for (i = 1; i < xdots; i++)
		{
			dx0[i] = (double)(dx0[0] + i*delxx);
			dy1[i] = (double)(dy1[0] - i*delyy2);
		}
		for (i = 1; i < ydots; i++)
		{
			dy0[i] = (double)(dy0[0] - i*delyy);
			dx1[i] = (double)(dx1[0] + i*delxx2);
		}
	}
}
void fill_lx_array(void)
{
	int i;
	/* note that lx1 & ly1 values can overflow into sign bit; since     */
	/* they're used only to add to lx0/ly0, 2s comp straightens it out  */
	if (g_use_grid)
	{
		lx0[0] = xmin;               /* fill up the x, y grids */
		ly0[0] = ymax;
		lx1[0] = ly1[0] = 0;
		for (i = 1; i < xdots; i++)
		{
			lx0[i] = lx0[i-1] + delx;
			ly1[i] = ly1[i-1] - dely2;
		}
		for (i = 1; i < ydots; i++)
		{
			ly0[i] = ly0[i-1] - dely;
			lx1[i] = lx1[i-1] + delx2;
		}
	}
}

void fractal_float_to_bf(void)
{
	int i;
	init_bf_dec(getprecdbl(CURRENTREZ));
	floattobf(bfxmin, xxmin);
	floattobf(bfxmax, xxmax);
	floattobf(bfymin, yymin);
	floattobf(bfymax, yymax);
	floattobf(bfx3rd, xx3rd);
	floattobf(bfy3rd, yy3rd);

	for (i = 0; i < MAXPARAMS; i++)
	{
		if (type_has_parameter(fractype, i, NULL))
		{
			floattobf(bfparms[i], param[i]);
		}
	}
	calc_status = CALCSTAT_PARAMS_CHANGED;
}


#ifdef _MSC_VER
#if _MSC_VER == 800
/* MSC8 doesn't correctly calculate the address of certain arrays here */
#pragma optimize("", off)
#endif
#endif

/* initialize a *pile* of stuff for fractal calculation */
void calculate_fractal_initialize(void)
{
	int tries = 0;
	int i, gotprec;
	long xytemp;
	double ftemp;
	g_color_iter = g_old_color_iter = 0L;
	for (i = 0; i < 10; i++)
	{
		rhombus_stack[i] = 0;
	}

  /* set up grid array compactly leaving space at end */
  /* space req for grid is 2(xdots + ydots)*sizeof(long or double) */
  /* space available in extraseg is 65536 Bytes */
	xytemp = xdots + ydots;
	if (((usr_floatflag == 0) && (xytemp*sizeof(long) > 32768))
		|| ((usr_floatflag == 1) && (xytemp*sizeof(double) > 32768))
		|| DEBUGFLAG_NO_PIXEL_GRID == debugflag)
	{
		g_use_grid = FALSE;
		g_float_flag = usr_floatflag = TRUE;
	}
	else
	{
		g_use_grid = TRUE;
	}

	set_grid_pointers();

	if (!(curfractalspecific->flags & BF_MATH))
	{
		int tofloat = curfractalspecific->tofloat;
		if (tofloat == NOFRACTAL)
		{
			bf_math = 0;
		}
		else if (!(fractalspecific[tofloat].flags & BF_MATH))
		{
			bf_math = 0;
		}
		else if (bf_math)
		{
			fractype = tofloat;
			curfractalspecific = &fractalspecific[fractype];
		}
	}

	/* switch back to double when zooming out if using arbitrary precision */
	if (bf_math)
	{
		gotprec = getprecbf(CURRENTREZ);
		if ((gotprec <= DBL_DIG + 1 && debugflag != DEBUGFLAG_NO_BIG_TO_FLOAT) || math_tol[1] >= 1.0)
		{
			corners_bf_to_float();
			bf_math = 0;
		}
		else
		{
			init_bf_dec(gotprec);
		}
	}
	else if ((fractype == MANDEL || fractype == MANDELFP) && DEBUGFLAG_NO_BIG_TO_FLOAT == debugflag)
	{
		fractype = MANDELFP;
		curfractalspecific = &fractalspecific[fractype];
		fractal_float_to_bf();
		usr_floatflag = 1;
	}
	else if ((fractype == JULIA || fractype == JULIAFP) && DEBUGFLAG_NO_BIG_TO_FLOAT == debugflag)
	{
		fractype = JULIAFP;
		curfractalspecific = &fractalspecific[fractype];
		fractal_float_to_bf();
		usr_floatflag = 1;
	}
	else if ((fractype == LMANDELZPOWER || fractype == FPMANDELZPOWER) && DEBUGFLAG_NO_BIG_TO_FLOAT == debugflag)
	{
		fractype = FPMANDELZPOWER;
		curfractalspecific = &fractalspecific[fractype];
		fractal_float_to_bf();
		usr_floatflag = 1;
	}
	else if ((fractype == LJULIAZPOWER || fractype == FPJULIAZPOWER) && DEBUGFLAG_NO_BIG_TO_FLOAT == debugflag)
	{
		fractype = FPJULIAZPOWER;
		curfractalspecific = &fractalspecific[fractype];
		fractal_float_to_bf();
		usr_floatflag = 1;
	}
	else
	{
		free_bf_vars();
	}
	g_float_flag = bf_math ? TRUE : usr_floatflag;
	if (calc_status == CALCSTAT_RESUMABLE)  /* on resume, ensure g_float_flag correct */
	{
		g_float_flag = curfractalspecific->isinteger ? FALSE : TRUE;
	}
	/* if floating pt only, set g_float_flag for TAB screen */
	if (!curfractalspecific->isinteger && curfractalspecific->tofloat == NOFRACTAL)
	{
		g_float_flag = TRUE;
	}
	if (usr_stdcalcmode == 's')
	{
		if (fractype == MANDEL || fractype == MANDELFP)
		{
			g_float_flag = TRUE;
		}
		else
		{
			usr_stdcalcmode = '1';
		}
	}

init_restart:
#if defined(_WIN32)
	_ASSERTE(_CrtCheckMemory());
#endif

	/* the following variables may be forced to a different setting due to
		calc routine constraints;  usr_xxx is what the user last said is wanted,
		xxx is what we actually do in the current situation */
	stdcalcmode      = usr_stdcalcmode;
	g_periodicity_check = usr_periodicitycheck;
	distest          = usr_distest;
	g_biomorph         = g_user_biomorph;

	g_potential_flag = FALSE;
	if (potparam[0] != 0.0
		&& colors >= 64
		&& (curfractalspecific->calculate_type == standard_fractal
			|| curfractalspecific->calculate_type == calculate_mandelbrot
			|| curfractalspecific->calculate_type == calculate_mandelbrot_fp))
	{
		g_potential_flag = TRUE;
		distest = usr_distest = 0;    /* can't do distest too */
	}

	if (distest)
	{
		g_float_flag = TRUE;  /* force floating point for dist est */
	}

	if (g_float_flag)  /* ensure type matches g_float_flag */
	{
		if (curfractalspecific->isinteger != 0
			&& curfractalspecific->tofloat != NOFRACTAL)
		{
			fractype = curfractalspecific->tofloat;
		}
	}
	else
	{
		if (curfractalspecific->isinteger == 0
			&& curfractalspecific->tofloat != NOFRACTAL)
		{
			fractype = curfractalspecific->tofloat;
		}
	}
	/* match Julibrot with integer mode of orbit */
	if (fractype == JULIBROTFP && fractalspecific[g_new_orbit_type].isinteger)
	{
		int i = fractalspecific[g_new_orbit_type].tofloat;
		if (i != NOFRACTAL)
		{
			g_new_orbit_type = i;
		}
		else
		{
			fractype = JULIBROT;
		}
	}
	else if (fractype == JULIBROT && fractalspecific[g_new_orbit_type].isinteger == 0)
	{
		int i = fractalspecific[g_new_orbit_type].tofloat;
		if (i != NOFRACTAL)
		{
			g_new_orbit_type = i;
		}
		else
		{
			fractype = JULIBROTFP;
		}
	}

	curfractalspecific = &fractalspecific[fractype];
	integerfractal = curfractalspecific->isinteger;

	if (g_potential_flag && potparam[2] != 0.0)
	{
		g_rq_limit = potparam[2];
	}
	else if (bailout) /* user input bailout */
	{
		g_rq_limit = bailout;
	}
	else if (g_biomorph != -1) /* biomorph benefits from larger bailout */
	{
		g_rq_limit = 100;
	}
	else
	{
		g_rq_limit = curfractalspecific->orbit_bailout;
	}
	if (integerfractal) /* the bailout limit mustn't be too high here */
	{
		if (g_rq_limit > 127.0)
		{
			g_rq_limit = 127.0;
		}
	}

	if ((curfractalspecific->flags&NOROTATE) != 0)
	{
		/* ensure min < max and unrotated rectangle */
		if (xxmin > xxmax)
		{
			ftemp = xxmax;
			xxmax = xxmin;
			xxmin = ftemp;
		}
		if (yymin > yymax)
		{
			ftemp = yymax;
			yymax = yymin;
			yymin = ftemp;
		}
		xx3rd = xxmin; yy3rd = yymin;
	}

	/* set up bitshift for integer math */
	bitshift = FUDGE_FACTOR2; /* by default, the smaller shift */
	if (integerfractal > 1)  /* use specific override from table */
	{
		bitshift = integerfractal;
	}
	if (integerfractal == 0)  /* float? */
	{
		i = curfractalspecific->tofloat;
		if (i != NOFRACTAL) /* -> int? */
		{
			if (fractalspecific[i].isinteger > 1) /* specific shift? */
			{
				bitshift = fractalspecific[i].isinteger;
			}
		}
		else
		{
			bitshift = 16;  /* to allow larger corners */
		}
	}
/* We want this code if we're using the assembler calculate_mandelbrot */
	if (fractype == MANDEL || fractype == JULIA)  /* adust shift bits if.. */
	{
		if (!g_potential_flag                            /* not using potential */
		&& (param[0] > -2.0 && param[0] < 2.0)  /* parameters not too large */
		&& (param[1] > -2.0 && param[1] < 2.0)
		&& !g_invert                                /* and not inverting */
		&& g_biomorph == -1                         /* and not biomorphing */
		&& g_rq_limit <= 4.0                           /* and bailout not too high */
		&& (outside > -2 || outside < -6)         /* and no funny outside stuff */
		&& debugflag != DEBUGFLAG_FORCE_BITSHIFT	/* and not debugging */
		&& g_proximity <= 2.0                       /* and g_proximity not too large */
		&& g_bail_out_test == Mod)                     /* and bailout test = mod */
			bitshift = FUDGE_FACTOR;                  /* use the larger bitshift */
		}

	fudge = 1L << bitshift;

	l_at_rad = fudge/32768L;
	f_at_rad = 1.0/32768L;

	/* now setup arrays of real coordinates corresponding to each pixel */
	if (bf_math)
	{
		adjust_to_limits_bf(1.0); /* make sure all corners in valid range */
	}
	else
	{
		adjust_to_limits(1.0); /* make sure all corners in valid range */
		delxx  = (LDBL)(xxmax - xx3rd) / (LDBL)dxsize; /* calculate stepsizes */
		delyy  = (LDBL)(yymax - yy3rd) / (LDBL)dysize;
		delxx2 = (LDBL)(xx3rd - xxmin) / (LDBL)dysize;
		delyy2 = (LDBL)(yy3rd - yymin) / (LDBL)dxsize;
		fill_dx_array();
	}

	if (fractype != CELLULAR && fractype != ANT)  /* fudge_to_long fails w >10 digits in double */
	{
		creal = fudge_to_long(param[0]); /* integer equivs for it all */
		cimag = fudge_to_long(param[1]);
		xmin  = fudge_to_long(xxmin);
		xmax  = fudge_to_long(xxmax);
		x3rd  = fudge_to_long(xx3rd);
		ymin  = fudge_to_long(yymin);
		ymax  = fudge_to_long(yymax);
		y3rd  = fudge_to_long(yy3rd);
		delx  = fudge_to_long((double)delxx);
		dely  = fudge_to_long((double)delyy);
		delx2 = fudge_to_long((double)delxx2);
		dely2 = fudge_to_long((double)delyy2);
	}

	/* skip this if plasma to avoid 3d problems */
	/* skip if bf_math to avoid extraseg conflict with dx0 arrays */
	/* skip if ifs, ifs3d, or lsystem to avoid crash when mathtolerance */
	/* is set.  These types don't auto switch between float and integer math */
	if (fractype != PLASMA && bf_math == 0
		&& fractype != IFS && fractype != IFS3D && fractype != LSYSTEM)
	{
		if (integerfractal && !g_invert && g_use_grid)
		{
			if ((delx  == 0 && delxx  != 0.0)
				|| (delx2 == 0 && delxx2 != 0.0)
				|| (dely  == 0 && delyy  != 0.0)
				|| (dely2 == 0 && delyy2 != 0.0))
			{
				goto expand_retry;
			}

			fill_lx_array();   /* fill up the x, y grids */
			/* past max res?  check corners within 10% of expected */
			if (ratio_bad((double)lx0[xdots-1]-xmin, (double)xmax-x3rd)
				|| ratio_bad((double)ly0[ydots-1]-ymax, (double)y3rd-ymax)
				|| ratio_bad((double)lx1[(ydots >> 1)-1], ((double)x3rd-xmin)/2)
				|| ratio_bad((double)ly1[(xdots >> 1)-1], ((double)ymin-y3rd)/2))
			{
expand_retry:
				if (integerfractal          /* integer fractal type? */
					&& curfractalspecific->tofloat != NOFRACTAL)
				{
					g_float_flag = TRUE;           /* switch to floating pt */
				}
				else
				{
					adjust_to_limits(2.0);   /* double the size */
				}
				if (calc_status == CALCSTAT_RESUMABLE)       /* due to restore of an old file? */
				{
					calc_status = CALCSTAT_PARAMS_CHANGED;         /*   whatever, it isn't resumable */
				}
				goto init_restart;
			} /* end if ratio bad */

			/* re-set corners to match reality */
			xmax = lx0[xdots-1] + lx1[ydots-1];
			ymin = ly0[ydots-1] + ly1[xdots-1];
			x3rd = xmin + lx1[ydots-1];
			y3rd = ly0[ydots-1];
			xxmin = fudge_to_double(xmin);
			xxmax = fudge_to_double(xmax);
			xx3rd = fudge_to_double(x3rd);
			yymin = fudge_to_double(ymin);
			yymax = fudge_to_double(ymax);
			yy3rd = fudge_to_double(y3rd);
		} /* end if (integerfractal && !g_invert && g_use_grid) */
		else
		{
			double dx0, dy0, dx1, dy1;
			/* set up dx0 and dy0 analogs of lx0 and ly0 */
			/* put fractal parameters in doubles */
			dx0 = xxmin;                /* fill up the x, y grids */
			dy0 = yymax;
			dx1 = dy1 = 0;
			/* this way of defining the dx and dy arrays is not the most
				accurate, but it is kept because it is used to determine
				the limit of resolution */
			for (i = 1; i < xdots; i++)
			{
				dx0 = (double)(dx0 + (double)delxx);
				dy1 = (double)(dy1 - (double)delyy2);
			}
			for (i = 1; i < ydots; i++)
			{
				dy0 = (double)(dy0 - (double)delyy);
				dx1 = (double)(dx1 + (double)delxx2);
			}
			if (bf_math == 0) /* redundant test, leave for now */
			{
				double testx_try, testx_exact;
				double testy_try, testy_exact;
				/* Following is the old logic for detecting failure of double
					precision. It has two advantages: it is independent of the
					representation of numbers, and it is sensitive to resolution
					(allows depper zooms at lower resolution. However it fails
					for rotations of exactly 90 degrees, so we added a safety net
					by using the magnification.  */
				if (++tries < 2) /* for safety */
				{
				if (tries > 1)
				{
					stopmsg(0, "precision-detection error");
				}
				/* Previously there were four tests of distortions in the
					zoom box used to detect precision limitations. In some
					cases of rotated/skewed zoom boxs, this causes the algorithm
					to bail out to arbitrary precision too soon. The logic
					now only tests the larger of the two deltas in an attempt
					to repair this bug. This should never make the transition
					to arbitrary precision sooner, but always later.*/
				if (fabs(xxmax-xx3rd) > fabs(xx3rd-xxmin))
				{
					testx_exact  = xxmax-xx3rd;
					testx_try    = dx0-xxmin;
				}
				else
				{
					testx_exact  = xx3rd-xxmin;
					testx_try    = dx1;
				}
				if (fabs(yy3rd-yymax) > fabs(yymin-yy3rd))
				{
					testy_exact = yy3rd-yymax;
					testy_try   = dy0-yymax;
				}
				else
				{
					testy_exact = yymin-yy3rd;
					testy_try   = dy1;
				}
				if (ratio_bad(testx_try, testx_exact) ||
					ratio_bad(testy_try, testy_exact))
				{
					if (curfractalspecific->flags & BF_MATH)
					{
						fractal_float_to_bf();
						goto init_restart;
					}
					goto expand_retry;
				} /* end if ratio_bad etc. */
				} /* end if tries < 2 */
			} /* end if bf_math == 0 */

			/* if long double available, this is more accurate */
			fill_dx_array();       /* fill up the x, y grids */

			/* re-set corners to match reality */
			xxmax = (double)(xxmin + (xdots-1)*delxx + (ydots-1)*delxx2);
			yymin = (double)(yymax - (ydots-1)*delyy - (xdots-1)*delyy2);
			xx3rd = (double)(xxmin + (ydots-1)*delxx2);
			yy3rd = (double)(yymax - (ydots-1)*delyy);

		} /* end else */
	} /* end if not plasma */

	/* for periodicity close-enough, and for unity: */
	/*     min(max(delx, delx2), max(dely, dely2)      */
	ddelmin = fabs((double)delxx);
	if (fabs((double)delxx2) > ddelmin)
	{
		ddelmin = fabs((double)delxx2);
	}
	if (fabs((double)delyy) > fabs((double)delyy2))
	{
		if (fabs((double)delyy) < ddelmin)
		{
			ddelmin = fabs((double)delyy);
		}
	}
	else if (fabs((double)delyy2) < ddelmin)
	{
		ddelmin = fabs((double)delyy2);
	}
	delmin = fudge_to_long(ddelmin);

	/* calculate factors which plot real values to screen co-ords */
	/* calcfrac.c plot_orbit routines have comments about this    */
	ftemp = (double)(-delyy2*delxx2*dxsize*dysize - (xxmax - xx3rd)*(yy3rd - yymax));
	if (ftemp != 0)
	{
		plotmx1 = (double)(delxx2*dxsize*dysize / ftemp);
		plotmx2 = (yy3rd-yymax)*dxsize / ftemp;
		plotmy1 = (double)(-delyy2*dxsize*dysize/ftemp);
		plotmy2 = (xxmax-xx3rd)*dysize / ftemp;
	}
	if (bf_math == 0)
	{
		free_bf_vars();
	}
}

#ifdef _MSC_VER
#if _MSC_VER == 800
#pragma optimize("", on) /* restore optimization options */
#endif
#endif

static long _fastcall fudge_to_long(double d)
{
	if ((d *= fudge) > 0)
	{
		d += 0.5;
	}
	else
	{
		d -= 0.5;
	}
	return (long)d;
}

static double _fastcall fudge_to_double(long l)
{
	char buf[30];
	double d;
	sprintf(buf, "%.9g", (double)l / fudge);
#ifndef XFRACT
	sscanf(buf, "%lg", &d);
#else
	sscanf(buf, "%lf", &d);
#endif
	return d;
}

void adjust_corner_bf(void)
{
	/* make edges very near vert/horiz exact, to ditch rounding errs and */
	/* to avoid problems when delta per axis makes too large a ratio     */
	double ftemp;
	double Xmagfactor, Rotation, Skew;
	LDBL Magnification;

	bf_t bftemp, bftemp2;
	bf_t btmp1;
	int saved; saved = save_stack();
	bftemp  = alloc_stack(rbflength + 2);
	bftemp2 = alloc_stack(rbflength + 2);
	btmp1  =  alloc_stack(rbflength + 2);

	/* While we're at it, let's adjust the Xmagfactor as well */
	/* use bftemp, bftemp2 as bfXctr, bfYctr */
	cvtcentermagbf(bftemp, bftemp2, &Magnification, &Xmagfactor, &Rotation, &Skew);
	ftemp = fabs(Xmagfactor);
	if (ftemp != 1 && ftemp >= (1-aspectdrift) && ftemp <= (1 + aspectdrift))
		{
		Xmagfactor = sign(Xmagfactor);
		cvtcornersbf(bftemp, bftemp2, Magnification, Xmagfactor, Rotation, Skew);
		}

	/* ftemp = fabs(xx3rd-xxmin); */
	abs_a_bf(sub_bf(bftemp, bfx3rd, bfxmin));

	/* ftemp2 = fabs(xxmax-xx3rd); */
	abs_a_bf(sub_bf(bftemp2, bfxmax, bfx3rd));

	/* if ((ftemp = fabs(xx3rd-xxmin)) < (ftemp2 = fabs(xxmax-xx3rd))) */
	if (cmp_bf(bftemp, bftemp2) < 0)
	{
		/* if (ftemp*10000 < ftemp2 && yy3rd != yymax) */
		if (cmp_bf(mult_bf_int(btmp1, bftemp, 10000), bftemp2) < 0
			&& cmp_bf(bfy3rd, bfymax) != 0)
			/* xx3rd = xxmin; */
			copy_bf(bfx3rd, bfxmin);
	}

	/* else if (ftemp2*10000 < ftemp && yy3rd != yymin) */
	if (cmp_bf(mult_bf_int(btmp1, bftemp2, 10000), bftemp) < 0
		&& cmp_bf(bfy3rd, bfymin) != 0)
	{
		/* xx3rd = xxmax; */
		copy_bf(bfx3rd, bfxmax);
	}

	/* ftemp = fabs(yy3rd-yymin); */
	abs_a_bf(sub_bf(bftemp, bfy3rd, bfymin));

	/* ftemp2 = fabs(yymax-yy3rd); */
	abs_a_bf(sub_bf(bftemp2, bfymax, bfy3rd));

	/* if ((ftemp = fabs(yy3rd-yymin)) < (ftemp2 = fabs(yymax-yy3rd))) */
	if (cmp_bf(bftemp, bftemp2) < 0)
	{
		/* if (ftemp*10000 < ftemp2 && xx3rd != xxmax) */
		if (cmp_bf(mult_bf_int(btmp1, bftemp, 10000), bftemp2) < 0
			&& cmp_bf(bfx3rd, bfxmax) != 0)
		{
			/* yy3rd = yymin; */
			copy_bf(bfy3rd, bfymin);
		}
	}

	/* else if (ftemp2*10000 < ftemp && xx3rd != xxmin) */
	if (cmp_bf(mult_bf_int(btmp1, bftemp2, 10000), bftemp) < 0
		&& cmp_bf(bfx3rd, bfxmin) != 0)
	{
		/* yy3rd = yymax; */
		copy_bf(bfy3rd, bfymax);
	}


	restore_stack(saved);
}

void adjust_corner(void)
{
	/* make edges very near vert/horiz exact, to ditch rounding errs and */
	/* to avoid problems when delta per axis makes too large a ratio     */
	double ftemp, ftemp2;
	double Xctr, Yctr, Xmagfactor, Rotation, Skew;
	LDBL Magnification;

	if (!integerfractal)
	{
		/* While we're at it, let's adjust the Xmagfactor as well */
		cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
		ftemp = fabs(Xmagfactor);
		if (ftemp != 1 && ftemp >= (1-aspectdrift) && ftemp <= (1 + aspectdrift))
		{
			Xmagfactor = sign(Xmagfactor);
			cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
		}
	}

	ftemp = fabs(xx3rd-xxmin);
	ftemp2 = fabs(xxmax-xx3rd);
	if (ftemp < ftemp2)
	{
		if (ftemp*10000 < ftemp2 && yy3rd != yymax)
		{
			xx3rd = xxmin;
		}
		}

	if (ftemp2*10000 < ftemp && yy3rd != yymin)
	{
		xx3rd = xxmax;
	}

	ftemp = fabs(yy3rd-yymin);
	ftemp2 = fabs(yymax-yy3rd);
	if (ftemp < ftemp2)
	{
		if (ftemp*10000 < ftemp2 && xx3rd != xxmax)
		{
			yy3rd = yymin;
		}
		}

	if (ftemp2*10000 < ftemp && xx3rd != xxmin)
	{
		yy3rd = yymax;
	}

}

static void _fastcall adjust_to_limits_bf(double expand)
{
	LDBL limit;
	bf_t bcornerx[4], bcornery[4];
	bf_t blowx, bhighx, blowy, bhighy, blimit, bftemp;
	bf_t bcenterx, bcentery, badjx, badjy, btmp1, btmp2;
	bf_t bexpand;
	int i;
	int saved; saved = save_stack();
	bcornerx[0] = alloc_stack(rbflength + 2);
	bcornerx[1] = alloc_stack(rbflength + 2);
	bcornerx[2] = alloc_stack(rbflength + 2);
	bcornerx[3] = alloc_stack(rbflength + 2);
	bcornery[0] = alloc_stack(rbflength + 2);
	bcornery[1] = alloc_stack(rbflength + 2);
	bcornery[2] = alloc_stack(rbflength + 2);
	bcornery[3] = alloc_stack(rbflength + 2);
	blowx       = alloc_stack(rbflength + 2);
	bhighx      = alloc_stack(rbflength + 2);
	blowy       = alloc_stack(rbflength + 2);
	bhighy      = alloc_stack(rbflength + 2);
	blimit      = alloc_stack(rbflength + 2);
	bftemp      = alloc_stack(rbflength + 2);
	bcenterx    = alloc_stack(rbflength + 2);
	bcentery    = alloc_stack(rbflength + 2);
	badjx       = alloc_stack(rbflength + 2);
	badjy       = alloc_stack(rbflength + 2);
	btmp1       = alloc_stack(rbflength + 2);
	btmp2       = alloc_stack(rbflength + 2);
	bexpand     = alloc_stack(rbflength + 2);

	limit = 32767.99;

	floattobf(blimit, limit);
	floattobf(bexpand, expand);

	add_bf(bcenterx, bfxmin, bfxmax);
	half_a_bf(bcenterx);

	/* centery = (yymin + yymax)/2; */
	add_bf(bcentery, bfymin, bfymax);
	half_a_bf(bcentery);

	/* if (xxmin == centerx) { */
	if (cmp_bf(bfxmin, bcenterx) == 0)  /* ohoh, infinitely thin, fix it */
	{
		smallest_add_bf(bfxmax);
		/* bfxmin -= bfxmax-centerx; */
		sub_a_bf(bfxmin, sub_bf(btmp1, bfxmax, bcenterx));
		}

	/* if (bfymin == centery) */
	if (cmp_bf(bfymin, bcentery) == 0)
	{
		smallest_add_bf(bfymax);
		/* bfymin -= bfymax-centery; */
		sub_a_bf(bfymin, sub_bf(btmp1, bfymax, bcentery));
		}

	/* if (bfx3rd == centerx) */
	if (cmp_bf(bfx3rd, bcenterx) == 0)
	{
		smallest_add_bf(bfx3rd);
	}

	/* if (bfy3rd == centery) */
	if (cmp_bf(bfy3rd, bcentery) == 0)
	{
		smallest_add_bf(bfy3rd);
	}

	/* setup array for easier manipulation */
	/* cornerx[0] = xxmin; */
	copy_bf(bcornerx[0], bfxmin);

	/* cornerx[1] = xxmax; */
	copy_bf(bcornerx[1], bfxmax);

	/* cornerx[2] = xx3rd; */
	copy_bf(bcornerx[2], bfx3rd);

	/* cornerx[3] = xxmin + (xxmax-xx3rd); */
	sub_bf(bcornerx[3], bfxmax, bfx3rd);
	add_a_bf(bcornerx[3], bfxmin);

	/* cornery[0] = yymax; */
	copy_bf(bcornery[0], bfymax);

	/* cornery[1] = yymin; */
	copy_bf(bcornery[1], bfymin);

	/* cornery[2] = yy3rd; */
	copy_bf(bcornery[2], bfy3rd);

	/* cornery[3] = yymin + (yymax-yy3rd); */
	sub_bf(bcornery[3], bfymax, bfy3rd);
	add_a_bf(bcornery[3], bfymin);

	/* if caller wants image size adjusted, do that first */
	if (expand != 1.0)
	{
		for (i = 0; i < 4; ++i)
		{
			/* cornerx[i] = centerx + (cornerx[i]-centerx)*expand; */
			sub_bf(btmp1, bcornerx[i], bcenterx);
			mult_bf(bcornerx[i], btmp1, bexpand);
			add_a_bf(bcornerx[i], bcenterx);

			/* cornery[i] = centery + (cornery[i]-centery)*expand; */
			sub_bf(btmp1, bcornery[i], bcentery);
			mult_bf(bcornery[i], btmp1, bexpand);
			add_a_bf(bcornery[i], bcentery);
		}
	}

	/* get min/max x/y values */
	/* lowx = highx = cornerx[0]; */
	copy_bf(blowx, bcornerx[0]); copy_bf(bhighx, bcornerx[0]);

	/* lowy = highy = cornery[0]; */
	copy_bf(blowy, bcornery[0]); copy_bf(bhighy, bcornery[0]);

	for (i = 1; i < 4; ++i)
	{
		/* if (cornerx[i] < lowx)               lowx  = cornerx[i]; */
		if (cmp_bf(bcornerx[i], blowx) < 0)
		{
			copy_bf(blowx, bcornerx[i]);
		}

		/* if (cornerx[i] > highx)              highx = cornerx[i]; */
		if (cmp_bf(bcornerx[i], bhighx) > 0)
		{
			copy_bf(bhighx, bcornerx[i]);
		}

		/* if (cornery[i] < lowy)               lowy  = cornery[i]; */
		if (cmp_bf(bcornery[i], blowy) < 0)
		{
			copy_bf(blowy, bcornery[i]);
		}

		/* if (cornery[i] > highy)              highy = cornery[i]; */
		if (cmp_bf(bcornery[i], bhighy) > 0)
		{
			copy_bf(bhighy, bcornery[i]);
		}
	}

	/* if image is too large, downsize it maintaining center */
	/* ftemp = highx-lowx; */
	sub_bf(bftemp, bhighx, blowx);

	/* if (highy-lowy > ftemp) ftemp = highy-lowy; */
	if (cmp_bf(sub_bf(btmp1, bhighy, blowy), bftemp) > 0)
	{
		copy_bf(bftemp, btmp1);
	}

	/* if image is too large, downsize it maintaining center */

	floattobf(btmp1, limit*2.0);
	copy_bf(btmp2, bftemp);
	div_bf(bftemp, btmp1, btmp2);
	floattobf(btmp1, 1.0);
	if (cmp_bf(bftemp, btmp1) < 0)
	{
		for (i = 0; i < 4; ++i)
		{
			/* cornerx[i] = centerx + (cornerx[i]-centerx)*ftemp; */
			sub_bf(btmp1, bcornerx[i], bcenterx);
			mult_bf(bcornerx[i], btmp1, bftemp);
			add_a_bf(bcornerx[i], bcenterx);

			/* cornery[i] = centery + (cornery[i]-centery)*ftemp; */
			sub_bf(btmp1, bcornery[i], bcentery);
			mult_bf(bcornery[i], btmp1, bftemp);
			add_a_bf(bcornery[i], bcentery);
		}
	}

	/* if any corner has x or y past limit, move the image */
	/* adjx = adjy = 0; */
	clear_bf(badjx); clear_bf(badjy);

	for (i = 0; i < 4; ++i)
	{
		/* if (cornerx[i] > limit && (ftemp = cornerx[i] - limit) > adjx)
			adjx = ftemp; */
		if (cmp_bf(bcornerx[i], blimit) > 0 &&
			cmp_bf(sub_bf(bftemp, bcornerx[i], blimit), badjx) > 0)
		{
			copy_bf(badjx, bftemp);
		}

		/* if (cornerx[i] < -limit && (ftemp = cornerx[i] + limit) < adjx)
			adjx = ftemp; */
		if (cmp_bf(bcornerx[i], neg_bf(btmp1, blimit)) < 0 &&
			cmp_bf(add_bf(bftemp, bcornerx[i], blimit), badjx) < 0)
		{
			copy_bf(badjx, bftemp);
		}

		/* if (cornery[i] > limit  && (ftemp = cornery[i] - limit) > adjy)
			adjy = ftemp; */
		if (cmp_bf(bcornery[i], blimit) > 0 &&
			cmp_bf(sub_bf(bftemp, bcornery[i], blimit), badjy) > 0)
		{
			copy_bf(badjy, bftemp);
		}

		/* if (cornery[i] < -limit && (ftemp = cornery[i] + limit) < adjy)
			adjy = ftemp; */
		if (cmp_bf(bcornery[i], neg_bf(btmp1, blimit)) < 0 &&
			cmp_bf(add_bf(bftemp, bcornery[i], blimit), badjy) < 0)
		{
			copy_bf(badjy, bftemp);
		}
	}

	/* if (calc_status == CALCSTAT_RESUMABLE && (adjx != 0 || adjy != 0) && (zwidth == 1.0))
		calc_status = CALCSTAT_PARAMS_CHANGED; */
	if (calc_status == CALCSTAT_RESUMABLE && (is_bf_not_zero(badjx)|| is_bf_not_zero(badjy)) && (zwidth == 1.0))
	{
		calc_status = CALCSTAT_PARAMS_CHANGED;
	}

	/* xxmin = cornerx[0] - adjx; */
	sub_bf(bfxmin, bcornerx[0], badjx);
	/* xxmax = cornerx[1] - adjx; */
	sub_bf(bfxmax, bcornerx[1], badjx);
	/* xx3rd = cornerx[2] - adjx; */
	sub_bf(bfx3rd, bcornerx[2], badjx);
	/* yymax = cornery[0] - adjy; */
	sub_bf(bfymax, bcornery[0], badjy);
	/* yymin = cornery[1] - adjy; */
	sub_bf(bfymin, bcornery[1], badjy);
	/* yy3rd = cornery[2] - adjy; */
	sub_bf(bfy3rd, bcornery[2], badjy);

	adjust_corner_bf(); /* make 3rd corner exact if very near other co-ords */
	restore_stack(saved);
}

static void _fastcall adjust_to_limits(double expand)
{
	double cornerx[4], cornery[4];
	double lowx, highx, lowy, highy, limit, ftemp;
	double centerx, centery, adjx, adjy;
	int i;

	limit = 32767.99;

	if (integerfractal)
	{
		if (save_release > 1940) /* let user reproduce old GIF's and PAR's */
		{
			limit = 1023.99;
		}
		if (bitshift >= 24)
		{
			limit = 31.99;
		}
		if (bitshift >= 29)
		{
			limit = 3.99;
		}
	}

	centerx = (xxmin + xxmax)/2;
	centery = (yymin + yymax)/2;

	if (xxmin == centerx)  /* ohoh, infinitely thin, fix it */
	{
		smallest_add(&xxmax);
		xxmin -= xxmax-centerx;
	}

	if (yymin == centery)
	{
		smallest_add(&yymax);
		yymin -= yymax-centery;
	}

	if (xx3rd == centerx)
	{
		smallest_add(&xx3rd);
	}

	if (yy3rd == centery)
	{
		smallest_add(&yy3rd);
	}

	/* setup array for easier manipulation */
	cornerx[0] = xxmin;
	cornerx[1] = xxmax;
	cornerx[2] = xx3rd;
	cornerx[3] = xxmin + (xxmax-xx3rd);

	cornery[0] = yymax;
	cornery[1] = yymin;
	cornery[2] = yy3rd;
	cornery[3] = yymin + (yymax-yy3rd);

	/* if caller wants image size adjusted, do that first */
	if (expand != 1.0)
	{
		for (i = 0; i < 4; ++i)
		{
			cornerx[i] = centerx + (cornerx[i]-centerx)*expand;
			cornery[i] = centery + (cornery[i]-centery)*expand;
		}
	}
	/* get min/max x/y values */
	lowx = highx = cornerx[0];
	lowy = highy = cornery[0];

	for (i = 1; i < 4; ++i)
	{
		if (cornerx[i] < lowx)
		{
			lowx  = cornerx[i];
		}
		if (cornerx[i] > highx)
		{
			highx = cornerx[i];
		}
		if (cornery[i] < lowy)
		{
			lowy  = cornery[i];
		}
		if (cornery[i] > highy)
		{
			highy = cornery[i];
		}
	}

	/* if image is too large, downsize it maintaining center */
	ftemp = highx-lowx;

	if (highy-lowy > ftemp)
	{
		ftemp = highy-lowy;
	}

	/* if image is too large, downsize it maintaining center */
	ftemp = limit*2/ftemp;
	if (ftemp < 1.0)
	{
		for (i = 0; i < 4; ++i)
		{
			cornerx[i] = centerx + (cornerx[i]-centerx)*ftemp;
			cornery[i] = centery + (cornery[i]-centery)*ftemp;
		}
	}

	/* if any corner has x or y past limit, move the image */
	adjx = adjy = 0;

	for (i = 0; i < 4; ++i)
	{
		if (cornerx[i] > limit && (ftemp = cornerx[i] - limit) > adjx)
		{
			adjx = ftemp;
		}
		if (cornerx[i] < -limit && (ftemp = cornerx[i] + limit) < adjx)
		{
			adjx = ftemp;
		}
		if (cornery[i] > limit     && (ftemp = cornery[i] - limit) > adjy)
		{
			adjy = ftemp;
		}
		if (cornery[i] < -limit && (ftemp = cornery[i] + limit) < adjy)
		{
			adjy = ftemp;
		}
	}
	if (calc_status == CALCSTAT_RESUMABLE && (adjx != 0 || adjy != 0) && (zwidth == 1.0))
	{
		calc_status = CALCSTAT_PARAMS_CHANGED;
	}
	xxmin = cornerx[0] - adjx;
	xxmax = cornerx[1] - adjx;
	xx3rd = cornerx[2] - adjx;
	yymax = cornery[0] - adjy;
	yymin = cornery[1] - adjy;
	yy3rd = cornery[2] - adjy;

	adjust_corner(); /* make 3rd corner exact if very near other co-ords */
}

static void _fastcall smallest_add(double *num)
{
	*num += *num*5.0e-16;
}

static void _fastcall smallest_add_bf(bf_t num)
{
	bf_t btmp1;
	int saved; saved = save_stack();
	btmp1 = alloc_stack(bflength + 2);
	mult_bf(btmp1, floattobf(btmp1, 5.0e-16), num);
	add_a_bf(num, btmp1);
	restore_stack(saved);
}

static int _fastcall ratio_bad(double actual, double desired)
{
	double ftemp, tol;
	tol = math_tol[integerfractal ? 0 : 1];
	if (tol <= 0.0)
	{
		return 1;
	}
	else if (tol >= 1.0)
	{
		return 0;
	}
	ftemp = 0;
	if (desired != 0 && debugflag != DEBUGFLAG_NO_INT_TO_FLOAT)
	{
		ftemp = actual / desired;
		if (ftemp < (1.0-tol) || ftemp > (1.0 + tol))
		{
			return 1;
		}
	}
	return 0;
}


/* Save/resume stuff:

	Engines which aren't resumable can simply ignore all this.

	Before calling the (per_image, calctype) routines (engine), calculate_fractal sets:
		"resuming" to 0 if new image, nonzero if resuming a partially done image
		"calc_status" to CALCSTAT_IN_PROGRESS
	If an engine is interrupted and wants to be able to resume it must:
		store whatever status info it needs to be able to resume later
		set calc_status to CALCSTAT_RESUMABLE and return
	If subsequently called with resuming!=0, the engine must restore status
	info and continue from where it left off.

	Since the info required for resume can get rather large for some types,
	it is not stored directly in save_info.  Instead, memory is dynamically
	allocated as required, and stored in .fra files as a separate block.
	To save info for later resume, an engine routine can use:
		alloc_resume(maxsize, version)
			Maxsize must be >= max bytes subsequently saved + 2; over-allocation
			is harmless except for possibility of insufficient mem available;
			undersize is not checked and probably causes serious misbehaviour.
			Version is an arbitrary number so that subsequent revisions of the
			engine can be made backward compatible.
			Alloc_resume sets calc_status to CALCSTAT_RESUMABLE if it succeeds;
			to CALCSTAT_NON_RESUMABLE if it cannot allocate memory
			(and issues warning to user).
		put_resume({bytes, &argument, } ... 0)
			Can be called as often as required to store the info.
			Arguments must not be far addresses.
			Is not protected against calls which use more space than allocated.
	To reload info when resuming, use:
		start_resume()
			initializes and returns version number
		get_resume({bytes, &argument, } ... 0)
			inverse of store_resume
		end_resume()
			optional, frees the memory area sooner than would happen otherwise

	Example, save info:
		alloc_resume(sizeof(parmarray) + 100, 2);
		put_resume(sizeof(row), &row, sizeof(col), &col,
				sizeof(parmarray), parmarray, 0);
	restore info:
		vsn = start_resume();
		get_resume(sizeof(row), &row, sizeof(col), &col, 0);
		if (vsn >= 2)
			get_resume(sizeof(parmarray), parmarray, 0);
		end_resume();

	Engines which allocate a large memory chunk of their own might
	directly set g_resume_info, g_resume_length, calc_status to avoid doubling
	transient memory needs by using these routines.

	standard_fractal, calculate_mandelbrot, solidguess, and bound_trace_main are a related
	set of engines for escape-time fractals.  They use a common g_work_list
	structure for save/resume.  Fractals using these must specify calculate_mandelbrot
	or standard_fractal as the engine in fractalspecificinfo.
	Other engines don't get btm nor ssg, don't get off-axis symmetry nor
	panning (the g_work_list stuff), and are on their own for save/resume.

	*/

#ifndef USE_VARARGS
int put_resume(int len, ...)
#else
int put_resume(va_alist)
va_dcl
#endif
{
	va_list arg_marker;  /* variable arg list */
	BYTE *source_ptr;
#ifdef USE_VARARGS
	int len;
#endif

	if (g_resume_info == NULL)
	{
		return -1;
	}
#ifndef USE_VARARGS
	va_start(arg_marker, len);
#else
	va_start(arg_marker);
	len = va_arg(arg_marker, int);
#endif
	while (len)
	{
		source_ptr = (BYTE *)va_arg(arg_marker, char *);
		assert(g_resume_length + len <= s_resume_info_length);
		memcpy(&g_resume_info[g_resume_length], source_ptr, len);
		g_resume_length += len;
		len = va_arg(arg_marker, int);
	}
	va_end(arg_marker);
	return 0;
}

int alloc_resume(int alloclen, int version)
{ /* WARNING! if alloclen > 4096B, problems may occur with GIF save/restore */
	end_resume();

	s_resume_info_length = alloclen*alloclen;
	g_resume_info = malloc(s_resume_info_length);
	if (g_resume_info == NULL)
	{
		stopmsg(0, "Warning - insufficient free memory to save status.\n"
			"You will not be able to resume calculating this image.");
		calc_status = CALCSTAT_NON_RESUMABLE;
		s_resume_info_length = 0;
		return -1;
	}
	g_resume_length = 0;
	put_resume(sizeof(version), &version, 0);
	calc_status = CALCSTAT_RESUMABLE;
	return 0;
}

#ifndef USE_VARARGS
int get_resume(int len, ...)
#else
int get_resume(va_alist)
va_dcl
#endif
{
	va_list arg_marker;  /* variable arg list */
	BYTE *dest_ptr;
#ifdef USE_VARARGS
	int len;
#endif

	if (g_resume_info == NULL)
	{
		return -1;
	}
#ifndef USE_VARARGS
	va_start(arg_marker, len);
#else
	va_start(arg_marker);
	len = va_arg(arg_marker, int);
#endif
	while (len)
	{
		dest_ptr = (BYTE *)va_arg(arg_marker, char *);
#if defined(_WIN32)
		_ASSERTE(s_resume_offset + len <= s_resume_info_length);
#endif
		memcpy(dest_ptr, &g_resume_info[s_resume_offset], len);
		s_resume_offset += len;
		len = va_arg(arg_marker, int);
	}
	va_end(arg_marker);
	return 0;
}

int start_resume(void)
{
	int version;
	if (g_resume_info == NULL)
	{
		return -1;
	}
	s_resume_offset = 0;
	get_resume(sizeof(version), &version, 0);
	return version;
}

void end_resume(void)
{
	if (g_resume_info != NULL) /* free the prior area if there is one */
	{
		free(g_resume_info);
		g_resume_info = NULL;
		s_resume_info_length = 0;
	}
}


/* Showing orbit requires converting real co-ords to screen co-ords.
	Define:
		Xs == xxmax-xx3rd               Ys == yy3rd-yymax
		W  == xdots-1                   D  == ydots-1
	We know that:
		realx == lx0[col] + lx1[row]
		realy == ly0[row] + ly1[col]
		lx0[col] == (col/width)*Xs + xxmin
		lx1[row] == row*delxx
		ly0[row] == (row/D)*Ys + yymax
		ly1[col] == col*(-delyy)
	so:
		realx == (col/W)*Xs + xxmin + row*delxx
		realy == (row/D)*Ys + yymax + col*(-delyy)
	and therefore:
		row == (realx-xxmin - (col/W)*Xs) / Xv    (1)
		col == (realy-yymax - (row/D)*Ys) / Yv    (2)
	substitute (2) into (1) and solve for row:
		row == ((realx-xxmin)*(-delyy2)*W*D - (realy-yymax)*Xs*D)
						/ ((-delyy2)*W*delxx2*D-Ys*Xs)
*/

/* sleep N*a tenth of a millisecond */

static void sleep_ms_old(long ms)
{
	static long scalems = 0L;
	int savehelpmode, savetabmode;
	struct timebx t1, t2;
#define SLEEPINIT 250 /* milliseconds for calibration */
	savetabmode  = tabmode;
	savehelpmode = helpmode;
	tabmode  = 0;
	helpmode = -1;
	if (scalems == 0L) /* calibrate */
	{
		/* selects a value of scalems that makes the units
			10000 per sec independent of CPU speed */
		int i, elapsed;
		scalems = 1L;
		if (driver_key_pressed()) /* check at start, hope to get start of timeslice */
		{
			goto sleepexit;
		}
		/* calibrate, assume slow computer first */
		showtempmsg("Calibrating timer");
		do
		{
			scalems *= 2;
			ftimex(&t2);
			do  /* wait for the start of a new tick */
			{
				ftimex(&t1);
			}
			while (t2.time == t1.time && t2.millitm == t1.millitm);
			sleep_ms_old(10L*SLEEPINIT); /* about 1/4 sec */
			ftimex(&t2);
			if (driver_key_pressed())
			{
				scalems = 0L;
				cleartempmsg();
				goto sleepexit;
			}
			elapsed = (int)(t2.time-t1.time)*1000 + t2.millitm-t1.millitm;
		}
		while (elapsed < SLEEPINIT);
		/* once more to see if faster (eg multi-tasking) */
		do  /* wait for the start of a new tick */
		{
			ftimex(&t1);
		}
		while (t2.time == t1.time && t2.millitm == t1.millitm);
		sleep_ms_old(10L*SLEEPINIT);
		ftimex(&t2);
		i = (int)(t2.time-t1.time)*1000 + t2.millitm-t1.millitm;
		if (i < elapsed)
		{
			elapsed = (i == 0) ? 1 : i;
		}
		scalems = (long)((float)SLEEPINIT/(float)(elapsed)*scalems);
		cleartempmsg();
	}
	if (ms > 10L*SLEEPINIT)  /* using ftime is probably more accurate */
	{
		ms /= 10;
		ftimex(&t1);
		while (1)
		{
			if (driver_key_pressed())
			{
				break;
			}
			ftimex(&t2);
			if ((long)((t2.time-t1.time)*1000 + t2.millitm-t1.millitm) >= ms)
			{
				break;
			}
		}
	}
	else if (!driver_key_pressed())
	{
		ms *= scalems;
		while (ms-- >= 0)
		{
		}
	}
sleepexit:
	tabmode  = savetabmode;
	helpmode = savehelpmode;
}

static void sleep_ms_new(long ms)
{
	uclock_t next_time;
	uclock_t now = usec_clock();
	next_time = now + ms*100;
	while ((now = usec_clock()) < next_time)
	{
		if (driver_key_pressed())
		{
			break;
		}
	}
}

void sleep_ms(long ms)
{
	if (DEBUGFLAG_OLD_TIMER == debugflag)
	{
		sleep_ms_old(ms);
	}
	else
	{
		sleep_ms_new(ms);
	}
}

/*
* wait until wait_time microseconds from the
* last call has elapsed.
*/
#define MAX_INDEX 2
static uclock_t next_time[MAX_INDEX];
void wait_until(int index, uclock_t wait_time)
{
	if (DEBUGFLAG_OLD_TIMER == debugflag)
	{
		sleep_ms_old(wait_time);
	}
	else
	{
		uclock_t now;
		while ((now = usec_clock()) < next_time[index])
		{
			if (driver_key_pressed())
			{
				break;
			}
		}
		next_time[index] = now + wait_time*100; /* wait until this time next call */
	}
}

void reset_clock(void)
{
	int i;
	restart_uclock();
	for (i = 0; i < MAX_INDEX; i++)
	{
		next_time[i] = 0;
	}
}

#define LOG2  0.693147180f
#define LOG32 3.465735902f

static FILE *snd_fp = NULL;

/* open sound file */
static int sound_open(void)
{
	static char soundname[] = {"sound001.txt"};
	if ((orbitsave & ORBITSAVE_SOUND) != 0 && snd_fp == NULL)
	{
		snd_fp = fopen(soundname, "w");
		if (snd_fp == NULL)
		{
			stopmsg(0, "Can't open SOUND*.TXT");
		}
		else
		{
			updatesavename(soundname);
		}
	}
	return snd_fp != NULL;
}

/* This routine plays a tone in the speaker and optionally writes a file
	if the orbitsave variable is turned on */
void sound_tone(int tone)
{
	if ((orbitsave & ORBITSAVE_SOUND) != 0)
	{
		if (sound_open())
		{
			fprintf(snd_fp, "%-d\n", tone);
		}
	}
	s_tab_or_help = 0;
	if (!driver_key_pressed())  /* driver_key_pressed calls driver_sound_off() if TAB or F1 pressed */
	{
		/* must not then call soundoff(), else indexes out of synch */
		if (driver_sound_on(tone))
		{
			wait_until(0, orbit_delay);
			if (!s_tab_or_help) /* kludge because wait_until() calls driver_key_pressed */
			{
				driver_sound_off();
			}
		}
	}
}

void sound_write_time(void)
{
	if (sound_open())
	{
		fprintf(snd_fp, "time=%-ld\n", (long)clock()*1000/CLK_TCK);
	}
}

void sound_close(void)
{
	if (snd_fp)
	{
		fclose(snd_fp);
	}
	snd_fp = NULL;
}

static void _fastcall plot_orbit_d(double dx, double dy, int color)
{
	int i, j, c;
	int save_sxoffs, save_syoffs;
	if (g_orbit_index >= 1500-3)
	{
		return;
	}
	i = (int)(dy*plotmx1 - dx*plotmx2); i += sxoffs;
	if (i < 0 || i >= sxdots)
	{
		return;
	}
	j = (int)(dx*plotmy1 - dy*plotmy2); j += syoffs;
	if (j < 0 || j >= sydots)
	{
		return;
	}
	save_sxoffs = sxoffs;
	save_syoffs = syoffs;
	sxoffs = syoffs = 0;
	/* save orbit value */
	if (color == -1)
	{
		*(s_save_orbit + g_orbit_index++) = i;
		*(s_save_orbit + g_orbit_index++) = j;
		*(s_save_orbit + g_orbit_index++) = c = getcolor(i, j);
		g_put_color(i, j, c^g_orbit_color);
	}
	else
	{
		g_put_color(i, j, color);
	}
	sxoffs = save_sxoffs;
	syoffs = save_syoffs;
	if (DEBUGFLAG_OLD_ORBIT_SOUND == debugflag)
	{
		if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X) /* sound = x */
		{
			sound_tone((int)(i*1000/xdots + basehertz));
		}
		else if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X) /* sound = y or z */
		{
			sound_tone((int)(j*1000/ydots + basehertz));
		}
		else if (orbit_delay > 0)
		{
			wait_until(0, orbit_delay);
		}
	}
	else
	{
		if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X) /* sound = x */
		{
			sound_tone((int)(i + basehertz));
		}
		else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y) /* sound = y */
		{
			sound_tone((int)(j + basehertz));
		}
		else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z) /* sound = z */
		{
			sound_tone((int)(i + j + basehertz));
		}
		else if (orbit_delay > 0)
		{
			wait_until(0, orbit_delay);
		}
	}

	/* placing sleep_ms here delays each dot */
}

void plot_orbit_i(long ix, long iy, int color)
{
	plot_orbit_d((double)ix/fudge-xxmin, (double)iy/fudge-yymax, color);
}

void plot_orbit(double real, double imag, int color)
{
	plot_orbit_d(real-xxmin, imag-yymax, color);
}

void orbit_scrub(void)
{
	int i, j, c;
	int save_sxoffs, save_syoffs;
	driver_mute();
	save_sxoffs = sxoffs;
	save_syoffs = syoffs;
	sxoffs = syoffs = 0;
	while (g_orbit_index >= 3)
	{
		c = *(s_save_orbit + --g_orbit_index);
		j = *(s_save_orbit + --g_orbit_index);
		i = *(s_save_orbit + --g_orbit_index);
		g_put_color(i, j, c);
	}
	sxoffs = save_sxoffs;
	syoffs = save_syoffs;
}


int work_list_add(int xfrom, int xto, int xbegin,
	int yfrom, int yto, int ybegin,
	int pass, int sym)
{
	if (g_num_work_list >= MAXCALCWORK)
	{
		return -1;
	}
	g_work_list[g_num_work_list].xx_start = xfrom;
	g_work_list[g_num_work_list].xx_stop  = xto;
	g_work_list[g_num_work_list].xx_begin = xbegin;
	g_work_list[g_num_work_list].yy_start = yfrom;
	g_work_list[g_num_work_list].yy_stop  = yto;
	g_work_list[g_num_work_list].yy_begin = ybegin;
	g_work_list[g_num_work_list].pass    = pass;
	g_work_list[g_num_work_list].sym     = sym;
	++g_num_work_list;
	work_list_tidy();
	return 0;
}

static int _fastcall work_list_combine(void) /* look for 2 entries which can freely merge */
{
	int i, j;
	for (i = 0; i < g_num_work_list; ++i)
	{
		if (g_work_list[i].yy_start == g_work_list[i].yy_begin)
		{
			for (j = i + 1; j < g_num_work_list; ++j)
			{
				if (g_work_list[j].sym == g_work_list[i].sym
					&& g_work_list[j].yy_start == g_work_list[j].yy_begin
					&& g_work_list[j].xx_start == g_work_list[j].xx_begin
					&& g_work_list[i].pass == g_work_list[j].pass)
				{
					if (g_work_list[i].xx_start == g_work_list[j].xx_start
						&& g_work_list[i].xx_begin == g_work_list[j].xx_begin
						&& g_work_list[i].xx_stop  == g_work_list[j].xx_stop)
					{
						if (g_work_list[i].yy_stop + 1 == g_work_list[j].yy_start)
						{
							g_work_list[i].yy_stop = g_work_list[j].yy_stop;
							return j;
						}
						if (g_work_list[j].yy_stop + 1 == g_work_list[i].yy_start)
						{
							g_work_list[i].yy_start = g_work_list[j].yy_start;
							g_work_list[i].yy_begin = g_work_list[j].yy_begin;
							return j;
						}
					}
					if (g_work_list[i].yy_start == g_work_list[j].yy_start
						&& g_work_list[i].yy_begin == g_work_list[j].yy_begin
						&& g_work_list[i].yy_stop  == g_work_list[j].yy_stop)
					{
						if (g_work_list[i].xx_stop + 1 == g_work_list[j].xx_start)
						{
							g_work_list[i].xx_stop = g_work_list[j].xx_stop;
							return j;
						}
						if (g_work_list[j].xx_stop + 1 == g_work_list[i].xx_start)
						{
							g_work_list[i].xx_start = g_work_list[j].xx_start;
							g_work_list[i].xx_begin = g_work_list[j].xx_begin;
							return j;
						}
					}
				}
			}
		}
	}
	return 0; /* nothing combined */
}

void work_list_tidy(void) /* combine mergeable entries, resort */
{
	int i, j;
	WORKLIST tempwork;
	while ((i = work_list_combine()) != 0)
	{ /* merged two, delete the gone one */
		while (++i < g_num_work_list)
		{
			g_work_list[i-1] = g_work_list[i];
		}
		--g_num_work_list;
	}
	for (i = 0; i < g_num_work_list; ++i)
	{
		for (j = i + 1; j < g_num_work_list; ++j)
		{
			if (g_work_list[j].pass < g_work_list[i].pass
				|| (g_work_list[j].pass == g_work_list[i].pass
				&& (g_work_list[j].yy_start < g_work_list[i].yy_start
				|| (g_work_list[j].yy_start == g_work_list[i].yy_start
				&& g_work_list[j].xx_start <  g_work_list[i].xx_start))))
			{ /* dumb sort, swap 2 entries to correct order */
				tempwork = g_work_list[i];
				g_work_list[i] = g_work_list[j];
				g_work_list[j] = tempwork;
			}
		}
	}
}

void get_julia_attractor(double real, double imag)
{
	_LCMPLX lresult;
	_CMPLX result;
	int savper;
	long savmaxit;
	int i;

	if (g_num_attractors == 0 && finattract == 0) /* not magnet & not requested */
	{
		return;
	}

	if (g_num_attractors >= N_ATTR)     /* space for more attractors ?  */
	{
		return;                  /* Bad luck - no room left !    */
	}

	savper = g_periodicity_check;
	savmaxit = maxit;
	g_periodicity_check = 0;
	g_old_z.x = real;                    /* prepare for f.p orbit calc */
	g_old_z.y = imag;
	g_temp_sqr_x = sqr(g_old_z.x);
	g_temp_sqr_y = sqr(g_old_z.y);

	g_old_z_l.x = (long)real;     /* prepare for int orbit calc */
	g_old_z_l.y = (long)imag;
	g_temp_sqr_x_l = (long)g_temp_sqr_x;
	g_temp_sqr_y_l = (long)g_temp_sqr_y;

	g_old_z_l.x = g_old_z_l.x << bitshift;
	g_old_z_l.y = g_old_z_l.y << bitshift;
	g_temp_sqr_x_l = g_temp_sqr_x_l << bitshift;
	g_temp_sqr_y_l = g_temp_sqr_y_l << bitshift;

	if (maxit < 500)         /* we're going to try at least this hard */
	{
		maxit = 500;
	}
	g_color_iter = 0;
	overflow = 0;
	while (++g_color_iter < maxit)
		if (curfractalspecific->orbitcalc() || overflow)
		{
			break;
		}
	if (g_color_iter >= maxit)      /* if orbit stays in the lake */
	{
		if (integerfractal)   /* remember where it went to */
		{
			lresult = g_new_z_l;
		}
		else
		{
			result =  g_new_z;
		}
		for (i = 0; i < 10; i++)
		{
			overflow = 0;
			if (!curfractalspecific->orbitcalc() && !overflow) /* if it stays in the lake */
			{                        /* and doesn't move far, probably */
				if (integerfractal)   /*   found a finite attractor    */
				{
					if (labs(lresult.x-g_new_z_l.x) < g_close_enough_l
						&& labs(lresult.y-g_new_z_l.y) < g_close_enough_l)
					{
						g_attractors_l[g_num_attractors] = g_new_z_l;
						g_attractor_period[g_num_attractors] = i + 1;
						g_num_attractors++;   /* another attractor - coloured lakes ! */
						break;
					}
				}
				else
				{
					if (fabs(result.x-g_new_z.x) < g_close_enough
						&& fabs(result.y-g_new_z.y) < g_close_enough)
					{
						g_attractors[g_num_attractors] = g_new_z;
						g_attractor_period[g_num_attractors] = i + 1;
						g_num_attractors++;   /* another attractor - coloured lakes ! */
						break;
					}
				}
			}
			else
			{
				break;
			}
		}
	}
	if (g_num_attractors == 0)
	{
		g_periodicity_check = savper;
	}
	maxit = savmaxit;
}


int solid_guess_block_size(void) /* used by solidguessing and by zoom panning */
{
	int blocksize, i;
	/* blocksize 4 if <300 rows, 8 if 300-599, 16 if 600-1199, 32 if >= 1200 */
	blocksize = 4;
	i = 300;
	while (i <= ydots)
	{
		blocksize += blocksize;
		i += i;
	}
	/* increase blocksize if prefix array not big enough */
	while (blocksize*(MAX_X_BLOCK-2) < xdots || blocksize*(MAX_Y_BLOCK-2)*16 < ydots)
	{
		blocksize += blocksize;
	}
	return blocksize;
}
