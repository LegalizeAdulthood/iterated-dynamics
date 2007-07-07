/*
 * soi.c --  SOI
 *
 *	Simultaneous Orbit Iteration Image Generation Method. Computes
 *      rectangular regions by tracking the orbits of only a few key points.
 *
 * Copyright (c) 1994-1997 Michael R. Ganss. All Rights Reserved.
 *
 * This file is distributed under the same conditions as
 * AlmondBread. For further information see
 * <URL:http://www.cs.tu-berlin.de/~rms/AlmondBread>.
 *
 */
#include <time.h>
#include <string.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif

#include "port.h"
#include "prototyp.h"

#include "drivers.h"
#include "soi.h"

#include "EscapeTime.h"

#define EVERY 15
#define BASIN_COLOR 0

extern int rhombus_depth;

static double twidth;
static double equal;

static long iteration(double cr, double ci,
	     	double re, double im,
	     	long start)
{
	g_old_z.x = re;
	g_old_z.y = im;
	g_temp_sqr_x = sqr(g_old_z.x);
	g_temp_sqr_y = sqr(g_old_z.y);
	g_float_parameter = &g_initial_z;
	g_float_parameter->x = cr;
	g_float_parameter->y = ci;
	while (g_fractal_specific[g_fractal_type].orbitcalc() == 0 && start < g_max_iteration)
	{
		start++;
	}
	if (start >= g_max_iteration)
	{
		start = BASIN_COLOR;
	}
	return start;
}

static void put_horizontal_line(int x1, int y1, int x2, int color)
{
	for (int x = x1; x <= x2; x++)
	{
		(*g_plot_color)(x, y1, color);
	}
}

static void put_box(int x1, int y1, int x2, int y2, int color)
{
	for (int y = y1; y <= y2; y++)
	{
		put_horizontal_line(x1, y, x2, color);
	}
}

/* maximum side length beyond which we start regular scanning instead of
	subdividing */
#define SCAN 16

/* pixel interleave used in scanning */
#define INTERLEAVE 4

/* compute the value of the interpolation polynomial at (x, y) */
#define GET_REAL(x, y) \
interpolate(cim1, midi, cim2, \
	interpolate(cre1, midr, cre2, zre1, zre5, zre2, x), \
	interpolate(cre1, midr, cre2, zre6, zre9, zre7, x), \
	interpolate(cre1, midr, cre2, zre3, zre8, zre4, x), y)
#define GET_IMAG(x, y) \
interpolate(cre1, midr, cre2, \
	interpolate(cim1, midi, cim2, zim1, zim6, zim3, y), \
	interpolate(cim1, midi, cim2, zim5, zim9, zim8, y), \
	interpolate(cim1, midi, cim2, zim2, zim7, zim4, y), x)

/* compute the value of the interpolation polynomial at (x, y)
	from saved values before interpolation failed to stay within tolerance */
#define GET_SAVED_REAL(x, y) \
interpolate(cim1, midi, cim2, \
	interpolate(cre1, midr, cre2, sr1, sr5, sr2, x), \
	interpolate(cre1, midr, cre2, sr6, sr9, sr7, x), \
	interpolate(cre1, midr, cre2, sr3, sr8, sr4, x), y)
#define GET_SAVED_IMAG(x, y) \
interpolate(cre1, midr, cre2, \
	interpolate(cim1, midi, cim2, si1, si6, si3, y), \
	interpolate(cim1, midi, cim2, si5, si9, si8, y), \
	interpolate(cim1, midi, cim2, si2, si7, si4, y), x)

/* compute the value of the interpolation polynomial at (x, y)
	during scanning. Here, key values do not change, so we can precompute
	coefficients in one direction and simply evaluate the polynomial
	during scanning. */
#define GET_SCAN_REAL(x, y) \
interpolate(cim1, midi, cim2, \
	EVALUATE(cre1, midr, br10, br11, br12, x), \
	EVALUATE(cre1, midr, br20, br21, br22, x), \
	EVALUATE(cre1, midr, br30, br31, br32, x), y)
#define GET_SCAN_IMAG(x, y) \
interpolate(cre1, midr, cre2, \
	EVALUATE(cim1, midi, bi10, bi11, bi12, y), \
	EVALUATE(cim1, midi, bi20, bi21, bi22, y), \
	EVALUATE(cim1, midi, bi30, bi31, bi32, y), x)

/* compute coefficients of Newton polynomial (b0, .., b2) from
	(x0, w0), .., (x2, w2). */
#define INTERPOLATE(x0, x1, x2, w0, w1, w2, b0, b1, b2) \
	do \
	{ \
		b0 = w0; \
		b1 = (w1-w0)/(x1-x0); \
		b2 = ((w2-w1)/(x2-x1)-b1)/(x2-x0); \
	} \
	while (0)

/* evaluate Newton polynomial given by (x0, b0), (x1, b1) at x:=t */
#define EVALUATE(x0, x1, b0, b1, b2, t) \
	((b2*(t-x1) + b1)*(t-x0) + b0)

/* Newton Interpolation.
	It computes the value of the interpolation polynomial given by
	(x0, w0)..(x2, w2) at x:=t */
static double interpolate(double x0, double x1, double x2,
	double w0, double w1, double w2,
	double t)
{
	double b0 = w0;
	double b1 = w1;
	double b2 = w2;
	/*b0 = (r0*b1-r1*b0)/(x1-x0);
	b1 = (r1*b2-r2*b1)/(x2-x1);
	b0 = (r0*b1-r2*b0)/(x2-x0);

	return (double)b0; */
	double b = (b1-b0)/(x1-x0);
	return (double)((((b2-b1)/(x2-x1)-b)/(x2-x0))*(t-x1) + b)*(t-x0) + b0;
	/*
	if (t < x1)
		return w0 + ((t-x0)/(x1-x0))*(w1-w0);
	else
		return w1 + ((t-x1)/(x2-x1))*(w2-w1); */
}

/* SOICompute - Perform simultaneous orbit iteration for a given rectangle

	Input: cre1..cim2 : values defining the four corners of the rectangle
		x1..y2     : corresponding pixel values
	  zre1..zim9 : intermediate iterated values of the key points (key values)

	  (cre1, cim1)               (cre2, cim1)
	  (zre1, zim1)  (zre5, zim5)  (zre2, zim2)
	       +------------+------------+
	       |            |            |
	       |            |            |
	  (zre6, zim6)  (zre9, zim9)  (zre7, zim7)
	       |            |            |
	       |            |            |
	       +------------+------------+
	  (zre3, zim3)  (zre8, zim8)  (zre4, zim4)
	  (cre1, cim2)               (cre2, cim2)

	  iter       : current number of iterations
	  */
static double zre1;
static double zim1;
static double zre2;
static double zim2;
static double zre3;
static double zim3;
static double zre4;
static double zim4;
static double zre5;
static double zim5;
static double zre6;
static double zim6;
static double zre7;
static double zim7;
static double zre8;
static double zim8;
static double zre9;
static double zim9;
/*
	The purpose of this macro is to reduce the number of parameters of the
	function rhombus(), since this is a recursive function, and stack space
	under DOS is extremely limited.
*/

#define RHOMBUS(CRE1, CRE2, CIM1, CIM2, X1, X2, Y1, Y2, ZRE1, ZIM1, ZRE2, ZIM2, ZRE3, ZIM3, \
				ZRE4, ZIM4, ZRE5, ZIM5, ZRE6, ZIM6, ZRE7, ZIM7, ZRE8, ZIM8, ZRE9, ZIM9, ITER) \
	do \
	{ \
		zre1 = (ZRE1); zim1 = (ZIM1); \
		zre2 = (ZRE2); zim2 = (ZIM2); \
		zre3 = (ZRE3); zim3 = (ZIM3); \
		zre4 = (ZRE4); zim4 = (ZIM4); \
		zre5 = (ZRE5); zim5 = (ZIM5); \
		zre6 = (ZRE6); zim6 = (ZIM6); \
		zre7 = (ZRE7); zim7 = (ZIM7); \
		zre8 = (ZRE8); zim8 = (ZIM8); \
		zre9 = (ZRE9); zim9 = (ZIM9); \
		status = rhombus((CRE1), (CRE2), (CIM1), (CIM2), (X1), (X2), (Y1), (Y2), (ITER)); \
	} \
	while (0)

static int rhombus(double cre1, double cre2, double cim1, double cim2,
	int x1, int x2, int y1, int y2, long iter)
{
	/* The following variables do not need their values saved */
	/* used in scanning */
	static long savecolor;
	static long color;
	static long helpcolor;
	static int x;
	static int y;
	static int z;
	static int savex;
	static double re;
	static double im;
	static double restep;
	static double imstep;
	static double interstep;
	static double helpre;
	static double zre;
	static double zim;
	/* interpolation coefficients */
	static double br10;
	static double br11;
	static double br12;
	static double br20;
	static double br21;
	static double br22;
	static double br30;
	static double br31;
	static double br32;
	static double bi10;
	static double bi11;
	static double bi12;
	static double bi20;
	static double bi21;
	static double bi22;
	static double bi30;
	static double bi31;
	static double bi32;
	/* ratio of interpolated test point to iterated one */
	static double l1;
	static double l2;
	/* squares of key values */
	static double rq1;
	static double iq1;
	static double rq2;
	static double iq2;
	static double rq3;
	static double iq3;
	static double rq4;
	static double iq4;
	static double rq5;
	static double iq5;
	static double rq6;
	static double iq6;
	static double rq7;
	static double iq7;
	static double rq8;
	static double iq8;
	static double rq9;
	static double iq9;

	/* test points */
	static double cr1;
	static double cr2;
	static double ci1;
	static double ci2;
	static double tzr1;
	static double tzi1;
	static double tzr2;
	static double tzi2;
	static double tzr3;
	static double tzi3;
	static double tzr4;
	static double tzi4;
	static double trq1;
	static double tiq1;
	static double trq2;
	static double tiq2;
	static double trq3;
	static double tiq3;
	static double trq4;
	static double tiq4;

	/* center of rectangle */
	double midr = (cre1 + cre2)/2, midi = (cim1 + cim2)/2;

	/* saved values of key values */
	double sr1;
	double si1;
	double sr2;
	double si2;
	double sr3;
	double si3;
	double sr4;
	double si4;
	double sr5;
	double si5;
	double sr6;
	double si6;
	double sr7;
	double si7;
	double sr8;
	double si8;
	double sr9;
	double si9;
	/* key values for subsequent rectangles */
	double re10;
	double re11;
	double re12;
	double re13;
	double re14;
	double re15;
	double re16;
	double re17;
	double re18;
	double re19;
	double re20;
	double re21;
	double im10;
	double im11;
	double im12;
	double im13;
	double im14;
	double im15;
	double im16;
	double im17;
	double im18;
	double im19;
	double im20;
	double im21;
	double re91;
	double re92;
	double re93;
	double re94;
	double im91;
	double im92;
	double im93;
	double im94;
	double esc1;
	double esc2;
	double esc3;
	double esc4;
	double esc5;
	double esc6;
	double esc7;
	double esc8;
	double esc9;
	double tesc1;
	double tesc2;
	double tesc3;
	double tesc4;

	int status = 0;
	rhombus_depth++;

	if (driver_key_pressed())
	{
		status = 1;
		goto rhombus_done;
	}
	if (iter > g_max_iteration)
	{
		put_box(x1, y1, x2, y2, 0);
		status = 0;
		goto rhombus_done;
	}

	if (y2-y1 <= SCAN)
	{
		/* finish up the image by scanning the rectangle */
scan:
		INTERPOLATE(cre1, midr, cre2, zre1, zre5, zre2, br10, br11, br12);
		INTERPOLATE(cre1, midr, cre2, zre6, zre9, zre7, br20, br21, br22);
		INTERPOLATE(cre1, midr, cre2, zre3, zre8, zre4, br30, br31, br32);

		INTERPOLATE(cim1, midi, cim2, zim1, zim6, zim3, bi10, bi11, bi12);
		INTERPOLATE(cim1, midi, cim2, zim5, zim9, zim8, bi20, bi21, bi22);
		INTERPOLATE(cim1, midi, cim2, zim2, zim7, zim4, bi30, bi31, bi32);

		restep = (cre2-cre1)/(x2-x1);
		imstep = (cim2-cim1)/(y2-y1);
		interstep = INTERLEAVE*restep;

		for (y = y1, im = cim1; y < y2; y++, im += imstep)
		{
			if (driver_key_pressed())
			{
				status = 1;
				goto rhombus_done;
			}
			zre = GET_SCAN_REAL(cre1, im);
			zim = GET_SCAN_IMAG(cre1, im);
			savecolor = iteration(cre1, im, zre, zim, iter);
			if (savecolor < 0)
			{
				status = 1;
				goto rhombus_done;
			}
			savex = x1;
			for (x = x1 + INTERLEAVE, re = cre1 + interstep;
				x < x2;
				x += INTERLEAVE, re += interstep)
			{
				zre = GET_SCAN_REAL(re, im);
				zim = GET_SCAN_IMAG(re, im);

				color = iteration(re, im, zre, zim, iter);
				if (color < 0)
				{
					status = 1;
					goto rhombus_done;
				}
				else if (color == savecolor)
				{
					continue;
				}

				for (z = x-1, helpre = re-restep; z > x-INTERLEAVE; z--, helpre -= restep)
				{
					zre = GET_SCAN_REAL(helpre, im);
					zim = GET_SCAN_IMAG(helpre, im);
					helpcolor = iteration(helpre, im, zre, zim, iter);
					if (helpcolor < 0)
					{
						status = 1;
						goto rhombus_done;
					}
					else if (helpcolor == savecolor)
					{
						break;
					}
					(*g_plot_color)(z, y, (int)(helpcolor&255));
				}

				if (savex < z)
				{
					put_horizontal_line(savex, y, z, (int)(savecolor&255));
				}
				else
				{
					(*g_plot_color)(savex, y, (int)(savecolor&255));
				}

				savex = x;
				savecolor = color;
			}

			for (z = x2-1, helpre = cre2-restep; z > savex; z--, helpre -= restep)
			{
				zre = GET_SCAN_REAL(helpre, im);
				zim = GET_SCAN_IMAG(helpre, im);
				helpcolor = iteration(helpre, im, zre, zim, iter);
				if (helpcolor < 0)
				{
					status = 1;
					goto rhombus_done;
				}
				else if (helpcolor == savecolor)
				{
					break;
				}

				(*g_plot_color)(z, y, (int)(helpcolor&255));
			}

			if (savex < z)
			{
				put_horizontal_line(savex, y, z, (int)(savecolor&255));
			}
			else
			{
				(*g_plot_color)(savex, y, (int)(savecolor&255));
			}
		}
		status = 0;
		goto rhombus_done;
	}

	rq1 = zre1*zre1; iq1 = zim1*zim1;
	rq2 = zre2*zre2; iq2 = zim2*zim2;
	rq3 = zre3*zre3; iq3 = zim3*zim3;
	rq4 = zre4*zre4; iq4 = zim4*zim4;
	rq5 = zre5*zre5; iq5 = zim5*zim5;
	rq6 = zre6*zre6; iq6 = zim6*zim6;
	rq7 = zre7*zre7; iq7 = zim7*zim7;
	rq8 = zre8*zre8; iq8 = zim8*zim8;
	rq9 = zre9*zre9; iq9 = zim9*zim9;

	cr1 = 0.75*cre1 + 0.25*cre2; cr2 = 0.25*cre1 + 0.75*cre2;
	ci1 = 0.75*cim1 + 0.25*cim2; ci2 = 0.25*cim1 + 0.75*cim2;

	tzr1 = GET_REAL(cr1, ci1);
	tzi1 = GET_IMAG(cr1, ci1);

	tzr2 = GET_REAL(cr2, ci1);
	tzi2 = GET_IMAG(cr2, ci1);

	tzr3 = GET_REAL(cr1, ci2);
	tzi3 = GET_IMAG(cr1, ci2);

	tzr4 = GET_REAL(cr2, ci2);
	tzi4 = GET_IMAG(cr2, ci2);

	trq1 = tzr1*tzr1;
	tiq1 = tzi1*tzi1;

	trq2 = tzr2*tzr2;
	tiq2 = tzi2*tzi2;

	trq3 = tzr3*tzr3;
	tiq3 = tzi3*tzi3;

	trq4 = tzr4*tzr4;
	tiq4 = tzi4*tzi4;

	static long before;
	before = iter;

	while (true)
	{
		sr1 = zre1; si1 = zim1;
		sr2 = zre2; si2 = zim2;
		sr3 = zre3; si3 = zim3;
		sr4 = zre4; si4 = zim4;
		sr5 = zre5; si5 = zim5;
		sr6 = zre6; si6 = zim6;
		sr7 = zre7; si7 = zim7;
		sr8 = zre8; si8 = zim8;
		sr9 = zre9; si9 = zim9;


#define SOI_ORBIT1(zr, rq, zi, iq, cr, ci, esc)	\
	do											\
	{											\
		g_temp_sqr_x = rq;						\
		g_temp_sqr_y = iq;						\
		old.x = zr;								\
		old.y = zi;								\
		g_float_parameter->x = cr;				\
		g_float_parameter->y = ci;				\
		esc = g_fractal_specific[g_fractal_type].orbitcalc();						\
		rq = g_temp_sqr_x;						\
		iq = g_temp_sqr_y;						\
		zr = g_new_z.x;							\
		zi = g_new_z.y;							\
	}											\
	while (0)

#define SOI_ORBIT(zr, rq, zi, iq, cr, ci, esc)	\
	do											\
	{											\
		zi = (zi + zi)*zr + ci;					\
		zr = rq-iq + cr;						\
		rq = zr*zr;								\
		iq = zi*zi;								\
		esc = ((rq + iq) > 16.0) ? 1 : 0;		\
	}											\
	while (0)

		/* iterate key values */
		SOI_ORBIT(zre1, rq1, zim1, iq1, cre1, cim1, esc1);
		/*
		zim1 = (zim1 + zim1)*zre1 + cim1;
		zre1 = rq1-iq1 + cre1;
		rq1 = zre1*zre1;
		iq1 = zim1*zim1;
		*/
		SOI_ORBIT(zre2, rq2, zim2, iq2, cre2, cim1, esc2);
		/*
		zim2 = (zim2 + zim2)*zre2 + cim1;
		zre2 = rq2-iq2 + cre2;
		rq2 = zre2*zre2;
		iq2 = zim2*zim2;
		*/
		SOI_ORBIT(zre3, rq3, zim3, iq3, cre1, cim2, esc3);
		/*
		zim3 = (zim3 + zim3)*zre3 + cim2;
		zre3 = rq3-iq3 + cre1;
		rq3 = zre3*zre3;
		iq3 = zim3*zim3;
		*/
		SOI_ORBIT(zre4, rq4, zim4, iq4, cre2, cim2, esc4);
		/*
		zim4 = (zim4 + zim4)*zre4 + cim2;
		zre4 = rq4-iq4 + cre2;
		rq4 = zre4*zre4;
		iq4 = zim4*zim4;
		*/
		SOI_ORBIT(zre5, rq5, zim5, iq5, midr, cim1, esc5);
		/*
		zim5 = (zim5 + zim5)*zre5 + cim1;
		zre5 = rq5-iq5 + midr;
		rq5 = zre5*zre5;
		iq5 = zim5*zim5;
		*/
		SOI_ORBIT(zre6, rq6, zim6, iq6, cre1, midi, esc6);
		/*
		zim6 = (zim6 + zim6)*zre6 + midi;
		zre6 = rq6-iq6 + cre1;
		rq6 = zre6*zre6;
		iq6 = zim6*zim6;
		*/
		SOI_ORBIT(zre7, rq7, zim7, iq7, cre2, midi, esc7);
		/*
		zim7 = (zim7 + zim7)*zre7 + midi;
		zre7 = rq7-iq7 + cre2;
		rq7 = zre7*zre7;
		iq7 = zim7*zim7;
		*/
		SOI_ORBIT(zre8, rq8, zim8, iq8, midr, cim2, esc8);
		/*
		zim8 = (zim8 + zim8)*zre8 + cim2;
		zre8 = rq8-iq8 + midr;
		rq8 = zre8*zre8;
		iq8 = zim8*zim8;
		*/
		SOI_ORBIT(zre9, rq9, zim9, iq9, midr, midi, esc9);
		/*
		zim9 = (zim9 + zim9)*zre9 + midi;
		zre9 = rq9-iq9 + midr;
		rq9 = zre9*zre9;
		iq9 = zim9*zim9;
		*/
		/* iterate test point */
		SOI_ORBIT(tzr1, trq1, tzi1, tiq1, cr1, ci1, tesc1);
		/*
		tzi1 = (tzi1 + tzi1)*tzr1 + ci1;
		tzr1 = trq1-tiq1 + cr1;
		trq1 = tzr1*tzr1;
		tiq1 = tzi1*tzi1;
		*/

		SOI_ORBIT(tzr2, trq2, tzi2, tiq2, cr2, ci1, tesc2);
		/*
		tzi2 = (tzi2 + tzi2)*tzr2 + ci1;
		tzr2 = trq2-tiq2 + cr2;
		trq2 = tzr2*tzr2;
		tiq2 = tzi2*tzi2;
		*/
		SOI_ORBIT(tzr3, trq3, tzi3, tiq3, cr1, ci2, tesc3);
		/*
		tzi3 = (tzi3 + tzi3)*tzr3 + ci2;
		tzr3 = trq3-tiq3 + cr1;
		trq3 = tzr3*tzr3;
		tiq3 = tzi3*tzi3;
		*/
		SOI_ORBIT(tzr4, trq4, tzi4, tiq4, cr2, ci2, tesc4);
		/*
		tzi4 = (tzi4 + tzi4)*tzr4 + ci2;
		tzr4 = trq4-tiq4 + cr2;
		trq4 = tzr4*tzr4;
		tiq4 = tzi4*tzi4;
		*/
		iter++;

		/* if one of the iterated values bails out, subdivide */
		/*
		if ((rq1 + iq1) > 16.0||
				(rq2 + iq2) > 16.0||
				(rq3 + iq3) > 16.0||
				(rq4 + iq4) > 16.0||
				(rq5 + iq5) > 16.0||
				(rq6 + iq6) > 16.0||
				(rq7 + iq7) > 16.0||
				(rq8 + iq8) > 16.0||
				(rq9 + iq9) > 16.0||
				(trq1 + tiq1) > 16.0||
				(trq2 + tiq2) > 16.0||
				(trq3 + tiq3) > 16.0||
				(trq4 + tiq4) > 16.0)
			break;
		*/
		if (esc1 || esc2 || esc3 || esc4 || esc5
			|| esc6 || esc7 || esc8 || esc9
			|| tesc1 || tesc2 || tesc3 || tesc4)
		{
			break;
		}

		/* if maximum number of iterations is reached, the whole rectangle
		can be assumed part of M. This is of course best case behavior
		of SOI, we seldomly get there */
		if (iter > g_max_iteration)
		{
			put_box(x1, y1, x2, y2, 0);
			status = 0;
			goto rhombus_done;
		}

		/* now for all test points, check whether they exceed the
		allowed tolerance. if so, subdivide */
		l1 = GET_REAL(cr1, ci1);
		l1 = (tzr1 == 0.0) ?
			((l1 == 0.0) ? 1.0 : 1000.0) : l1/tzr1;
		if (fabs(1.0-l1) > twidth)
		{
			break;
		}

		l2 = GET_IMAG(cr1, ci1);
		l2 = (tzi1 == 0.0) ?
			((l2 == 0.0) ? 1.0 : 1000.0) : l2/tzi1;
		if (fabs(1.0-l2) > twidth)
		{
			break;
		}

		l1 = GET_REAL(cr2, ci1);
		l1 = (tzr2 == 0.0) ?
			((l1 == 0.0) ? 1.0 : 1000.0) : l1/tzr2;
		if (fabs(1.0-l1) > twidth)
		{
			break;
		}

		l2 = GET_IMAG(cr2, ci1);
		l2 = (tzi2 == 0.0) ?
			((l2 == 0.0) ? 1.0 : 1000.0) : l2/tzi2;
		if (fabs(1.0-l2) > twidth)
		{
			break;
		}

		l1 = GET_REAL(cr1, ci2);
		l1 = (tzr3 == 0.0) ?
			((l1 == 0.0) ? 1.0 : 1000.0) : l1/tzr3;
		if (fabs(1.0-l1) > twidth)
		{
			break;
		}

		l2 = GET_IMAG(cr1, ci2);
		l2 = (tzi3 == 0.0) ?
			((l2 == 0.0) ? 1.0 : 1000.0) : l2/tzi3;
		if (fabs(1.0-l2) > twidth)
		{
			break;
		}

		l1 = GET_REAL(cr2, ci2);
		l1 = (tzr4 == 0.0) ?
			((l1 == 0.0) ? 1.0 : 1000.0) : l1/tzr4;
		if (fabs(1.0-l1) > twidth)
		{
			break;
		}

		l2 = GET_IMAG(cr2, ci2);
		l2 = (tzi4 == 0.0) ?
			((l2 == 0.0) ? 1.0 : 1000.0) : l2/tzi4;
		if (fabs(1.0-l2) > twidth)
		{
			break;
		}
	}

	iter--;

	/* this is a little heuristic I tried to improve performance. */
	if (iter-before < 10)
	{
		zre1 = sr1; zim1 = si1;
		zre2 = sr2; zim2 = si2;
		zre3 = sr3; zim3 = si3;
		zre4 = sr4; zim4 = si4;
		zre5 = sr5; zim5 = si5;
		zre6 = sr6; zim6 = si6;
		zre7 = sr7; zim7 = si7;
		zre8 = sr8; zim8 = si8;
		zre9 = sr9; zim9 = si9;
		goto scan;
	}

	/* compute key values for subsequent rectangles */

	re10 = interpolate(cre1, midr, cre2, sr1, sr5, sr2, cr1);
	im10 = interpolate(cre1, midr, cre2, si1, si5, si2, cr1);

	re11 = interpolate(cre1, midr, cre2, sr1, sr5, sr2, cr2);
	im11 = interpolate(cre1, midr, cre2, si1, si5, si2, cr2);

	re20 = interpolate(cre1, midr, cre2, sr3, sr8, sr4, cr1);
	im20 = interpolate(cre1, midr, cre2, si3, si8, si4, cr1);

	re21 = interpolate(cre1, midr, cre2, sr3, sr8, sr4, cr2);
	im21 = interpolate(cre1, midr, cre2, si3, si8, si4, cr2);

	re15 = interpolate(cre1, midr, cre2, sr6, sr9, sr7, cr1);
	im15 = interpolate(cre1, midr, cre2, si6, si9, si7, cr1);

	re16 = interpolate(cre1, midr, cre2, sr6, sr9, sr7, cr2);
	im16 = interpolate(cre1, midr, cre2, si6, si9, si7, cr2);

	re12 = interpolate(cim1, midi, cim2, sr1, sr6, sr3, ci1);
	im12 = interpolate(cim1, midi, cim2, si1, si6, si3, ci1);

	re14 = interpolate(cim1, midi, cim2, sr2, sr7, sr4, ci1);
	im14 = interpolate(cim1, midi, cim2, si2, si7, si4, ci1);

	re17 = interpolate(cim1, midi, cim2, sr1, sr6, sr3, ci2);
	im17 = interpolate(cim1, midi, cim2, si1, si6, si3, ci2);

	re19 = interpolate(cim1, midi, cim2, sr2, sr7, sr4, ci2);
	im19 = interpolate(cim1, midi, cim2, si2, si7, si4, ci2);

	re13 = interpolate(cim1, midi, cim2, sr5, sr9, sr8, ci1);
	im13 = interpolate(cim1, midi, cim2, si5, si9, si8, ci1);

	re18 = interpolate(cim1, midi, cim2, sr5, sr9, sr8, ci2);
	im18 = interpolate(cim1, midi, cim2, si5, si9, si8, ci2);

	re91 = GET_SAVED_REAL(cr1, ci1);
	re92 = GET_SAVED_REAL(cr2, ci1);
	re93 = GET_SAVED_REAL(cr1, ci2);
	re94 = GET_SAVED_REAL(cr2, ci2);

	im91 = GET_SAVED_IMAG(cr1, ci1);
	im92 = GET_SAVED_IMAG(cr2, ci1);
	im93 = GET_SAVED_IMAG(cr1, ci2);
	im94 = GET_SAVED_IMAG(cr2, ci2);

	RHOMBUS(cre1, midr, cim1, midi, x1, ((x1 + x2) >> 1), y1, ((y1 + y2) >> 1),
			sr1, si1,
			sr5, si5,
			sr6, si6,
			sr9, si9,
			re10, im10,
			re12, im12,
			re13, im13,
			re15, im15,
			re91, im91,
			iter);
	RHOMBUS(midr, cre2, cim1, midi, (x1 + x2) >> 1, x2, y1, (y1 + y2) >> 1,
			sr5, si5,
			sr2, si2,
			sr9, si9,
			sr7, si7,
			re11, im11,
			re13, im13,
			re14, im14,
			re16, im16,
			re92, im92,
			iter);
	RHOMBUS(cre1, midr, midi, cim2, x1, (x1 + x2) >> 1, (y1 + y2) >> 1, y2,
			sr6, si6,
			sr9, si9,
			sr3, si3,
			sr8, si8,
			re15, im15,
			re17, im17,
			re18, im18,
			re20, im20,
			re93, im93,
			iter);
	RHOMBUS(midr, cre2, midi, cim2, (x1 + x2) >> 1, x2, (y1 + y2) >> 1, y2,
			sr9, si9,
			sr7, si7,
			sr8, si8,
			sr4, si4,
			re16, im16,
			re18, im18,
			re19, im19,
			re21, im21,
			re94, im94,
			iter);

rhombus_done:
	rhombus_depth--;
	return status;
}

static void soi_double()
{
	int status;
	double tolerance = 0.1;
	double stepx;
	double stepy;
	double xxminl;
	double xxmaxl;
	double yyminl;
	double yymaxl;
	rhombus_depth = -1;
	if (g_bf_math)
	{
		xxminl = (double)bftofloat(g_escape_time_state.m_grid_bf.x_min());
		yyminl = (double)bftofloat(g_escape_time_state.m_grid_bf.y_min());
		xxmaxl = (double)bftofloat(g_escape_time_state.m_grid_bf.x_max());
		yymaxl = (double)bftofloat(g_escape_time_state.m_grid_bf.y_max());
	}
	else
	{
		xxminl = g_escape_time_state.m_grid_fp.x_min();
		yyminl = g_escape_time_state.m_grid_fp.y_min();
		xxmaxl = g_escape_time_state.m_grid_fp.x_max();
		yymaxl = g_escape_time_state.m_grid_fp.y_max();
	}
	twidth = tolerance/(g_x_dots-1);
	stepx = (xxmaxl - xxminl) / g_x_dots;
	stepy = (yyminl - yymaxl) / g_y_dots;
	equal = (stepx < stepy ? stepx : stepy);

	RHOMBUS(xxminl, xxmaxl, yymaxl, yyminl,
			0, g_x_dots, 0, g_y_dots,
			xxminl, yymaxl,
			xxmaxl, yymaxl,
			xxminl, yyminl,
			xxmaxl, yyminl,
			(xxmaxl + xxminl)/2, yymaxl,
			xxminl, (yymaxl + yyminl)/2,
			xxmaxl, (yymaxl + yyminl)/2,
			(xxmaxl + xxminl)/2, yyminl,
			(xxminl + xxmaxl)/2, (yymaxl + yyminl)/2,
			1);
}

void soi()
{
	if (DEBUGMODE_SOI_LONG_DOUBLE == g_debug_mode)
	{
		soi_long_double();
	}
	else
	{
		soi_double();
	}
}
