// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/arg.h"

#include <array>

namespace id::math
{
struct Arg;
}

namespace id::fractals
{

struct RuntimeState
{
    std::array<math::Arg, 20> stack{};
    int op_ptr{};
    int jump_index{};

    int init_op_ptr{};
    int init_jump_index{};
    int init_load_ptr{};
    int init_store_ptr{};

    bool set_random{};
    bool randomized{};
    unsigned long rand_num{};
    long rand_x{};
    long rand_y{};
};

extern RuntimeState s_runtime;

void d_stk_add();
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
