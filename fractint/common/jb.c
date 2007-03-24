/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "drivers.h"

/* these need to be accessed elsewhere for saving data */
double mxminfp = -.83;
double myminfp = -.25;
double mxmaxfp = -.83;
double mymaxfp =  .25;

static long mxmin, mymin;
static long x_per_inch, y_per_inch, inch_per_xdot, inch_per_ydot;
static double x_per_inchfp, y_per_inchfp, inch_per_xdotfp, inch_per_ydotfp;
static int bbase;
static long xpixel, ypixel;
static double xpixelfp, ypixelfp;
static long initz, djx, djy, dmx, dmy;
static double initzfp, djxfp, djyfp, dmxfp, dmyfp;
static long jx, jy, mx, my, xoffset, yoffset;
static double jxfp, jyfp, mxfp, myfp, xoffsetfp, yoffsetfp;

struct Perspective
{
	long x, y, zx, zy;
};

struct Perspectivefp
{
	double x, y, zx, zy;
};

struct Perspective LeftEye, RightEye, *Per;
struct Perspectivefp LeftEyefp, RightEyefp, *Perfp;

_LCMPLX jbc;
_CMPLX jbcfp;

#ifndef XFRACT
static double fg, fg16;
#endif
int zdots = 128;

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
int juli3Dmode = 0;

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

	xoffsetfp = (xxmax + xxmin) / 2;     /* Calculate average */
	yoffsetfp = (yymax + yymin) / 2;     /* Calculate average */
	dmxfp = (mxmaxfp - mxminfp) / zdots;
	dmyfp = (mymaxfp - myminfp) / zdots;
	floatparm = &jbcfp;
	x_per_inchfp = (xxmin - xxmax) / widthfp;
	y_per_inchfp = (yymax - yymin) / heightfp;
	inch_per_xdotfp = widthfp / xdots;
	inch_per_ydotfp = heightfp / ydots;
	initzfp = originfp - (depthfp / 2);
	if (juli3Dmode == 0)
		RightEyefp.x = 0.0;
	else
		RightEyefp.x = eyesfp / 2;
	LeftEyefp.x = -RightEyefp.x;
	LeftEyefp.y = RightEyefp.y = 0;
	LeftEyefp.zx = RightEyefp.zx = distfp;
	LeftEyefp.zy = RightEyefp.zy = distfp;
	bbase = 128;

#ifndef XFRACT
	if (fractalspecific[fractype].isinteger > 0)
	{
		long jxmin, jxmax, jymin, jymax, mxmax, mymax;
		if (fractalspecific[neworbittype].isinteger == 0)
		{
			stopmsg(0, "Julibrot orbit type isinteger mismatch");
		}
		if (fractalspecific[neworbittype].isinteger > 1)
			bitshift = fractalspecific[neworbittype].isinteger;
		fg = (double) (1L << bitshift);
		fg16 = (double) (1L << 16);
		jxmin = (long) (xxmin*fg);
		jxmax = (long) (xxmax*fg);
		xoffset = (jxmax + jxmin) / 2;    /* Calculate average */
		jymin = (long) (yymin*fg);
		jymax = (long) (yymax*fg);
		yoffset = (jymax + jymin) / 2;    /* Calculate average */
		mxmin = (long) (mxminfp*fg);
		mxmax = (long) (mxmaxfp*fg);
		mymin = (long) (myminfp*fg);
		mymax = (long) (mymaxfp*fg);
		origin = (long) (originfp*fg16);
		depth = (long) (depthfp*fg16);
		width = (long) (widthfp*fg16);
		dist = (long) (distfp*fg16);
		eyes = (long) (eyesfp*fg16);
		brratio = (long) (brratiofp*fg16);
		dmx = (mxmax - mxmin) / zdots;
		dmy = (mymax - mymin) / zdots;
		longparm = &jbc;

		x_per_inch = (long) ((xxmin - xxmax) / widthfp*fg);
		y_per_inch = (long) ((yymax - yymin) / heightfp*fg);
		inch_per_xdot = (long) ((widthfp / xdots)*fg16);
		inch_per_ydot = (long) ((heightfp / ydots)*fg16);
		initz = origin - (depth / 2);
		if (juli3Dmode == 0)
			RightEye.x = 0l;
		else
			RightEye.x = eyes / 2;
		LeftEye.x = -RightEye.x;
		LeftEye.y = RightEye.y = 0l;
		LeftEye.zx = RightEye.zx = dist;
		LeftEye.zy = RightEye.zy = dist;
		bbase = (int) (128.0*brratiofp);
	}
#endif

	if (juli3Dmode == 3)
	{
		savedac = 0;
		mapname = Glasses1Map;
	}
	else
		mapname = GreyFile;
	if (savedac != 1)
	{
	if (ValidateLuts(mapname) != 0)
		return 0;
	spindac(0, 1);               /* load it, but don't spin */
		if (savedac == 2)
		savedac = 1;
	}
	return r >= 0;
}


int
jb_per_pixel(void)
{
	jx = multiply(Per->x - xpixel, initz, 16);
	jx = divide(jx, dist, 16) - xpixel;
	jx = multiply(jx << (bitshift - 16), x_per_inch, bitshift);
	jx += xoffset;
	djx = divide(depth, dist, 16);
	djx = multiply(djx, Per->x - xpixel, 16) << (bitshift - 16);
	djx = multiply(djx, x_per_inch, bitshift) / zdots;

	jy = multiply(Per->y - ypixel, initz, 16);
	jy = divide(jy, dist, 16) - ypixel;
	jy = multiply(jy << (bitshift - 16), y_per_inch, bitshift);
	jy += yoffset;
	djy = divide(depth, dist, 16);
	djy = multiply(djy, Per->y - ypixel, 16) << (bitshift - 16);
	djy = multiply(djy, y_per_inch, bitshift) / zdots;

	return 1;
}

int
jbfp_per_pixel(void)
{
	jxfp = ((Perfp->x - xpixelfp)*initzfp / distfp - xpixelfp)*x_per_inchfp;
	jxfp += xoffsetfp;
	djxfp = (depthfp / distfp)*(Perfp->x - xpixelfp)*x_per_inchfp / zdots;

	jyfp = ((Perfp->y - ypixelfp)*initzfp / distfp - ypixelfp)*y_per_inchfp;
	jyfp += yoffsetfp;
	djyfp = depthfp / distfp*(Perfp->y - ypixelfp)*y_per_inchfp / zdots;

	return 1;
}

static int zpixel, plotted;
static long n;

int
zline(long x, long y)
{
	xpixel = x;
	ypixel = y;
	mx = mxmin;
	my = mymin;
	switch (juli3Dmode)
	{
	case 0:
	case 1:
		Per = &LeftEye;
		break;
	case 2:
		Per = &RightEye;
		break;
	case 3:
		if ((row + col) & 1)
			Per = &LeftEye;
		else
			Per = &RightEye;
		break;
	}
	jb_per_pixel();
	for (zpixel = 0; zpixel < zdots; zpixel++)
	{
		lold.x = jx;
		lold.y = jy;
		jbc.x = mx;
		jbc.y = my;
		if (driver_key_pressed())
			return -1;
		ltempsqrx = multiply(lold.x, lold.x, bitshift);
		ltempsqry = multiply(lold.y, lold.y, bitshift);
		for (n = 0; n < maxit; n++)
			if (fractalspecific[neworbittype].orbitcalc())
				break;
		if (n == maxit)
		{
			if (juli3Dmode == 3)
			{
				color = (int) (128l*zpixel / zdots);
				if ((row + col) & 1)
				{

					(*plot) (col, row, 127 - color);
				}
				else
				{
					color = (int) (multiply((long) color << 16, brratio, 16) >> 16);
					if (color < 1)
						color = 1;
					if (color > 127)
						color = 127;
					(*plot) (col, row, 127 + bbase - color);
				}
			}
			else
			{
				color = (int) (254l*zpixel / zdots);
				(*plot) (col, row, color + 1);
			}
			plotted = 1;
			break;
		}
		mx += dmx;
		my += dmy;
		jx += djx;
		jy += djy;
	}
	return 0;
}

int
zlinefp(double x, double y)
{
#ifdef XFRACT
	static int keychk = 0;
#endif
	xpixelfp = x;
	ypixelfp = y;
	mxfp = mxminfp;
	myfp = myminfp;
	switch (juli3Dmode)
	{
	case 0:
	case 1:
		Perfp = &LeftEyefp;
		break;
	case 2:
		Perfp = &RightEyefp;
		break;
	case 3:
		if ((row + col) & 1)
			Perfp = &LeftEyefp;
		else
			Perfp = &RightEyefp;
		break;
	}
	jbfp_per_pixel();
	for (zpixel = 0; zpixel < zdots; zpixel++)
	{
		/* Special initialization for Mandelbrot types */
		if ((neworbittype == QUATFP || neworbittype == HYPERCMPLXFP)
          && save_release > 2002)
		{
			old.x = 0.0;
			old.y = 0.0;
			jbcfp.x = 0.0;
			jbcfp.y = 0.0;
			qc = jxfp;
			qci = jyfp;
			qcj = mxfp;
			qck = myfp;
		}
		else
		{
			old.x = jxfp;
			old.y = jyfp;
			jbcfp.x = mxfp;
			jbcfp.y = myfp;
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
				return -1;
		}
#else
		if (driver_key_pressed())
			return -1;
#endif
		tempsqrx = sqr(old.x);
		tempsqry = sqr(old.y);

		for (n = 0; n < maxit; n++)
			if (fractalspecific[neworbittype].orbitcalc())
				break;
		if (n == maxit)
		{
			if (juli3Dmode == 3)
			{
				color = (int) (128l*zpixel / zdots);
				if ((row + col) & 1)
					(*plot) (col, row, 127 - color);
				else
				{
					color = (int)(color*brratiofp);
					if (color < 1)
						color = 1;
					if (color > 127)
						color = 127;
					(*plot) (col, row, 127 + bbase - color);
				}
			}
			else
			{
				color = (int) (254l*zpixel / zdots);
				(*plot) (col, row, color + 1);
			}
			plotted = 1;
			break;
		}
		mxfp += dmxfp;
		myfp += dmyfp;
		jxfp += djxfp;
		jyfp += djyfp;
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
			c_exp = 1;
		if (param[3] == 0.0 && debugflag != 6000 && (double)c_exp == param[2])
          fractalspecific[neworbittype].orbitcalc = longZpowerFractal;
		else
          fractalspecific[neworbittype].orbitcalc = longCmplxZpowerFractal;
	}

	for (y = 0, ydot = (ydots >> 1) - 1; ydot >= 0; ydot--, y -= inch_per_ydot)
	{
		plotted = 0;
		x = -(width >> 1);
		for (xdot = 0; xdot < xdots; xdot++, x += inch_per_xdot)
		{
			col = xdot;
			row = ydot;
			if (zline(x, y) < 0)
				return -1;
			col = xdots - col - 1;
			row = ydots - row - 1;
			if (zline(-x, -y) < 0)
				return -1;
		}
		if (plotted == 0)
		{
			if (y == 0)
           plotted = -1;  /* no points first pass; don't give up */
			else
           break;
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
		if (param[3] == 0.0 && debugflag != 6000 && (double)c_exp == param[2])
          fractalspecific[neworbittype].orbitcalc = floatZpowerFractal;
		else
          fractalspecific[neworbittype].orbitcalc = floatCmplxZpowerFractal;
		get_julia_attractor (param[0], param[1]); /* another attractor? */
	}

	for (y = 0, ydot = (ydots >> 1) - 1; ydot >= 0; ydot--, y -= inch_per_ydotfp)
	{
		plotted = 0;
		x = -widthfp / 2;
		for (xdot = 0; xdot < xdots; xdot++, x += inch_per_xdotfp)
		{
			col = xdot;
			row = ydot;
			if (zlinefp(x, y) < 0)
				return -1;
			col = xdots - col - 1;
			row = ydots - row - 1;
			if (zlinefp(-x, -y) < 0)
				return -1;
		}
		if (plotted == 0)
		{
			if (y == 0)
           plotted = -1;  /* no points first pass; don't give up */
			else
           break;
		}
	}
	return 0;
}
