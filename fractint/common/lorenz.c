/*
	This file contains two 3 dimensional orbit-type fractal
	generators - IFS and LORENZ3D, along with code to generate
	red/blue 3D images. Tim Wegner
*/
#include <assert.h>
#include <string.h>
/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "drivers.h"

/* orbitcalc is declared with no arguments so jump through hoops here */
#define LORBIT(x, y, z) \
	(*(int(*)(long *, long *, long *))curfractalspecific->orbitcalc)(x, y, z)
#define FORBIT(x, y, z) \
	(*(int(*)(double*, double*, double*))curfractalspecific->orbitcalc)(x, y, z)

#define RANDOM(x)  (rand() % (x))
/* BAD_PIXEL is used to cutoff orbits that are diverging. It might be better
to test the actual floating point orbit values, but this seems safe for now.
A higher value cannot be used - to test, turn off math coprocessor and
use +2.24 for type ICONS. If BAD_PIXEL is set to 20000, this will abort
Fractint with a math error. Note that this approach precludes zooming in very
far to an orbit type. */

#define BAD_PIXEL  10000L    /* pixels can't get this big */

struct l_affine
{
	/* weird order so a, b, e and c, d, f are vectors */
	long a;
	long b;
	long e;
	long c;
	long d;
	long f;
};
struct threed_vt_inf /* data used by 3d view transform subroutine */
{
	long orbit[3];       /* interated function orbit value */
	long iview[3];       /* perspective viewer's coordinates */
	long viewvect[3];    /* orbit transformed for viewing */
	long viewvect1[3];   /* orbit transformed for viewing */
	long maxvals[3];
	long minvals[3];
	MATRIX doublemat;    /* transformation matrix */
	MATRIX doublemat1;   /* transformation matrix */
	long longmat[4][4];  /* long version of matrix */
	long longmat1[4][4]; /* long version of matrix */
	int row, col;         /* results */
	int row1, col1;
	struct l_affine cvt;
};

struct threed_vt_inf_fp /* data used by 3d view transform subroutine */
{
	double orbit[3];                /* interated function orbit value */
	double viewvect[3];        /* orbit transformed for viewing */
	double viewvect1[3];        /* orbit transformed for viewing */
	double maxvals[3];
	double minvals[3];
	MATRIX doublemat;    /* transformation matrix */
	MATRIX doublemat1;   /* transformation matrix */
	int row, col;         /* results */
	int row1, col1;
	struct affine cvt;
};

/* global data provided by this module */
long g_max_count;
enum Major g_major_method;
enum Minor g_minor_method;
int g_keep_screen_coords = 0;
int g_set_orbit_corners = 0;
long g_orbit_interval;
double g_orbit_x_min, g_orbit_y_min, g_orbit_x_max, g_orbit_y_max, g_orbit_x_3rd, g_orbit_y_3rd;

/* local data in this module */
static struct affine s_o_cvt;
static int s_o_color;
static struct affine s_cvt;
static struct l_affine s_lcvt;
static double s_cx, s_cy;
static long   s_x_long, s_y_long;
/* s_connect, s_euler, s_waste are potential user parameters */
static int s_connect = 1;    /* flag to connect points with a line */
static int s_euler = 0;      /* use implicit euler approximation for dynamic system */
static int s_waste = 100;    /* waste this many points before plotting */
static int s_run_length;
static int s_real_time;
static int s_t;
static long s_l_dx, s_l_dy, s_l_dz, s_l_dt, s_l_a, s_l_b, s_l_c, s_l_d;
static long s_l_adt, s_l_bdt, s_l_cdt, s_l_xdt, s_l_ydt;
static long s_init_orbit_long[3];
static double s_dx, s_dy, s_dz, s_dt, s_a, s_b, s_c, s_d;
static double s_adt, s_bdt, s_cdt, s_xdt, s_ydt, s_zdt;
static double s_init_orbit_fp[3];
static char s_no_queue[] = "Not enough memory: switching to random walk.\n";
static int s_max_hits;
static int s_projection = PROJECTION_XY; /* projection plane - default is to plot x-y */
static double s_orbit;
static long s_l_orbit;
static long s_l_sinx, s_l_cosx;

/* local routines in this module */
static int  ifs_2d(void);
static int  ifs_3d(void);
static int  ifs_3d_long(void);
static int  ifs_3d_float(void);
static int  l_setup_convert_to_screen(struct l_affine *);
static void setup_matrix(MATRIX);
static int  threed_view_trans(struct threed_vt_inf *inf);
static int  threed_view_trans_fp(struct threed_vt_inf_fp *inf);
static FILE *open_orbit_save(void);
static void _fastcall plot_hist(int x, int y, int color);

/******************************************************************/
/*                 zoom box conversion functions                  */
/******************************************************************/

/*
	Conversion of complex plane to screen coordinates for rotating zoom box.
	Assume there is an affine transformation mapping complex zoom parallelogram
	to rectangular screen. We know this map must map parallelogram corners to
	screen corners, so we have following equations:

		a*xxmin + b*yymax + e == 0        (upper left)
		c*xxmin + d*yymax + f == 0

		a*xx3rd + b*yy3rd + e == 0        (lower left)
		c*xx3rd + d*yy3rd + f == ydots-1

		a*xxmax + b*yymin + e == xdots-1  (lower right)
		c*xxmax + d*yymin + f == ydots-1

		First we must solve for a, b, c, d, e, f - (which we do once per image),
		then we just apply the transformation to each orbit value.
*/

/*
	Thanks to Sylvie Gallet for the following. The original code for
	setup_convert_to_screen() solved for coefficients of the
	complex-plane-to-screen transformation using a very straight-forward
	application of determinants to solve a set of simulataneous
	equations. The procedure was simple and general, but inefficient.
	The inefficiecy wasn't hurting anything because the routine was called
	only once per image, but it seemed positively sinful to use it
	because the code that follows is SO much more compact, at the
	expense of being less general. Here are Sylvie's notes. I have further
	optimized the code a slight bit.
											Tim Wegner
											July, 1996
  Sylvie's notes, slightly edited follow:

  You don't need 3x3 determinants to solve these sets of equations because
  the unknowns e and f have the same coefficient: 1.

  First set of 3 equations:
     a*xxmin + b*yymax + e == 0
     a*xx3rd + b*yy3rd + e == 0
     a*xxmax + b*yymin + e == xdots-1
  To make things easy to read, I just replace xxmin, xxmax, xx3rd by x1,
  x2, x3 (ditto for yy...) and xdots-1 by xd.

     a*x1 + b*y2 + e == 0    (1)
     a*x3 + b*y3 + e == 0    (2)
     a*x2 + b*y1 + e == xd   (3)

  I subtract (1) to (2) and (3):
     a*x1      + b*y2      + e == 0   (1)
     a*(x3-x1) + b*(y3-y2)     == 0   (2)-(1)
     a*(x2-x1) + b*(y1-y2)     == xd  (3)-(1)

  I just have to calculate a 2x2 determinant:
     det == (x3-x1)*(y1-y2) - (y3-y2)*(x2-x1)

  And the solution is:
     a = -xd*(y3-y2)/det
     b =  xd*(x3-x1)/det
     e = - a*x1 - b*y2

The same technique can be applied to the second set of equations:

	c*xxmin + d*yymax + f == 0
	c*xx3rd + d*yy3rd + f == ydots-1
	c*xxmax + d*yymin + f == ydots-1

	c*x1 + d*y2 + f == 0    (1)
	c*x3 + d*y3 + f == yd   (2)
	c*x2 + d*y1 + f == yd   (3)

	c*x1      + d*y2      + f == 0    (1)
	c*(x3-x2) + d*(y3-y1)     == 0    (2)-(3)
	c*(x2-x1) + d*(y1-y2)     == yd   (3)-(1)

	det == (x3-x2)*(y1-y2) - (y3-y1)*(x2-x1)

	c = -yd*(y3-y1)/det
	d =  yd*(x3-x2))det
	f = - c*x1 - d*y2

		-  Sylvie
*/

int setup_convert_to_screen(struct affine *scrn_cnvt)
{
	double det, xd, yd;

	det = (xx3rd-xxmin)*(yymin-yymax) + (yymax-yy3rd)*(xxmax-xxmin);
	if (det == 0)
	{
		return -1;
	}
	xd = dxsize/det;
	scrn_cnvt->a =  xd*(yymax-yy3rd);
	scrn_cnvt->b =  xd*(xx3rd-xxmin);
	scrn_cnvt->e = -scrn_cnvt->a*xxmin - scrn_cnvt->b*yymax;

	det = (xx3rd-xxmax)*(yymin-yymax) + (yymin-yy3rd)*(xxmax-xxmin);
	if (det == 0)
	{
		return -1;
	}
	yd = dysize/det;
	scrn_cnvt->c =  yd*(yymin-yy3rd);
	scrn_cnvt->d =  yd*(xx3rd-xxmax);
	scrn_cnvt->f = -scrn_cnvt->c*xxmin - scrn_cnvt->d*yymax;
	return 0;
}

static int l_setup_convert_to_screen(struct l_affine *l_cvt)
{
	struct affine cvt;

	/* MCP 7-7-91, This function should return a something! */
	if (setup_convert_to_screen(&cvt))
	{
		return -1;
	}
	l_cvt->a = (long) (cvt.a*fudge);
	l_cvt->b = (long) (cvt.b*fudge);
	l_cvt->c = (long) (cvt.c*fudge);
	l_cvt->d = (long) (cvt.d*fudge);
	l_cvt->e = (long) (cvt.e*fudge);
	l_cvt->f = (long) (cvt.f*fudge);

	/* MCP 7-7-91 */
	return 0;
}

/******************************************************************/
/*   setup functions - put in fractalspecific[fractype].per_image */
/******************************************************************/

int orbit_3d_setup(void)
{
	g_max_count = 0L;
	s_connect = 1;
	s_waste = 100;
	s_projection = PROJECTION_XY;
	if (fractype == LHENON || fractype == KAM || fractype == KAM3D ||
		fractype == INVERSEJULIA)
	{
		s_connect = 0;
	}
	if (fractype == LROSSLER)
	{
		s_waste = 500;
	}
	if (fractype == LLORENZ)
	{
		s_projection = PROJECTION_XZ;
	}

	s_init_orbit_long[0] = fudge;  /* initial conditions */
	s_init_orbit_long[1] = fudge;
	s_init_orbit_long[2] = fudge;

	if (fractype == LHENON)
	{
		s_l_a =  (long) (param[0]*fudge);
		s_l_b =  (long) (param[1]*fudge);
		s_l_c =  (long) (param[2]*fudge);
		s_l_d =  (long) (param[3]*fudge);
	}
	else if (fractype == KAM || fractype == KAM3D)
	{
		g_max_count = 1L;
		s_a   = param[0];           /* angle */
		if (param[1] <= 0.0)
		{
			param[1] = .01;
		}
		s_l_b =  (long) (param[1]*fudge);    /* stepsize */
		s_l_c =  (long) (param[2]*fudge);    /* stop */
		s_l_d =  (long) param[3];
		s_t = (int) s_l_d;     /* points per orbit */

		s_l_sinx = (long) (sin(s_a)*fudge);
		s_l_cosx = (long) (cos(s_a)*fudge);
		s_l_orbit = 0;
		s_init_orbit_long[0] = s_init_orbit_long[1] = s_init_orbit_long[2] = 0;
	}
	else if (fractype == INVERSEJULIA)
	{
		LCMPLX Sqrt;

		s_x_long = (long) (param[0]*fudge);
		s_y_long = (long) (param[1]*fudge);

		s_max_hits    = (int) param[2];
		s_run_length = (int) param[3];
		if (s_max_hits <= 0)
		{
			s_max_hits = 1;
		}
		else if (s_max_hits >= colors)
		{
			s_max_hits = colors - 1;
		}
		param[2] = s_max_hits;

		setup_convert_to_screen(&s_cvt);
		/* Note: using bitshift of 21 for affine, 24 otherwise */

		s_lcvt.a = (long) (s_cvt.a*(1L << 21));
		s_lcvt.b = (long) (s_cvt.b*(1L << 21));
		s_lcvt.c = (long) (s_cvt.c*(1L << 21));
		s_lcvt.d = (long) (s_cvt.d*(1L << 21));
		s_lcvt.e = (long) (s_cvt.e*(1L << 21));
		s_lcvt.f = (long) (s_cvt.f*(1L << 21));

		Sqrt = ComplexSqrtLong(fudge - 4*s_x_long, -4*s_y_long);

		switch (g_major_method)
		{
		case breadth_first:
			if (Init_Queue((long)32*1024) == 0)
			{ /* can't get queue memory: fall back to random walk */
				stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, s_no_queue);
				g_major_method = random_walk;
				goto lrwalk;
			}
			EnQueueLong((fudge + Sqrt.x) / 2,  Sqrt.y / 2);
			EnQueueLong((fudge - Sqrt.x) / 2, -Sqrt.y / 2);
			break;
		case depth_first:
			if (Init_Queue((long)32*1024) == 0)
			{ /* can't get queue memory: fall back to random walk */
				stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, s_no_queue);
				g_major_method = random_walk;
				goto lrwalk;
			}
			switch (g_minor_method)
			{
				case left_first:
					PushLong((fudge + Sqrt.x) / 2,  Sqrt.y / 2);
					PushLong((fudge - Sqrt.x) / 2, -Sqrt.y / 2);
					break;
				case right_first:
					PushLong((fudge - Sqrt.x) / 2, -Sqrt.y / 2);
					PushLong((fudge + Sqrt.x) / 2,  Sqrt.y / 2);
					break;
			}
			break;
		case random_walk:
lrwalk:
			lnew.x = s_init_orbit_long[0] = fudge + Sqrt.x / 2;
			lnew.y = s_init_orbit_long[1] =         Sqrt.y / 2;
			break;
		case random_run:
			lnew.x = s_init_orbit_long[0] = fudge + Sqrt.x / 2;
			lnew.y = s_init_orbit_long[1] =         Sqrt.y / 2;
			break;
		}
	}
	else
	{
		s_l_dt = (long) (param[0]*fudge);
		s_l_a =  (long) (param[1]*fudge);
		s_l_b =  (long) (param[2]*fudge);
		s_l_c =  (long) (param[3]*fudge);
	}

	/* precalculations for speed */
	s_l_adt = multiply(s_l_a, s_l_dt, bitshift);
	s_l_bdt = multiply(s_l_b, s_l_dt, bitshift);
	s_l_cdt = multiply(s_l_c, s_l_dt, bitshift);
	return 1;
}

#define COSB   s_dx
#define SINABC s_dy

int orbit_3d_setup_fp()
{
	g_max_count = 0L;
	s_connect = 1;
	s_waste = 100;
	s_projection = PROJECTION_XY;

	if (fractype == FPHENON || fractype == FPPICKOVER || fractype == FPGINGERBREAD
				|| fractype == KAMFP || fractype == KAM3DFP
				|| fractype == FPHOPALONG || fractype == INVERSEJULIAFP)
	{
		s_connect = 0;
	}
	if (fractype == FPLORENZ3D1 || fractype == FPLORENZ3D3 ||
		fractype == FPLORENZ3D4)
	{
		s_waste = 750;
	}
	if (fractype == FPROSSLER)
	{
		s_waste = 500;
	}
	if (fractype == FPLORENZ)
	{
		s_projection = PROJECTION_XZ; /* plot x and z */
	}

	s_init_orbit_fp[0] = 1;  /* initial conditions */
	s_init_orbit_fp[1] = 1;
	s_init_orbit_fp[2] = 1;
	if (fractype == FPGINGERBREAD)
	{
		s_init_orbit_fp[0] = param[0];        /* initial conditions */
		s_init_orbit_fp[1] = param[1];
	}

	if (fractype == ICON || fractype == ICON3D)        /* DMF */
	{
		s_init_orbit_fp[0] = 0.01;  /* initial conditions */
		s_init_orbit_fp[1] = 0.003;
		s_connect = 0;
		s_waste = 2000;
	}

	if (fractype == LATOO)        /* HB */
	{
		s_connect = 0;
	}

	if (fractype == FPHENON || fractype == FPPICKOVER)
	{
		s_a =  param[0];
		s_b =  param[1];
		s_c =  param[2];
		s_d =  param[3];
	}
	else if (fractype == ICON || fractype == ICON3D)        /* DMF */
	{
		s_init_orbit_fp[0] = 0.01;  /* initial conditions */
		s_init_orbit_fp[1] = 0.003;
		s_connect = 0;
		s_waste = 2000;
		/* Initialize parameters */
		s_a  =   param[0];
		s_b  =   param[1];
		s_c  =   param[2];
		s_d  =   param[3];
	}
	else if (fractype == KAMFP || fractype == KAM3DFP)
	{
		g_max_count = 1L;
		s_a = param[0];           /* angle */
		if (param[1] <= 0.0)
		{
			param[1] = .01;
		}
		s_b =  param[1];    /* stepsize */
		s_c =  param[2];    /* stop */
		s_l_d =  (long) param[3];
		s_t = (int) s_l_d;     /* points per orbit */
		sinx = sin(s_a);
		cosx = cos(s_a);
		s_orbit = 0;
		s_init_orbit_fp[0] = s_init_orbit_fp[1] = s_init_orbit_fp[2] = 0;
	}
	else if (fractype == FPHOPALONG || fractype == FPMARTIN || fractype == CHIP
		|| fractype == QUADRUPTWO || fractype == THREEPLY)
	{
		s_init_orbit_fp[0] = 0;  /* initial conditions */
		s_init_orbit_fp[1] = 0;
		s_init_orbit_fp[2] = 0;
		s_connect = 0;
		s_a =  param[0];
		s_b =  param[1];
		s_c =  param[2];
		s_d =  param[3];
		if (fractype == THREEPLY)
		{
			COSB   = cos(s_b);
			SINABC = sin(s_a + s_b + s_c);
		}
	}
	else if (fractype == INVERSEJULIAFP)
	{
		_CMPLX Sqrt;

		s_cx = param[0];
		s_cy = param[1];

		s_max_hits    = (int) param[2];
		s_run_length = (int) param[3];
		if (s_max_hits <= 0)
		{
			s_max_hits = 1;
		}
		else if (s_max_hits >= colors)
		{
			s_max_hits = colors - 1;
		}
		param[2] = s_max_hits;

		setup_convert_to_screen(&s_cvt);

		/* find fixed points: guaranteed to be in the set */
		Sqrt = ComplexSqrtFloat(1 - 4*s_cx, -4*s_cy);
		switch (g_major_method)
		{
		case breadth_first:
			if (Init_Queue((long)32*1024) == 0)
			{ /* can't get queue memory: fall back to random walk */
				stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, s_no_queue);
				g_major_method = random_walk;
				goto rwalk;
			}
			EnQueueFloat((float) ((1 + Sqrt.x) / 2), (float) (Sqrt.y / 2));
			EnQueueFloat((float) ((1 - Sqrt.x) / 2), (float) (-Sqrt.y / 2));
			break;
		case depth_first:                      /* depth first (choose direction) */
			if (Init_Queue((long)32*1024) == 0)
			{ /* can't get queue memory: fall back to random walk */
				stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, s_no_queue);
				g_major_method = random_walk;
				goto rwalk;
			}
			switch (g_minor_method)
			{
			case left_first:
				PushFloat((float) ((1 + Sqrt.x) / 2), (float) (Sqrt.y / 2));
				PushFloat((float) ((1 - Sqrt.x) / 2), (float) (-Sqrt.y / 2));
				break;
			case right_first:
				PushFloat((float) ((1 - Sqrt.x) / 2), (float) (-Sqrt.y / 2));
				PushFloat((float) ((1 + Sqrt.x) / 2), (float) (Sqrt.y / 2));
				break;
			}
			break;
		case random_walk:
rwalk:
			g_new_z.x = s_init_orbit_fp[0] = 1 + Sqrt.x / 2;
			g_new_z.y = s_init_orbit_fp[1] = Sqrt.y / 2;
			break;
		case random_run:       /* random run, choose intervals */
			g_major_method = random_run;
			g_new_z.x = s_init_orbit_fp[0] = 1 + Sqrt.x / 2;
			g_new_z.y = s_init_orbit_fp[1] = Sqrt.y / 2;
			break;
		}
	}
	else
	{
		s_dt = param[0];
		s_a =  param[1];
		s_b =  param[2];
		s_c =  param[3];

	}

	/* precalculations for speed */
	s_adt = s_a*s_dt;
	s_bdt = s_b*s_dt;
	s_cdt = s_c*s_dt;

	return 1;
}

/******************************************************************/
/*   orbit functions - put in fractalspecific[fractype].orbitcalc */
/******************************************************************/

/* Julia sets by inverse iterations added by Juan J. Buhler 4/3/92 */
/* Integrated with Lorenz by Tim Wegner 7/20/92 */
/* Add Modified Inverse Iteration Method, 11/92 by Michael Snyder  */

int Minverse_julia_orbit()
{
	static int   random_dir = 0, random_len = 0;
	int    newrow, newcol;
	int    color,  leftright;

	/*
	* First, compute new point
	*/
	switch (g_major_method)
	{
	case breadth_first:
		if (QueueEmpty())
		{
			return -1;
		}
		g_new_z = DeQueueFloat();
		break;
	case depth_first:
		if (QueueEmpty())
		{
			return -1;
		}
		g_new_z = PopFloat();
		break;
	case random_walk:
#if 0
		g_new_z = ComplexSqrtFloat(g_new_z.x - s_cx, g_new_z.y - s_cy);
		if (RANDOM(2))
		{
			g_new_z.x = -g_new_z.x;
			g_new_z.y = -g_new_z.y;
		}
#endif
		break;
	case random_run:
#if 0
		g_new_z = ComplexSqrtFloat(g_new_z.x - s_cx, g_new_z.y - s_cy);
		if (random_len == 0)
		{
			random_len = RANDOM(s_run_length);
			random_dir = RANDOM(3);
		}
		switch (random_dir)
		{
		case DIRECTION_LEFT:
			break;
		case DIRECTION_RIGHT:
			g_new_z.x = -g_new_z.x;
			g_new_z.y = -g_new_z.y;
			break;
		case DIRECTION_RANDOM:
			if (RANDOM(2))
			{
				g_new_z.x = -g_new_z.x;
				g_new_z.y = -g_new_z.y;
			}
			break;
		}
#endif
		break;
	}

	/*
	* Next, find its pixel position
	*/
	newcol = (int) (s_cvt.a*g_new_z.x + s_cvt.b*g_new_z.y + s_cvt.e);
	newrow = (int) (s_cvt.c*g_new_z.x + s_cvt.d*g_new_z.y + s_cvt.f);

	/*
	* Now find the next point(s), and flip a coin to choose one.
	*/

	g_new_z       = ComplexSqrtFloat(g_new_z.x - s_cx, g_new_z.y - s_cy);
	leftright = (RANDOM(2)) ? 1 : -1;

	if (newcol < 1 || newcol >= xdots || newrow < 1 || newrow >= ydots)
	{
		/*
		* MIIM must skip points that are off the screen boundary,
		* since it cannot read their color.
		*/
		switch (g_major_method)
		{
		case breadth_first:
			EnQueueFloat((float) (leftright*g_new_z.x), (float) (leftright*g_new_z.y));
			return 1;
		case depth_first:
			PushFloat   ((float) (leftright*g_new_z.x), (float) (leftright*g_new_z.y));
			return 1;
		case random_run:
		case random_walk:
			break;
		}
	}

	/*
	* Read the pixel's color:
	* For MIIM, if color >= s_max_hits, discard the point
	*           else put the point's children onto the queue
	*/
	color  = getcolor(newcol, newrow);
	switch (g_major_method)
	{
	case breadth_first:
		if (color < s_max_hits)
		{
			putcolor(newcol, newrow, color + 1);
			/* g_new_z = ComplexSqrtFloat(g_new_z.x - s_cx, g_new_z.y - s_cy); */
			EnQueueFloat((float)g_new_z.x, (float)g_new_z.y);
			EnQueueFloat((float)-g_new_z.x, (float)-g_new_z.y);
		}
		break;
	case depth_first:
		if (color < s_max_hits)
		{
			putcolor(newcol, newrow, color + 1);
			/* g_new_z = ComplexSqrtFloat(g_new_z.x - s_cx, g_new_z.y - s_cy); */
			if (g_minor_method == left_first)
			{
				if (QueueFullAlmost())
				{
					PushFloat((float)-g_new_z.x, (float)-g_new_z.y);
				}
				else
				{
					PushFloat((float)g_new_z.x, (float)g_new_z.y);
					PushFloat((float)-g_new_z.x, (float)-g_new_z.y);
				}
			}
			else
			{
				if (QueueFullAlmost())
				{
					PushFloat((float)g_new_z.x, (float)g_new_z.y);
				}
				else
				{
					PushFloat((float)-g_new_z.x, (float)-g_new_z.y);
					PushFloat((float)g_new_z.x, (float)g_new_z.y);
				}
			}
		}
		break;
	case random_run:
		if (random_len-- == 0)
		{
			random_len = RANDOM(s_run_length);
			random_dir = RANDOM(3);
		}
		switch (random_dir)
		{
		case DIRECTION_LEFT:
			break;
		case DIRECTION_RIGHT:
			g_new_z.x = -g_new_z.x;
			g_new_z.y = -g_new_z.y;
			break;
		case DIRECTION_RANDOM:
			g_new_z.x = leftright*g_new_z.x;
			g_new_z.y = leftright*g_new_z.y;
			break;
		}
		if (color < colors-1)
		{
			putcolor(newcol, newrow, color + 1);
		}
		break;
	case random_walk:
		if (color < colors-1)
		{
			putcolor(newcol, newrow, color + 1);
		}
		g_new_z.x = leftright*g_new_z.x;
		g_new_z.y = leftright*g_new_z.y;
		break;
	}
	return 1;
}

int Linverse_julia_orbit()
{
	static int   random_dir = 0, random_len = 0;
	int    newrow, newcol;
	int    color;

	/*
	* First, compute new point
	*/
	switch (g_major_method)
	{
	case breadth_first:
		if (QueueEmpty())
		{
			return -1;
		}
		lnew = DeQueueLong();
		break;
	case depth_first:
		if (QueueEmpty())
		{
			return -1;
		}
		lnew = PopLong();
		break;
	case random_walk:
		lnew = ComplexSqrtLong(lnew.x - s_x_long, lnew.y - s_y_long);
		if (RANDOM(2))
		{
			lnew.x = -lnew.x;
			lnew.y = -lnew.y;
		}
		break;
	case random_run:
		lnew = ComplexSqrtLong(lnew.x - s_x_long, lnew.y - s_y_long);
		if (random_len == 0)
		{
			random_len = RANDOM(s_run_length);
			random_dir = RANDOM(3);
		}
		switch (random_dir)
		{
		case DIRECTION_LEFT:
			break;
		case DIRECTION_RIGHT:
			lnew.x = -lnew.x;
			lnew.y = -lnew.y;
			break;
		case DIRECTION_RANDOM:
			if (RANDOM(2))
			{
				lnew.x = -lnew.x;
				lnew.y = -lnew.y;
			}
			break;
		}
	}

	/*
	* Next, find its pixel position
	*
	* Note: had to use a bitshift of 21 for this operation because
	* otherwise the values of s_lcvt were truncated.  Used bitshift
	* of 24 otherwise, for increased precision.
	*/
	newcol = (int) ((multiply(s_lcvt.a, lnew.x >> (bitshift - 21), 21) +
			multiply(s_lcvt.b, lnew.y >> (bitshift - 21), 21) + s_lcvt.e) >> 21);
	newrow = (int) ((multiply(s_lcvt.c, lnew.x >> (bitshift - 21), 21) +
			multiply(s_lcvt.d, lnew.y >> (bitshift - 21), 21) + s_lcvt.f) >> 21);

	if (newcol < 1 || newcol >= xdots || newrow < 1 || newrow >= ydots)
	{
		/*
		* MIIM must skip points that are off the screen boundary,
		* since it cannot read their color.
		*/
		color = RANDOM(2) ? 1 : -1;
		switch (g_major_method)
		{
		case breadth_first:
			lnew = ComplexSqrtLong(lnew.x - s_x_long, lnew.y - s_y_long);
			EnQueueLong(color*lnew.x, color*lnew.y);
			break;
		case depth_first:
			lnew = ComplexSqrtLong(lnew.x - s_x_long, lnew.y - s_y_long);
			PushLong(color*lnew.x, color*lnew.y);
			break;
		case random_run:
			random_len--;
		case random_walk:
			break;
		}
		return 1;
	}

	/*
	* Read the pixel's color:
	* For MIIM, if color >= s_max_hits, discard the point
	*           else put the point's children onto the queue
	*/
	color  = getcolor(newcol, newrow);
	switch (g_major_method)
	{
	case breadth_first:
		if (color < s_max_hits)
		{
			putcolor(newcol, newrow, color + 1);
			lnew = ComplexSqrtLong(lnew.x - s_x_long, lnew.y - s_y_long);
			EnQueueLong(lnew.x,  lnew.y);
			EnQueueLong(-lnew.x, -lnew.y);
		}
		break;
	case depth_first:
		if (color < s_max_hits)
		{
			putcolor(newcol, newrow, color + 1);
			lnew = ComplexSqrtLong(lnew.x - s_x_long, lnew.y - s_y_long);
			if (g_minor_method == left_first)
			{
				if (QueueFullAlmost())
				{
					PushLong(-lnew.x, -lnew.y);
				}
				else
				{
					PushLong(lnew.x,  lnew.y);
					PushLong(-lnew.x, -lnew.y);
				}
			}
			else
			{
				if (QueueFullAlmost())
				{
					PushLong(lnew.x,  lnew.y);
				}
				else
				{
					PushLong(-lnew.x, -lnew.y);
					PushLong(lnew.x,  lnew.y);
				}
			}
		}
		break;
	case random_run:
		random_len--;
		/* fall through */
	case random_walk:
		if (color < colors-1)
		{
			putcolor(newcol, newrow, color + 1);
		}
		break;
	}
	return 1;
}

int lorenz_3d_orbit(long *l_x, long *l_y, long *l_z)
{
	s_l_xdt = multiply(*l_x, s_l_dt, bitshift);
	s_l_ydt = multiply(*l_y, s_l_dt, bitshift);
	s_l_dx  = -multiply(s_l_adt, *l_x, bitshift) + multiply(s_l_adt, *l_y, bitshift);
	s_l_dy  =  multiply(s_l_bdt, *l_x, bitshift) -s_l_ydt -multiply(*l_z, s_l_xdt, bitshift);
	s_l_dz  = -multiply(s_l_cdt, *l_z, bitshift) + multiply(*l_x, s_l_ydt, bitshift);

	*l_x += s_l_dx;
	*l_y += s_l_dy;
	*l_z += s_l_dz;
	return 0;
}

int lorenz_3d_orbit_fp(double *x, double *y, double *z)
{
	s_xdt = (*x)*s_dt;
	s_ydt = (*y)*s_dt;
	s_zdt = (*z)*s_dt;

	/* 2-lobe Lorenz (the original) */
	s_dx  = -s_adt*(*x) + s_adt*(*y);
	s_dy  =  s_bdt*(*x) - s_ydt - (*z)*s_xdt;
	s_dz  = -s_cdt*(*z) + (*x)*s_ydt;

	*x += s_dx;
	*y += s_dy;
	*z += s_dz;
	return 0;
}

int lorenz_3d1_orbit_fp(double *x, double *y, double *z)
{
	double norm;

	s_xdt = (*x)*s_dt;
	s_ydt = (*y)*s_dt;
	s_zdt = (*z)*s_dt;

	/* 1-lobe Lorenz */
	norm = sqrt((*x)*(*x) + (*y)*(*y));
	s_dx   = (-s_adt-s_dt)*(*x) + (s_adt-s_bdt)*(*y) + (s_dt-s_adt)*norm + s_ydt*(*z);
	s_dy   = (s_bdt-s_adt)*(*x) - (s_adt + s_dt)*(*y) + (s_bdt + s_adt)*norm - s_xdt*(*z) -
			norm*s_zdt;
	s_dz   = (s_ydt/2) - s_cdt*(*z);

	*x += s_dx;
	*y += s_dy;
	*z += s_dz;
	return 0;
}

int lorenz_3d3_orbit_fp(double *x, double *y, double *z)
{
	double norm;

	s_xdt = (*x)*s_dt;
	s_ydt = (*y)*s_dt;
	s_zdt = (*z)*s_dt;

	/* 3-lobe Lorenz */
	norm = sqrt((*x)*(*x) + (*y)*(*y));
	s_dx   = (-(s_adt + s_dt)*(*x) + (s_adt-s_bdt + s_zdt)*(*y)) / 3 +
			((s_dt-s_adt)*((*x)*(*x)-(*y)*(*y)) +
			2*(s_bdt + s_adt-s_zdt)*(*x)*(*y))/(3*norm);
	s_dy   = ((s_bdt-s_adt-s_zdt)*(*x) - (s_adt + s_dt)*(*y)) / 3 +
			(2*(s_adt-s_dt)*(*x)*(*y) +
			(s_bdt + s_adt-s_zdt)*((*x)*(*x)-(*y)*(*y)))/(3*norm);
	s_dz   = (3*s_xdt*(*x)*(*y)-s_ydt*(*y)*(*y))/2 - s_cdt*(*z);

	*x += s_dx;
	*y += s_dy;
	*z += s_dz;
	return 0;
}

int lorenz_3d4_orbit_fp(double *x, double *y, double *z)
{
	s_xdt = (*x)*s_dt;
	s_ydt = (*y)*s_dt;
	s_zdt = (*z)*s_dt;

	/* 4-lobe Lorenz */
	s_dx   = (-s_adt*(*x)*(*x)*(*x) + (2*s_adt + s_bdt-s_zdt)*(*x)*(*x)*(*y) +
			(s_adt-2*s_dt)*(*x)*(*y)*(*y) + (s_zdt-s_bdt)*(*y)*(*y)*(*y)) /
			(2*((*x)*(*x) + (*y)*(*y)));
	s_dy   = ((s_bdt-s_zdt)*(*x)*(*x)*(*x) + (s_adt-2*s_dt)*(*x)*(*x)*(*y) +
			(-2*s_adt-s_bdt + s_zdt)*(*x)*(*y)*(*y) - s_adt*(*y)*(*y)*(*y)) /
			(2*((*x)*(*x) + (*y)*(*y)));
	s_dz   = (2*s_xdt*(*x)*(*x)*(*y) - 2*s_xdt*(*y)*(*y)*(*y) - s_cdt*(*z));

	*x += s_dx;
	*y += s_dy;
	*z += s_dz;
	return 0;
}

int henon_orbit_fp(double *x, double *y, double *z)
{
	double newx, newy;
	*z = *x; /* for warning only */
	newx  = 1 + *y - s_a*(*x)*(*x);
	newy  = s_b*(*x);
	*x = newx;
	*y = newy;
	return 0;
}

int henon_orbit(long *l_x, long *l_y, long *l_z)
{
	long newx, newy;
	*l_z = *l_x; /* for warning only */
	newx = multiply(*l_x, *l_x, bitshift);
	newx = multiply(newx, s_l_a, bitshift);
	newx  = fudge + *l_y - newx;
	newy  = multiply(s_l_b, *l_x, bitshift);
	*l_x = newx;
	*l_y = newy;
	return 0;
}

int rossler_orbit_fp(double *x, double *y, double *z)
{
	s_xdt = (*x)*s_dt;
	s_ydt = (*y)*s_dt;

	s_dx = -s_ydt - (*z)*s_dt;
	s_dy = s_xdt + (*y)*s_adt;
	s_dz = s_bdt + (*z)*s_xdt - (*z)*s_cdt;

	*x += s_dx;
	*y += s_dy;
	*z += s_dz;
	return 0;
}

int pickover_orbit_fp(double *x, double *y, double *z)
{
	double newx, newy, newz;
	newx = sin(s_a*(*y)) - (*z)*cos(s_b*(*x));
	newy = (*z)*sin(s_c*(*x)) - cos(s_d*(*y));
	newz = sin(*x);
	*x = newx;
	*y = newy;
	*z = newz;
	return 0;
}

/* page 149 "Science of Fractal Images" */
int gingerbread_orbit_fp(double *x, double *y, double *z)
{
	double newx;
	*z = *x; /* for warning only */
	newx = 1 - (*y) + fabs(*x);
	*y = *x;
	*x = newx;
	return 0;
}

int rossler_orbit(long *l_x, long *l_y, long *l_z)
{
	s_l_xdt = multiply(*l_x, s_l_dt, bitshift);
	s_l_ydt = multiply(*l_y, s_l_dt, bitshift);

	s_l_dx  = -s_l_ydt - multiply(*l_z, s_l_dt, bitshift);
	s_l_dy  =  s_l_xdt + multiply(*l_y, s_l_adt, bitshift);
	s_l_dz  =  s_l_bdt + multiply(*l_z, s_l_xdt, bitshift)
						- multiply(*l_z, s_l_cdt, bitshift);

	*l_x += s_l_dx;
	*l_y += s_l_dy;
	*l_z += s_l_dz;

	return 0;
}

/* OSTEP  = Orbit Step (and inner orbit value) */
/* NTURNS = Outside Orbit */
/* TURN2  = Points per orbit */
/* a      = Angle */


int kam_torus_orbit_fp(double *r, double *s, double *z)
{
	double srr;
	if (s_t++ >= s_l_d)
	{
		s_orbit += s_b;
		(*r) = (*s) = s_orbit/3;
		s_t = 0;
		*z = s_orbit;
		if (s_orbit > s_c)
		{
			return 1;
		}
	}
	srr = (*s) - (*r)*(*r);
	(*s) = (*r)*sinx + srr*cosx;
	(*r) = (*r)*cosx - srr*sinx;
	return 0;
}

int kam_torus_orbit(long *r, long *s, long *z)
{
	long srr;
	if (s_t++ >= s_l_d)
	{
		s_l_orbit += s_l_b;
		(*r) = (*s) = s_l_orbit/3;
		s_t = 0;
		*z = s_l_orbit;
		if (s_l_orbit > s_l_c)
		{
			return 1;
		}
	}
	srr = (*s)-multiply((*r), (*r), bitshift);
	(*s) = multiply((*r), s_l_sinx, bitshift) + multiply(srr, s_l_cosx, bitshift);
	(*r) = multiply((*r), s_l_cosx, bitshift)-multiply(srr, s_l_sinx, bitshift);
	return 0;
}

int hopalong_2d_orbit_fp(double *x, double *y, double *z)
{
	double tmp;
	*z = *x; /* for warning only */
	tmp = *y - sign(*x)*sqrt(fabs(s_b*(*x)-s_c));
	*y = s_a - *x;
	*x = tmp;
	return 0;
}

/* common subexpressions for chip_2d and quadrup_two_2d */
static double log_fabs(double b, double c, double x)
{
	return log(fabs(b*x - c));
}

static double atan_sqr_log_fabs(double b, double c, double x)
{
	return atan(sqr(log(fabs(c*x - b))));
}

static double cos_sqr(double x)
{
	return cos(sqr(x));
}

static int orbit_aux(double (*fn)(double x), double *x, double *y, double *z)
{
	double tmp;
	*z = *x; /* for warning only */
	tmp = *y - sign(*x)*fn(log_fabs(s_b, s_c, *x))*atan_sqr_log_fabs(s_b, s_c, *x);
	*y = s_a - *x;
	*x = tmp;
	return 0;
}

/* from Michael Peters and HOP */
int chip_2d_orbit_fp(double *x, double *y, double *z)
{
	return orbit_aux(cos_sqr, x, y, z);
}

/* from Michael Peters and HOP */
int quadrup_two_2d_orbit_fp(double *x, double *y, double *z)
{
	return orbit_aux(sin, x, y, z);
}

/* from Michael Peters and HOP */
int three_ply_2d_orbit_fp(double *x, double *y, double *z)
{
	double tmp;
	*z = *x; /* for warning only */
	tmp = *y - sign(*x)*(fabs(sin(*x)*COSB + s_c-(*x)*SINABC));
	*y = s_a - *x;
	*x = tmp;
	return 0;
}

int martin_2d_orbit_fp(double *x, double *y, double *z)
{
	double tmp;
	*z = *x;  /* for warning only */
	tmp = *y - sin(*x);
	*y = s_a - *x;
	*x = tmp;
	return 0;
}

int mandel_cloud_orbit_fp(double *x, double *y, double *z)
{
	double newx, newy, x2, y2;
#ifndef XFRACT
	newx = *z; /* for warning only */
#endif
	x2 = (*x)*(*x);
	y2 = (*y)*(*y);
	if (x2 + y2 > 2)
	{
		return 1;
	}
	newx = x2-y2 + s_a;
	newy = 2*(*x)*(*y) + s_b;
	*x = newx;
	*y = newy;
	return 0;
}

int dynamic_orbit_fp(double *x, double *y, double *z)
{
	_CMPLX cp, tmp;
	double newx, newy;
	cp.x = s_b* *x;
	cp.y = 0;
	CMPLXtrig0(cp, tmp);
	newy = *y + s_dt*sin(*x + s_a*tmp.x);
	if (s_euler)
	{
		*y = newy;
	}

	cp.x = s_b* *y;
	cp.y = 0;
	CMPLXtrig0(cp, tmp);
	newx = *x - s_dt*sin(*y + s_a*tmp.x);
	*x = newx;
	*y = newy;
	return 0;
}

/* dmf */
#undef  LAMBDA
#define LAMBDA  param[0]
#define ALPHA   param[1]
#define BETA    param[2]
#define GAMMA   param[3]
#define OMEGA   param[4]
#define DEGREE  param[5]

int icon_orbit_fp(double *x, double *y, double *z)
{

	double oldx, oldy, zzbar, zreal, zimag, za, zb, zn, p;
	int i;

	oldx = *x;
	oldy = *y;

	zzbar = oldx*oldx + oldy*oldy;
	zreal = oldx;
	zimag = oldy;

	for (i = 1; i <= DEGREE-2; i++)
	{
		za = zreal*oldx - zimag*oldy;
		zb = zimag*oldx + zreal*oldy;
		zreal = za;
		zimag = zb;
	}
	zn = oldx*zreal - oldy*zimag;
	p = LAMBDA + ALPHA*zzbar + BETA*zn;
	*x = p*oldx + GAMMA*zreal - OMEGA*oldy;
	*y = p*oldy - GAMMA*zimag + OMEGA*oldx;

	*z = zzbar;
	return 0;
}
#ifdef LAMBDA  /* Tim says this will make me a "good citizen" */
#undef LAMBDA  /* Yeah, but you were bad, Dan - LAMBDA was already */
#undef ALPHA   /* defined! <grin!> TW */
#undef BETA
#undef GAMMA
#endif

/* hb */
#define PAR_A   param[0]
#define PAR_B   param[1]
#define PAR_C   param[2]
#define PAR_D   param[3]

int latoo_orbit_fp(double *x, double *y, double *z)
{

	double xold, yold, tmp;

	xold = *z; /* for warning only */

	xold = *x;
	yold = *y;

/*    *x = sin(yold*PAR_B) + PAR_C*sin(xold*PAR_B); */
	g_old_z.x = yold*PAR_B;
	g_old_z.y = 0;          /* old = (y*B) + 0i (in the complex)*/
	CMPLXtrig0(g_old_z, g_new_z);
	tmp = (double) g_new_z.x;
	g_old_z.x = xold*PAR_B;
	g_old_z.y = 0;          /* old = (x*B) + 0i */
	CMPLXtrig1(g_old_z, g_new_z);
	*x  = PAR_C*g_new_z.x + tmp;

/*    *y = sin(xold*PAR_A) + PAR_D*sin(yold*PAR_A); */
	g_old_z.x = xold*PAR_A;
	g_old_z.y = 0;          /* old = (y*A) + 0i (in the complex)*/
	CMPLXtrig2(g_old_z, g_new_z);
	tmp = (double) g_new_z.x;
	g_old_z.x = yold*PAR_A;
	g_old_z.y = 0;          /* old = (x*B) + 0i */
	CMPLXtrig3(g_old_z, g_new_z);
	*y  = PAR_D*g_new_z.x + tmp;

	return 0;
}

#undef PAR_A
#undef PAR_B
#undef PAR_C
#undef PAR_D

/**********************************************************************/
/*   Main fractal engines - put in fractalspecific[fractype].calctype */
/**********************************************************************/

int inverse_julia_per_image()
{
	int color = 0;

	if (resuming)            /* can't resume */
	{
		return -1;
	}

	while (color >= 0)       /* generate points */
	{
		if (check_key())
		{
			Free_Queue();
			return -1;
		}
		color = curfractalspecific->orbitcalc();
		g_old_z = g_new_z;
	}
	Free_Queue();
	return 0;
}

int orbit_2d_fp()
{
	FILE *fp;
	double *soundvar;
	double x, y, z;
	int color, col, row;
	int count;
	int oldrow, oldcol;
	double *p0, *p1, *p2;
	struct affine cvt;
	int ret;

	soundvar = p0 = p1 = p2 = NULL;

	fp = open_orbit_save();
	/* setup affine screen coord conversion */
	setup_convert_to_screen(&cvt);

	/* set up projection scheme */
	switch (s_projection)
	{
	case PROJECTION_ZX: p0 = &z; p1 = &x; p2 = &y; break;
	case PROJECTION_XZ: p0 = &x; p1 = &z; p2 = &y; break;
	case PROJECTION_XY: p0 = &x; p1 = &y; p2 = &z; break;
	}
	switch (soundflag & SOUNDFLAG_ORBITMASK)
	{
	case SOUNDFLAG_X: soundvar = &x; break;
	case SOUNDFLAG_Y: soundvar = &y; break;
	case SOUNDFLAG_Z: soundvar = &z; break;
	}

	color = (inside > 0) ? inside : 2;

	oldcol = oldrow = -1;
	x = s_init_orbit_fp[0];
	y = s_init_orbit_fp[1];
	z = s_init_orbit_fp[2];
	coloriter = 0L;
	count = ret = 0;
	g_max_count = (maxit > 0x1fffffL || g_max_count) ? 0x7fffffffL : maxit*1024L;

	if (resuming)
	{
		start_resume();
		get_resume(sizeof(count), &count, sizeof(color), &color,
			sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
			sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(s_t), &s_t,
			sizeof(s_orbit), &s_orbit, sizeof(coloriter), &coloriter,
			0);
		end_resume();
	}

	while (coloriter++ <= g_max_count) /* loop until keypress or maxit */
	{
		if (driver_key_pressed())
		{
			driver_mute();
			alloc_resume(100, 1);
			put_resume(sizeof(count), &count, sizeof(color), &color,
				sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
				sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(s_t), &s_t,
				sizeof(s_orbit), &s_orbit, sizeof(coloriter), &coloriter,
				0);
			ret = -1;
			break;
		}
		if (++count > 1000)
		{        /* time to switch colors? */
			count = 0;
			if (++color >= colors)   /* another color to switch to? */
			{
				color = 1;  /* (don't use the background color) */
			}
		}

		col = (int) (cvt.a*x + cvt.b*y + cvt.e);
		row = (int) (cvt.c*x + cvt.d*y + cvt.f);
		if (col >= 0 && col < xdots && row >= 0 && row < ydots)
		{
			if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
			{
				w_snd((int) (*soundvar*100 + basehertz));
			}
			if ((fractype != ICON) && (fractype != LATOO))
			{
				if (oldcol != -1 && s_connect)
				{
					driver_draw_line(col, row, oldcol, oldrow, color % colors);
				}
				else
				{
					(*plot)(col, row, color % colors);
				}
			}
			else
			{
				/* should this be using plot_hist()? */
				color = getcolor(col, row) + 1;
				if (color < colors) /* color sticks on last value */
				{
					(*plot)(col, row, color);
				}
			}

			oldcol = col;
			oldrow = row;
		}
		else if ((long) abs(row) + (long) abs(col) > BAD_PIXEL) /* sanity check */
		{
			return ret;
		}
		else
		{
			oldrow = oldcol = -1;
		}

		if (FORBIT(p0, p1, p2))
		{
			break;
		}
		if (fp)
		{
			fprintf(fp, "%g %g %g 15\n", *p0, *p1, 0.0);
		}
	}
	if (fp)
	{
		fclose(fp);
	}
	return ret;
}

int orbit_2d()
{
	FILE *fp;
	long *soundvar;
	long x, y, z;
	int color, col, row;
	int count;
	int oldrow, oldcol;
	long *p0, *p1, *p2;
	struct l_affine cvt;
	int ret, start;

	start = 1;
	soundvar = p0 = p1 = p2 = NULL;
	fp = open_orbit_save();

	/* setup affine screen coord conversion */
	l_setup_convert_to_screen(&cvt);

	/* set up projection scheme */
	switch (s_projection)
	{
	case PROJECTION_ZX: p0 = &z; p1 = &x; p2 = &y; break;
	case PROJECTION_XZ: p0 = &x; p1 = &z; p2 = &y; break;
	case PROJECTION_XY: p0 = &x; p1 = &y; p2 = &z; break;
	}
	switch (soundflag & SOUNDFLAG_ORBITMASK)
	{
	case SOUNDFLAG_X: soundvar = &x; break;
	case SOUNDFLAG_Y: soundvar = &y; break;
	case SOUNDFLAG_Z: soundvar = &z; break;
	}

	color = (inside > 0) ? inside : 2;
	if (color >= colors)
	{
		color = 1;
	}
	oldcol = oldrow = -1;
	x = s_init_orbit_long[0];
	y = s_init_orbit_long[1];
	z = s_init_orbit_long[2];
	count = ret = 0;
	g_max_count = (maxit > 0x1fffffL || g_max_count) ? 0x7fffffffL : maxit*1024L;
	coloriter = 0L;

	if (resuming)
	{
		start_resume();
		get_resume(sizeof(count), &count, sizeof(color), &color,
			sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
			sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(s_t), &s_t,
			sizeof(s_l_orbit), &s_l_orbit, sizeof(coloriter), &coloriter,
			0);
		end_resume();
	}

	while (coloriter++ <= g_max_count) /* loop until keypress or maxit */
	{
		if (driver_key_pressed())
		{
			driver_mute();
			alloc_resume(100, 1);
			put_resume(sizeof(count), &count, sizeof(color), &color,
				sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
				sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(s_t), &s_t,
				sizeof(s_l_orbit), &s_l_orbit, sizeof(coloriter), &coloriter,
				0);
			ret = -1;
			break;
		}
		if (++count > 1000)
		{        /* time to switch colors? */
			count = 0;
			if (++color >= colors)   /* another color to switch to? */
			{
				color = 1;  /* (don't use the background color) */
			}
		}

		col = (int) ((multiply(cvt.a, x, bitshift) + multiply(cvt.b, y, bitshift) + cvt.e) >> bitshift);
		row = (int) ((multiply(cvt.c, x, bitshift) + multiply(cvt.d, y, bitshift) + cvt.f) >> bitshift);
		if (overflow)
		{
			overflow = 0;
			return ret;
		}
		if (col >= 0 && col < xdots && row >= 0 && row < ydots)
		{
			if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
			{
				double yy;
				yy = *soundvar;
				yy = yy/fudge;
				w_snd((int) (yy*100 + basehertz));
			}
			if (oldcol != -1 && s_connect)
			{
				driver_draw_line(col, row, oldcol, oldrow, color % colors);
			}
			else if (!start)
			{
				(*plot)(col, row, color % colors);
			}
			oldcol = col;
			oldrow = row;
			start = 0;
		}
		else if ((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
		{
			return ret;
		}
		else
		{
			oldrow = oldcol = -1;
		}

		/* Calculate the next point */
		if (LORBIT(p0, p1, p2))
		{
			break;
		}
		if (fp)
		{
			fprintf(fp, "%g %g %g 15\n", (double) *p0/fudge, (double) *p1/fudge, 0.0);
		}
	}
	if (fp)
	{
		fclose(fp);
	}
	return ret;
}

static int orbit_3d_calc(void)
{
	FILE *fp;
	unsigned long count;
	int oldcol, oldrow;
	int oldcol1, oldrow1;
	struct threed_vt_inf inf;
	int color;
	int ret;

	/* setup affine screen coord conversion */
	l_setup_convert_to_screen(&inf.cvt);

	oldcol1 = oldrow1 = oldcol = oldrow = -1;
	color = 2;
	if (color >= colors)
	{
		color = 1;
	}

	inf.orbit[0] = s_init_orbit_long[0];
	inf.orbit[1] = s_init_orbit_long[1];
	inf.orbit[2] = s_init_orbit_long[2];

	if (driver_diskp())                /* this would KILL a disk drive! */
	{
		notdiskmsg();
	}

	fp = open_orbit_save();

	count = ret = 0;
	g_max_count = (maxit > 0x1fffffL || g_max_count) ? 0x7fffffffL : maxit*1024L;
	coloriter = 0L;
	while (coloriter++ <= g_max_count) /* loop until keypress or maxit */
	{
		/* calc goes here */
		if (++count > 1000)
		{        /* time to switch colors? */
			count = 0;
			if (++color >= colors)   /* another color to switch to? */
			{
				color = 1;        /* (don't use the background color) */
			}
		}
		if (driver_key_pressed())
		{
			driver_mute();
			ret = -1;
			break;
		}

		LORBIT(&inf.orbit[0], &inf.orbit[1], &inf.orbit[2]);
		if (fp)
		{
			fprintf(fp, "%g %g %g 15\n", (double)inf.orbit[0]/fudge, (double)inf.orbit[1]/fudge, (double)inf.orbit[2]/fudge);
		}
		if (threed_view_trans(&inf))
		{
			/* plot if inside window */
			if (inf.col >= 0)
			{
				if (s_real_time)
				{
					g_which_image = WHICHIMAGE_RED;
				}
				if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
				{
					double yy;
					yy = inf.viewvect[((soundflag & SOUNDFLAG_ORBITMASK) - SOUNDFLAG_X)];
					yy = yy/fudge;
					w_snd((int) (yy*100 + basehertz));
				}
				if (oldcol != -1 && s_connect)
				{
					driver_draw_line(inf.col, inf.row, oldcol, oldrow, color % colors);
				}
				else
				{
					(*plot)(inf.col, inf.row, color % colors);
				}
			}
			else if (inf.col == -2)
			{
				return ret;
			}
			oldcol = inf.col;
			oldrow = inf.row;
			if (s_real_time)
			{
				g_which_image = WHICHIMAGE_BLUE;
				/* plot if inside window */
				if (inf.col1 >= 0)
				{
					if (oldcol1 != -1 && s_connect)
					{
						driver_draw_line(inf.col1, inf.row1, oldcol1, oldrow1, color % colors);
					}
					else
					{
						(*plot)(inf.col1, inf.row1, color % colors);
					}
				}
				else if (inf.col1 == -2)
				{
					return ret;
				}
				oldcol1 = inf.col1;
				oldrow1 = inf.row1;
			}
		}
	}
	if (fp)
	{
		fclose(fp);
	}
	return ret;
}


static int orbit_3d_calc_fp(void)
{
	FILE *fp;
	unsigned long count;
	int oldcol, oldrow;
	int oldcol1, oldrow1;
	int color;
	int ret;
	struct threed_vt_inf_fp inf;

	/* setup affine screen coord conversion */
	setup_convert_to_screen(&inf.cvt);

	oldcol = oldrow = -1;
	oldcol1 = oldrow1 = -1;
	color = 2;
	if (color >= colors)
	{
		color = 1;
	}
	inf.orbit[0] = s_init_orbit_fp[0];
	inf.orbit[1] = s_init_orbit_fp[1];
	inf.orbit[2] = s_init_orbit_fp[2];

	if (driver_diskp())                /* this would KILL a disk drive! */
	{
		notdiskmsg();
	}

	fp = open_orbit_save();

	ret = 0;
	g_max_count = (maxit > 0x1fffffL || g_max_count) ? 0x7fffffffL : maxit*1024L;
	count = coloriter = 0L;
	while (coloriter++ <= g_max_count) /* loop until keypress or maxit */
	{
		/* calc goes here */
		if (++count > 1000)
		{        /* time to switch colors? */
			count = 0;
			if (++color >= colors)   /* another color to switch to? */
			{
				color = 1;        /* (don't use the background color) */
			}
		}

		if (driver_key_pressed())
		{
			driver_mute();
			ret = -1;
			break;
		}

		FORBIT(&inf.orbit[0], &inf.orbit[1], &inf.orbit[2]);
		if (fp)
		{
			fprintf(fp, "%g %g %g 15\n", inf.orbit[0], inf.orbit[1], inf.orbit[2]);
		}
		if (threed_view_trans_fp(&inf))
		{
			/* plot if inside window */
			if (inf.col >= 0)
			{
				if (s_real_time)
				{
					g_which_image = WHICHIMAGE_RED;
				}
				if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
				{
					w_snd((int) (inf.viewvect[((soundflag & SOUNDFLAG_ORBITMASK) - SOUNDFLAG_X)]*100 + basehertz));
				}
				if (oldcol != -1 && s_connect)
				{
					driver_draw_line(inf.col, inf.row, oldcol, oldrow, color % colors);
				}
				else
				{
					(*plot)(inf.col, inf.row, color % colors);
				}
			}
			else if (inf.col == -2)
			{
				return ret;
			}
			oldcol = inf.col;
			oldrow = inf.row;
			if (s_real_time)
			{
				g_which_image = WHICHIMAGE_BLUE;
				/* plot if inside window */
				if (inf.col1 >= 0)
				{
					if (oldcol1 != -1 && s_connect)
					{
						driver_draw_line(inf.col1, inf.row1, oldcol1, oldrow1, color % colors);
					}
					else
					{
						(*plot)(inf.col1, inf.row1, color % colors);
					}
				}
				else if (inf.col1 == -2)
				{
					return ret;
				}
				oldcol1 = inf.col1;
				oldrow1 = inf.row1;
			}
		}
	}
	if (fp)
	{
		fclose(fp);
	}
	return ret;
}

int dynamic_2d_setup_fp()
{
	s_connect = 0;
	s_euler = 0;
	s_d = param[0]; /* number of intervals */
	if (s_d < 0)
	{
		s_d = -s_d;
		s_connect = 1;
	}
	else if (s_d == 0)
	{
		s_d = 1;
	}
	if (fractype == DYNAMICFP)
	{
		s_a = param[2]; /* parameter */
		s_b = param[3]; /* parameter */
		s_dt = param[1]; /* step size */
		if (s_dt < 0)
		{
			s_dt = -s_dt;
			s_euler = 1;
		}
		if (s_dt == 0)
		{
			s_dt = 0.01;
		}
	}
	if (outside == SUM)
	{
		plot = plot_hist;
	}
	return 1;
}

/*
 * This is the routine called to perform a time-discrete dynamical
 * system image.
 * The starting positions are taken by stepping across the image in steps
 * of parameter1 pixels.  maxit differential equation steps are taken, with
 * a step size of parameter2.
 */
int dynamic_2d_fp()
{
	FILE *fp;
	double *soundvar = NULL;
	double x, y, z;
	int color, col, row;
	long count;
	int oldrow, oldcol;
	double *p0, *p1;
	struct affine cvt;
	int ret;
	int xstep, ystep; /* The starting position step number */
	double xpixel, ypixel; /* Our pixel position on the screen */

	fp = open_orbit_save();
	/* setup affine screen coord conversion */
	setup_convert_to_screen(&cvt);

	p0 = &x;
	p1 = &y;


	if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)
	{
		soundvar = &x;
	}
	else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)
	{
		soundvar = &y;
	}
	else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)
	{
		soundvar = &z;
	}

	count = 0;
	color = (inside > 0) ? inside : 1;
	if (color >= colors)
	{
		color = 1;
	}
	oldcol = oldrow = -1;

	xstep = -1;
	ystep = 0;

	if (resuming)
	{
		start_resume();
		get_resume(sizeof(count), &count, sizeof(color), &color,
					sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
					sizeof(x), &x, sizeof(y), &y, sizeof(xstep), &xstep,
					sizeof(ystep), &ystep, 0);
		end_resume();
	}

	ret = 0;
	while (1)
	{
		if (driver_key_pressed())
		{
			driver_mute();
			alloc_resume(100, 1);
			put_resume(sizeof(count), &count, sizeof(color), &color,
							sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
							sizeof(x), &x, sizeof(y), &y, sizeof(xstep), &xstep,
							sizeof(ystep), &ystep, 0);
			ret = -1;
			break;
		}

		xstep ++;
		if (xstep >= s_d)
		{
			xstep = 0;
			ystep ++;
			if (ystep > s_d)
			{
				driver_mute();
				ret = -1;
				break;
			}
		}

		xpixel = dxsize*(xstep + .5)/s_d;
		ypixel = dysize*(ystep + .5)/s_d;
		x = (double) ((xxmin + delxx*xpixel) + (delxx2*ypixel));
		y = (double) ((yymax-delyy*ypixel) + (-delyy2*xpixel));
		if (fractype == MANDELCLOUD)
		{
			s_a = x;
			s_b = y;
		}
		oldcol = -1;

		if (++color >= colors)   /* another color to switch to? */
		{
			color = 1;    /* (don't use the background color) */
		}

		for (count = 0; count < maxit; count++)
		{
			if (count % 2048L == 0)
			{
				if (driver_key_pressed())
				{
					break;
				}
			}

			col = (int) (cvt.a*x + cvt.b*y + cvt.e);
			row = (int) (cvt.c*x + cvt.d*y + cvt.f);
			if (col >= 0 && col < xdots && row >= 0 && row < ydots)
			{
				if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
				{
					w_snd((int) (*soundvar*100 + basehertz));
				}

				if (count >= orbit_delay)
				{
					if (oldcol != -1 && s_connect)
					{
						driver_draw_line(col, row, oldcol, oldrow, color % colors);
					}
					else if (count > 0 || fractype != MANDELCLOUD)
					{
						(*plot)(col, row, color % colors);
					}
				}
				oldcol = col;
				oldrow = row;
			}
			else if ((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
			{
				return ret;
			}
			else
			{
				oldrow = oldcol = -1;
			}

			if (FORBIT(p0, p1, NULL))
			{
				break;
			}
			if (fp)
			{
				fprintf(fp, "%g %g %g 15\n", *p0, *p1, 0.0);
			}
		}
	}
	if (fp)
	{
		fclose(fp);
	}
	return ret;
}

int setup_orbits_to_screen(struct affine *scrn_cnvt)
{
	double det, xd, yd;

	det = (g_orbit_x_3rd-g_orbit_x_min)*(g_orbit_y_min-g_orbit_y_max) + (g_orbit_y_max-g_orbit_y_3rd)*(g_orbit_x_max-g_orbit_x_min);
	if (det == 0)
	{
		return -1;
	}
	xd = dxsize/det;
	scrn_cnvt->a =  xd*(g_orbit_y_max-g_orbit_y_3rd);
	scrn_cnvt->b =  xd*(g_orbit_x_3rd-g_orbit_x_min);
	scrn_cnvt->e = -scrn_cnvt->a*g_orbit_x_min - scrn_cnvt->b*g_orbit_y_max;

	det = (g_orbit_x_3rd-g_orbit_x_max)*(g_orbit_y_min-g_orbit_y_max) + (g_orbit_y_min-g_orbit_y_3rd)*(g_orbit_x_max-g_orbit_x_min);
	if (det == 0)
	{
		return -1;
	}
	yd = dysize/det;
	scrn_cnvt->c =  yd*(g_orbit_y_min-g_orbit_y_3rd);
	scrn_cnvt->d =  yd*(g_orbit_x_3rd-g_orbit_x_max);
	scrn_cnvt->f = -scrn_cnvt->c*g_orbit_x_min - scrn_cnvt->d*g_orbit_y_max;
	return 0;
}

int plotorbits2dsetup(void)
{

#ifndef XFRACT
	if (curfractalspecific->isinteger != 0)
	{
		int tofloat = curfractalspecific->tofloat;
		if (tofloat == NOFRACTAL)
		{
			return -1;
		}
		floatflag = usr_floatflag = 1; /* force floating point */
		fractype = tofloat;
		curfractalspecific = &fractalspecific[fractype];
	}
#endif

	PER_IMAGE();

	/* setup affine screen coord conversion */
	if (g_keep_screen_coords)
	{
		if (setup_orbits_to_screen(&s_o_cvt))
		{
			return -1;
		}
	}
	else if (setup_convert_to_screen(&s_o_cvt))
	{
		return -1;
	}

	/* set so truncation to int rounds to nearest */
	s_o_cvt.e += 0.5;
	s_o_cvt.f += 0.5;

	if (orbit_delay >= maxit) /* make sure we get an image */
	{
		orbit_delay = (int) (maxit - 1);
	}

	s_o_color = 1;

	if (outside == SUM)
	{
		plot = plot_hist;
	}
	return 1;
}

int plotorbits2dfloat(void)
{
	double *soundvar = NULL;
	double x, y, z;
	int col, row;
	long count;

	if (driver_key_pressed())
	{
		driver_mute();
		alloc_resume(100, 1);
		put_resume(sizeof(s_o_color), &s_o_color, 0);
		return -1;
	}

#if 0
	col = (int) (s_o_cvt.a*g_new_z.x + s_o_cvt.b*g_new_z.y + s_o_cvt.e);
	row = (int) (s_o_cvt.c*g_new_z.x + s_o_cvt.d*g_new_z.y + s_o_cvt.f);
	if (col >= 0 && col < xdots && row >= 0 && row < ydots)
	{
		(*plot)(col, row, 1);
	}
	return 0;
#endif

	if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)
	{
		soundvar = &x;
	}
	else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)
	{
		soundvar = &y;
	}
	else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)
	{
		soundvar = &z;
	}

	if (resuming)
	{
		start_resume();
		get_resume(sizeof(s_o_color), &s_o_color, 0);
		end_resume();
	}

	if (inside > 0)
	{
		s_o_color = inside;
	}
	else  /* inside <= 0 */
	{
		s_o_color++;
		if (s_o_color >= colors) /* another color to switch to? */
		{
			s_o_color = 1;    /* (don't use the background color) */
		}
	}

	PER_PIXEL(); /* initialize the calculations */

	for (count = 0; count < maxit; count++)
	{
		if (ORBITCALC() == 1 && periodicitycheck)
		{
			continue;  /* bailed out, don't plot */
		}

		if (count < orbit_delay || count % g_orbit_interval)
		{
			continue;  /* don't plot it */
		}

		/* else count >= orbit_delay and we want to plot it */
		col = (int) (s_o_cvt.a*g_new_z.x + s_o_cvt.b*g_new_z.y + s_o_cvt.e);
		row = (int) (s_o_cvt.c*g_new_z.x + s_o_cvt.d*g_new_z.y + s_o_cvt.f);
#ifdef XFRACT
		if (col >= 0 && col < xdots && row >= 0 && row < ydots)
#else
		/* don't know why the next line is necessary, the one above should work */
		if (col > 0 && col < xdots && row > 0 && row < ydots)
#endif
		{             /* plot if on the screen */
			if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
			{
				w_snd((int) (*soundvar*100 + basehertz));
			}

			(*plot)(col, row, s_o_color % colors);
		}
		else
		{             /* off screen, don't continue unless periodicity=0 */
			if (periodicitycheck)
			{
				return 0; /* skip to next pixel */
			}
		}
	}
	return 0;
}

/* this function's only purpose is to manage funnyglasses related */
/* stuff so the code is not duplicated for ifs_3d() and lorenz3d() */
int funny_glasses_call(int (*calc)(void))
{
	int status;
	status = 0;
	g_which_image = g_glasses_type ? WHICHIMAGE_RED : WHICHIMAGE_NONE;
	plot_setup();
	assert(g_standard_plot);
	plot = g_standard_plot;
	status = calc();
	if (s_real_time && g_glasses_type < STEREO_PHOTO)
	{
		s_real_time = 0;
		goto done;
	}
	if (g_glasses_type && status == 0 && display3d)
	{
		if (g_glasses_type == STEREO_PHOTO)   /* photographer's mode */
		{
				int i;
				stopmsg(STOPMSG_INFO_ONLY,
				"First image (left eye) is ready.  Hit any key to see it, \n"
				"then hit <s> to save, hit any other key to create second image.");
				for (i = driver_get_key(); i == 's' || i == 'S'; i = driver_get_key())
				{
					savetodisk(savename);
				}
				/* is there a better way to clear the screen in graphics mode? */
				driver_set_video_mode(&g_video_entry);
		}
		g_which_image = WHICHIMAGE_BLUE;
		if (curfractalspecific->flags & INFCALC)
		{
			curfractalspecific->per_image(); /* reset for 2nd image */
		}
		plot_setup();
		assert(g_standard_plot);
		plot = g_standard_plot;
		/* is there a better way to clear the graphics screen ? */
		status = calc();
		if (status != 0)
		{
			goto done;
		}
		if (g_glasses_type == STEREO_PHOTO) /* photographer's mode */
		{
				stopmsg(STOPMSG_INFO_ONLY, "Second image (right eye) is ready");
		}
	}
done:
	if (g_glasses_type == STEREO_PAIR && sxdots >= 2*xdots)
	{
		/* turn off view windows so will save properly */
		sxoffs = syoffs = 0;
		xdots = sxdots;
		ydots = sydots;
		viewwindow = 0;
	}
	return status;
}

/* double version - mainly for testing */
static int ifs_3d_float(void)
{
	int color_method;
	FILE *fp;
	int color;

	double newx, newy, newz, r, sum;

	int k;
	int ret;

	struct threed_vt_inf_fp inf;

	float *ffptr;

	/* setup affine screen coord conversion */
	setup_convert_to_screen(&inf.cvt);
	srand(1);
	color_method = (int) param[0];
	if (driver_diskp())                /* this would KILL a disk drive! */
	{
		notdiskmsg();
	}

	inf.orbit[0] = 0;
	inf.orbit[1] = 0;
	inf.orbit[2] = 0;

	fp = open_orbit_save();

	ret = 0;
	g_max_count = (maxit > 0x1fffffL) ? 0x7fffffffL : maxit*1024;

	coloriter = 0L;
	while (coloriter++ <= g_max_count) /* loop until keypress or maxit */
	{
		if (driver_key_pressed())  /* keypress bails out */
		{
			ret = -1;
			break;
		}
		r = rand();      /* generate a random number between 0 and 1 */
		r /= RAND_MAX;

		/* pick which iterated function to execute, weighted by probability */
		sum = ifs_defn[12]; /* [0][12] */
		k = 0;
		while (sum < r && ++k < numaffine*IFS3DPARM)
		{
			sum += ifs_defn[k*IFS3DPARM + 12];
			if (ifs_defn[(k + 1)*IFS3DPARM + 12] == 0) /* for safety  */
			{
				break;
			}
		}

		/* calculate image of last point under selected iterated function */
		ffptr = ifs_defn + k*IFS3DPARM; /* point to first parm in row */
		newx = *ffptr*inf.orbit[0] +
				*(ffptr + 1)*inf.orbit[1] +
				*(ffptr + 2)*inf.orbit[2] + *(ffptr + 9);
		newy = *(ffptr + 3)*inf.orbit[0] +
				*(ffptr + 4)*inf.orbit[1] +
				*(ffptr + 5)*inf.orbit[2] + *(ffptr + 10);
		newz = *(ffptr + 6)*inf.orbit[0] +
				*(ffptr + 7)*inf.orbit[1] +
				*(ffptr + 8)*inf.orbit[2] + *(ffptr + 11);

		inf.orbit[0] = newx;
		inf.orbit[1] = newy;
		inf.orbit[2] = newz;
		if (fp)
		{
			fprintf(fp, "%g %g %g 15\n", newx, newy, newz);
		}
		if (threed_view_trans_fp(&inf))
		{
			/* plot if inside window */
			if (inf.col >= 0)
			{
				if (s_real_time)
				{
					g_which_image = WHICHIMAGE_RED;
				}
				if (color_method)
				{
					color = (k % colors) + 1;
				}
				else
				{
					color = getcolor(inf.col, inf.row) + 1;
				}
				if (color < colors) /* color sticks on last value */
				{
					(*plot)(inf.col, inf.row, color);
				}
			}
			else if (inf.col == -2)
			{
				return ret;
			}
			if (s_real_time)
			{
				g_which_image = WHICHIMAGE_BLUE;
				/* plot if inside window */
				if (inf.col1 >= 0)
				{
					if (color_method)
					{
						color = (k % colors) + 1;
					}
					else
					{
						color = getcolor(inf.col1, inf.row1) + 1;
					}
					if (color < colors) /* color sticks on last value */
					{
						(*plot)(inf.col1, inf.row1, color);
					}
				}
				else if (inf.col1 == -2)
				{
					return ret;
				}
			}
		}
	} /* end while */
	if (fp)
	{
		fclose(fp);
	}
	return ret;
}

int ifs()                       /* front-end for ifs_2d and ifs_3d */
{
	if (ifs_defn == NULL && ifsload() < 0)
	{
		return -1;
	}
	if (driver_diskp())                /* this would KILL a disk drive! */
	{
		notdiskmsg();
	}
	return (ifs_type == 0) ? ifs_2d() : ifs_3d();
}


/* IFS logic shamelessly converted to integer math */
static int ifs_2d(void)
{
	int color_method;
	FILE *fp;
	int col;
	int row;
	int color;
	int ret;
	long *localifs;
	long *lfptr;
	long x, y, newx, newy, r, sum, tempr;

	int i, j, k;
	struct l_affine cvt;
	/* setup affine screen coord conversion */
	l_setup_convert_to_screen(&cvt);

	srand(1);
	color_method = (int) param[0];
	localifs = (long *) malloc(numaffine*IFSPARM*sizeof(long));
	if (localifs == NULL)
	{
		stopmsg(0, insufficient_ifs_mem);
		return -1;
	}

	for (i = 0; i < numaffine; i++)    /* fill in the local IFS array */
	{
		for (j = 0; j < IFSPARM; j++)
		{
			localifs[i*IFSPARM + j] = (long) (ifs_defn[i*IFSPARM + j]*fudge);
		}
	}

	tempr = fudge / 32767;        /* find the proper rand() fudge */

	fp = open_orbit_save();

	x = y = 0;
	ret = 0;
	g_max_count = (maxit > 0x1fffffL) ? 0x7fffffffL : maxit*1024L;
	coloriter = 0L;
	while (coloriter++ <= g_max_count) /* loop until keypress or maxit */
	{
		if (driver_key_pressed())  /* keypress bails out */
		{
			ret = -1;
			break;
		}
		r = rand15();      /* generate fudged random number between 0 and 1 */
		r *= tempr;

		/* pick which iterated function to execute, weighted by probability */
		sum = localifs[6];  /* [0][6] */
		k = 0;
		while (sum < r && k < numaffine-1) /* fixed bug of error if sum < 1 */
		{
			sum += localifs[++k*IFSPARM + 6];
		}
		/* calculate image of last point under selected iterated function */
		lfptr = localifs + k*IFSPARM; /* point to first parm in row */
		newx = multiply(lfptr[0], x, bitshift) +
				multiply(lfptr[1], y, bitshift) + lfptr[4];
		newy = multiply(lfptr[2], x, bitshift) +
				multiply(lfptr[3], y, bitshift) + lfptr[5];
		x = newx;
		y = newy;
		if (fp)
		{
			fprintf(fp, "%g %g %g 15\n", (double)newx/fudge, (double)newy/fudge, 0.0);
		}

		/* plot if inside window */
		col = (int) ((multiply(cvt.a, x, bitshift) + multiply(cvt.b, y, bitshift) + cvt.e) >> bitshift);
		row = (int) ((multiply(cvt.c, x, bitshift) + multiply(cvt.d, y, bitshift) + cvt.f) >> bitshift);
		if (col >= 0 && col < xdots && row >= 0 && row < ydots)
		{
			/* color is count of hits on this pixel */
			if (color_method)
			{
				color = (k % colors) + 1;
			}
			else
			{
				color = getcolor(col, row) + 1;
			}
			if (color < colors) /* color sticks on last value */
			{
				(*plot)(col, row, color);
			}
		}
		else if ((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
		{
				return ret;
		}
	}
	if (fp)
	{
		fclose(fp);
	}
	free(localifs);
	return ret;
}

static int ifs_3d_long(void)
{
	int color_method;
	FILE *fp;
	int color;
	int ret;

	long *localifs;
	long *lfptr;
	long newx, newy, newz, r, sum, tempr;

	int i, j, k;

	struct threed_vt_inf inf;
	srand(1);
	color_method = (int) param[0];
	localifs = (long *) malloc(numaffine*IFS3DPARM*sizeof(long));
	if (localifs == NULL)
	{
		stopmsg(0, insufficient_ifs_mem);
		return -1;
	}

	/* setup affine screen coord conversion */
	l_setup_convert_to_screen(&inf.cvt);

	for (i = 0; i < numaffine; i++)    /* fill in the local IFS array */
	{
		for (j = 0; j < IFS3DPARM; j++)
		{
			localifs[i*IFS3DPARM + j] = (long) (ifs_defn[i*IFS3DPARM + j]*fudge);
		}
	}

	tempr = fudge / 32767;        /* find the proper rand() fudge */

	inf.orbit[0] = 0;
	inf.orbit[1] = 0;
	inf.orbit[2] = 0;

	fp = open_orbit_save();

	ret = 0;
	g_max_count = (maxit > 0x1fffffL) ? 0x7fffffffL : maxit*1024L;
	coloriter = 0L;
	while (coloriter++ <= g_max_count) /* loop until keypress or maxit */
	{
		if (driver_key_pressed())  /* keypress bails out */
		{
			ret = -1;
			break;
		}
		r = rand15();      /* generate fudged random number between 0 and 1 */
		r *= tempr;

		/* pick which iterated function to execute, weighted by probability */
		sum = localifs[12];  /* [0][12] */
		k = 0;
		while (sum < r && ++k < numaffine*IFS3DPARM)
		{
			sum += localifs[k*IFS3DPARM + 12];
			if (ifs_defn[(k + 1)*IFS3DPARM + 12] == 0) /* for safety  */
			{
				break;
			}
		}

		/* calculate image of last point under selected iterated function */
		lfptr = localifs + k*IFS3DPARM; /* point to first parm in row */

		/* calculate image of last point under selected iterated function */
		newx = multiply(lfptr[0], inf.orbit[0], bitshift) +
				multiply(lfptr[1], inf.orbit[1], bitshift) +
				multiply(lfptr[2], inf.orbit[2], bitshift) + lfptr[9];
		newy = multiply(lfptr[3], inf.orbit[0], bitshift) +
				multiply(lfptr[4], inf.orbit[1], bitshift) +
				multiply(lfptr[5], inf.orbit[2], bitshift) + lfptr[10];
		newz = multiply(lfptr[6], inf.orbit[0], bitshift) +
				multiply(lfptr[7], inf.orbit[1], bitshift) +
				multiply(lfptr[8], inf.orbit[2], bitshift) + lfptr[11];

		inf.orbit[0] = newx;
		inf.orbit[1] = newy;
		inf.orbit[2] = newz;
		if (fp)
		{
			fprintf(fp, "%g %g %g 15\n", (double)newx/fudge, (double)newy/fudge, (double)newz/fudge);
		}

		if (threed_view_trans(&inf))
		{
			if ((long)abs(inf.row) + (long)abs(inf.col) > BAD_PIXEL) /* sanity check */
			{
				return ret;
			}
			/* plot if inside window */
			if (inf.col >= 0)
			{
				if (s_real_time)
				{
					g_which_image = WHICHIMAGE_RED;
				}
				if (color_method)
				{
					color = (k % colors) + 1;
				}
				else
				{
					color = getcolor(inf.col, inf.row) + 1;
				}
				if (color < colors) /* color sticks on last value */
				{
					(*plot)(inf.col, inf.row, color);
				}
			}
			if (s_real_time)
			{
				g_which_image = WHICHIMAGE_BLUE;
				/* plot if inside window */
				if (inf.col1 >= 0)
				{
					if (color_method)
					{
						color = (k % colors) + 1;
					}
					else
					{
						color = getcolor(inf.col1, inf.row1) + 1;
					}
					if (color < colors) /* color sticks on last value */
					{
						(*plot)(inf.col1, inf.row1, color);
					}
				}
			}
		}
	}
	if (fp)
	{
		fclose(fp);
	}
	free(localifs);
	return ret;
}

static void setup_matrix(MATRIX doublemat)
{
	/* build transformation matrix */
	identity (doublemat);

	/* apply rotations - uses the same rotation variables as line3d.c */
	xrot ((double)XROT / 57.29577, doublemat);
	yrot ((double)YROT / 57.29577, doublemat);
	zrot ((double)ZROT / 57.29577, doublemat);

	/* apply scale */
/*   scale((double)XSCALE/100.0, (double)YSCALE/100.0, (double)ROUGH/100.0, doublemat); */

}

int orbit_3d_fp()
{
	display3d = -1;
	s_real_time = (STEREO_NONE < g_glasses_type && g_glasses_type < STEREO_PHOTO) ? 1 : 0;
	return funny_glasses_call(orbit_3d_calc_fp);
}

int orbit_3d()
{
	display3d = -1;
	s_real_time = (STEREO_NONE < g_glasses_type && g_glasses_type < STEREO_PHOTO) ? 1 : 0;
	return funny_glasses_call(orbit_3d_calc);
}

static int ifs_3d(void)
{
	display3d = -1;

	s_real_time = (STEREO_NONE < g_glasses_type && g_glasses_type < STEREO_PHOTO) ? 1 : 0;
	return funny_glasses_call(floatflag ? ifs_3d_float : ifs_3d_long); /* double, long version of ifs_3d */
}

static int threed_view_trans(struct threed_vt_inf *inf)
{
	int i, j;
	double tmpx, tmpy, tmpz;
	long tmp;

	if (coloriter == 1)  /* initialize on first call */
	{
		for (i = 0; i < 3; i++)
		{
			inf->minvals[i] =  1L << 30;
			inf->maxvals[i] = -inf->minvals[i];
		}
		setup_matrix(inf->doublemat);
		if (s_real_time)
		{
			setup_matrix(inf->doublemat1);
		}
		/* copy xform matrix to long for for fixed point math */
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 4; j++)
			{
				inf->longmat[i][j] = (long) (inf->doublemat[i][j]*fudge);
				if (s_real_time)
				{
					inf->longmat1[i][j] = (long) (inf->doublemat1[i][j]*fudge);
				}
			}
		}
	}

	/* 3D VIEWING TRANSFORM */
	longvmult(inf->orbit, inf->longmat, inf->viewvect, bitshift);
	if (s_real_time)
	{
		longvmult(inf->orbit, inf->longmat1, inf->viewvect1, bitshift);
	}

	if (coloriter <= s_waste) /* waste this many points to find minz and maxz */
	{
		/* find minz and maxz */
		for (i = 0; i < 3; i++)
		{
			tmp = inf->viewvect[i];
			if (tmp < inf->minvals[i])
			{
				inf->minvals[i] = tmp;
			}
			else if (tmp > inf->maxvals[i])
			{
				inf->maxvals[i] = tmp;
			}
		}
		if (coloriter == s_waste) /* time to work it out */
		{
			inf->iview[0] = inf->iview[1] = 0L; /* center viewer on origin */

			/* z value of user's eye - should be more negative than extreme
								negative part of image */
			inf->iview[2] = (long) ((inf->minvals[2]-inf->maxvals[2])*(double)ZVIEWER/100.0);

			/* center image on origin */
			tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*fudge); /* center x */
			tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*fudge); /* center y */

			/* apply perspective shift */
			tmpx += ((double)g_x_shift*(xxmax-xxmin))/(xdots);
			tmpy += ((double)g_y_shift*(yymax-yymin))/(ydots);
			tmpz = -((double)inf->maxvals[2]) / fudge;
			trans(tmpx, tmpy, tmpz, inf->doublemat);

			if (s_real_time)
			{
				/* center image on origin */
				tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*fudge); /* center x */
				tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*fudge); /* center y */

				tmpx += ((double)g_x_shift1*(xxmax-xxmin))/(xdots);
				tmpy += ((double)g_y_shift1*(yymax-yymin))/(ydots);
				tmpz = -((double)inf->maxvals[2]) / fudge;
				trans(tmpx, tmpy, tmpz, inf->doublemat1);
			}
			for (i = 0; i < 3; i++)
			{
				g_view[i] = (double)inf->iview[i] / fudge;
			}

			/* copy xform matrix to long for for fixed point math */
			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
				{
					inf->longmat[i][j] = (long) (inf->doublemat[i][j]*fudge);
					if (s_real_time)
					{
						inf->longmat1[i][j] = (long) (inf->doublemat1[i][j]*fudge);
					}
				}
			}
		}
		return 0;
	}

	/* apply perspective if requested */
	if (ZVIEWER)
	{
		if ((DEBUGFLAG_LORENZ_FLOAT == debugflag) || (ZVIEWER < 100)) /* use float for small persp */
		{
			/* use float perspective calc */
			VECTOR tmpv;
			for (i = 0; i < 3; i++)
			{
				tmpv[i] = (double)inf->viewvect[i] / fudge;
			}
			perspective(tmpv);
			for (i = 0; i < 3; i++)
			{
				inf->viewvect[i] = (long) (tmpv[i]*fudge);
			}
			if (s_real_time)
			{
				for (i = 0; i < 3; i++)
				{
					tmpv[i] = (double)inf->viewvect1[i] / fudge;
				}
				perspective(tmpv);
				for (i = 0; i < 3; i++)
				{
					inf->viewvect1[i] = (long) (tmpv[i]*fudge);
				}
			}
		}
		else
		{
			longpersp(inf->viewvect, inf->iview, bitshift);
			if (s_real_time)
			{
				longpersp(inf->viewvect1, inf->iview, bitshift);
			}
		}
	}

	/* work out the screen positions */
	inf->row = (int) (((multiply(inf->cvt.c, inf->viewvect[0], bitshift) +
			multiply(inf->cvt.d, inf->viewvect[1], bitshift) + inf->cvt.f)
			>> bitshift)
		+ g_yy_adjust);
	inf->col = (int) (((multiply(inf->cvt.a, inf->viewvect[0], bitshift) +
			multiply(inf->cvt.b, inf->viewvect[1], bitshift) + inf->cvt.e)
			>> bitshift)
		+ g_xx_adjust);
	if (inf->col < 0 || inf->col >= xdots || inf->row < 0 || inf->row >= ydots)
	{
		inf->col = inf->row =
			((long)abs(inf->col) + (long)abs(inf->row) > BAD_PIXEL)
			? -2 : -1;
	}
	if (s_real_time)
	{
		inf->row1 = (int) (((multiply(inf->cvt.c, inf->viewvect1[0], bitshift) +
						multiply(inf->cvt.d, inf->viewvect1[1], bitshift) +
						inf->cvt.f) >> bitshift)
						+ g_yy_adjust1);
		inf->col1 = (int) (((multiply(inf->cvt.a, inf->viewvect1[0], bitshift) +
						multiply(inf->cvt.b, inf->viewvect1[1], bitshift) +
						inf->cvt.e) >> bitshift)
						+ g_xx_adjust1);
		if (inf->col1 < 0 || inf->col1 >= xdots || inf->row1 < 0 || inf->row1 >= ydots)
		{
			inf->col1 = inf->row1 =
				((long)abs(inf->col1) + (long)abs(inf->row1) > BAD_PIXEL)
				? -2 : -1;
		}
	}
	return 1;
}

static int threed_view_trans_fp(struct threed_vt_inf_fp *inf)
{
	int i;
	double tmpx, tmpy, tmpz;
	double tmp;

	if (coloriter == 1)  /* initialize on first call */
	{
		for (i = 0; i < 3; i++)
		{
			inf->minvals[i] =  100000.0; /* impossible value */
			inf->maxvals[i] = -100000.0;
		}
		setup_matrix(inf->doublemat);
		if (s_real_time)
		{
			setup_matrix(inf->doublemat1);
		}
	}

	/* 3D VIEWING TRANSFORM */
	vmult(inf->orbit, inf->doublemat, inf->viewvect);
	if (s_real_time)
	{
		vmult(inf->orbit, inf->doublemat1, inf->viewvect1);
	}

	if (coloriter <= s_waste) /* waste this many points to find minz and maxz */
	{
		/* find minz and maxz */
		for (i = 0; i < 3; i++)
		{
			tmp = inf->viewvect[i];
			if (tmp < inf->minvals[i])
			{
				inf->minvals[i] = tmp;
			}
			else if (tmp > inf->maxvals[i])
			{
				inf->maxvals[i] = tmp;
			}
		}
		if (coloriter == s_waste) /* time to work it out */
		{
			g_view[0] = g_view[1] = 0; /* center on origin */
			/* z value of user's eye - should be more negative than extreme
									negative part of image */
			g_view[2] = (inf->minvals[2]-inf->maxvals[2])*(double)ZVIEWER/100.0;

			/* center image on origin */
			tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); /* center x */
			tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); /* center y */

			/* apply perspective shift */
			tmpx += ((double)g_x_shift*(xxmax-xxmin))/(xdots);
			tmpy += ((double)g_y_shift*(yymax-yymin))/(ydots);
			tmpz = -(inf->maxvals[2]);
			trans(tmpx, tmpy, tmpz, inf->doublemat);

			if (s_real_time)
			{
				/* center image on origin */
				tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); /* center x */
				tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); /* center y */

				tmpx += ((double)g_x_shift1*(xxmax-xxmin))/(xdots);
				tmpy += ((double)g_y_shift1*(yymax-yymin))/(ydots);
				tmpz = -(inf->maxvals[2]);
				trans(tmpx, tmpy, tmpz, inf->doublemat1);
				}
			}
		return 0;
		}

	/* apply perspective if requested */
	if (ZVIEWER)
	{
		perspective(inf->viewvect);
		if (s_real_time)
		{
			perspective(inf->viewvect1);
		}
	}
	inf->row = (int) (inf->cvt.c*inf->viewvect[0] + inf->cvt.d*inf->viewvect[1]
				+ inf->cvt.f + g_yy_adjust);
	inf->col = (int) (inf->cvt.a*inf->viewvect[0] + inf->cvt.b*inf->viewvect[1]
				+ inf->cvt.e + g_xx_adjust);
	if (inf->col < 0 || inf->col >= xdots || inf->row < 0 || inf->row >= ydots)
	{
		inf->col = inf->row =
			((long)abs(inf->col) + (long)abs(inf->row) > BAD_PIXEL)
			? -2 : -1;
	}
	if (s_real_time)
	{
		inf->row1 = (int) (inf->cvt.c*inf->viewvect1[0] + inf->cvt.d*inf->viewvect1[1]
					+ inf->cvt.f + g_yy_adjust1);
		inf->col1 = (int) (inf->cvt.a*inf->viewvect1[0] + inf->cvt.b*inf->viewvect1[1]
					+ inf->cvt.e + g_xx_adjust1);
		if (inf->col1 < 0 || inf->col1 >= xdots || inf->row1 < 0 || inf->row1 >= ydots)
		{
			inf->col1 = inf->row1 =
				((long)abs(inf->col1) + (long)abs(inf->row1) > BAD_PIXEL)
				? -2 : -1;
		}
	}
	return 1;
}

static FILE *open_orbit_save(void)
{
	FILE *fp = NULL;
	if (orbitsave & ORBITSAVE_RAW)
	{
		fp = fopen("orbits.raw.txt", "wt");
		if (fp)
		{
			fprintf(fp, "pointlist x y z color\n");
		}
	}
	return fp;
}

/* Plot a histogram by incrementing the pixel each time it it touched */
static void _fastcall plot_hist(int x, int y, int color)
{
	color = getcolor(x, y) + 1;
	if (color >= colors)
	{
		color = 1;
	}
	putcolor(x, y, color);
}
