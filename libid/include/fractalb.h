// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "big.h"

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

void compare_values(char const *s, LDBL x, bn_t bnx);
void compare_values_bf(char const *s, LDBL x, bf_t bfx);
void show_var_bf(char const *s, bf_t n);
void show_two_bf(char const *s1, bf_t t1, char const *s2, bf_t t2, int digits);
void bf_corners_to_float();
void show_corners_dbl(char const *s);
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
int bn_mod_bailout();
int bn_real_bailout();
int bn_imag_bailout();
int bn_or_bailout();
int bn_and_bailout();
int bn_manh_bailout();
int bn_manr_bailout();
int bf_mod_bailout();
int bf_real_bailout();
int bf_imag_bailout();
int bf_or_bailout();
int bf_and_bailout();
int bf_manh_bailout();
int bf_manr_bailout();
