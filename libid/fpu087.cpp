#include "port.h"
#include "prototyp.h"

#include "fpu087.h"

#include "fixed_pt.h"

#include <cfloat>
#include <cmath>

void FPUcplxmul(DComplex const *x, DComplex const *y, DComplex *z)
{
    double tx;
    double ty;
    if (y->y == 0.0) // y is real
    {
        tx = x->x * y->x;
        ty = x->y * y->x;
    }
    else if (x->y == 0.0) // x is real
    {
        tx = x->x * y->x;
        ty = x->x * y->y;
    }
    else
    {
        tx = x->x * y->x - x->y * y->y;
        ty = x->x * y->y + x->y * y->x;
    }
    z->x = std::isnan(tx) || std::isinf(tx) ? ID_INFINITY : tx;
    z->y = std::isnan(ty) || std::isinf(ty) ? ID_INFINITY : ty;
}

void FPUcplxdiv(DComplex const *x, DComplex const *y, DComplex *z)
{
    const double mod = y->x * y->x + y->y * y->y;
    if (mod == 0.0 || std::abs(mod) <= DBL_MIN)
    {
        z->x = ID_INFINITY;
        z->y = ID_INFINITY;
        g_overflow = true;
        return;
    }

    if (y->y == 0.0) // if y is real
    {
        z->x = x->x / y->x;
        z->y = x->y / y->x;
    }
    else
    {
        const double yxmod = y->x / mod;
        const double yymod = -y->y / mod;
        // Need to compute into temporaries to avoid pointer aliasing
        const double tx = x->x * yxmod - x->y * yymod;
        const double ty = x->x * yymod + x->y * yxmod;
        z->x = tx;
        z->y = ty;
    }
}

void sin_cos(const double *angle, double *sine, double *cosine)
{
    if (std::isnan(*angle) || std::isinf(*angle))
    {
        *sine = 0.0;
        *cosine = 1.0;
    }
    else
    {
        *sine = std::sin(*angle);
        *cosine = std::cos(*angle);
    }
}

void sinh_cosh(const double *angle, double *sine, double *cosine)
{
    if (std::isnan(*angle) || std::isinf(*angle))
    {
        *sine = 1.0;
        *cosine = 1.0;
    }
    else
    {
        *sine = std::sinh(*angle);
        *cosine = std::cosh(*angle);
        if (std::isnan(*sine) || std::isinf(*sine))
        {
            *sine = 1.0;
        }
        if (std::isnan(*cosine) || std::isinf(*cosine))
        {
            *cosine = 1.0;
        }
    }
}

void FPUcplxlog(const DComplex *x, DComplex *z)
{
    if (x->x == 0.0 && x->y == 0.0)
    {
        z->x = 0.0;
        z->y = 0.0;
        return;
    }
    if (x->y == 0.0)// x is real
    { 
        z->x = logl(x->x);
        z->y = 0.0;
        return;
    }
    const double mod = x->x * x->x + x->y * x->y;
    z->x = std::isnan(mod) || std::islessequal(mod, 0) || std::isinf(mod) //
        ? 0.5
        : 0.5 * std::log(mod);
    z->y = std::isnan(x->x) || std::isnan(x->y) || std::isinf(x->x) || std::isinf(x->y) //
        ? 1.0
        : std::atan2(x->y, x->x);
}

void FPUcplxexp(const DComplex *x, DComplex *z)
{
    const double y = x->y;
    double pow = std::exp(x->x);
    if (std::isnan(pow) || std::isinf(pow))
    {
        pow = 1.0;
    }
    if (x->y == 0.0) /* x is real */
    {
        z->x = pow;
        z->y = 0.0;
    }
    else
    {
        z->x = pow * std::cos(y);
        z->y = pow * std::sin(y);
    }
}

// Integer Routines
void sin_cos(long angle, long *sine, long *cosine)
{
    const double a = angle / (double)(1 << 16);
    *sine = (long)(std::sin(a)*(double)(1 << 16));
    *cosine = (long)(std::cos(a)*(double)(1 << 16));
}

void sinh_cosh(long angle, long *sine, long *cosine)
{
    const double a = angle / (double)(1 << 16);
    *sine = (long)(std::sinh(a)*(double)(1 << 16));
    *cosine = (long)(std::cosh(a)*(double)(1 << 16));
}

long Exp086(long x)
{
    return (long)(std::exp((double) x / (double)(1 << 16))*(double)(1 << 16));
}

#define em2float(l) (*(float *) &(l))
#define float2em(f) (*(long *) &(f))

// Input is a 16 bit offset number.  Output is shifted by Fudge.
unsigned long ExpFudged(long x, int Fudge)
{
    return (long)(std::exp((double) x / (double)(1 << 16))*(double)(1 << Fudge));
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
    return (long) std::log((double) em2float(x))*(1 << 16);
}

// This divides two e/m numbers and returns an e/m number.
long RegDivFloat(long x, long y)
{
    float f = em2float(x)/em2float(y);
    return float2em(f);
}

// This routine on the IBM converts shifted integer x, FudgeFact to
// the 4 byte number: exp, mant, mant, mant
// Instead of using exp/mant format, we'll just use floats.
// Note: If sizeof(float) != sizeof(long), we're hosed.
//
long RegFg2Float(long x, int FudgeFact)
{
    float f = (float) x / (float)(1 << FudgeFact);
    return float2em(f);
}

// This converts em to shifted integer format.
//
long RegFloat2Fg(long x, int Fudge)
{
    return (long)(em2float(x)*(float)(1 << Fudge));
}

long RegSftFloat(long x, int Shift)
{
    float f = em2float(x);
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
