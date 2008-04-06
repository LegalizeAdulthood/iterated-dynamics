/* Fpu087.c
 * This file contains routines to replace fpu087.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#include <string>

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

void FPUcplxmul(ComplexD const *x, ComplexD const *y, ComplexD *z)
{
	double tx = x->real()*y->real() - x->imag()*y->imag();
	z->imag(x->real()*y->imag() + x->imag()*y->real());
	z->real(tx);
}

int DivideOverflow = 0;

void FPUcplxdiv(ComplexD *x, ComplexD *y, ComplexD *z)
{
	double mod, tx, yxmod, yymod;
	mod = y->real()*y->real() + y->imag()*y->imag();
	if (mod == 0)
	{
		DivideOverflow++;
	}
	yxmod = y->real()/mod;
	yymod = - y->imag()/mod;
	tx = x->real()*yxmod - x->imag()*yymod;
	z->imag(x->real()*yymod + x->imag()*yxmod);
	z->real(tx);
}

void FPUsincos(double angle, double *Sin, double *Cos)
{
	*Sin = sin(angle);
	*Cos = cos(angle);
}

void FPUsinhcosh(double angle, double *Sinh, double *Cosh)
{
	*Sinh = sinh(angle);
	*Cosh = cosh(angle);
}

void FPUcplxlog(ComplexD const *x, ComplexD *z)
{
	double const mod = sqrt(x->real()*x->real() + x->imag()*x->imag());
	double const zx = log(mod);
	double const zy = atan2(x->imag(), x->real());
	z->real(zx);
	z->imag(zy);
}

void FPUcplxexp387(ComplexD const *x, ComplexD *z)
{
	double const pow = exp(x->real());
	z->real(pow*cos(x->imag()));
	z->imag(pow*sin(x->imag()));
}

// Integer Routines
void SinCos086(long x, long *sinx, long *cosx)
{
	double const angle = x/double(1 << 16);
	*sinx = long(sin(angle)*double(1 << 16));
	*cosx = long(cos(angle)*double(1 << 16));
}

void SinhCosh086(long x, long *sinx, long *cosx)
{
	double const angle = x/double(1 << 16);
	*sinx = long(sinh(angle)*double(1 << 16));
	*cosx = long(cosh(angle)*double(1 << 16));
}

long Exp086(long x)
{
	return long(exp(double(x)/double(1 << 16))*double(1 << 16));
}

/*
 * Input is a 16 bit offset number.  Output is shifted by Fudge.
 */
unsigned long ExpFudged(long x, int Fudge)
{
	return long(exp(double(x)/double(1 << 16))*double(1 << Fudge));
}

// This multiplies two e/m numbers and returns an e/m number.
long r16Mul(long x, long y)
{
	float f = em2float(x)*em2float(y);
	return float2em(f);
}

// This takes an exp/mant number and returns a shift-16 number
long LogFloat14(unsigned long x)
{
	return long(log(double(em2float(x))))*(1 << 16);
}

// This divides two e/m numbers and returns an e/m number.
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
	float f = float(x)/float(1 << FudgeFact);
	return float2em(f);
}

/*
 * This converts em to shifted integer format.
 */
long RegFloat2Fg(long x, int Fudge)
{
	return long(em2float(x)*float(1 << Fudge));
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
