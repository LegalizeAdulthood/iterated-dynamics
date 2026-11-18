// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/BigFloat.h>
#include <math/BigNum.h>

#include <config/port.h>

#include <string>
#include <vector>

namespace id::engine
{

inline math::DComplex cmplx_bn_to_float(const math::BNComplex &s)
{
    math::DComplex t;
    t.x = static_cast<double>(s.x.to_float());
    t.y = static_cast<double>(s.y.to_float());
    return t;
}

inline math::DComplex cmplx_bf_to_float(const math::BFComplex &s)
{
    math::DComplex t;
    t.x = static_cast<double>(s.x.to_float());
    t.y = static_cast<double>(s.y.to_float());
    return t;
}

void bf_corners_to_float();
bool mandel_per_image_bn();
int mandel_per_pixel_bn();
int julia_per_pixel_bn();
int julia_orbit_bn();
bool mandel_per_image_bf();
int mandel_per_pixel_bf();
int julia_per_pixel_bf();
int julia_orbit_bf();
int julia_z_power_orbit_bf();

// Numeric snapshots collected by debug checks and reported by UI owners.
extern std::vector<std::string> g_precision_diagnostics;

std::string format_var_bn(const char *s, const math::BigNum &n);
std::string format_corners_dbl(const char *s);
std::string format_corners_bn(const char *s);
std::string format_globals_bf(const char *s);
std::string format_corners_bf(const char *s);
std::string format_corners_bf_save(const char *s);
std::string format_two_bf(
    const char *s1, const math::BigFloat &t1, const char *s2, const math::BigFloat &t2, int digits);
std::string format_three_bf(const char *s1, const math::BigFloat &t1, const char *s2, const math::BigFloat &t2,
    const char *s3, const math::BigFloat &t3, int digits);
std::string format_aspect(const char *s);
std::string format_compare_values(const char *s, LDouble x, const math::BigNum &bnx);
std::string format_compare_values_bf(const char *s, LDouble x, const math::BigFloat &bfx);
std::string format_var_bf(const char *s, const math::BigFloat &n);

} // namespace id::engine
