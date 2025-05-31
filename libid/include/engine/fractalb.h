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
bool mandel_per_image_bn();
int mandel_per_pixel_bn();
int julia_per_pixel_bn();
int julia_orbit_bn();
int julia_z_power_bn_fractal();
BNComplex *cmplx_log_bn(BNComplex *t, BNComplex *s);
BNComplex *cmplx_mul_bn(BNComplex *t, BNComplex *x, BNComplex *y);
BNComplex *cmplx_div_bn(BNComplex *t, BNComplex *x, BNComplex *y);
BNComplex *cmplx_pow_bn(BNComplex *t, BNComplex *xx, BNComplex *yy);
bool mandel_per_image_bf();
int mandel_per_pixel_bf();
int julia_per_pixel_bf();
int julia_orbit_bf();
int julia_z_power_orbit_bf();
BFComplex *cmplx_log_bf(BFComplex *t, BFComplex *s);
BFComplex *cmplx_mul_bf(BFComplex *t, BFComplex *x, BFComplex *y);
BFComplex *cmplx_div_bf(BFComplex *t, BFComplex *x, BFComplex *y);
BFComplex *cmplx_pow_bf(BFComplex *t, BFComplex *xx, BFComplex *yy);

// Helper routines for debugging
#ifndef NDEBUG
void show_var_bn(const char *s, BigNum n);
void show_corners_dbl(const char *s);
void show_corners_bn(const char *s);
void show_globals_bf(const char *s);
void show_corners_bf(const char *s);
void show_corners_bf_save(const char *s);
void show_two_bf(const char *s1, BigFloat t1, const char *s2, BigFloat t2, int digits);
void show_three_bf(
    const char *s1, BigFloat t1, const char *s2, BigFloat t2, const char *s3, BigFloat t3, int digits);
void show_aspect(const char *s);
void compare_values(const char *s, LDouble x, BigNum bnx);
void compare_values_bf(const char *s, LDouble x, BigFloat bfx);
void show_var_bf(const char *s, BigFloat n);
#endif
