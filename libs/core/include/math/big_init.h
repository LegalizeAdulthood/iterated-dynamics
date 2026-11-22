// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/PrecisionError.h>

#include <config/port.h>

#include <cstddef>
#include <vector>

namespace id::fractals
{
enum class FractalType;
}

namespace id::math
{

class MPPrecision;

// values that g_bf_math can hold,
// 0 = g_bf_math is not being used
// 2 = g_bf_math is being used
enum class BFMathType
{
    // cppcheck-suppress variableHidingEnum
    NONE = 0,
    BIG_FLT = 2 // g_bf_math is being used with BigFloat numbers
};

// globals
extern BFMathType g_bf_math;
extern int g_int_length;
extern int g_bn_length;
extern int g_decimals;
extern int g_bf_length;
extern int g_bf_decimals;
extern int g_bf_digits; // digits to use (force) for g_bf_math

// Request the precision needed to distinguish adjacent pixels at the
// maximum resolution of MAX_PIXELS by MAX_PIXELS or at current resolution
enum class ResolutionFlag
{
    MAX = 0,
    CURRENT = 1,
};

// Legacy file precision uses base-256 mantissas.
inline constexpr double LOG10_256{2.4082399653118};
MPPrecision bf_precision();

std::vector<Byte> pack_bf_parameters();
void unpack_bf_parameters(const std::vector<Byte> &parameters);

void free_bf_vars();
void init_bf_dec(int dec);
void init_bf_length(int bnl);
// Validate stored precision and return the corner/parameter payload size.
size_t bf_parameter_bytes(int bn_length, fractals::FractalType type);

} // namespace id::math
