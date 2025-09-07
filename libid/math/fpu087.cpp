// SPDX-License-Identifier: GPL-3.0-only
//
#include "math/fpu087.h"

#include "math/fixed_pt.h"

#include <cfloat>
#include <cmath>

namespace id::math
{

void fpu_cmplx_mul(const DComplex *x, const DComplex *y, DComplex *z)
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

void fpu_cmplx_div(const DComplex *x, const DComplex *y, DComplex *z)
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
        const double tmp{y->x};
        z->x = x->x / tmp;
        z->y = x->y / tmp;
    }
    else
    {
        const double y_x_mod = y->x / mod;
        const double y_y_mod = -y->y / mod;
        // Need to compute into temporaries to avoid pointer aliasing
        const double tx = x->x * y_x_mod - x->y * y_y_mod;
        const double ty = x->x * y_y_mod + x->y * y_x_mod;
        z->x = tx;
        z->y = ty;
    }
}

void sin_cos(double angle, double *sine, double *cosine)
{
    if (std::isnan(angle) || std::isinf(angle))
    {
        *sine = 0.0;
        *cosine = 1.0;
    }
    else
    {
        *sine = std::sin(angle);
        *cosine = std::cos(angle);
    }
}

void sinh_cosh(double angle, double *sine, double *cosine)
{
    if (std::isnan(angle) || std::isinf(angle))
    {
        *sine = 1.0;
        *cosine = 1.0;
    }
    else
    {
        *sine = std::sinh(angle);
        *cosine = std::cosh(angle);
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

void fpu_cmplx_log(const DComplex *x, DComplex *z)
{
    if (x->x == 0.0 && x->y == 0.0)
    {
        z->x = 0.0;
        z->y = 0.0;
        return;
    }
    if (x->y == 0.0)// x is real
    {
        z->x = std::log(x->x);
        z->y = 0.0;
        return;
    }
    const double mod = x->x * x->x + x->y * x->y;
    double real = std::isnan(mod) || std::islessequal(mod, 0) || std::isinf(mod) //
        ? 0.5
        : 0.5 * std::log(mod);
    double imag = std::isnan(x->x) || std::isnan(x->y) || std::isinf(x->x) || std::isinf(x->y) //
        ? 1.0
        : std::atan2(x->y, x->x);
    z->x = real;
    z->y = imag;
}

void fpu_cmplx_exp(const DComplex *x, DComplex *z)
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

} // namespace id::math
