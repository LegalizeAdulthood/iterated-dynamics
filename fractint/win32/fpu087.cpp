/* Fpu087.c
 * This file contains routines to replace fpu087.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#include "port.h"
#include "prototyp.h"
#include "fpu.h"

/*
 *----------------------------------------------------------------------
 *
 * xxx086 --
 *
 *	Simulate integer math routines using floating point.
 *	This will of course slow things down, so this should all be
 *	changed at some point.
 *
 *----------------------------------------------------------------------
 */

void FPUaptan387(double *y, double *x, double *atan)
{
	*atan = atan2(*y, *x);
}

void FPUcplxmul(ComplexD *x, ComplexD *y, ComplexD *z)
{
	double tx;
	tx = x->x*y->x - x->y*y->y;
	z->y = x->x*y->y + x->y*y->x;
	z->x = tx;
}

void FPUcplxdiv(ComplexD *x, ComplexD *y, ComplexD *z)
{
	double mod, tx, yxmod, yymod;
	mod = y->x*y->x + y->y*y->y;
	if (mod == 0)
	{
		DivideOverflow++;
	}
	yxmod = y->x/mod;
	yymod = - y->y/mod;
	tx = x->x*yxmod - x->y*yymod;
	z->y = x->x*yymod + x->y*yxmod;
	z->x = tx;
}

void FPUsincos(double *Angle, double *Sin, double *Cos)
{
	*Sin = sin(*Angle);
	*Cos = cos(*Angle);
}

void FPUsinhcosh(double *Angle, double *Sinh, double *Cosh)
{
	*Sinh = sinh(*Angle);
	*Cosh = cosh(*Angle);
}

void FPUcplxlog(ComplexD *x, ComplexD *z)
{
	double mod, zx, zy;
	mod = sqrt(x->x*x->x + x->y*x->y);
	zx = log(mod);
	zy = atan2(x->y, x->x);

	z->x = zx;
	z->y = zy;
}

void FPUcplxexp387(ComplexD *x, ComplexD *z)
{
	double pow = exp(x->x);
	z->x = pow*cos(x->y);
	z->y = pow*sin(x->y);
}

/* Integer Routines */
void SinCos086(long x, long *sinx, long *cosx)
{
	double a;
	a = x/double(1 << 16);
	*sinx = long(sin(a)*double(1 << 16));
	*cosx = long(cos(a)*double(1 << 16));
}

void SinhCosh086(long x, long *sinx, long *cosx)
{
	double a;
	a = x/double(1 << 16);
	*sinx = long(sinh(a)*double(1 << 16));
	*cosx = long(cosh(a)*double(1 << 16));
}

long Exp086(long x)
{
	return long(exp(double(x)/double(1 << 16))*double(1 << 16));
}

#define em2float(l) (*(float *) &(l))
#define float2em(f) (*(long *) &(f))

/*
 * Input is a 16 bit offset number.  Output is shifted by Fudge.
 */
unsigned long ExpFudged(long x, int Fudge)
{
	return (long) (exp(double(x)/double(1 << 16))*double(1 << Fudge));
}

/* This multiplies two e/m numbers and returns an e/m number. */
long r16Mul(long x, long y)
{
	float f = em2float(x)*em2float(y);
	return float2em(f);
}

/* This takes an exp/mant number and returns a shift-16 number */
long LogFloat14(unsigned long x)
{
	return long(log(double(em2float(x))))*(1 << 16);
}

/* This divides two e/m numbers and returns an e/m number. */
long RegDivFloat(long x, long y)
{
	float f = em2float(x)/em2float(y);
	return float2em(f);
}

/*
 * This routine on the IBM converts shifted integer x, FudgeFact to
 * the 4 byte number: exp, mant, mant, mant
 * Instead of using exp/mant format, we'll just use floats.
 * Note: If sizeof(float) != sizeof(long), we're hosed.
 */
long RegFg2Float(long x, int FudgeFact)
{
	float f = (float) x/(float) (1 << FudgeFact);
	return float2em(f);
}

/*
 * This converts em to shifted integer format.
 */
long RegFloat2Fg(long x, int Fudge)
{
	return (long) (em2float(x)*(float) (1 << Fudge));
}

long RegSftFloat(long x, int Shift)
{
	float f;
	f = em2float(x);
	if (Shift > 0)
	{
		f *= (1 << Shift);
	}
	else
	{
		f /= (1 << Shift);
	}
	return float2em(f);
}
