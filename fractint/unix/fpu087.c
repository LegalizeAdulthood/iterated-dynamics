/* Fpu087.c
 * This file contains routines to replace fpu087.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */


#include "port.h"
#include "prototyp.h"

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

double _2_ = 2.0;
double _1_ = 1.0;
double PointFive = 0.5;
/* infinity is declared in sunmath.h */
#ifndef USE_SUNMATH
double infinity = 1.0E+300;
#endif

void FPUaptan387(double *y, double *x, double *atan)
{
    *atan = atan2(*y,*x);
}

void FPUcplxmul(_CMPLX *x, _CMPLX *y, _CMPLX *z)
{
    double tx;
    tx = x->x * y->x - x->y * y->y;
    z->y = x->x * y->y + x->y * y->x;
    z->x = tx;
}

void FPUcplxdiv(_CMPLX *x, _CMPLX *y, _CMPLX *z)
{
    double mod,tx,yxmod,yymod;
    mod = y->x * y->x + y->y * y->y;
    if (mod==0) {
	DivideOverflow++;
    }
    yxmod = y->x/mod;
    yymod = - y->y/mod;
    tx = x->x * yxmod - x->y * yymod;
    z->y = x->x * yymod + x->y * yxmod;
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

void FPUcplxlog(_CMPLX *x, _CMPLX *z)
{
    double mod,zx,zy;
    mod = sqrt(x->x*x->x + x->y*x->y);
    zx = log(mod);
    zy = atan2(x->y,x->x);

    z->x = zx;
    z->y = zy;
}

void FPUcplxexp387(_CMPLX *x, _CMPLX *z)
{
    double pow,y;
    y = x->y;
    pow = exp(x->x);
    z->x = pow*cos(y);
    z->y = pow*sin(y);
}

/* Integer Routines */
void SinCos086(long x, long *sinx, long *cosx)
{
    double a;
    a = x/(double)(1<<16);
    *sinx = (long) (sin(a)*(double)(1<<16));
    *cosx = (long) (cos(a)*(double)(1<<16));
}

void SinhCosh086(long x, long *sinx, long *cosx)
{
    double a;
    a = x/(double)(1<<16);
    *sinx = (long) (sinh(a)*(double)(1<<16));
    *cosx = (long) (cosh(a)*(double)(1<<16));
}

long Exp086(x)
long x;
{
    return (long) (exp((double)x/(double)(1<<16))*(double)(1<<16));
}

#define em2float(l) (*(float *)&(l))
#define float2em(f) (*(long *)&(f))

/*
 * Input is a 16 bit offset number.  Output is shifted by Fudge.
 */
unsigned long ExpFudged(x, Fudge)
long x;
int Fudge;
{
    return (long) (exp((double)x/(double)(1<<16))*(double)(1<<Fudge));
}

/* This multiplies two e/m numbers and returns an e/m number. */
long r16Mul(x,y)
long x,y;
{
    float f;
    f = em2float(x)*em2float(y);
    return float2em(f);
}

/* This takes an exp/mant number and returns a shift-16 number */
long LogFloat14(x)
unsigned long x;
{
    return log((double)em2float(x))*(1<<16);
}

/* This divides two e/m numbers and returns an e/m number. */
long RegDivFloat(x,y)
long x,y;
{
    float f;
    f = em2float(x)/em2float(y);
    return float2em(f);
}

/*
 * This routine on the IBM converts shifted integer x,FudgeFact to 
 * the 4 byte number: exp,mant,mant,mant
 * Instead of using exp/mant format, we'll just use floats.
 * Note: If sizeof(float) != sizeof(long), we're hosed.
 */
long RegFg2Float(x,FudgeFact)
long x;
int FudgeFact;
{
    float f;
    long l;
    f = x/(float)(1<<FudgeFact);
    l = float2em(f);
    return l;
}

/*
 * This converts em to shifted integer format.
 */
long RegFloat2Fg(x,Fudge)
long x;
int Fudge;
{
    return em2float(x)*(float)(1<<Fudge);
}

long RegSftFloat(x, Shift)
long x;
int Shift;
{
    float f;
    f = em2float(x);
    if (Shift>0) {
	f *= (1<<Shift);
    } else {
	f /= (1<<Shift);
    }
    return float2em(f);
}
