// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/arg.h"

#include <array>
#include <cstdio>

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

    void orbit_begin();
    void orbit_end();
    void per_pixel_begin();
    void per_pixel_init();
};

extern RuntimeState s_runtime;

void d_stk_lod_dup();
void d_stk_lod_sqr();
void d_stk_lod_sqr2();
void d_stk_lod_dbl();
void d_stk_sqr0();
void d_stk_sqr3();
void d_stk_sub();
void d_stk_real();
void d_stk_imag();
void d_stk_neg();
void d_stk_div();
void d_stk_mod();
void stk_sto();
void stk_lod();
void d_stk_lt();
void d_stk_gt();
void d_stk_lte();
void d_stk_gte();
void d_stk_eq();
void d_stk_ne();
void d_stk_or();
void d_stk_and();
void d_stk_fn1();
void stk_clr();
void d_stk_fn2();
void d_stk_fn3();
void d_stk_fn4();
void end_init();
void stk_jump();
void d_stk_jump_on_false();
void d_stk_jump_on_true();
void stk_jump_label();

extern const std::array<const char *, 19> VARIABLES;

void debug_trace_init();
void random_seed();
void d_stk_srand();
void d_random();
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
