#include "fractals/interpreter.h"

#include "math/arg.h"
#include "math/fixed_pt.h"

using namespace id::math;

namespace id::fractals
{

static bool check_denom(const double denom)
{
    if (std::abs(denom) <= DBL_MIN)
    {
        g_overflow = true;
        return true;
    }
    return false;
}

void d_stk_mul()
{
    debug_trace_operation("MUL", g_arg1, g_arg2);
    fpu_cmplx_mul(g_arg2->d, g_arg1->d, g_arg2->d);
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_abs()
{
    debug_trace_operation("ABS", g_arg1);
    g_arg1->d.x = std::abs(g_arg1->d.x);
    g_arg1->d.y = std::abs(g_arg1->d.y);
    debug_trace_stack_state();
}

void d_stk_conj()
{
    debug_trace_operation("CONJ", g_arg1);
    g_arg1->d.y = -g_arg1->d.y;
    debug_trace_stack_state();
}

void d_stk_zero()
{
    debug_trace_operation("ZERO");
    g_arg1->d.x = 0.0;
    g_arg1->d.y = 0.0;
    debug_trace_stack_state();
}

void d_stk_one()
{
    debug_trace_operation("ONE");
    g_arg1->d.x = 1.0;
    g_arg1->d.y = 0.0;
    debug_trace_stack_state();
}

void d_stk_flip()
{
    debug_trace_operation("FLIP", g_arg1);
    const double t = g_arg1->d.x;
    g_arg1->d.x = g_arg1->d.y;
    g_arg1->d.y = t;
    debug_trace_stack_state();
}

void d_stk_sin()
{
    debug_trace_operation("SIN", g_arg1);
    double sin_x;
    double cos_x;
    double sinh_y;
    double cosh_y;

    sin_cos(g_arg1->d.x, sin_x, cos_x);
    sinh_cosh(g_arg1->d.y, sinh_y, cosh_y);
    g_arg1->d.x = sin_x*cosh_y;
    g_arg1->d.y = cos_x*sinh_y;
    debug_trace_stack_state();
}

/* The following functions are supported by both the parser and for fn
   variable replacement.
*/
void d_stk_tan()
{
    debug_trace_operation("TAN", g_arg1);
    double sin_x;
    double cos_x;
    double sinh_y;
    double cosh_y;
    g_arg1->d.x *= 2;
    g_arg1->d.y *= 2;
    sin_cos(g_arg1->d.x, sin_x, cos_x);
    sinh_cosh(g_arg1->d.y, sinh_y, cosh_y);
    const double denom = cos_x + cosh_y;
    if (check_denom(denom))
    {
        return;
    }
    g_arg1->d.x = sin_x/denom;
    g_arg1->d.y = sinh_y/denom;
    debug_trace_stack_state();
}

void d_stk_tanh()
{
    debug_trace_operation("TANH", g_arg1);
    double sin_y;
    double cos_y;
    double sinh_x;
    double cosh_x;
    g_arg1->d.x *= 2;
    g_arg1->d.y *= 2;
    sin_cos(g_arg1->d.y, sin_y, cos_y);
    sinh_cosh(g_arg1->d.x, sinh_x, cosh_x);
    const double denom = cosh_x + cos_y;
    if (check_denom(denom))
    {
        return;
    }
    g_arg1->d.x = sinh_x/denom;
    g_arg1->d.y = sin_y/denom;
    debug_trace_stack_state();
}

void d_stk_cotan()
{
    debug_trace_operation("COTAN", g_arg1);
    double sin_x;
    double cos_x;
    double sinh_y;
    double cosh_y;
    g_arg1->d.x *= 2;
    g_arg1->d.y *= 2;
    sin_cos(g_arg1->d.x, sin_x, cos_x);
    sinh_cosh(g_arg1->d.y, sinh_y, cosh_y);
    const double denom = cosh_y - cos_x;
    if (check_denom(denom))
    {
        debug_trace_stack_state();
        return;
    }
    g_arg1->d.x = sin_x/denom;
    g_arg1->d.y = -sinh_y/denom;
    debug_trace_stack_state();
}

void d_stk_cotanh()
{
    debug_trace_operation("COTANH", g_arg1);
    double sin_y;
    double cos_y;
    double sinh_x;
    double cosh_x;
    g_arg1->d.x *= 2;
    g_arg1->d.y *= 2;
    sin_cos(g_arg1->d.y, sin_y, cos_y);
    sinh_cosh(g_arg1->d.x, sinh_x, cosh_x);
    const double denom = cosh_x - cos_y;
    if (check_denom(denom))
    {
        debug_trace_stack_state();
        return;
    }
    g_arg1->d.x = sinh_x/denom;
    g_arg1->d.y = -sin_y/denom;
    debug_trace_stack_state();
}

/* The following functions are not directly used by the parser - support
   for the parser was not provided because the existing parser language
   represents these quite easily. They are used for fn variable support
   in miscres.c but are placed here because they follow the pattern of
   the other parser functions.
*/

void d_stk_recip()
{
    debug_trace_operation("RECIP", g_arg1);
    const double mod = g_arg1->d.x * g_arg1->d.x + g_arg1->d.y * g_arg1->d.y;
    if (check_denom(mod))
    {
        debug_trace_stack_state();
        return;
    }
    g_arg1->d.x =  g_arg1->d.x/mod;
    g_arg1->d.y = -g_arg1->d.y/mod;
    debug_trace_stack_state();
}

void stk_ident()
{
    debug_trace_operation("IDENT", g_arg1);
    // do nothing - the function Z
    debug_trace_stack_state();
}

void d_stk_sinh()
{
    debug_trace_operation("SINH", g_arg1);
    double sin_y;
    double cos_y;
    double sinh_x;
    double cosh_x;

    sin_cos(g_arg1->d.y, sin_y, cos_y);
    sinh_cosh(g_arg1->d.x, sinh_x, cosh_x);
    g_arg1->d.x = sinh_x*cos_y;
    g_arg1->d.y = cosh_x*sin_y;
    debug_trace_stack_state();
}

void d_stk_cos()
{
    debug_trace_operation("COS", g_arg1);
    double sin_x;
    double cos_x;
    double sinh_y;
    double cosh_y;

    sin_cos(g_arg1->d.x, sin_x, cos_x);
    sinh_cosh(g_arg1->d.y, sinh_y, cosh_y);
    g_arg1->d.x = cos_x*cosh_y;
    g_arg1->d.y = -sin_x*sinh_y;
    debug_trace_stack_state();
}

// Bogus version of cos, to replicate bug which was in regular cos till v16:
void d_stk_cosxx()
{
    debug_trace_operation("COSXX", g_arg1);
    d_stk_cos();
    g_arg1->d.y = -g_arg1->d.y;
    debug_trace_stack_state();
}

void d_stk_cosh()
{
    debug_trace_operation("COSH", g_arg1);
    double sin_y;
    double cos_y;
    double sinh_x;
    double cosh_x;

    sin_cos(g_arg1->d.y, sin_y, cos_y);
    sinh_cosh(g_arg1->d.x, sinh_x, cosh_x);
    g_arg1->d.x = cosh_x*cos_y;
    g_arg1->d.y = sinh_x*sin_y;
    debug_trace_stack_state();
}

void d_stk_log()
{
    debug_trace_operation("LOG", g_arg1);
    cmplx_log(g_arg1->d, g_arg1->d);
    debug_trace_stack_state();
}

void d_stk_exp()
{
    debug_trace_operation("EXP", g_arg1);
    cmplx_exp(g_arg1->d, g_arg1->d);
    debug_trace_stack_state();
}

void d_stk_pwr()
{
    debug_trace_operation("PWR", g_arg1, g_arg2);
    g_arg2->d = pow(g_arg2->d, g_arg1->d);
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_asin()
{
    debug_trace_operation("ASIN", g_arg1);
    asin(g_arg1->d, g_arg1->d);
    debug_trace_stack_state();
}

void d_stk_asinh()
{
    debug_trace_operation("ASINH", g_arg1);
    asinh(g_arg1->d, g_arg1->d);
    debug_trace_stack_state();
}

void d_stk_acos()
{
    debug_trace_operation("ACOS", g_arg1);
    acos(g_arg1->d, g_arg1->d);
    debug_trace_stack_state();
}

void d_stk_acosh()
{
    debug_trace_operation("ACOSH", g_arg1);
    acosh(g_arg1->d, g_arg1->d);
    debug_trace_stack_state();
}

void d_stk_atan()
{
    debug_trace_operation("ATAN", g_arg1);
    atan(g_arg1->d, g_arg1->d);
    debug_trace_stack_state();
}

void d_stk_atanh()
{
    debug_trace_operation("ATANH", g_arg1);
    atanh(g_arg1->d, g_arg1->d);
    debug_trace_stack_state();
}

void d_stk_cabs()
{
    debug_trace_operation("CABS", g_arg1);
    g_arg1->d.x = std::sqrt(sqr(g_arg1->d.x)+sqr(g_arg1->d.y));
    g_arg1->d.y = 0.0;
    debug_trace_stack_state();
}

void d_stk_sqrt()
{
    debug_trace_operation("SQRT", g_arg1);
    g_arg1->d = sqrt(g_arg1->d.x, g_arg1->d.y);
    debug_trace_stack_state();
}

void d_stk_floor()
{
    debug_trace_operation("FLOOR", g_arg1);
    g_arg1->d.x = floor(g_arg1->d.x);
    g_arg1->d.y = floor(g_arg1->d.y);
    debug_trace_stack_state();
}

void d_stk_ceil()
{
    debug_trace_operation("CEIL", g_arg1);
    g_arg1->d.x = ceil(g_arg1->d.x);
    g_arg1->d.y = ceil(g_arg1->d.y);
    debug_trace_stack_state();
}

void d_stk_trunc()
{
    debug_trace_operation("TRUNC", g_arg1);
    g_arg1->d.x = static_cast<int>(g_arg1->d.x);
    g_arg1->d.y = static_cast<int>(g_arg1->d.y);
    debug_trace_stack_state();
}

void d_stk_round()
{
    debug_trace_operation("ROUND", g_arg1);
    g_arg1->d.x = floor(g_arg1->d.x+.5);
    g_arg1->d.y = floor(g_arg1->d.y+.5);
    debug_trace_stack_state();
}

} // namespace id::fractals
