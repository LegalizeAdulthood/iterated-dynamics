#include "fractals/interpreter.h"

#include "fractals/parser.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "math/rand15.h"

#include <fmt/format.h>

#include <cassert>
#include <cfloat>
#include <cmath>

using namespace id::math;

namespace id::fractals
{

RuntimeState s_runtime;

static bool check_denom(const double denom)
{
    if (std::abs(denom) <= DBL_MIN)
    {
        g_overflow = true;
        return true;
    }
    return false;
}

void d_stk_add()
{
    debug_trace_operation("ADD", g_arg1, g_arg2);
    g_arg2->d.x += g_arg1->d.x;
    g_arg2->d.y += g_arg1->d.y;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
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
    cmplx_sin(g_arg1->d, g_arg1->d);
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
    cmplx_sinh(g_arg1->d, g_arg1->d);
    debug_trace_stack_state();
}

void d_stk_cos()
{
    debug_trace_operation("COS", g_arg1);
    cmplx_cos(g_arg1->d, g_arg1->d);
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
    cmplx_cosh(g_arg1->d, g_arg1->d);
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

static unsigned long new_random_num()
{
    s_runtime.rand_num = (s_runtime.rand_num << 15) + RAND15() ^ s_runtime.rand_num;
    return s_runtime.rand_num;
}

void d_random()
{
    /* Use the same algorithm as for fixed math so that they will generate
           the same fractals when the srand() function is used. */
    const long x = new_random_num() >> (32 - BIT_SHIFT);
    const long y = new_random_num() >> (32 - BIT_SHIFT);
    s_formula.vars[7].a.d.x = static_cast<double>(x) / (1L << BIT_SHIFT);
    s_formula.vars[7].a.d.y = static_cast<double>(y) / (1L << BIT_SHIFT);
}

static void set_random()
{
    if (!s_runtime.set_random)
    {
        s_runtime.rand_num = s_runtime.rand_x ^ s_runtime.rand_y;
    }

    const unsigned int seed = static_cast<unsigned>(s_runtime.rand_num) ^ static_cast<unsigned>(s_runtime.rand_num >> 16);
    std::srand(seed);
    s_runtime.set_random = true;

    // Clear out the seed
    new_random_num();
    new_random_num();
    new_random_num();
}

void random_seed()
{
    std::time_t now;

    // Use the current time to randomize the random number sequence.
    std::time(&now);
    std::srand(static_cast<unsigned int>(now));

    new_random_num();
    new_random_num();
    new_random_num();
    s_runtime.randomized = true;
}

void d_stk_srand()
{
    debug_trace_operation("SRAND", g_arg1);
    s_runtime.rand_x = static_cast<long>(g_arg1->d.x * (1L << BIT_SHIFT));
    s_runtime.rand_y = static_cast<long>(g_arg1->d.y * (1L << BIT_SHIFT));
    set_random();
    d_random();
    g_arg1->d = s_formula.vars[7].a.d;
    debug_trace_stack_state();
}

void d_stk_lod_dup()
{
    debug_trace_operation("LOD_DUP", s_formula.load[g_load_index]);
    g_arg1 += 2;
    g_arg2 += 2;
    *g_arg1 = *s_formula.load[g_load_index];
    *g_arg2 = *g_arg1;
    g_load_index += 2;
    debug_trace_stack_state();
}

void d_stk_lod_sqr()
{
    debug_trace_operation("LOD_SQR", s_formula.load[g_load_index]);
    g_arg1++;
    g_arg2++;
    g_arg1->d.y = s_formula.load[g_load_index]->d.x * s_formula.load[g_load_index]->d.y * 2.0;
    g_arg1->d.x = s_formula.load[g_load_index]->d.x * s_formula.load[g_load_index]->d.x - s_formula.load[g_load_index]->d.y * s_formula.load[g_load_index]->d.y;
    g_load_index++;
    debug_trace_stack_state();
}

void d_stk_lod_sqr2()
{
    debug_trace_operation("LOD_SQR2", s_formula.load[g_load_index]);
    g_arg1++;
    g_arg2++;
    LAST_SQR.d.x = s_formula.load[g_load_index]->d.x * s_formula.load[g_load_index]->d.x;
    LAST_SQR.d.y = s_formula.load[g_load_index]->d.y * s_formula.load[g_load_index]->d.y;
    g_arg1->d.y = s_formula.load[g_load_index]->d.x * s_formula.load[g_load_index]->d.y * 2.0;
    g_arg1->d.x = LAST_SQR.d.x - LAST_SQR.d.y;
    LAST_SQR.d.x += LAST_SQR.d.y;
    LAST_SQR.d.y = 0;
    g_load_index++;
    debug_trace_stack_state();
}

void d_stk_lod_dbl()
{
    debug_trace_operation("LOD_DBL", s_formula.load[g_load_index]);
    g_arg1++;
    g_arg2++;
    g_arg1->d.x = s_formula.load[g_load_index]->d.x * 2.0;
    g_arg1->d.y = s_formula.load[g_load_index]->d.y * 2.0;
    g_load_index++;
    debug_trace_stack_state();
}

void d_stk_sqr0()
{
    debug_trace_operation("SQR0", g_arg1);
    LAST_SQR.d.y = g_arg1->d.y * g_arg1->d.y; // use LastSqr as temp storage
    g_arg1->d.y = g_arg1->d.x * g_arg1->d.y * 2.0;
    g_arg1->d.x = g_arg1->d.x * g_arg1->d.x - LAST_SQR.d.y;
    debug_trace_stack_state();
}

void d_stk_sqr3()
{
    debug_trace_operation("SQR3", g_arg1);
    g_arg1->d.x = g_arg1->d.x * g_arg1->d.x;
    debug_trace_stack_state();
}

void d_stk_sqr()
{
    debug_trace_operation("SQR", g_arg1);
    LAST_SQR.d.x = g_arg1->d.x * g_arg1->d.x;
    LAST_SQR.d.y = g_arg1->d.y * g_arg1->d.y;
    g_arg1->d.y = g_arg1->d.x * g_arg1->d.y * 2.0;
    g_arg1->d.x = LAST_SQR.d.x - LAST_SQR.d.y;
    LAST_SQR.d.x += LAST_SQR.d.y;
    LAST_SQR.d.y = 0;
    debug_trace_stack_state();
}

void d_stk_sub()
{
    debug_trace_operation("SUB", g_arg1, g_arg2);
    g_arg2->d.x -= g_arg1->d.x;
    g_arg2->d.y -= g_arg1->d.y;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_real()
{
    debug_trace_operation("REAL", g_arg1);
    g_arg1->d.y = 0.0;
    debug_trace_stack_state();
}

void d_stk_imag()
{
    debug_trace_operation("IMAG", g_arg1);
    g_arg1->d.x = g_arg1->d.y;
    g_arg1->d.y = 0.0;
    debug_trace_stack_state();
}

void d_stk_neg()
{
    debug_trace_operation("NEG", g_arg1);
    g_arg1->d.x = -g_arg1->d.x;
    g_arg1->d.y = -g_arg1->d.y;
    debug_trace_stack_state();
}

void d_stk_div()
{
    debug_trace_operation("DIV", g_arg1, g_arg2);
    fpu_cmplx_div(g_arg2->d, g_arg1->d, g_arg2->d);
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_mod()
{
    debug_trace_operation("MOD", g_arg1);
    g_arg1->d.x = g_arg1->d.x * g_arg1->d.x + g_arg1->d.y * g_arg1->d.y;
    g_arg1->d.y = 0.0;
    debug_trace_stack_state();
}

void stk_sto()
{
    debug_trace_operation("STO", g_arg1);
    assert(s_formula.store[g_store_index] != nullptr);
    *s_formula.store[g_store_index++] = *g_arg1;
    debug_trace_stack_state();
}

void stk_lod()
{
    if (s_debug.trace_enabled && s_debug.trace_file)
    {
        // Try to identify which variable we're loading
        const char* var_name = "unknown";
        if (g_load_index < static_cast<int>(s_formula.vars.size()))
        {
            for (size_t i = 0; i < VARIABLES.size(); ++i)
            {
                if (&s_formula.vars[i].a == s_formula.load[g_load_index])
                {
                    var_name = VARIABLES[i];
                    break;
                }
            }
        }
        fmt::print(s_debug.trace_file, "{:04d}: {}LOAD {}\n", //
            s_debug.operation_count++, std::string(s_debug.indent_level * 2, ' '), var_name);
    }
    g_arg1++;
    g_arg2++;
    *g_arg1 = *s_formula.load[g_load_index++];
    debug_trace_stack_state();
}

void stk_clr()
{
    debug_trace_operation("CLR", g_arg1);
    s_runtime.stack[0] = *g_arg1;
    g_arg1 = s_runtime.stack.data();
    g_arg2 = s_runtime.stack.data();
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_fn1()
{
    g_d_trig0();
}

void d_stk_fn2()
{
    g_d_trig1();
}

void d_stk_fn3()
{
    g_d_trig2();
}

void d_stk_fn4()
{
    g_d_trig3();
}

void d_stk_lt()
{
    debug_trace_operation("LT", g_arg1, g_arg2);
    g_arg2->d.x = static_cast<double>(g_arg2->d.x < g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_gt()
{
    debug_trace_operation("GT", g_arg1, g_arg2);
    g_arg2->d.x = static_cast<double>(g_arg2->d.x > g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_lte()
{
    debug_trace_operation("LTE", g_arg1, g_arg2);
    g_arg2->d.x = static_cast<double>(g_arg2->d.x <= g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_gte()
{
    debug_trace_operation("GTE", g_arg1, g_arg2);
    g_arg2->d.x = static_cast<double>(g_arg2->d.x >= g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_eq()
{
    debug_trace_operation("EQ", g_arg1, g_arg2);
    g_arg2->d.x = static_cast<double>(g_arg2->d.x == g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_ne()
{
    debug_trace_operation("NE", g_arg1, g_arg2);
    g_arg2->d.x = static_cast<double>(g_arg2->d.x != g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_or()
{
    debug_trace_operation("OR", g_arg1, g_arg2);
    g_arg2->d.x = static_cast<double>(g_arg2->d.x != 0.0 || g_arg1->d.x != 0.0);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void d_stk_and()
{
    debug_trace_operation("AND", g_arg1, g_arg2);
    g_arg2->d.x = static_cast<double>(g_arg2->d.x != 0.0 && g_arg1->d.x != 0.0);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
    debug_trace_stack_state();
}

void end_init()
{
    g_last_init_op = s_runtime.op_ptr;
    s_runtime.init_jump_index = s_runtime.jump_index;
}

void stk_jump()
{
    if (s_debug.trace_enabled && s_debug.trace_file)
    {
        fmt::print(s_debug.trace_file, "{:04d}: {}JUMP\n", s_debug.operation_count++,
            std::string(s_debug.indent_level * 2, ' '));
        fmt::print(s_debug.trace_file, "      from op_ptr: {} to: {}\n", s_runtime.op_ptr,
            s_formula.jump_control[s_runtime.jump_index].ptrs.jump_op_ptr);
    }

    s_runtime.op_ptr =  s_formula.jump_control[s_runtime.jump_index].ptrs.jump_op_ptr;
    g_load_index = s_formula.jump_control[s_runtime.jump_index].ptrs.jump_lod_ptr;
    g_store_index = s_formula.jump_control[s_runtime.jump_index].ptrs.jump_sto_ptr;
    s_runtime.jump_index = s_formula.jump_control[s_runtime.jump_index].dest_jump_index;
}

void d_stk_jump_on_false()
{
    const bool will_jump = g_arg1->d.x == 0.0;

    if (s_debug.trace_enabled && s_debug.trace_file)
    {
        fmt::print(
            s_debug.trace_file, "      condition: {:.6f}, will jump: {}\n", g_arg1->d.x, will_jump ? "YES" : "NO");
        if (will_jump)
        {
            fmt::print(
                s_debug.trace_file, "      jumping to index: {}\n", s_formula.jump_control[s_runtime.jump_index].dest_jump_index);
        }
    }

    if (will_jump)
    {
        stk_jump();
    }
    else
    {
        s_runtime.jump_index++;
    }
}

void d_stk_jump_on_true()
{
    const bool will_jump = g_arg1->d.x != 0.0;

    if (s_debug.trace_enabled && s_debug.trace_file)
    {
        fmt::print(
            s_debug.trace_file, "      condition: {:.6f}, will jump: {}\n", g_arg1->d.x, will_jump ? "YES" : "NO");
        if (will_jump)
        {
            fmt::print(
                s_debug.trace_file, "      jumping to index: {}\n", s_formula.jump_control[s_runtime.jump_index].dest_jump_index);
        }
    }

    if (will_jump)
    {
        stk_jump();
    }
    else
    {
        s_runtime.jump_index++;
    }
}

void stk_jump_label()
{
    s_runtime.jump_index++;
}

} // namespace id::fractals
