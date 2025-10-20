// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>

namespace id::fractals
{

extern std::filesystem::path g_formula_filename;
extern std::string           g_formula_name;
extern bool                  g_frm_uses_ismand;
extern bool                  g_frm_uses_p1;
extern bool                  g_frm_uses_p2;
extern bool                  g_frm_uses_p3;
extern bool                  g_frm_uses_p4;
extern bool                  g_frm_uses_p5;
extern bool                  g_is_mandelbrot;
extern int                   g_last_init_op;
extern int                   g_load_index;
extern char                  g_max_function;
extern unsigned              g_max_function_args;
extern unsigned              g_max_function_ops;
extern unsigned              g_operation_index;
extern int                   g_store_index;
extern unsigned              g_variable_index;

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
int formula_orbit();
int bad_formula();
int formula_per_pixel();
int frm_get_param_stuff(const char *name);
bool run_formula(const std::string &name, bool report_bad_sym);
bool formula_per_image();
void init_misc();
void free_work_area();

} // namespace id::fractals
