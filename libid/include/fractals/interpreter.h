// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::math
{
struct Arg;
}

namespace id::fractals
{

void d_stk_mul();
void d_stk_abs();
void d_stk_sqr();
void d_stk_conj();
void d_stk_zero();
void d_stk_one();
void d_stk_flip();
void d_stk_sin();
void d_stk_tan();
void d_stk_tanh();
void d_stk_cotan();
void d_stk_cotanh();
void d_stk_recip();
void stk_ident();
void d_stk_sinh();
void d_stk_cos();
void d_stk_cosxx();
void d_stk_cosh();
void d_stk_log();
void d_stk_exp();
void d_stk_pwr();
void d_stk_asin();
void d_stk_asinh();
void d_stk_acos();
void d_stk_acosh();
void d_stk_atan();
void d_stk_atanh();
void d_stk_cabs();
void d_stk_sqrt();
void d_stk_floor();
void d_stk_ceil();
void d_stk_trunc();
void d_stk_round();

void debug_trace_close();
void debug_trace_operation(const char* op_name, const math::Arg * arg1 = nullptr, const math::Arg * arg2 = nullptr);
void debug_trace_stack_state();

} // namespace id::fractals
