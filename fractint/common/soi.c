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

#define DBLS LDBL
#define FABS(x)  fabsl(x)
/* the following needs to be changed back to frexpl once the portability
	issue has been addressed JCO */
#ifndef XFRACT
#define FREXP(x, y) frexpl(x, y)
#else
#define FREXP(x, y) frexp(x, y)
#endif

#define TRUE 1
#define FALSE 0
#define EVERY 15
#define BASIN_COLOR 0

int rhombus_stack[10];
int rhombus_depth;
int max_rhombus_depth;
int minstackavail;
/* int minstack = 1700; */ /* need this much stack to recurse */
int minstack = 2200; /* and this much stack to not crash when <tab> is pressed */
static DBLS twidth;
static DBLS equal;
static char baxinxx = FALSE;

long iteration(register DBLS cr, register DBLS ci,
				register DBLS re, register DBLS im,
				long start)
{
	register long iter, offset = 0;
	int k, n;
	register DBLS ren, imn, sre, sim;
#ifdef INTEL
	register float mag;
	register unsigned long bail = 0x41800000, magi; /* bail = 16.0 */
	register unsigned long eq = *(unsigned long *)&equal;
#else
	register DBLS mag;
#endif
	DBLS d;
	int exponent;

	if (baxinxx)
	{
		sre = re;
		sim = im;
		ren = re*re;
		imn = im*im;
		if (start != 0)
		{
			offset = maxit-start + 7;
			iter = offset >> 3;
			offset &= 7;
			offset = (8-offset);
		}
		else
		{
			iter = maxit >> 3;
		}
		k = n = 8;

		do
		{
			im = im*re;
			re = ren-imn;
			im += im;
			re += cr;
			im += ci;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

#ifdef INTEL
			mag = FABS(sre-re);
			magi = *(unsigned long *)&mag;
			if (magi < eq)
			{
				mag = FABS(sim-im);
				magi = *(unsigned long *)&mag;
				if (magi < eq)
				{
					return BASIN_COLOR;
				}
			}
#else /* INTEL */
			if (FABS(sre-re) < equal && FABS(sim-im) < equal)
			{
				return BASIN_COLOR;
			}
#endif /* INTEL */

			k -= 8;
			if (k <= 0)
			{
				n <<= 1;
				sre = re;
				sim = im;
				k = n;
			}

			imn = im*im;
			ren = re*re;
			mag = ren + imn;
#ifdef INTEL
			magi = *(unsigned long *)&mag;
#endif
		}
#ifdef INTEL
		while (magi < bail && --iter != 0);
#else
		while (mag < 16.0 && --iter != 0);
#endif
	}
	else
	{
		ren = re*re;
		imn = im*im;
		if (start != 0)
		{
			offset = maxit-start + 7;
			iter = offset >> 3;
			offset &= 7;
			offset = (8-offset);
		}
		else
		{
			iter = maxit >> 3;
		}

		do
		{
			im = im*re;
			re = ren-imn;
			im += im;
			re += cr;
			im += ci;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*re;
			ren = re + im;
			re = re-im;
			imn += imn;
			re = ren*re;
			im = imn + ci;
			re += cr;

			imn = im*im;
			ren = re*re;
			mag = ren + imn;
#ifdef INTEL
			magi = *(unsigned long *)&mag;
#endif
		}
#ifdef INTEL
		while (magi < bail && --iter != 0);
#else
		while (mag < 16.0 && --iter != 0);
#endif
	}

	if (iter == 0)
	{
		baxinxx = TRUE;
		return BASIN_COLOR;
	}
	else
	{
		static char adjust[256]=
		{
			0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
			5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
			6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
			6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
			7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
			7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
			7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
			7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
			8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
			8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
			8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
			8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
			8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
			8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
			8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
			8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
		};

		baxinxx = FALSE;
#ifdef INTEL
		d = ren + imn;
#else
		d = mag;
#endif
		FREXP(d, &exponent);
		return maxit + offset - (((iter - 1) << 3) + (long) adjust[exponent >> 3]);
	}
}

static void put_horizontal_line(int x1, int y1, int x2, int color)
{
	int x;
	for (x = x1; x <= x2; x++)
	{
		(*g_plot_color)(x, y1, color);
	}
}

static void put_box(int x1, int y1, int x2, int y2, int color)
{
	for (; y1 <= y2; y1++)
	{
		put_horizontal_line(x1, y1, x2, color);
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
	b0 = w0; \
	b1 = (w1-w0)/(LDBL)(x1-x0); \
	b2 = ((w2-w1)/(LDBL)(x2-x1)-b1)/(x2-x0)

/* evaluate Newton polynomial given by (x0, b0), (x1, b1) at x:=t */
#define EVALUATE(x0, x1, b0, b1, b2, t) \
	((b2*(t-x1) + b1)*(t-x0) + b0)

/* Newton Interpolation.
	It computes the value of the interpolation polynomial given by
	(x0, w0)..(x2, w2) at x:=t */
static DBLS interpolate(DBLS x0, DBLS x1, DBLS x2,
		DBLS w0, DBLS w1, DBLS w2,
		DBLS t)
{
	register DBLS b0 = w0, b1 = w1, b2 = w2, b;

	/*b0 = (r0*b1-r1*b0)/(x1-x0);
	b1 = (r1*b2-r2*b1)/(x2-x1);
	b0 = (r0*b1-r2*b0)/(x2-x0);

	return (DBLS)b0; */
	b = (b1-b0)/(x1-x0);
	return (DBLS)((((b2-b1)/(x2-x1)-b)/(x2-x0))*(t-x1) + b)*(t-x0) + b0;
	/*
	if (t < x1)
		return w0 + ((t-x0)/(LDBL)(x1-x0))*(w1-w0);
	else
		return w1 + ((t-x1)/(LDBL)(x2-x1))*(w2-w1); */
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
static DBLS zre1, zim1, zre2, zim2, zre3, zim3, zre4, zim4, zre5, zim5,
				zre6, zim6, zre7, zim7, zre8, zim8, zre9, zim9;
/*
	The purpose of this macro is to reduce the number of parameters of the
	function rhombus(), since this is a recursive function, and stack space
	under DOS is extremely limited.
*/

#define RHOMBUS(CRE1, CRE2, CIM1, CIM2, X1, X2, Y1, Y2, ZRE1, ZIM1, ZRE2, ZIM2, ZRE3, ZIM3, \
	ZRE4, ZIM4, ZRE5, ZIM5, ZRE6, ZIM6, ZRE7, ZIM7, ZRE8, ZIM8, ZRE9, ZIM9, ITER) \
	zre1 = (ZRE1); zim1 = (ZIM1); \
	zre2 = (ZRE2); zim2 = (ZIM2); \
	zre3 = (ZRE3); zim3 = (ZIM3); \
	zre4 = (ZRE4); zim4 = (ZIM4); \
	zre5 = (ZRE5); zim5 = (ZIM5); \
	zre6 = (ZRE6); zim6 = (ZIM6); \
	zre7 = (ZRE7); zim7 = (ZIM7); \
	zre8 = (ZRE8); zim8 = (ZIM8); \
	zre9 = (ZRE9); zim9 = (ZIM9); \
	status = rhombus((CRE1), (CRE2), (CIM1), (CIM2), (X1), (X2), (Y1), (Y2), (ITER))

static int rhombus(DBLS cre1, DBLS cre2, DBLS cim1, DBLS cim2,
				int x1, int x2, int y1, int y2, long iter)
{
	/* The following variables do not need their values saved */
	/* used in scanning */
	static long savecolor, color, helpcolor;
	static int x, y, z, savex;

#if 0
	static DBLS re, im, restep, imstep, interstep, helpre;
	static DBLS zre, zim;
	/* interpolation coefficients */
	static DBLS br10, br11, br12, br20, br21, br22, br30, br31, br32;
	static DBLS bi10, bi11, bi12, bi20, bi21, bi22, bi30, bi31, bi32;
	/* ratio of interpolated test point to iterated one */
	static DBLS l1, l2;
	/* squares of key values */
	static DBLS rq1, iq1;
	static DBLS rq2, iq2;
	static DBLS rq3, iq3;
	static DBLS rq4, iq4;
	static DBLS rq5, iq5;
	static DBLS rq6, iq6;
	static DBLS rq7, iq7;
	static DBLS rq8, iq8;
	static DBLS rq9, iq9;

	/* test points */
	static DBLS cr1, cr2;
	static DBLS ci1, ci2;
	static DBLS tzr1, tzi1, tzr2, tzi2, tzr3, tzi3, tzr4, tzi4;
	static DBLS trq1, tiq1, trq2, tiq2, trq3, tiq3, trq4, tiq4;
#else
#define re        mem_static[ 0]
#define im        mem_static[ 1]
#define restep    mem_static[ 2]
#define imstep    mem_static[ 3]
#define interstep mem_static[ 4]
#define helpre    mem_static[ 5]
#define zre       mem_static[ 6]
#define zim       mem_static[ 7]
#define br10      mem_static[ 8]
#define br11      mem_static[ 9]
#define br12      mem_static[10]
#define br20      mem_static[11]
#define br21      mem_static[12]
#define br22      mem_static[13]
#define br30      mem_static[14]
#define br31      mem_static[15]
#define br32      mem_static[16]
#define bi10      mem_static[17]
#define bi11      mem_static[18]
#define bi12      mem_static[19]
#define bi20      mem_static[20]
#define bi21      mem_static[21]
#define bi22      mem_static[22]
#define bi30      mem_static[23]
#define bi31      mem_static[24]
#define bi32      mem_static[25]
#define l1        mem_static[26]
#define l2        mem_static[27]
#define rq1       mem_static[28]
#define iq1       mem_static[29]
#define rq2       mem_static[30]
#define iq2       mem_static[31]
#define rq3       mem_static[32]
#define iq3       mem_static[33]
#define rq4       mem_static[34]
#define iq4       mem_static[35]
#define rq5       mem_static[36]
#define iq5       mem_static[37]
#define rq6       mem_static[38]
#define iq6       mem_static[39]
#define rq7       mem_static[40]
#define iq7       mem_static[41]
#define rq8       mem_static[42]
#define iq8       mem_static[43]
#define rq9       mem_static[44]
#define iq9       mem_static[45]
#define cr1       mem_static[46]
#define cr2       mem_static[47]
#define ci1       mem_static[48]
#define ci2       mem_static[49]
#define tzr1      mem_static[50]
#define tzi1      mem_static[51]
#define tzr2      mem_static[52]
#define tzi2      mem_static[53]
#define tzr3      mem_static[54]
#define tzi3      mem_static[55]
#define tzr4      mem_static[56]
#define tzi4      mem_static[57]
#define trq1      mem_static[58]
#define tiq1      mem_static[59]
#define trq2      mem_static[60]
#define tiq2      mem_static[61]
#define trq3      mem_static[62]
#define tiq3      mem_static[63]
#define trq4      mem_static[64]
#define tiq4      mem_static[65]

#endif
	/* number of iterations before SOI iteration cycle */
	static long before;
	static int avail;

	/* the variables below need to have local copis for recursive calls */
	DBLS *mem;
	DBLS *mem_static;
	/* center of rectangle */
	DBLS midr = (cre1 + cre2)/2, midi = (cim1 + cim2)/2;

#if 0
	/* saved values of key values */
	DBLS sr1, si1, sr2, si2, sr3, si3, sr4, si4;
	DBLS sr5, si5, sr6, si6, sr7, si7, sr8, si8, sr9, si9;
	/* key values for subsequent rectangles */
	DBLS re10, re11, re12, re13, re14, re15, re16, re17, re18, re19, re20, re21;
	DBLS im10, im11, im12, im13, im14, im15, im16, im17, im18, im19, im20, im21;
	DBLS re91, re92, re93, re94, im91, im92, im93, im94;
#else
#define sr1  mem[ 0]
#define si1  mem[ 1]
#define sr2  mem[ 2]
#define si2  mem[ 3]
#define sr3  mem[ 4]
#define si3  mem[ 5]
#define sr4  mem[ 6]
#define si4  mem[ 7]
#define sr5  mem[ 8]
#define si5  mem[ 9]
#define sr6  mem[10]
#define si6  mem[11]
#define sr7  mem[12]
#define si7  mem[13]
#define sr8  mem[14]
#define si8  mem[15]
#define sr9  mem[16]
#define si9  mem[17]
#define re10 mem[18]
#define re11 mem[19]
#define re12 mem[20]
#define re13 mem[21]
#define re14 mem[22]
#define re15 mem[23]
#define re16 mem[24]
#define re17 mem[25]
#define re18 mem[26]
#define re19 mem[27]
#define re20 mem[28]
#define re21 mem[29]
#define im10 mem[30]
#define im11 mem[31]
#define im12 mem[32]
#define im13 mem[33]
#define im14 mem[34]
#define im15 mem[35]
#define im16 mem[36]
#define im17 mem[37]
#define im18 mem[38]
#define im19 mem[39]
#define im20 mem[40]
#define im21 mem[41]
#define re91 mem[42]
#define re92 mem[43]
#define re93 mem[44]
#define re94 mem[45]
#define im91 mem[46]
#define im92 mem[47]
#define im93 mem[48]
#define im94 mem[49]
#endif

	int status = 0;
	rhombus_depth++;

#if 1
	/* TODO: eliminate this memory aliasing monstrosity */
	/* what we go through under DOS to deal with memory! We re-use
		the g_size_of_string array (8k). The first 660 bytes is for
		static variables, then we make our own "stack" with copies
		for each recursive call of rhombus() for the rest.
		*/
	mem_static = (DBLS *)g_size_of_string;
	mem = mem_static+ 66 + 50*rhombus_depth;
#endif

	avail = stackavail();
	if (avail < minstackavail)
	{
		minstackavail = avail;
	}
	if (rhombus_depth > max_rhombus_depth)
	{
		max_rhombus_depth = rhombus_depth;
	}
	rhombus_stack[rhombus_depth] = avail;

	if (driver_key_pressed())
	{
		status = 1;
		goto rhombus_done;
	}
	if (iter > maxit)
	{
		put_box(x1, y1, x2, y2, 0);
		status = 0;
		goto rhombus_done;
	}

	if ((y2-y1 <= SCAN) || (avail < minstack))
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

	before = iter;

	while (1)
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

		/* iterate key values */
		zim1 = (zim1 + zim1)*zre1 + cim1;
		zre1 = rq1-iq1 + cre1;
		rq1 = zre1*zre1;
		iq1 = zim1*zim1;

		zim2 = (zim2 + zim2)*zre2 + cim1;
		zre2 = rq2-iq2 + cre2;
		rq2 = zre2*zre2;
		iq2 = zim2*zim2;

		zim3 = (zim3 + zim3)*zre3 + cim2;
		zre3 = rq3-iq3 + cre1;
		rq3 = zre3*zre3;
		iq3 = zim3*zim3;

		zim4 = (zim4 + zim4)*zre4 + cim2;
		zre4 = rq4-iq4 + cre2;
		rq4 = zre4*zre4;
		iq4 = zim4*zim4;

		zim5 = (zim5 + zim5)*zre5 + cim1;
		zre5 = rq5-iq5 + midr;
		rq5 = zre5*zre5;
		iq5 = zim5*zim5;

		zim6 = (zim6 + zim6)*zre6 + midi;
		zre6 = rq6-iq6 + cre1;
		rq6 = zre6*zre6;
		iq6 = zim6*zim6;

		zim7 = (zim7 + zim7)*zre7 + midi;
		zre7 = rq7-iq7 + cre2;
		rq7 = zre7*zre7;
		iq7 = zim7*zim7;

		zim8 = (zim8 + zim8)*zre8 + cim2;
		zre8 = rq8-iq8 + midr;
		rq8 = zre8*zre8;
		iq8 = zim8*zim8;

		zim9 = (zim9 + zim9)*zre9 + midi;
		zre9 = rq9-iq9 + midr;
		rq9 = zre9*zre9;
		iq9 = zim9*zim9;

		/* iterate test point */
		tzi1 = (tzi1 + tzi1)*tzr1 + ci1;
		tzr1 = trq1-tiq1 + cr1;
		trq1 = tzr1*tzr1;
		tiq1 = tzi1*tzi1;

		tzi2 = (tzi2 + tzi2)*tzr2 + ci1;
		tzr2 = trq2-tiq2 + cr2;
		trq2 = tzr2*tzr2;
		tiq2 = tzi2*tzi2;

		tzi3 = (tzi3 + tzi3)*tzr3 + ci2;
		tzr3 = trq3-tiq3 + cr1;
		trq3 = tzr3*tzr3;
		tiq3 = tzi3*tzi3;

		tzi4 = (tzi4 + tzi4)*tzr4 + ci2;
		tzr4 = trq4-tiq4 + cr2;
		trq4 = tzr4*tzr4;
		tiq4 = tzi4*tzi4;

		iter++;

		/* if one of the iterated values bails out, subdivide */
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

		/* if maximum number of iterations is reached, the whole rectangle
			can be assumed part of M. This is of course best case behavior
			of SOI, we seldomly get there */
		if (iter > maxit)
		{
			put_box(x1, y1, x2, y2, 0);
			status = 0;
			goto rhombus_done;
		}

		/* now for all test points, check whether they exceed the
			allowed tolerance. if so, subdivide */
		l1 = GET_REAL(cr1, ci1);
		l1 = (tzr1 == 0.0) ? (l1 == 0.0) ? 1.0 : 1000.0 : l1/tzr1;
		if (FABS(1.0-l1) > twidth)
		{
			break;
		}

		l2 = GET_IMAG(cr1, ci1);
		l2 = (tzi1 == 0.0) ?
			((l2 == 0.0) ? 1.0 : 1000.0) : l2/tzi1;
		if (FABS(1.0-l2) > twidth)
		{
			break;
		}

		l1 = GET_REAL(cr2, ci1);
		l1 = (tzr2 == 0.0) ?
			((l1 == 0.0) ? 1.0 : 1000.0) : l1/tzr2;
		if (FABS(1.0-l1) > twidth)
		{
			break;
		}

		l2 = GET_IMAG(cr2, ci1);
		l2 = (tzi2 == 0.0) ?
			((l2 == 0.0) ? 1.0 : 1000.0) : l2/tzi2;
		if (FABS(1.0-l2) > twidth)
		{
			break;
		}

		l1 = GET_REAL(cr1, ci2);
		l1 = (tzr3 == 0.0) ?
			((l1 == 0.0) ? 1.0 : 1000.0) : l1/tzr3;
		if (FABS(1.0-l1) > twidth)
		{
			break;
		}

		l2 = GET_IMAG(cr1, ci2);
		l2 = (tzi3 == 0.0) ?
			((l2 == 0.0) ? 1.0 : 1000.0) : l2/tzi3;
		if (FABS(1.0-l2) > twidth)
		{
			break;
		}

		l1 = GET_REAL(cr2, ci2);
		l1 = (tzr4 == 0.0) ?
			((l1 == 0.0) ? 1.0 : 1000.0) : l1/tzr4;
		if (FABS(1.0-l1) > twidth)
		{
			break;
		}

		l2 = GET_IMAG(cr2, ci2);
		l2 = (tzi4 == 0.0) ?
			((l2 == 0.0) ? 1.0 : 1000.0) : l2/tzi4;
		if (FABS(1.0-l2) > twidth)
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

void soi_long_double(void)
{
	int status;
	DBLS tolerance = 0.1;
	DBLS stepx, stepy;
	DBLS xxminl, xxmaxl, yyminl, yymaxl;
	minstackavail = 30000;
	rhombus_depth = -1;
	max_rhombus_depth = 0;
	if (bf_math)
	{
		xxminl = bftofloat(bfxmin);
		yyminl = bftofloat(bfymin);
		xxmaxl = bftofloat(bfxmax);
		yymaxl = bftofloat(bfymax);
	}
	else
	{
		xxminl = xxmin;
		yyminl = yymin;
		xxmaxl = xxmax;
		yymaxl = yymax;
	}
	twidth = tolerance/(xdots-1);
	stepx = (xxmaxl - xxminl) / xdots;
	stepy = (yyminl - yymaxl) / ydots;
	equal = (stepx < stepy ? stepx : stepy);

	RHOMBUS(xxminl, xxmaxl, yymaxl, yyminl,
		0, xdots, 0, ydots,
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
