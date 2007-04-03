/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "drivers.h"

/* these need to be accessed elsewhere for saving data */
double g_m_x_min_fp = -.83;
double g_m_y_min_fp = -.25;
double g_m_x_max_fp = -.83;
double g_m_y_max_fp =  .25;

static long s_m_x_min, s_m_y_min;
static long s_x_per_inch, s_y_per_inch, s_inch_per_x_dot, s_inch_per_y_dot;
static double s_x_per_inch_fp, s_y_per_inch_fp, s_inch_per_x_dot_fp, s_inch_per_y_dot_fp;
static int s_b_base;
static long s_x_pixel, s_y_pixel;
static double s_x_pixel_fp, s_y_pixel_fp;
static long s_init_z, s_djx, s_djy, s_dmx, s_dmy;
static double s_init_z_fp, s_djx_fp, s_djy_fp, s_dmx_fp, s_dmy_fp;
static long s_jx, s_jy, s_mx, s_my, s_x_offset, s_y_offset;
static double s_jx_fp, s_jy_fp, s_mx_fp, s_my_fp, s_x_offset_fp, s_y_offset_fp;

struct Perspective
{
	long x, y, zx, zy;
};

struct Perspectivefp
{
	double x, y, zx, zy;
};

static struct Perspective LeftEye, RightEye, *Per;
static struct Perspectivefp LeftEyefp, RightEyefp, *Perfp;

static _LCMPLX jbc;
static _CMPLX jbcfp;

#ifndef XFRACT
static double fg, fg16;
#endif
int g_z_dots = 128;

float originfp  = 8.0f;
float heightfp  = 7.0f;
float widthfp   = 10.0f;
float distfp    = 24.0f;
float eyesfp    = 2.5f;
float depthfp   = 8.0f;
float brratiofp = 1.0f;
static long width, dist, depth, brratio;
#ifndef XFRACT
static long eyes;
#endif
int juli3Dmode = JULI3DMODE_MONOCULAR;

int neworbittype = JULIA;

int
JulibrotSetup(void)
{
#ifndef XFRACT
	long origin;
#endif
	int r = 0;
	char *mapname;

#ifndef XFRACT
	if (colors < 255)
	{
		stopmsg(0, "Sorry, but Julibrots require a 256-color video mode");
		return 0;
	}
#endif

	s_x_offset_fp = (xxmax + xxmin) / 2;     /* Calculate average */
	s_y_offset_fp = (yymax + yymin) / 2;     /* Calculate average */
	s_dmx_fp = (g_m_x_max_fp - g_m_x_min_fp) / g_z_dots;
	s_dmy_fp = (g_m_y_max_fp - g_m_y_min_fp) / g_z_dots;
	floatparm = &jbcfp;
	s_x_per_inch_fp = (xxmin - xxmax) / widthfp;
	s_y_per_inch_fp = (yymax - yymin) / heightfp;
	s_inch_per_x_dot_fp = widthfp / xdots;
	s_inch_per_y_dot_fp = heightfp / ydots;
	s_init_z_fp = originfp - (depthfp / 2);
	RightEyefp.x = (juli3Dmode == JULI3DMODE_MONOCULAR) ? 0.0 : (eyesfp / 2);
	LeftEyefp.x = -RightEyefp.x;
	LeftEyefp.y = RightEyefp.y = 0;
	LeftEyefp.zx = RightEyefp.zx = distfp;
	LeftEyefp.zy = RightEyefp.zy = distfp;
	s_b_base = 128;

#ifndef XFRACT
	if (fractalspecific[fractype].isinteger > 0)
	{
		long jxmin, jxmax, jymin, jymax, mxmax, mymax;
		if (fractalspecific[neworbittype].isinteger == 0)
		{
			stopmsg(0, "Julibrot orbit type isinteger mismatch");
		}
		if (fractalspecific[neworbittype].isinteger > 1)
		{
			bitshift = fractalspecific[neworbittype].isinteger;
		}
		fg = (double) (1L << bitshift);
		fg16 = (double) (1L << 16);
		jxmin = (long) (xxmin*fg);
		jxmax = (long) (xxmax*fg);
		s_x_offset = (jxmax + jxmin) / 2;    /* Calculate average */
		jymin = (long) (yymin*fg);
		jymax = (long) (yymax*fg);
		s_y_offset = (jymax + jymin) / 2;    /* Calculate average */
		s_m_x_min = (long) (g_m_x_min_fp*fg);
		mxmax = (long) (g_m_x_max_fp*fg);
		s_m_y_min = (long) (g_m_y_min_fp*fg);
		mymax = (long) (g_m_y_max_fp*fg);
		origin = (long) (originfp*fg16);
		depth = (long) (depthfp*fg16);
		width = (long) (widthfp*fg16);
		dist = (long) (distfp*fg16);
		eyes = (long) (eyesfp*fg16);
		brratio = (long) (brratiofp*fg16);
		s_dmx = (mxmax - s_m_x_min) / g_z_dots;
		s_dmy = (mymax - s_m_y_min) / g_z_dots;
		longparm = &jbc;

		s_x_per_inch = (long) ((xxmin - xxmax) / widthfp*fg);
		s_y_per_inch = (long) ((yymax - yymin) / heightfp*fg);
		s_inch_per_x_dot = (long) ((widthfp / xdots)*fg16);
		s_inch_per_y_dot = (long) ((heightfp / ydots)*fg16);
		s_init_z = origin - (depth / 2);
		RightEye.x = (juli3Dmode == JULI3DMODE_MONOCULAR) ? 0L : (eyes/2);
		LeftEye.x = -RightEye.x;
		LeftEye.y = RightEye.y = 0l;
		LeftEye.zx = RightEye.zx = dist;
		LeftEye.zy = RightEye.zy = dist;
		s_b_base = (int) (128.0*brratiofp);
	}
#endif

	if (juli3Dmode == 3)
	{
		savedac = 0;
		mapname = Glasses1Map;
	}
	else
	{
		mapname = GreyFile;
	}
	if (savedac != 1)
	{
		if (ValidateLuts(mapname) != 0)
		{
			return 0;
		}
		spindac(0, 1);               /* load it, but don't spin */
		if (savedac == 2)
		{
			savedac = 1;
		}
	}
	return r >= 0;
}


int
jb_per_pixel(void)
{
	s_jx = multiply(Per->x - s_x_pixel, s_init_z, 16);
	s_jx = divide(s_jx, dist, 16) - s_x_pixel;
	s_jx = multiply(s_jx << (bitshift - 16), s_x_per_inch, bitshift);
	s_jx += s_x_offset;
	s_djx = divide(depth, dist, 16);
	s_djx = multiply(s_djx, Per->x - s_x_pixel, 16) << (bitshift - 16);
	s_djx = multiply(s_djx, s_x_per_inch, bitshift) / g_z_dots;

	s_jy = multiply(Per->y - s_y_pixel, s_init_z, 16);
	s_jy = divide(s_jy, dist, 16) - s_y_pixel;
	s_jy = multiply(s_jy << (bitshift - 16), s_y_per_inch, bitshift);
	s_jy += s_y_offset;
	s_djy = divide(depth, dist, 16);
	s_djy = multiply(s_djy, Per->y - s_y_pixel, 16) << (bitshift - 16);
	s_djy = multiply(s_djy, s_y_per_inch, bitshift) / g_z_dots;

	return 1;
}

int
jbfp_per_pixel(void)
{
	s_jx_fp = ((Perfp->x - s_x_pixel_fp)*s_init_z_fp / distfp - s_x_pixel_fp)*s_x_per_inch_fp;
	s_jx_fp += s_x_offset_fp;
	s_djx_fp = (depthfp / distfp)*(Perfp->x - s_x_pixel_fp)*s_x_per_inch_fp / g_z_dots;

	s_jy_fp = ((Perfp->y - s_y_pixel_fp)*s_init_z_fp / distfp - s_y_pixel_fp)*s_y_per_inch_fp;
	s_jy_fp += s_y_offset_fp;
	s_djy_fp = depthfp / distfp*(Perfp->y - s_y_pixel_fp)*s_y_per_inch_fp / g_z_dots;

	return 1;
}

static int zpixel, plotted;
static long n;

int
zline(long x, long y)
{
	s_x_pixel = x;
	s_y_pixel = y;
	s_mx = s_m_x_min;
	s_my = s_m_y_min;
	switch (juli3Dmode)
	{
	case JULI3DMODE_MONOCULAR:
	case JULI3DMODE_LEFT_EYE:
		Per = &LeftEye;
		break;
	case JULI3DMODE_RIGHT_EYE:
		Per = &RightEye;
		break;
	case JULI3DMODE_RED_BLUE:
		Per = ((row + col) & 1) ? &LeftEye : &RightEye;
		break;
	}
	jb_per_pixel();
	for (zpixel = 0; zpixel < g_z_dots; zpixel++)
	{
		lold.x = s_jx;
		lold.y = s_jy;
		jbc.x = s_mx;
		jbc.y = s_my;
		if (driver_key_pressed())
		{
			return -1;
		}
		ltempsqrx = multiply(lold.x, lold.x, bitshift);
		ltempsqry = multiply(lold.y, lold.y, bitshift);
		for (n = 0; n < maxit; n++)
		{
			if (fractalspecific[neworbittype].orbitcalc())
			{
				break;
			}
		}
		if (n == maxit)
		{
			if (juli3Dmode == 3)
			{
				color = (int) (128l*zpixel / g_z_dots);
				if ((row + col) & 1)
				{

					(*plot) (col, row, 127 - color);
				}
				else
				{
					color = (int) (multiply((long) color << 16, brratio, 16) >> 16);
					if (color < 1)
					{
						color = 1;
					}
					if (color > 127)
					{
						color = 127;
					}
					(*plot) (col, row, 127 + s_b_base - color);
				}
			}
			else
			{
				color = (int) (254l*zpixel / g_z_dots);
				(*plot) (col, row, color + 1);
			}
			plotted = 1;
			break;
		}
		s_mx += s_dmx;
		s_my += s_dmy;
		s_jx += s_djx;
		s_jy += s_djy;
	}
	return 0;
}

int
zlinefp(double x, double y)
{
#ifdef XFRACT
	static int keychk = 0;
#endif
	s_x_pixel_fp = x;
	s_y_pixel_fp = y;
	s_mx_fp = g_m_x_min_fp;
	s_my_fp = g_m_y_min_fp;
	switch (juli3Dmode)
	{
	case JULI3DMODE_MONOCULAR:
	case JULI3DMODE_LEFT_EYE:
		Perfp = &LeftEyefp;
		break;
	case JULI3DMODE_RIGHT_EYE:
		Perfp = &RightEyefp;
		break;
	case JULI3DMODE_RED_BLUE:
		Perfp = ((row + col) & 1) ? &LeftEyefp : &RightEyefp;
		break;
	}
	jbfp_per_pixel();
	for (zpixel = 0; zpixel < g_z_dots; zpixel++)
	{
		/* Special initialization for Mandelbrot types */
		if ((neworbittype == QUATFP || neworbittype == HYPERCMPLXFP)
			&& save_release > 2002)
		{
			old.x = 0.0;
			old.y = 0.0;
			jbcfp.x = 0.0;
			jbcfp.y = 0.0;
			qc = s_jx_fp;
			qci = s_jy_fp;
			qcj = s_mx_fp;
			qck = s_my_fp;
		}
		else
		{
			old.x = s_jx_fp;
			old.y = s_jy_fp;
			jbcfp.x = s_mx_fp;
			jbcfp.y = s_my_fp;
			qc = param[0];
			qci = param[1];
			qcj = param[2];
			qck = param[3];
		}
#ifdef XFRACT
		if (keychk++ > 500)
		{
			keychk = 0;
			if (driver_key_pressed())
			{
				return -1;
			}
		}
#else
		if (driver_key_pressed())
		{
			return -1;
		}
#endif
		tempsqrx = sqr(old.x);
		tempsqry = sqr(old.y);

		for (n = 0; n < maxit; n++)
		{
			if (fractalspecific[neworbittype].orbitcalc())
			{
				break;
			}
		}
		if (n == maxit)
		{
			if (juli3Dmode == 3)
			{
				color = (int) (128l*zpixel / g_z_dots);
				if ((row + col) & 1)
				{
					(*plot) (col, row, 127 - color);
				}
				else
				{
					color = (int)(color*brratiofp);
					if (color < 1)
					{
						color = 1;
					}
					if (color > 127)
					{
						color = 127;
					}
					(*plot) (col, row, 127 + s_b_base - color);
				}
			}
			else
			{
				color = (int) (254l*zpixel / g_z_dots);
				(*plot) (col, row, color + 1);
			}
			plotted = 1;
			break;
		}
		s_mx_fp += s_dmx_fp;
		s_my_fp += s_dmy_fp;
		s_jx_fp += s_djx_fp;
		s_jy_fp += s_djy_fp;
	}
	return 0;
}

int
Std4dFractal(void)
{
	long x, y;
	int xdot, ydot;
	c_exp = (int)param[2];
	if (neworbittype == LJULIAZPOWER)
	{
		if (c_exp < 1)
		{
			c_exp = 1;
		}
		fractalspecific[neworbittype].orbitcalc =
			(param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)c_exp == param[2])
			? longZpowerFractal : longCmplxZpowerFractal;
	}

	for (y = 0, ydot = (ydots >> 1) - 1; ydot >= 0; ydot--, y -= s_inch_per_y_dot)
	{
		plotted = 0;
		x = -(width >> 1);
		for (xdot = 0; xdot < xdots; xdot++, x += s_inch_per_x_dot)
		{
			col = xdot;
			row = ydot;
			if (zline(x, y) < 0)
			{
				return -1;
			}
			col = xdots - col - 1;
			row = ydots - row - 1;
			if (zline(-x, -y) < 0)
			{
				return -1;
			}
		}
		if (plotted == 0)
		{
			if (y == 0)
			{
				plotted = -1;  /* no points first pass; don't give up */
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}
int
Std4dfpFractal(void)
{
	double x, y;
	int xdot, ydot;
	c_exp = (int)param[2];

	if (neworbittype == FPJULIAZPOWER)
	{
		fractalspecific[neworbittype].orbitcalc =
			(param[3] == 0.0 && debugflag != DEBUGFLAG_UNOPT_POWER && (double)c_exp == param[2])
			? floatZpowerFractal : floatCmplxZpowerFractal;
		get_julia_attractor (param[0], param[1]); /* another attractor? */
	}

	for (y = 0, ydot = (ydots >> 1) - 1; ydot >= 0; ydot--, y -= s_inch_per_y_dot_fp)
	{
		plotted = 0;
		x = -widthfp / 2;
		for (xdot = 0; xdot < xdots; xdot++, x += s_inch_per_x_dot_fp)
		{
			col = xdot;
			row = ydot;
			if (zlinefp(x, y) < 0)
			{
				return -1;
			}
			col = xdots - col - 1;
			row = ydots - row - 1;
			if (zlinefp(-x, -y) < 0)
			{
				return -1;
			}
		}
		if (plotted == 0)
		{
			if (y == 0)
			{
				plotted = -1;  /* no points first pass; don't give up */
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}
