/*
	This file contains two 3 dimensional orbit-type fractal
	generators - IFS and LORENZ3D, along with code to generate
	red/blue 3D images. Tim Wegner
*/
#include <assert.h>
#include <string.h>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "3d.h"
#include "drivers.h"
#include "encoder.h"
#include "fracsubr.h"
#include "jiim.h"
#include "lorenz.h"
#include "miscres.h"
#include "plot3d.h"
#include "realdos.h"

#include "EscapeTime.h"
#include "SoundState.h"
#include "ThreeDimensionalState.h"

/* orbitcalc is declared with no arguments so jump through hoops here */
#define LORBIT(x, y, z) \
	(*(int(*)(long *, long *, long *))g_current_fractal_specific->orbitcalc)(x, y, z)
#define FORBIT(x, y, z) \
	(*(int(*)(double*, double*, double*))g_current_fractal_specific->orbitcalc)(x, y, z)

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
	int row;
	int col;         /* results */
	int row1;
	int col1;
	l_affine cvt;
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
	int row;
	int col;         /* results */
	int row1;
	int col1;
	affine cvt;
};

/* global data provided by this module */
long g_max_count;
MajorMethodType g_major_method;
MinorMethodType g_minor_method;
bool g_keep_screen_coords = false;
bool g_set_orbit_corners = false;
long g_orbit_interval;
double g_orbit_x_min;
double g_orbit_y_min;
double g_orbit_x_max;
double g_orbit_y_max;
double g_orbit_x_3rd;
double g_orbit_y_3rd;

/* local data in this module */
static affine s_o_cvt;
static int s_o_color;
static affine s_cvt;
static l_affine s_lcvt;
static double s_cx;
static double s_cy;
static long   s_x_long, s_y_long;
/* s_connect, s_euler, s_waste are potential user parameters */
static bool s_connect = true;		/* flag to connect points with a line */
static bool s_euler = false;		/* use implicit euler approximation for dynamic system */
static int s_waste = 100;			/* waste this many points before plotting */
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
static int  ifs_2d();
static int  ifs_3d();
static int  ifs_3d_long();
static int  ifs_3d_float();
static int  l_setup_convert_to_screen(l_affine *);
static void setup_matrix(MATRIX);
static int  threed_view_trans(threed_vt_inf *inf);
static int  threed_view_trans_fp(threed_vt_inf_fp *inf);
static FILE *open_orbit_save();
static void _fastcall plot_color_histogram(int x, int y, int color);

/******************************************************************/
/*                 zoom box conversion functions                  */
/******************************************************************/

/*
	Conversion of complex plane to screen coordinates for rotating zoom box.
	Assume there is an affine transformation mapping complex zoom parallelogram
	to rectangular screen. We know this map must map parallelogram corners to
	screen corners, so we have following equations:

		a*xmin + b*ymax + e == 0        (upper left)
		c*xmin + d*ymax + f == 0

		a*x3rd + b*y3rd + e == 0        (lower left)
		c*x3rd + d*y3rd + f == g_y_dots-1

		a*xmax + b*ymin + e == g_x_dots-1  (lower right)
		c*xmax + d*ymin + f == g_y_dots-1

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
     a*xmin + b*ymax + e == 0
     a*x3rd + b*y3rd + e == 0
     a*xmax + b*ymin + e == g_x_dots-1
  To make things easy to read, I just replace xmin, xmax, x3rd by x1,
  x2, x3 (ditto for yy...) and g_x_dots-1 by xd.

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

	c*xmin + d*ymax + f == 0
	c*x3rd + d*y3rd + f == g_y_dots-1
	c*xmax + d*ymin + f == g_y_dots-1

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
int setup_convert_to_screen(affine *scrn_cnvt)
{
	double det;
	double xd;
	double yd;

	det = (g_escape_time_state.m_grid_fp.x_3rd()-g_escape_time_state.m_grid_fp.x_min())*(g_escape_time_state.m_grid_fp.y_min()-g_escape_time_state.m_grid_fp.y_max()) + (g_escape_time_state.m_grid_fp.y_max()-g_escape_time_state.m_grid_fp.y_3rd())*g_escape_time_state.m_grid_fp.width();
	if (det == 0)
	{
		return -1;
	}
	xd = g_dx_size/det;
	scrn_cnvt->a =  xd*(g_escape_time_state.m_grid_fp.y_max()-g_escape_time_state.m_grid_fp.y_3rd());
	scrn_cnvt->b =  xd*(g_escape_time_state.m_grid_fp.x_3rd()-g_escape_time_state.m_grid_fp.x_min());
	scrn_cnvt->e = -scrn_cnvt->a*g_escape_time_state.m_grid_fp.x_min() - scrn_cnvt->b*g_escape_time_state.m_grid_fp.y_max();

	det = (g_escape_time_state.m_grid_fp.x_3rd()-g_escape_time_state.m_grid_fp.x_max())*(-g_escape_time_state.m_grid_fp.height()) + (g_escape_time_state.m_grid_fp.y_min()-g_escape_time_state.m_grid_fp.y_3rd())*g_escape_time_state.m_grid_fp.width();
	if (det == 0)
	{
		return -1;
	}
	yd = g_dy_size/det;
	scrn_cnvt->c =  yd*(g_escape_time_state.m_grid_fp.y_min()-g_escape_time_state.m_grid_fp.y_3rd());
	scrn_cnvt->d =  yd*(g_escape_time_state.m_grid_fp.x_3rd()-g_escape_time_state.m_grid_fp.x_max());
	scrn_cnvt->f = -scrn_cnvt->c*g_escape_time_state.m_grid_fp.x_min() - scrn_cnvt->d*g_escape_time_state.m_grid_fp.y_max();
	return 0;
}

static int l_setup_convert_to_screen(l_affine *l_cvt)
{
	affine cvt;

	if (setup_convert_to_screen(&cvt))
	{
		return -1;
	}
	l_cvt->a = (long) (cvt.a*g_fudge);
	l_cvt->b = (long) (cvt.b*g_fudge);
	l_cvt->c = (long) (cvt.c*g_fudge);
	l_cvt->d = (long) (cvt.d*g_fudge);
	l_cvt->e = (long) (cvt.e*g_fudge);
	l_cvt->f = (long) (cvt.f*g_fudge);

	return 0;
}

/******************************************************************/
/*   setup functions - put in g_fractal_specific[g_fractal_type].per_image */
/******************************************************************/

int orbit_3d_setup()
{
	g_max_count = 0L;
	s_connect = true;
	s_waste = 100;
	s_projection = PROJECTION_XY;
	if (g_fractal_type == FRACTYPE_HENON_L || g_fractal_type == FRACTYPE_KAM_TORUS || g_fractal_type == FRACTYPE_KAM_TORUS_3D ||
		g_fractal_type == FRACTYPE_INVERSE_JULIA)
	{
		s_connect = false;
	}
	if (g_fractal_type == FRACTYPE_ROSSLER_L)
	{
		s_waste = 500;
	}
	if (g_fractal_type == FRACTYPE_LORENZ_L)
	{
		s_projection = PROJECTION_XZ;
	}

	s_init_orbit_long[0] = g_fudge;  /* initial conditions */
	s_init_orbit_long[1] = g_fudge;
	s_init_orbit_long[2] = g_fudge;

	if (g_fractal_type == FRACTYPE_HENON_L)
	{
		s_l_a =  (long) (g_parameters[0]*g_fudge);
		s_l_b =  (long) (g_parameters[1]*g_fudge);
		s_l_c =  (long) (g_parameters[2]*g_fudge);
		s_l_d =  (long) (g_parameters[3]*g_fudge);
	}
	else if (g_fractal_type == FRACTYPE_KAM_TORUS || g_fractal_type == FRACTYPE_KAM_TORUS_3D)
	{
		g_max_count = 1L;
		s_a   = g_parameters[0];           /* angle */
		if (g_parameters[1] <= 0.0)
		{
			g_parameters[1] = .01;
		}
		s_l_b =  (long) (g_parameters[1]*g_fudge);    /* stepsize */
		s_l_c =  (long) (g_parameters[2]*g_fudge);    /* stop */
		s_l_d =  (long) g_parameters[3];
		s_t = (int) s_l_d;     /* points per orbit */

		s_l_sinx = (long) (sin(s_a)*g_fudge);
		s_l_cosx = (long) (cos(s_a)*g_fudge);
		s_l_orbit = 0;
		s_init_orbit_long[0] = s_init_orbit_long[1] = s_init_orbit_long[2] = 0;
	}
	else if (g_fractal_type == FRACTYPE_INVERSE_JULIA)
	{
		ComplexL Sqrt;

		s_x_long = (long) (g_parameters[0]*g_fudge);
		s_y_long = (long) (g_parameters[1]*g_fudge);

		s_max_hits    = (int) g_parameters[2];
		s_run_length = (int) g_parameters[3];
		if (s_max_hits <= 0)
		{
			s_max_hits = 1;
		}
		else if (s_max_hits >= g_colors)
		{
			s_max_hits = g_colors - 1;
		}
		g_parameters[2] = s_max_hits;

		setup_convert_to_screen(&s_cvt);
		/* Note: using g_bit_shift of 21 for affine, 24 otherwise */

		s_lcvt.a = (long) (s_cvt.a*(1L << 21));
		s_lcvt.b = (long) (s_cvt.b*(1L << 21));
		s_lcvt.c = (long) (s_cvt.c*(1L << 21));
		s_lcvt.d = (long) (s_cvt.d*(1L << 21));
		s_lcvt.e = (long) (s_cvt.e*(1L << 21));
		s_lcvt.f = (long) (s_cvt.f*(1L << 21));

		Sqrt = ComplexSqrtLong(g_fudge - 4*s_x_long, -4*s_y_long);

		switch (g_major_method)
		{
		case MAJORMETHOD_BREADTH_FIRST:
			if (Init_Queue((long)32*1024) == 0)
			{ /* can't get queue memory: fall back to random walk */
				stop_message(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, s_no_queue);
				g_major_method = MAJORMETHOD_RANDOM_WALK;
				goto lrwalk;
			}
			EnQueueLong((g_fudge + Sqrt.x) / 2,  Sqrt.y / 2);
			EnQueueLong((g_fudge - Sqrt.x) / 2, -Sqrt.y / 2);
			break;
		case MAJORMETHOD_DEPTH_FIRST:
			if (Init_Queue((long)32*1024) == 0)
			{ /* can't get queue memory: fall back to random walk */
				stop_message(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, s_no_queue);
				g_major_method = MAJORMETHOD_RANDOM_WALK;
				goto lrwalk;
			}
			switch (g_minor_method)
			{
				case MINORMETHOD_LEFT_FIRST:
					PushLong((g_fudge + Sqrt.x) / 2,  Sqrt.y / 2);
					PushLong((g_fudge - Sqrt.x) / 2, -Sqrt.y / 2);
					break;
				case MINORMETHOD_RIGHT_FIRST:
					PushLong((g_fudge - Sqrt.x) / 2, -Sqrt.y / 2);
					PushLong((g_fudge + Sqrt.x) / 2,  Sqrt.y / 2);
					break;
			}
			break;
		case MAJORMETHOD_RANDOM_WALK:
lrwalk:
			g_new_z_l.x = s_init_orbit_long[0] = g_fudge + Sqrt.x / 2;
			g_new_z_l.y = s_init_orbit_long[1] =         Sqrt.y / 2;
			break;
		case MAJORMETHOD_RANDOM_RUN:
			g_new_z_l.x = s_init_orbit_long[0] = g_fudge + Sqrt.x / 2;
			g_new_z_l.y = s_init_orbit_long[1] =         Sqrt.y / 2;
			break;
		}
	}
	else
	{
		s_l_dt = (long) (g_parameters[0]*g_fudge);
		s_l_a =  (long) (g_parameters[1]*g_fudge);
		s_l_b =  (long) (g_parameters[2]*g_fudge);
		s_l_c =  (long) (g_parameters[3]*g_fudge);
	}

	/* precalculations for speed */
	s_l_adt = multiply(s_l_a, s_l_dt, g_bit_shift);
	s_l_bdt = multiply(s_l_b, s_l_dt, g_bit_shift);
	s_l_cdt = multiply(s_l_c, s_l_dt, g_bit_shift);
	return 1;
}

#define COSB   s_dx
#define SINABC s_dy

int orbit_3d_setup_fp()
{
	g_max_count = 0L;
	s_connect = true;
	s_waste = 100;
	s_projection = PROJECTION_XY;

	if (g_fractal_type == FRACTYPE_HENON_FP || g_fractal_type == FRACTYPE_PICKOVER_FP || g_fractal_type == FRACTYPE_GINGERBREAD_FP
				|| g_fractal_type == FRACTYPE_KAM_TORUS_FP || g_fractal_type == FRACTYPE_KAM_TORUS_3D_FP
				|| g_fractal_type == FRACTYPE_HOPALONG_FP || g_fractal_type == FRACTYPE_INVERSE_JULIA_FP)
	{
		s_connect = false;
	}
	if (g_fractal_type == FRACTYPE_LORENZ_3D_1_FP || g_fractal_type == FRACTYPE_LORENZ_3D_3_FP ||
		g_fractal_type == FRACTYPE_LORENZ_3D_4_FP)
	{
		s_waste = 750;
	}
	if (g_fractal_type == FRACTYPE_ROSSLER_FP)
	{
		s_waste = 500;
	}
	if (g_fractal_type == FRACTYPE_LORENZ_FP)
	{
		s_projection = PROJECTION_XZ; /* plot x and z */
	}

	s_init_orbit_fp[0] = 1;  /* initial conditions */
	s_init_orbit_fp[1] = 1;
	s_init_orbit_fp[2] = 1;
	if (g_fractal_type == FRACTYPE_GINGERBREAD_FP)
	{
		s_init_orbit_fp[0] = g_parameters[0];        /* initial conditions */
		s_init_orbit_fp[1] = g_parameters[1];
	}

	if (g_fractal_type == FRACTYPE_ICON || g_fractal_type == FRACTYPE_ICON_3D)        /* DMF */
	{
		s_init_orbit_fp[0] = 0.01;  /* initial conditions */
		s_init_orbit_fp[1] = 0.003;
		s_connect = false;
		s_waste = 2000;
	}

	if (g_fractal_type == FRACTYPE_LATOOCARFIAN)        /* HB */
	{
		s_connect = false;
	}

	if (g_fractal_type == FRACTYPE_HENON_FP || g_fractal_type == FRACTYPE_PICKOVER_FP)
	{
		s_a =  g_parameters[0];
		s_b =  g_parameters[1];
		s_c =  g_parameters[2];
		s_d =  g_parameters[3];
	}
	else if (g_fractal_type == FRACTYPE_ICON || g_fractal_type == FRACTYPE_ICON_3D)        /* DMF */
	{
		s_init_orbit_fp[0] = 0.01;  /* initial conditions */
		s_init_orbit_fp[1] = 0.003;
		s_connect = false;
		s_waste = 2000;
		/* Initialize parameters */
		s_a  =   g_parameters[0];
		s_b  =   g_parameters[1];
		s_c  =   g_parameters[2];
		s_d  =   g_parameters[3];
	}
	else if (g_fractal_type == FRACTYPE_KAM_TORUS_FP || g_fractal_type == FRACTYPE_KAM_TORUS_3D_FP)
	{
		g_max_count = 1L;
		s_a = g_parameters[0];           /* angle */
		if (g_parameters[1] <= 0.0)
		{
			g_parameters[1] = .01;
		}
		s_b =  g_parameters[1];    /* stepsize */
		s_c =  g_parameters[2];    /* stop */
		s_l_d =  (long) g_parameters[3];
		s_t = (int) s_l_d;     /* points per orbit */
		g_sin_x = sin(s_a);
		g_cos_x = cos(s_a);
		s_orbit = 0;
		s_init_orbit_fp[0] = s_init_orbit_fp[1] = s_init_orbit_fp[2] = 0;
	}
	else if (g_fractal_type == FRACTYPE_HOPALONG_FP || g_fractal_type == FRACTYPE_MARTIN_FP || g_fractal_type == FRACTYPE_CHIP
		|| g_fractal_type == FRACTYPE_QUADRUP_TWO || g_fractal_type == FRACTYPE_THREE_PLY)
	{
		s_init_orbit_fp[0] = 0;  /* initial conditions */
		s_init_orbit_fp[1] = 0;
		s_init_orbit_fp[2] = 0;
		s_connect = false;
		s_a =  g_parameters[0];
		s_b =  g_parameters[1];
		s_c =  g_parameters[2];
		s_d =  g_parameters[3];
		if (g_fractal_type == FRACTYPE_THREE_PLY)
		{
			COSB   = cos(s_b);
			SINABC = sin(s_a + s_b + s_c);
		}
	}
	else if (g_fractal_type == FRACTYPE_INVERSE_JULIA_FP)
	{
		ComplexD Sqrt;

		s_cx = g_parameters[0];
		s_cy = g_parameters[1];

		s_max_hits    = (int) g_parameters[2];
		s_run_length = (int) g_parameters[3];
		if (s_max_hits <= 0)
		{
			s_max_hits = 1;
		}
		else if (s_max_hits >= g_colors)
		{
			s_max_hits = g_colors - 1;
		}
		g_parameters[2] = s_max_hits;

		setup_convert_to_screen(&s_cvt);

		/* find fixed points: guaranteed to be in the set */
		Sqrt = ComplexSqrtFloat(1 - 4*s_cx, -4*s_cy);
		switch (g_major_method)
		{
		case MAJORMETHOD_BREADTH_FIRST:
			if (Init_Queue((long)32*1024) == 0)
			{ /* can't get queue memory: fall back to random walk */
				stop_message(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, s_no_queue);
				g_major_method = MAJORMETHOD_RANDOM_WALK;
				goto rwalk;
			}
			EnQueueFloat((float) ((1 + Sqrt.x) / 2), (float) (Sqrt.y / 2));
			EnQueueFloat((float) ((1 - Sqrt.x) / 2), (float) (-Sqrt.y / 2));
			break;
		case MAJORMETHOD_DEPTH_FIRST:                      /* depth first (choose direction) */
			if (Init_Queue((long)32*1024) == 0)
			{ /* can't get queue memory: fall back to random walk */
				stop_message(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, s_no_queue);
				g_major_method = MAJORMETHOD_RANDOM_WALK;
				goto rwalk;
			}
			switch (g_minor_method)
			{
			case MINORMETHOD_LEFT_FIRST:
				PushFloat((float) ((1 + Sqrt.x) / 2), (float) (Sqrt.y / 2));
				PushFloat((float) ((1 - Sqrt.x) / 2), (float) (-Sqrt.y / 2));
				break;
			case MINORMETHOD_RIGHT_FIRST:
				PushFloat((float) ((1 - Sqrt.x) / 2), (float) (-Sqrt.y / 2));
				PushFloat((float) ((1 + Sqrt.x) / 2), (float) (Sqrt.y / 2));
				break;
			}
			break;
		case MAJORMETHOD_RANDOM_WALK:
rwalk:
			g_new_z.x = s_init_orbit_fp[0] = 1 + Sqrt.x / 2;
			g_new_z.y = s_init_orbit_fp[1] = Sqrt.y / 2;
			break;
		case MAJORMETHOD_RANDOM_RUN:       /* random run, choose intervals */
			g_major_method = MAJORMETHOD_RANDOM_RUN;
			g_new_z.x = s_init_orbit_fp[0] = 1 + Sqrt.x / 2;
			g_new_z.y = s_init_orbit_fp[1] = Sqrt.y / 2;
			break;
		}
	}
	else
	{
		s_dt = g_parameters[0];
		s_a =  g_parameters[1];
		s_b =  g_parameters[2];
		s_c =  g_parameters[3];

	}

	/* precalculations for speed */
	s_adt = s_a*s_dt;
	s_bdt = s_b*s_dt;
	s_cdt = s_c*s_dt;

	return 1;
}

/******************************************************************/
/*   orbit functions - put in g_fractal_specific[g_fractal_type].orbitcalc */
/******************************************************************/

/* Julia sets by inverse iterations added by Juan J. Buhler 4/3/92 */
/* Integrated with Lorenz by Tim Wegner 7/20/92 */
/* Add Modified Inverse Iteration Method, 11/92 by Michael Snyder  */

int Minverse_julia_orbit()
{
	static int random_dir = 0;
	static int random_len = 0;
	int newrow;
	int newcol;
	int color;
	int leftright;

	/*
	* First, compute new point
	*/
	switch (g_major_method)
	{
	case MAJORMETHOD_BREADTH_FIRST:
		if (QueueEmpty())
		{
			return -1;
		}
		g_new_z = DeQueueFloat();
		break;
	case MAJORMETHOD_DEPTH_FIRST:
		if (QueueEmpty())
		{
			return -1;
		}
		g_new_z = PopFloat();
		break;
	case MAJORMETHOD_RANDOM_WALK:
#if 0
		g_new_z = ComplexSqrtFloat(g_new_z.x - s_cx, g_new_z.y - s_cy);
		if (RANDOM(2))
		{
			g_new_z.x = -g_new_z.x;
			g_new_z.y = -g_new_z.y;
		}
#endif
		break;
	case MAJORMETHOD_RANDOM_RUN:
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

	if (newcol < 1 || newcol >= g_x_dots || newrow < 1 || newrow >= g_y_dots)
	{
		/*
		* MIIM must skip points that are off the screen boundary,
		* since it cannot read their color.
		*/
		switch (g_major_method)
		{
		case MAJORMETHOD_BREADTH_FIRST:
			EnQueueFloat((float) (leftright*g_new_z.x), (float) (leftright*g_new_z.y));
			return 1;
		case MAJORMETHOD_DEPTH_FIRST:
			PushFloat   ((float) (leftright*g_new_z.x), (float) (leftright*g_new_z.y));
			return 1;
		case MAJORMETHOD_RANDOM_RUN:
		case MAJORMETHOD_RANDOM_WALK:
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
	case MAJORMETHOD_BREADTH_FIRST:
		if (color < s_max_hits)
		{
			g_plot_color_put_color(newcol, newrow, color + 1);
			/* g_new_z = ComplexSqrtFloat(g_new_z.x - s_cx, g_new_z.y - s_cy); */
			EnQueueFloat((float)g_new_z.x, (float)g_new_z.y);
			EnQueueFloat((float)-g_new_z.x, (float)-g_new_z.y);
		}
		break;
	case MAJORMETHOD_DEPTH_FIRST:
		if (color < s_max_hits)
		{
			g_plot_color_put_color(newcol, newrow, color + 1);
			/* g_new_z = ComplexSqrtFloat(g_new_z.x - s_cx, g_new_z.y - s_cy); */
			if (g_minor_method == MINORMETHOD_LEFT_FIRST)
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
	case MAJORMETHOD_RANDOM_RUN:
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
		if (color < g_colors-1)
		{
			g_plot_color_put_color(newcol, newrow, color + 1);
		}
		break;
	case MAJORMETHOD_RANDOM_WALK:
		if (color < g_colors-1)
		{
			g_plot_color_put_color(newcol, newrow, color + 1);
		}
		g_new_z.x = leftright*g_new_z.x;
		g_new_z.y = leftright*g_new_z.y;
		break;
	}
	return 1;
}

int Linverse_julia_orbit()
{
	static int random_dir = 0;
	static int random_len = 0;
	int newrow;
	int newcol;
	int color;

	/*
	* First, compute new point
	*/
	switch (g_major_method)
	{
	case MAJORMETHOD_BREADTH_FIRST:
		if (QueueEmpty())
		{
			return -1;
		}
		g_new_z_l = DeQueueLong();
		break;
	case MAJORMETHOD_DEPTH_FIRST:
		if (QueueEmpty())
		{
			return -1;
		}
		g_new_z_l = PopLong();
		break;
	case MAJORMETHOD_RANDOM_WALK:
		g_new_z_l = ComplexSqrtLong(g_new_z_l.x - s_x_long, g_new_z_l.y - s_y_long);
		if (RANDOM(2))
		{
			g_new_z_l.x = -g_new_z_l.x;
			g_new_z_l.y = -g_new_z_l.y;
		}
		break;
	case MAJORMETHOD_RANDOM_RUN:
		g_new_z_l = ComplexSqrtLong(g_new_z_l.x - s_x_long, g_new_z_l.y - s_y_long);
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
			g_new_z_l.x = -g_new_z_l.x;
			g_new_z_l.y = -g_new_z_l.y;
			break;
		case DIRECTION_RANDOM:
			if (RANDOM(2))
			{
				g_new_z_l.x = -g_new_z_l.x;
				g_new_z_l.y = -g_new_z_l.y;
			}
			break;
		}
	}

	/*
	* Next, find its pixel position
	*
	* Note: had to use a g_bit_shift of 21 for this operation because
	* otherwise the values of s_lcvt were truncated.  Used g_bit_shift
	* of 24 otherwise, for increased precision.
	*/
	newcol = (int) ((multiply(s_lcvt.a, g_new_z_l.x >> (g_bit_shift - 21), 21) +
			multiply(s_lcvt.b, g_new_z_l.y >> (g_bit_shift - 21), 21) + s_lcvt.e) >> 21);
	newrow = (int) ((multiply(s_lcvt.c, g_new_z_l.x >> (g_bit_shift - 21), 21) +
			multiply(s_lcvt.d, g_new_z_l.y >> (g_bit_shift - 21), 21) + s_lcvt.f) >> 21);

	if (newcol < 1 || newcol >= g_x_dots || newrow < 1 || newrow >= g_y_dots)
	{
		/*
		* MIIM must skip points that are off the screen boundary,
		* since it cannot read their color.
		*/
		color = RANDOM(2) ? 1 : -1;
		switch (g_major_method)
		{
		case MAJORMETHOD_BREADTH_FIRST:
			g_new_z_l = ComplexSqrtLong(g_new_z_l.x - s_x_long, g_new_z_l.y - s_y_long);
			EnQueueLong(color*g_new_z_l.x, color*g_new_z_l.y);
			break;
		case MAJORMETHOD_DEPTH_FIRST:
			g_new_z_l = ComplexSqrtLong(g_new_z_l.x - s_x_long, g_new_z_l.y - s_y_long);
			PushLong(color*g_new_z_l.x, color*g_new_z_l.y);
			break;
		case MAJORMETHOD_RANDOM_RUN:
			random_len--;
		case MAJORMETHOD_RANDOM_WALK:
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
	case MAJORMETHOD_BREADTH_FIRST:
		if (color < s_max_hits)
		{
			g_plot_color_put_color(newcol, newrow, color + 1);
			g_new_z_l = ComplexSqrtLong(g_new_z_l.x - s_x_long, g_new_z_l.y - s_y_long);
			EnQueueLong(g_new_z_l.x,  g_new_z_l.y);
			EnQueueLong(-g_new_z_l.x, -g_new_z_l.y);
		}
		break;
	case MAJORMETHOD_DEPTH_FIRST:
		if (color < s_max_hits)
		{
			g_plot_color_put_color(newcol, newrow, color + 1);
			g_new_z_l = ComplexSqrtLong(g_new_z_l.x - s_x_long, g_new_z_l.y - s_y_long);
			if (g_minor_method == MINORMETHOD_LEFT_FIRST)
			{
				if (QueueFullAlmost())
				{
					PushLong(-g_new_z_l.x, -g_new_z_l.y);
				}
				else
				{
					PushLong(g_new_z_l.x,  g_new_z_l.y);
					PushLong(-g_new_z_l.x, -g_new_z_l.y);
				}
			}
			else
			{
				if (QueueFullAlmost())
				{
					PushLong(g_new_z_l.x,  g_new_z_l.y);
				}
				else
				{
					PushLong(-g_new_z_l.x, -g_new_z_l.y);
					PushLong(g_new_z_l.x,  g_new_z_l.y);
				}
			}
		}
		break;
	case MAJORMETHOD_RANDOM_RUN:
		random_len--;
		/* fall through */
	case MAJORMETHOD_RANDOM_WALK:
		if (color < g_colors-1)
		{
			g_plot_color_put_color(newcol, newrow, color + 1);
		}
		break;
	}
	return 1;
}

int lorenz_3d_orbit(long *l_x, long *l_y, long *l_z)
{
	s_l_xdt = multiply(*l_x, s_l_dt, g_bit_shift);
	s_l_ydt = multiply(*l_y, s_l_dt, g_bit_shift);
	s_l_dx  = -multiply(s_l_adt, *l_x, g_bit_shift) + multiply(s_l_adt, *l_y, g_bit_shift);
	s_l_dy  =  multiply(s_l_bdt, *l_x, g_bit_shift) -s_l_ydt -multiply(*l_z, s_l_xdt, g_bit_shift);
	s_l_dz  = -multiply(s_l_cdt, *l_z, g_bit_shift) + multiply(*l_x, s_l_ydt, g_bit_shift);

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
	double newx;
	double newy;
	*z = *x; /* for warning only */
	newx  = 1 + *y - s_a*(*x)*(*x);
	newy  = s_b*(*x);
	*x = newx;
	*y = newy;
	return 0;
}

int henon_orbit(long *l_x, long *l_y, long *l_z)
{
	long newx;
	long newy;
	*l_z = *l_x; /* for warning only */
	newx = multiply(*l_x, *l_x, g_bit_shift);
	newx = multiply(newx, s_l_a, g_bit_shift);
	newx  = g_fudge + *l_y - newx;
	newy  = multiply(s_l_b, *l_x, g_bit_shift);
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
	double newx;
	double newy;
	double newz;
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
	s_l_xdt = multiply(*l_x, s_l_dt, g_bit_shift);
	s_l_ydt = multiply(*l_y, s_l_dt, g_bit_shift);

	s_l_dx  = -s_l_ydt - multiply(*l_z, s_l_dt, g_bit_shift);
	s_l_dy  =  s_l_xdt + multiply(*l_y, s_l_adt, g_bit_shift);
	s_l_dz  =  s_l_bdt + multiply(*l_z, s_l_xdt, g_bit_shift)
						- multiply(*l_z, s_l_cdt, g_bit_shift);

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
	(*s) = (*r)*g_sin_x + srr*g_cos_x;
	(*r) = (*r)*g_cos_x - srr*g_sin_x;
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
	srr = (*s)-multiply((*r), (*r), g_bit_shift);
	(*s) = multiply((*r), s_l_sinx, g_bit_shift) + multiply(srr, s_l_cosx, g_bit_shift);
	(*r) = multiply((*r), s_l_cosx, g_bit_shift)-multiply(srr, s_l_sinx, g_bit_shift);
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
	double newx;
	double newy;
	double x2;
	double y2;
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
	ComplexD cp;
	ComplexD tmp;
	double newx;
	double newy;
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

/*
	LAMBDA  g_parameters[0]
	ALPHA   g_parameters[1]
	BETA    g_parameters[2]
	GAMMA   g_parameters[3]
	OMEGA   g_parameters[4]
	DEGREE  g_parameters[5]
*/
int icon_orbit_fp(double *x, double *y, double *z)
{
	double oldx;
	double oldy;
	double zzbar;
	double zreal;
	double zimag;
	double za;
	double zb;
	double zn;
	double p;

	oldx = *x;
	oldy = *y;

	zzbar = oldx*oldx + oldy*oldy;
	zreal = oldx;
	zimag = oldy;

	int degree = static_cast<int>(g_parameters[5]);
	for (int i = 1; i <= degree-2; i++)
	{
		za = zreal*oldx - zimag*oldy;
		zb = zimag*oldx + zreal*oldy;
		zreal = za;
		zimag = zb;
	}
	zn = oldx*zreal - oldy*zimag;
	p = g_parameters[0] + g_parameters[1]*zzbar + g_parameters[2]*zn;
	*x = p*oldx + g_parameters[3]*zreal - g_parameters[4]*oldy;
	*y = p*oldy - g_parameters[3]*zimag + g_parameters[4]*oldx;

	*z = zzbar;
	return 0;
}

/* hb */
#define PAR_A   g_parameters[0]
#define PAR_B   g_parameters[1]
#define PAR_C   g_parameters[2]
#define PAR_D   g_parameters[3]

int latoo_orbit_fp(double *x, double *y, double *z)
{
	double xold;
	double yold;
	double tmp;

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
/*   Main fractal engines - put in g_fractal_specific[g_fractal_type].calculate_type */
/**********************************************************************/

int inverse_julia_per_image()
{
	int color = 0;

	if (g_resuming)            /* can't resume */
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
		color = g_current_fractal_specific->orbitcalc();
		g_old_z = g_new_z;
	}
	Free_Queue();
	return 0;
}

int orbit_2d_fp()
{
	FILE *fp;
	double x;
	double y;
	double z;
	int color;
	int col;
	int row;
	int count;
	int oldrow;
	int oldcol;
	double *p0;
	double *p1;
	double *p2;
	affine cvt;
	int ret;

	p0 = p1 = p2 = NULL;

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

	color = (g_inside > 0) ? g_inside : 2;

	oldcol = oldrow = -1;
	x = s_init_orbit_fp[0];
	y = s_init_orbit_fp[1];
	z = s_init_orbit_fp[2];
	g_color_iter = 0L;
	count = ret = 0;
	g_max_count = (g_max_iteration > 0x1fffffL || g_max_count) ? 0x7fffffffL : g_max_iteration*1024L;

	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(count), &count, sizeof(color), &color,
			sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
			sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(s_t), &s_t,
			sizeof(s_orbit), &s_orbit, sizeof(g_color_iter), &g_color_iter,
			0);
		end_resume();
	}

	while (g_color_iter++ <= g_max_count) /* loop until keypress or maxit */
	{
		if (driver_key_pressed())
		{
			driver_mute();
			alloc_resume(100, 1);
			put_resume(sizeof(count), &count, sizeof(color), &color,
				sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
				sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(s_t), &s_t,
				sizeof(s_orbit), &s_orbit, sizeof(g_color_iter), &g_color_iter,
				0);
			ret = -1;
			break;
		}
		if (++count > 1000)
		{        /* time to switch colors? */
			count = 0;
			if (++color >= g_colors)   /* another color to switch to? */
			{
				color = 1;  /* (don't use the background color) */
			}
		}

		col = (int) (cvt.a*x + cvt.b*y + cvt.e);
		row = (int) (cvt.c*x + cvt.d*y + cvt.f);
		if (col >= 0 && col < g_x_dots && row >= 0 && row < g_y_dots)
		{
			g_sound_state.orbit(x, y, z);
			if ((g_fractal_type != FRACTYPE_ICON) && (g_fractal_type != FRACTYPE_LATOOCARFIAN))
			{
				if (oldcol != -1 && s_connect)
				{
					driver_draw_line(col, row, oldcol, oldrow, color % g_colors);
				}
				else
				{
					(*g_plot_color)(col, row, color % g_colors);
				}
			}
			else
			{
				/* should this be using plot_hist()? */
				color = getcolor(col, row) + 1;
				if (color < g_colors) /* color sticks on last value */
				{
					(*g_plot_color)(col, row, color);
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
	l_affine cvt;
	/* setup affine screen coord conversion */
	l_setup_convert_to_screen(&cvt);

	/* set up projection scheme */
	long *p0 = NULL;
	long *p1 = NULL;
	long *p2 = NULL;
	long x;
	long y;
	long z;
	switch (s_projection)
	{
	case PROJECTION_ZX: p0 = &z; p1 = &x; p2 = &y; break;
	case PROJECTION_XZ: p0 = &x; p1 = &z; p2 = &y; break;
	case PROJECTION_XY: p0 = &x; p1 = &y; p2 = &z; break;
	}

	int color;
	color = (g_inside > 0) ? g_inside : 2;
	if (color >= g_colors)
	{
		color = 1;
	}
	int oldcol;
	int oldrow;
	oldcol = oldrow = -1;
	x = s_init_orbit_long[0];
	y = s_init_orbit_long[1];
	z = s_init_orbit_long[2];
	g_max_count = (g_max_iteration > 0x1fffffL || g_max_count) ? 0x7fffffffL : g_max_iteration*1024L;
	g_color_iter = 0L;

	int count = 0;
	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(count), &count, sizeof(color), &color,
			sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
			sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(s_t), &s_t,
			sizeof(s_l_orbit), &s_l_orbit, sizeof(g_color_iter), &g_color_iter,
			0);
		end_resume();
	}

	FILE *fp = open_orbit_save();
	int ret = 0;
	int start = 1;
	while (g_color_iter++ <= g_max_count) /* loop until keypress or maxit */
	{
		if (driver_key_pressed())
		{
			driver_mute();
			alloc_resume(100, 1);
			put_resume(sizeof(count), &count, sizeof(color), &color,
				sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
				sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(s_t), &s_t,
				sizeof(s_l_orbit), &s_l_orbit, sizeof(g_color_iter), &g_color_iter,
				0);
			ret = -1;
			break;
		}
		if (++count > 1000)
		{        /* time to switch colors? */
			count = 0;
			if (++color >= g_colors)   /* another color to switch to? */
			{
				color = 1;  /* (don't use the background color) */
			}
		}

		int col = (int) ((multiply(cvt.a, x, g_bit_shift) + multiply(cvt.b, y, g_bit_shift) + cvt.e) >> g_bit_shift);
		int row = (int) ((multiply(cvt.c, x, g_bit_shift) + multiply(cvt.d, y, g_bit_shift) + cvt.f) >> g_bit_shift);
		if (g_overflow)
		{
			g_overflow = 0;
			return ret;
		}
		if (col >= 0 && col < g_x_dots && row >= 0 && row < g_y_dots)
		{
			g_sound_state.orbit(x/g_fudge, y/g_fudge, z/g_fudge);
			if (oldcol != -1 && s_connect)
			{
				driver_draw_line(col, row, oldcol, oldrow, color % g_colors);
			}
			else if (!start)
			{
				(*g_plot_color)(col, row, color % g_colors);
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
			fprintf(fp, "%g %g %g 15\n", (double) *p0/g_fudge, (double) *p1/g_fudge, 0.0);
		}
	}
	if (fp)
	{
		fclose(fp);
	}
	return ret;
}

static int orbit_3d_calc()
{
	FILE *fp;
	unsigned long count;
	int oldcol;
	int oldrow;
	int oldcol1;
	int oldrow1;
	threed_vt_inf inf;
	int color;
	int ret;

	/* setup affine screen coord conversion */
	l_setup_convert_to_screen(&inf.cvt);

	oldcol1 = oldrow1 = oldcol = oldrow = -1;
	color = 2;
	if (color >= g_colors)
	{
		color = 1;
	}

	inf.orbit[0] = s_init_orbit_long[0];
	inf.orbit[1] = s_init_orbit_long[1];
	inf.orbit[2] = s_init_orbit_long[2];

	if (driver_diskp())                /* this would KILL a disk drive! */
	{
		not_disk_message();
	}

	fp = open_orbit_save();

	count = ret = 0;
	g_max_count = (g_max_iteration > 0x1fffffL || g_max_count) ? 0x7fffffffL : g_max_iteration*1024L;
	g_color_iter = 0L;
	while (g_color_iter++ <= g_max_count) /* loop until keypress or maxit */
	{
		/* calc goes here */
		if (++count > 1000)
		{        /* time to switch colors? */
			count = 0;
			if (++color >= g_colors)   /* another color to switch to? */
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
			fprintf(fp, "%g %g %g 15\n", (double)inf.orbit[0]/g_fudge, (double)inf.orbit[1]/g_fudge, (double)inf.orbit[2]/g_fudge);
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
				if ((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
				{
					double yy;
					yy = inf.viewvect[((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) - SOUNDFLAG_X)];
					yy /= g_fudge;
					g_sound_state.tone((int) (yy*100 + g_sound_state.base_hertz()));
				}
				if (oldcol != -1 && s_connect)
				{
					driver_draw_line(inf.col, inf.row, oldcol, oldrow, color % g_colors);
				}
				else
				{
					(*g_plot_color)(inf.col, inf.row, color % g_colors);
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
						driver_draw_line(inf.col1, inf.row1, oldcol1, oldrow1, color % g_colors);
					}
					else
					{
						(*g_plot_color)(inf.col1, inf.row1, color % g_colors);
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


static int orbit_3d_calc_fp()
{
	FILE *fp;
	unsigned long count;
	int oldcol;
	int oldrow;
	int oldcol1;
	int oldrow1;
	int color;
	int ret;
	threed_vt_inf_fp inf;

	/* setup affine screen coord conversion */
	setup_convert_to_screen(&inf.cvt);

	oldcol = oldrow = -1;
	oldcol1 = oldrow1 = -1;
	color = 2;
	if (color >= g_colors)
	{
		color = 1;
	}
	inf.orbit[0] = s_init_orbit_fp[0];
	inf.orbit[1] = s_init_orbit_fp[1];
	inf.orbit[2] = s_init_orbit_fp[2];

	if (driver_diskp())                /* this would KILL a disk drive! */
	{
		not_disk_message();
	}

	fp = open_orbit_save();

	ret = 0;
	g_max_count = (g_max_iteration > 0x1fffffL || g_max_count) ? 0x7fffffffL : g_max_iteration*1024L;
	count = g_color_iter = 0L;
	while (g_color_iter++ <= g_max_count) /* loop until keypress or maxit */
	{
		/* calc goes here */
		if (++count > 1000)
		{        /* time to switch colors? */
			count = 0;
			if (++color >= g_colors)   /* another color to switch to? */
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
				if ((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
				{
					g_sound_state.tone((int) (inf.viewvect[((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) - SOUNDFLAG_X)]*100 + g_sound_state.base_hertz()));
				}
				if (oldcol != -1 && s_connect)
				{
					driver_draw_line(inf.col, inf.row, oldcol, oldrow, color % g_colors);
				}
				else
				{
					(*g_plot_color)(inf.col, inf.row, color % g_colors);
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
						driver_draw_line(inf.col1, inf.row1, oldcol1, oldrow1, color % g_colors);
					}
					else
					{
						(*g_plot_color)(inf.col1, inf.row1, color % g_colors);
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
	s_connect = false;
	s_euler = false;
	s_d = g_parameters[0]; /* number of intervals */
	if (s_d < 0)
	{
		s_d = -s_d;
		s_connect = true;
	}
	else if (s_d == 0)
	{
		s_d = 1;
	}
	if (g_fractal_type == FRACTYPE_DYNAMIC_FP)
	{
		s_a = g_parameters[2]; /* parameter */
		s_b = g_parameters[3]; /* parameter */
		s_dt = g_parameters[1]; /* step size */
		if (s_dt < 0)
		{
			s_dt = -s_dt;
			s_euler = true;
		}
		if (s_dt == 0)
		{
			s_dt = 0.01;
		}
	}
	if (g_outside == COLORMODE_SUM)
	{
		g_plot_color = plot_color_histogram;
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
	double x;
	double y;
	double z;
	int color;
	int col;
	int row;
	long count;
	int oldrow;
	int oldcol;
	double *p0;
	double *p1;
	affine cvt;
	int ret;
	int xstep;
	int ystep; /* The starting position step number */
	double xpixel;
	double ypixel; /* Our pixel position on the screen */

	fp = open_orbit_save();
	/* setup affine screen coord conversion */
	setup_convert_to_screen(&cvt);

	p0 = &x;
	p1 = &y;

	count = 0;
	color = (g_inside > 0) ? g_inside : 1;
	if (color >= g_colors)
	{
		color = 1;
	}
	oldcol = oldrow = -1;

	xstep = -1;
	ystep = 0;

	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(count), &count, sizeof(color), &color,
					sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
					sizeof(x), &x, sizeof(y), &y, sizeof(xstep), &xstep,
					sizeof(ystep), &ystep, 0);
		end_resume();
	}

	ret = 0;
	while (true)
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

		xpixel = g_dx_size*(xstep + .5)/s_d;
		ypixel = g_dy_size*(ystep + .5)/s_d;
		x = (double) ((g_escape_time_state.m_grid_fp.x_min() + g_escape_time_state.m_grid_fp.delta_x()*xpixel) + (g_escape_time_state.m_grid_fp.delta_x2()*ypixel));
		y = (double) ((g_escape_time_state.m_grid_fp.y_max()-g_escape_time_state.m_grid_fp.delta_y()*ypixel) + (-g_escape_time_state.m_grid_fp.delta_y2()*xpixel));
		z = 0.0;
		if (g_fractal_type == FRACTYPE_MANDELBROT_CLOUD)
		{
			s_a = x;
			s_b = y;
		}
		oldcol = -1;

		if (++color >= g_colors)   /* another color to switch to? */
		{
			color = 1;    /* (don't use the background color) */
		}

		for (count = 0; count < g_max_iteration; count++)
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
			if (col >= 0 && col < g_x_dots && row >= 0 && row < g_y_dots)
			{
				g_sound_state.orbit(x, y, z);

				if (count >= g_orbit_delay)
				{
					if (oldcol != -1 && s_connect)
					{
						driver_draw_line(col, row, oldcol, oldrow, color % g_colors);
					}
					else if (count > 0 || g_fractal_type != FRACTYPE_MANDELBROT_CLOUD)
					{
						(*g_plot_color)(col, row, color % g_colors);
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

int setup_orbits_to_screen(affine *scrn_cnvt)
{
	double det;
	double xd;
	double yd;

	det = (g_orbit_x_3rd-g_orbit_x_min)*(g_orbit_y_min-g_orbit_y_max) + (g_orbit_y_max-g_orbit_y_3rd)*(g_orbit_x_max-g_orbit_x_min);
	if (det == 0)
	{
		return -1;
	}
	xd = g_dx_size/det;
	scrn_cnvt->a =  xd*(g_orbit_y_max-g_orbit_y_3rd);
	scrn_cnvt->b =  xd*(g_orbit_x_3rd-g_orbit_x_min);
	scrn_cnvt->e = -scrn_cnvt->a*g_orbit_x_min - scrn_cnvt->b*g_orbit_y_max;

	det = (g_orbit_x_3rd-g_orbit_x_max)*(g_orbit_y_min-g_orbit_y_max) + (g_orbit_y_min-g_orbit_y_3rd)*(g_orbit_x_max-g_orbit_x_min);
	if (det == 0)
	{
		return -1;
	}
	yd = g_dy_size/det;
	scrn_cnvt->c =  yd*(g_orbit_y_min-g_orbit_y_3rd);
	scrn_cnvt->d =  yd*(g_orbit_x_3rd-g_orbit_x_max);
	scrn_cnvt->f = -scrn_cnvt->c*g_orbit_x_min - scrn_cnvt->d*g_orbit_y_max;
	return 0;
}

int plot_orbits_2d_setup()
{

#ifndef XFRACT
	if (g_current_fractal_specific->isinteger != 0)
	{
		int tofloat = g_current_fractal_specific->tofloat;
		if (tofloat == FRACTYPE_NO_FRACTAL)
		{
			return -1;
		}
		g_float_flag = true;
		g_user_float_flag = true;
		g_fractal_type = tofloat;
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	}
#endif

	g_fractal_specific[g_fractal_type].per_image();

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

	if (g_orbit_delay >= g_max_iteration) /* make sure we get an image */
	{
		g_orbit_delay = (int) (g_max_iteration - 1);
	}

	s_o_color = 1;

	if (g_outside == COLORMODE_SUM)
	{
		g_plot_color = plot_color_histogram;
	}
	return 1;
}

int plotorbits2dfloat()
{
	int col;
	int row;
	long count;

	if (driver_key_pressed())
	{
		driver_mute();
		alloc_resume(100, 1);
		put_resume(sizeof(s_o_color), &s_o_color, 0);
		return -1;
	}

	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(s_o_color), &s_o_color, 0);
		end_resume();
	}

	if (g_inside > 0)
	{
		s_o_color = g_inside;
	}
	else  /* inside <= 0 */
	{
		s_o_color++;
		if (s_o_color >= g_colors) /* another color to switch to? */
		{
			s_o_color = 1;    /* (don't use the background color) */
		}
	}

	g_fractal_specific[g_fractal_type].per_pixel(); /* initialize the calculations */

	for (count = 0; count < g_max_iteration; count++)
	{
		if (g_fractal_specific[g_fractal_type].orbitcalc() == 1 && g_periodicity_check)
		{
			continue;  /* bailed out, don't plot */
		}

		if (count < g_orbit_delay || count % g_orbit_interval)
		{
			continue;  /* don't plot it */
		}

		/* else count >= g_orbit_delay and we want to plot it */
		col = (int) (s_o_cvt.a*g_new_z.x + s_o_cvt.b*g_new_z.y + s_o_cvt.e);
		row = (int) (s_o_cvt.c*g_new_z.x + s_o_cvt.d*g_new_z.y + s_o_cvt.f);
		if (col >= 0 && col < g_x_dots && row >= 0 && row < g_y_dots)
		{             /* plot if on the screen */
			(*g_plot_color)(col, row, s_o_color % g_colors);
		}
		else
		{             /* off screen, don't continue unless periodicity=0 */
			if (g_periodicity_check)
			{
				return 0; /* skip to next pixel */
			}
		}
	}
	return 0;
}

/* this function's only purpose is to manage funnyglasses related */
/* stuff so the code is not duplicated for ifs_3d() and lorenz3d() */
int funny_glasses_call(int (*calc)())
{
	int status;
	status = 0;
	g_which_image = g_3d_state.glasses_type() ? WHICHIMAGE_RED : WHICHIMAGE_NONE;
	plot_setup();
	assert(g_plot_color_standard);
	g_plot_color = g_plot_color_standard;
	status = calc();
	if (s_real_time && g_3d_state.glasses_type() < STEREO_PHOTO)
	{
		s_real_time = 0;
		goto done;
	}
	if (g_3d_state.glasses_type() && status == 0 && g_display_3d)
	{
		if (g_3d_state.glasses_type() == STEREO_PHOTO)   /* photographer's mode */
		{
				int i;
				stop_message(STOPMSG_INFO_ONLY,
				"First image (left eye) is ready.  Hit any key to see it, \n"
				"then hit <s> to save, hit any other key to create second image.");
				for (i = driver_get_key(); i == 's' || i == 'S'; i = driver_get_key())
				{
					save_to_disk(g_save_name);
				}
				/* is there a better way to clear the screen in graphics mode? */
				driver_set_video_mode(g_video_entry);
		}
		g_which_image = WHICHIMAGE_BLUE;
		if (g_current_fractal_specific->flags & FRACTALFLAG_INFINITE_CALCULATION)
		{
			g_current_fractal_specific->per_image(); /* reset for 2nd image */
		}
		plot_setup();
		assert(g_plot_color_standard);
		g_plot_color = g_plot_color_standard;
		/* is there a better way to clear the graphics screen ? */
		status = calc();
		if (status != 0)
		{
			goto done;
		}
		if (g_3d_state.glasses_type() == STEREO_PHOTO) /* photographer's mode */
		{
			stop_message(STOPMSG_INFO_ONLY, "Second image (right eye) is ready");
		}
	}
done:
	if (g_3d_state.glasses_type() == STEREO_PAIR && g_screen_width >= 2*g_x_dots)
	{
		/* turn off view windows so will save properly */
		g_sx_offset = g_sy_offset = 0;
		g_x_dots = g_screen_width;
		g_y_dots = g_screen_height;
		g_view_window = false;
	}
	return status;
}

/* double version - mainly for testing */
static int ifs_3d_float()
{
	int color_method;
	FILE *fp;
	int color;

	double newx;
	double newy;
	double newz;
	double r;
	double sum;

	int k;
	int ret;

	threed_vt_inf_fp inf;

	float *ffptr;

	/* setup affine screen coord conversion */
	setup_convert_to_screen(&inf.cvt);
	srand(1);
	color_method = (int) g_parameters[0];
	if (driver_diskp())                /* this would KILL a disk drive! */
	{
		not_disk_message();
	}

	inf.orbit[0] = 0;
	inf.orbit[1] = 0;
	inf.orbit[2] = 0;

	fp = open_orbit_save();

	ret = 0;
	g_max_count = (g_max_iteration > 0x1fffffL) ? 0x7fffffffL : g_max_iteration*1024;

	g_color_iter = 0L;
	while (g_color_iter++ <= g_max_count) /* loop until keypress or maxit */
	{
		if (driver_key_pressed())  /* keypress bails out */
		{
			ret = -1;
			break;
		}
		r = rand();      /* generate a random number between 0 and 1 */
		r /= RAND_MAX;

		/* pick which iterated function to execute, weighted by probability */
		sum = g_ifs_definition[12]; /* [0][12] */
		k = 0;
		while (sum < r && ++k < g_num_affine*IFS3DPARM)
		{
			sum += g_ifs_definition[k*IFS3DPARM + 12];
			if (g_ifs_definition[(k + 1)*IFS3DPARM + 12] == 0) /* for safety  */
			{
				break;
			}
		}

		/* calculate image of last point under selected iterated function */
		ffptr = g_ifs_definition + k*IFS3DPARM; /* point to first parm in row */
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
					color = (k % g_colors) + 1;
				}
				else
				{
					color = getcolor(inf.col, inf.row) + 1;
				}
				if (color < g_colors) /* color sticks on last value */
				{
					(*g_plot_color)(inf.col, inf.row, color);
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
						color = (k % g_colors) + 1;
					}
					else
					{
						color = getcolor(inf.col1, inf.row1) + 1;
					}
					if (color < g_colors) /* color sticks on last value */
					{
						(*g_plot_color)(inf.col1, inf.row1, color);
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
	if (g_ifs_definition == NULL && ifs_load() < 0)
	{
		return -1;
	}
	if (driver_diskp())                /* this would KILL a disk drive! */
	{
		not_disk_message();
	}
	return (g_ifs_type == IFSTYPE_2D) ? ifs_2d() : ifs_3d();
}


/* IFS logic shamelessly converted to integer math */
static int ifs_2d()
{
	int color_method;
	FILE *fp;
	int col;
	int row;
	int color;
	int ret;
	long *localifs;
	long *lfptr;
	long x;
	long y;
	long newx;
	long newy;
	long r;
	long sum;
	long tempr;

	int i;
	int j;
	int k;
	l_affine cvt;
	/* setup affine screen coord conversion */
	l_setup_convert_to_screen(&cvt);

	srand(1);
	color_method = (int) g_parameters[0];
	localifs = (long *) malloc(g_num_affine*IFSPARM*sizeof(long));
	if (localifs == NULL)
	{
		stop_message(0, g_insufficient_ifs_memory);
		return -1;
	}

	for (i = 0; i < g_num_affine; i++)    /* fill in the local IFS array */
	{
		for (j = 0; j < IFSPARM; j++)
		{
			localifs[i*IFSPARM + j] = (long) (g_ifs_definition[i*IFSPARM + j]*g_fudge);
		}
	}

	tempr = g_fudge / 32767;        /* find the proper rand() g_fudge */

	fp = open_orbit_save();

	x = y = 0;
	ret = 0;
	g_max_count = (g_max_iteration > 0x1fffffL) ? 0x7fffffffL : g_max_iteration*1024L;
	g_color_iter = 0L;
	while (g_color_iter++ <= g_max_count) /* loop until keypress or maxit */
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
		while (sum < r && k < g_num_affine-1) /* fixed bug of error if sum < 1 */
		{
			sum += localifs[++k*IFSPARM + 6];
		}
		/* calculate image of last point under selected iterated function */
		lfptr = localifs + k*IFSPARM; /* point to first parm in row */
		newx = multiply(lfptr[0], x, g_bit_shift) +
				multiply(lfptr[1], y, g_bit_shift) + lfptr[4];
		newy = multiply(lfptr[2], x, g_bit_shift) +
				multiply(lfptr[3], y, g_bit_shift) + lfptr[5];
		x = newx;
		y = newy;
		if (fp)
		{
			fprintf(fp, "%g %g %g 15\n", (double)newx/g_fudge, (double)newy/g_fudge, 0.0);
		}

		/* plot if inside window */
		col = (int) ((multiply(cvt.a, x, g_bit_shift) + multiply(cvt.b, y, g_bit_shift) + cvt.e) >> g_bit_shift);
		row = (int) ((multiply(cvt.c, x, g_bit_shift) + multiply(cvt.d, y, g_bit_shift) + cvt.f) >> g_bit_shift);
		if (col >= 0 && col < g_x_dots && row >= 0 && row < g_y_dots)
		{
			/* color is count of hits on this pixel */
			if (color_method)
			{
				color = (k % g_colors) + 1;
			}
			else
			{
				color = getcolor(col, row) + 1;
			}
			if (color < g_colors) /* color sticks on last value */
			{
				(*g_plot_color)(col, row, color);
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

static int ifs_3d_long()
{
	int color_method;
	FILE *fp;
	int color;
	int ret;

	long *localifs;
	long *lfptr;
	long newx;
	long newy;
	long newz;
	long r;
	long sum;
	long tempr;

	int i;
	int j;
	int k;

	threed_vt_inf inf;
	srand(1);
	color_method = (int) g_parameters[0];
	localifs = (long *) malloc(g_num_affine*IFS3DPARM*sizeof(long));
	if (localifs == NULL)
	{
		stop_message(0, g_insufficient_ifs_memory);
		return -1;
	}

	/* setup affine screen coord conversion */
	l_setup_convert_to_screen(&inf.cvt);

	for (i = 0; i < g_num_affine; i++)    /* fill in the local IFS array */
	{
		for (j = 0; j < IFS3DPARM; j++)
		{
			localifs[i*IFS3DPARM + j] = (long) (g_ifs_definition[i*IFS3DPARM + j]*g_fudge);
		}
	}

	tempr = g_fudge / 32767;        /* find the proper rand() g_fudge */

	inf.orbit[0] = 0;
	inf.orbit[1] = 0;
	inf.orbit[2] = 0;

	fp = open_orbit_save();

	ret = 0;
	g_max_count = (g_max_iteration > 0x1fffffL) ? 0x7fffffffL : g_max_iteration*1024L;
	g_color_iter = 0L;
	while (g_color_iter++ <= g_max_count) /* loop until keypress or maxit */
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
		while (sum < r && ++k < g_num_affine*IFS3DPARM)
		{
			sum += localifs[k*IFS3DPARM + 12];
			if (g_ifs_definition[(k + 1)*IFS3DPARM + 12] == 0) /* for safety  */
			{
				break;
			}
		}

		/* calculate image of last point under selected iterated function */
		lfptr = localifs + k*IFS3DPARM; /* point to first parm in row */

		/* calculate image of last point under selected iterated function */
		newx = multiply(lfptr[0], inf.orbit[0], g_bit_shift) +
				multiply(lfptr[1], inf.orbit[1], g_bit_shift) +
				multiply(lfptr[2], inf.orbit[2], g_bit_shift) + lfptr[9];
		newy = multiply(lfptr[3], inf.orbit[0], g_bit_shift) +
				multiply(lfptr[4], inf.orbit[1], g_bit_shift) +
				multiply(lfptr[5], inf.orbit[2], g_bit_shift) + lfptr[10];
		newz = multiply(lfptr[6], inf.orbit[0], g_bit_shift) +
				multiply(lfptr[7], inf.orbit[1], g_bit_shift) +
				multiply(lfptr[8], inf.orbit[2], g_bit_shift) + lfptr[11];

		inf.orbit[0] = newx;
		inf.orbit[1] = newy;
		inf.orbit[2] = newz;
		if (fp)
		{
			fprintf(fp, "%g %g %g 15\n", (double)newx/g_fudge, (double)newy/g_fudge, (double)newz/g_fudge);
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
					color = (k % g_colors) + 1;
				}
				else
				{
					color = getcolor(inf.col, inf.row) + 1;
				}
				if (color < g_colors) /* color sticks on last value */
				{
					(*g_plot_color)(inf.col, inf.row, color);
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
						color = (k % g_colors) + 1;
					}
					else
					{
						color = getcolor(inf.col1, inf.row1) + 1;
					}
					if (color < g_colors) /* color sticks on last value */
					{
						(*g_plot_color)(inf.col1, inf.row1, color);
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
	const double scale_factor = 57.29577;
	xrot((double) g_3d_state.x_rotation()/scale_factor, doublemat);
	yrot((double) g_3d_state.y_rotation()/scale_factor, doublemat);
	zrot((double) g_3d_state.z_rotation()/scale_factor, doublemat);

	/* apply scale */
/*   scale((double)g_3d_state.x_scale()/100.0, (double)g_3d_state.y_scale()/100.0, (double)ROUGH/100.0, doublemat); */

}

static int orbit_3d_aux(int (*orbit)())
{
	g_display_3d = DISPLAY3D_GENERATED;
	s_real_time = (STEREO_NONE < g_3d_state.glasses_type() && g_3d_state.glasses_type() < STEREO_PHOTO) ? 1 : 0;
	return funny_glasses_call(orbit);
}

int orbit_3d_fp()
{
	return orbit_3d_aux(orbit_3d_calc_fp);
}

int orbit_3d()
{
	return orbit_3d_aux(orbit_3d_calc);
}

static int ifs_3d()
{
	return orbit_3d_aux(g_float_flag ? ifs_3d_float : ifs_3d_long);
}

static int threed_view_trans(threed_vt_inf *inf)
{
	int i;
	int j;
	double tmpx;
	double tmpy;
	double tmpz;
	long tmp;

	if (g_color_iter == 1)  /* initialize on first call */
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
				inf->longmat[i][j] = (long) (inf->doublemat[i][j]*g_fudge);
				if (s_real_time)
				{
					inf->longmat1[i][j] = (long) (inf->doublemat1[i][j]*g_fudge);
				}
			}
		}
	}

	/* 3D VIEWING TRANSFORM */
	longvmult(inf->orbit, inf->longmat, inf->viewvect, g_bit_shift);
	if (s_real_time)
	{
		longvmult(inf->orbit, inf->longmat1, inf->viewvect1, g_bit_shift);
	}

	if (g_color_iter <= s_waste) /* waste this many points to find minz and maxz */
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
		if (g_color_iter == s_waste) /* time to work it out */
		{
			inf->iview[0] = inf->iview[1] = 0L; /* center viewer on origin */

			/* z value of user's eye - should be more negative than extreme
								negative part of image */
			inf->iview[2] = (long) ((inf->minvals[2]-inf->maxvals[2])*(double)g_3d_state.z_viewer()/100.0);

			/* center image on origin */
			tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*g_fudge); /* center x */
			tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*g_fudge); /* center y */

			/* apply perspective shift */
			tmpx += ((double) g_x_shift*g_escape_time_state.m_grid_fp.width())/g_x_dots;
			tmpy += ((double) g_y_shift*g_escape_time_state.m_grid_fp.height())/g_y_dots;
			tmpz = -((double) inf->maxvals[2])/g_fudge;
			trans(tmpx, tmpy, tmpz, inf->doublemat);

			if (s_real_time)
			{
				/* center image on origin */
				tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*g_fudge); /* center x */
				tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*g_fudge); /* center y */

				tmpx += ((double) g_x_shift1*g_escape_time_state.m_grid_fp.width())/g_x_dots;
				tmpy += ((double) g_y_shift1*g_escape_time_state.m_grid_fp.height())/g_y_dots;
				tmpz = -((double) inf->maxvals[2])/g_fudge;
				trans(tmpx, tmpy, tmpz, inf->doublemat1);
			}
			for (i = 0; i < 3; i++)
			{
				g_view[i] = (double)inf->iview[i] / g_fudge;
			}

			/* copy xform matrix to long for for fixed point math */
			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
				{
					inf->longmat[i][j] = (long) (inf->doublemat[i][j]*g_fudge);
					if (s_real_time)
					{
						inf->longmat1[i][j] = (long) (inf->doublemat1[i][j]*g_fudge);
					}
				}
			}
		}
		return 0;
	}

	/* apply perspective if requested */
	if (g_3d_state.z_viewer())
	{
		if ((DEBUGMODE_LORENZ_FLOAT == g_debug_mode) || (g_3d_state.z_viewer() < 100)) /* use float for small persp */
		{
			/* use float perspective calc */
			VECTOR tmpv;
			for (i = 0; i < 3; i++)
			{
				tmpv[i] = (double)inf->viewvect[i] / g_fudge;
			}
			perspective(tmpv);
			for (i = 0; i < 3; i++)
			{
				inf->viewvect[i] = (long) (tmpv[i]*g_fudge);
			}
			if (s_real_time)
			{
				for (i = 0; i < 3; i++)
				{
					tmpv[i] = (double)inf->viewvect1[i] / g_fudge;
				}
				perspective(tmpv);
				for (i = 0; i < 3; i++)
				{
					inf->viewvect1[i] = (long) (tmpv[i]*g_fudge);
				}
			}
		}
		else
		{
			longpersp(inf->viewvect, inf->iview, g_bit_shift);
			if (s_real_time)
			{
				longpersp(inf->viewvect1, inf->iview, g_bit_shift);
			}
		}
	}

	/* work out the screen positions */
	inf->row = (int) (((multiply(inf->cvt.c, inf->viewvect[0], g_bit_shift) +
			multiply(inf->cvt.d, inf->viewvect[1], g_bit_shift) + inf->cvt.f)
			>> g_bit_shift)
		+ g_yy_adjust);
	inf->col = (int) (((multiply(inf->cvt.a, inf->viewvect[0], g_bit_shift) +
			multiply(inf->cvt.b, inf->viewvect[1], g_bit_shift) + inf->cvt.e)
			>> g_bit_shift)
		+ g_xx_adjust);
	if (inf->col < 0 || inf->col >= g_x_dots || inf->row < 0 || inf->row >= g_y_dots)
	{
		inf->col = inf->row =
			((long)abs(inf->col) + (long)abs(inf->row) > BAD_PIXEL)
			? -2 : -1;
	}
	if (s_real_time)
	{
		inf->row1 = (int) (((multiply(inf->cvt.c, inf->viewvect1[0], g_bit_shift) +
						multiply(inf->cvt.d, inf->viewvect1[1], g_bit_shift) +
						inf->cvt.f) >> g_bit_shift)
						+ g_yy_adjust1);
		inf->col1 = (int) (((multiply(inf->cvt.a, inf->viewvect1[0], g_bit_shift) +
						multiply(inf->cvt.b, inf->viewvect1[1], g_bit_shift) +
						inf->cvt.e) >> g_bit_shift)
						+ g_xx_adjust1);
		if (inf->col1 < 0 || inf->col1 >= g_x_dots || inf->row1 < 0 || inf->row1 >= g_y_dots)
		{
			inf->col1 = inf->row1 =
				((long)abs(inf->col1) + (long)abs(inf->row1) > BAD_PIXEL)
				? -2 : -1;
		}
	}
	return 1;
}

static int threed_view_trans_fp(threed_vt_inf_fp *inf)
{
	int i;
	double tmpx;
	double tmpy;
	double tmpz;
	double tmp;

	if (g_color_iter == 1)  /* initialize on first call */
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

	if (g_color_iter <= s_waste) /* waste this many points to find minz and maxz */
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
		if (g_color_iter == s_waste) /* time to work it out */
		{
			g_view[0] = g_view[1] = 0; /* center on origin */
			/* z value of user's eye - should be more negative than extreme
									negative part of image */
			g_view[2] = (inf->minvals[2]-inf->maxvals[2])*(double)g_3d_state.z_viewer()/100.0;

			/* center image on origin */
			tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); /* center x */
			tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); /* center y */

			/* apply perspective shift */
			tmpx += ((double) g_x_shift*g_escape_time_state.m_grid_fp.width())/g_x_dots;
			tmpy += ((double) g_y_shift*g_escape_time_state.m_grid_fp.height())/g_y_dots;
			tmpz = -(inf->maxvals[2]);
			trans(tmpx, tmpy, tmpz, inf->doublemat);

			if (s_real_time)
			{
				/* center image on origin */
				tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); /* center x */
				tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); /* center y */

				tmpx += ((double) g_x_shift1*g_escape_time_state.m_grid_fp.width())/g_x_dots;
				tmpy += ((double) g_y_shift1*g_escape_time_state.m_grid_fp.height())/g_y_dots;
				tmpz = -(inf->maxvals[2]);
				trans(tmpx, tmpy, tmpz, inf->doublemat1);
				}
			}
		return 0;
		}

	/* apply perspective if requested */
	if (g_3d_state.z_viewer())
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
	if (inf->col < 0 || inf->col >= g_x_dots || inf->row < 0 || inf->row >= g_y_dots)
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
		if (inf->col1 < 0 || inf->col1 >= g_x_dots || inf->row1 < 0 || inf->row1 >= g_y_dots)
		{
			inf->col1 = inf->row1 =
				((long)abs(inf->col1) + (long)abs(inf->row1) > BAD_PIXEL)
				? -2 : -1;
		}
	}
	return 1;
}

static FILE *open_orbit_save()
{
	FILE *fp = NULL;
	if (g_orbit_save & ORBITSAVE_RAW)
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
static void _fastcall plot_color_histogram(int x, int y, int color)
{
	color = getcolor(x, y) + 1;
	if (color >= g_colors)
	{
		color = 1;
	}
	g_plot_color_put_color(x, y, color);
}
