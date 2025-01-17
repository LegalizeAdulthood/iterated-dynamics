// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/big.h"

#include <config/port.h>

inline DComplex cmplx_bn_to_float(const BNComplex &s)
{
    DComplex t;
    t.x = (double)bn_to_float(s.x);
    t.y = (double)bn_to_float(s.y);
    return t;
}

inline DComplex cmplx_bf_to_float(const BFComplex &s)
{
    DComplex t;
    t.x = (double)bf_to_float(s.x);
    t.y = (double)bf_to_float(s.y);
    return t;
}

void bf_corners_to_float();
bool mandel_bn_setup();
int mandel_bn_per_pixel();
int julia_bn_per_pixel();
int julia_bn_fractal();
int julia_z_power_bn_fractal();
BNComplex *cmplx_log_bn(BNComplex *t, BNComplex *s);
BNComplex *cmplx_mul_bn(BNComplex *t, BNComplex *x, BNComplex *y);
BNComplex *cmplx_div_bn(BNComplex *t, BNComplex *x, BNComplex *y);
BNComplex *cmplx_pow_bn(BNComplex *t, BNComplex *xx, BNComplex *yy);
bool mandel_bf_setup();
int mandel_bf_per_pixel();
int julia_bf_per_pixel();
int julia_bf_fractal();
int julia_z_power_bf_fractal();
BFComplex *cmplx_log_bf(BFComplex *t, BFComplex *s);
BFComplex *cmplx_mul_bf(BFComplex *t, BFComplex *x, BFComplex *y);
BFComplex *cmplx_div_bf(BFComplex *t, BFComplex *x, BFComplex *y);
BFComplex *cmplx_pow_bf(BFComplex *t, BFComplex *xx, BFComplex *yy);

// Helper routines for debugging
#ifndef NDEBUG
void show_var_bn(char const *s, BigNum n);
void show_corners_dbl(char const *s);
void show_corners_bn(char const *s);
void show_globals_bf(char const *s);
void show_corners_bf(char const *s);
void show_corners_bf_save(char const *s);
void show_two_bf(char const *s1, BigFloat t1, char const *s2, BigFloat t2, int digits);
void show_three_bf(char const *s1, BigFloat t1, char const *s2, BigFloat t2, char const *s3, BigFloat t3, int digits);
void show_aspect(char const *s);
void compare_values(char const *s, LDouble x, BigNum bnx);
void compare_values_bf(char const *s, LDouble x, BigFloat bfx);
void show_var_bf(char const *s, BigFloat n);
#endif
