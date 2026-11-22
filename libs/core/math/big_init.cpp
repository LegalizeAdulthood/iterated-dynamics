// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/big_init.h>

#include <engine/bailout_formula.h>
#include <engine/calcfrac.h>
#include <engine/UserData.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <math/BigFloat.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;

namespace id::math
{

int g_bf_digits{};                      // digits to use (force) for g_bf_math

int g_bn_length{};
int g_int_length{};
int g_decimals{};
int g_bf_length{};
int g_bf_decimals{};
BigFloat g_delta_x_bf{};
BigFloat g_delta_y_bf{};
BigFloat g_delta2_x_bf{};
BigFloat g_delta2_y_bf{};
BigFloat g_close_enough_bf{};
BigFloat g_tmp_sqr_x_bf{};
BigFloat g_tmp_sqr_y_bf{};
BFComplex g_param_z_bf{};
BFComplex g_saved_z_bf{};
BFComplex g_old_z_bf{};
BFComplex g_new_z_bf{};
BigFloat g_bf_x_min{};
BigFloat g_bf_x_max{};
BigFloat g_bf_y_min{};
BigFloat g_bf_y_max{};
BigFloat g_bf_x_3rd{};
BigFloat g_bf_y_3rd{};
BigFloat g_bf_save_x_min{};
BigFloat g_bf_save_x_max{};
BigFloat g_bf_save_y_min{};
BigFloat g_bf_save_y_max{};
BigFloat g_bf_save_x_3rd{};
BigFloat g_bf_save_y_3rd{};
BigFloat g_bf_params[10]{};
BigFloat g_bf_tmp{};

// Preserve the accepted saved-file precision range.
constexpr int MAX_BIGFLOAT_LENGTH{708};

static bool supported_bn_length(const int bn_length)
{
    return bn_length > 0 && bn_length <= MAX_BIGFLOAT_LENGTH;
}

static int bf_length_for_bn_length(const int bn_length)
{
    return (bn_length + 3) / 4 * 4 + 4;
}

size_t bf_parameter_bytes(const int bn_length, const FractalType)
{
    if (!supported_bn_length(bn_length))
    {
        throw PrecisionError(fmt::format("Requested bignumber length of {} is unsupported", bn_length));
    }
    return 22 * static_cast<size_t>(bf_length_for_bn_length(bn_length) + 2);
}

namespace
{

std::array<BigFloat *, 22> parameter_bindings()
{
    return {&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd, &g_bf_params[0],
        &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4], &g_bf_params[5], &g_bf_params[6],
        &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min, &g_bf_save_x_max, &g_bf_save_y_min,
        &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
}

std::array<BigFloat *, 16> working_values()
{
    return {&g_delta_x_bf, &g_delta_y_bf, &g_delta2_x_bf, &g_delta2_y_bf, &g_param_z_bf.x, &g_param_z_bf.y,
        &g_old_z_bf.x, &g_old_z_bf.y, &g_new_z_bf.x, &g_new_z_bf.y, &g_saved_z_bf.x, &g_saved_z_bf.y,
        &g_close_enough_bf, &g_tmp_sqr_x_bf, &g_tmp_sqr_y_bf, &g_bf_tmp};
}

} // namespace

static void init_bf2(const int bn_length, const int int_length, const int decimals)
{
    if (!supported_bn_length(bn_length))
    {
        throw PrecisionError(fmt::format("Requested precision of {} too high", decimals));
    }
    const int rounded_length = bf_length_for_bn_length(bn_length) - 4;
    const bool active = static_cast<bool>(g_bf_x_min);
    const bool same_precision = active && g_decimals == decimals;
    const bool same_layout = active && g_bn_length == rounded_length && g_int_length == int_length;
    g_bn_length = rounded_length;
    g_int_length = int_length;
    g_decimals = decimals;
    g_bf_length = rounded_length + 4;
    g_bf_decimals = static_cast<int>((g_bf_length - 2) * LOG10_256);
    g_bf_math = BFMathType::BIG_FLT;
    const MPPrecision precision{bf_precision()};
    for (BigFloat *target : working_values())
    {
        *target = BigFloat{0};
    }
    for (BigFloat *target : parameter_bindings())
    {
        if (!*target)
        {
            *target = BigFloat{0};
        }
        if (!same_precision)
        {
            target->set_precision(precision);
        }
    }
    if (active && !same_layout)
    {
        for (int i = 0; i < MAX_PARAMS; ++i)
        {
            g_bf_params[i] = g_params[i];
        }
    }
}

MPPrecision bf_precision()
{
    // A stored byte layout can have no requested decimal fraction digits.
    const int native_digits = std::numeric_limits<LDouble>::max_digits10 - static_cast<int>(MPPrecision::GUARD_DIGITS);
    const int digits = std::max(g_decimals > 0 ? g_decimals : g_bf_decimals, native_digits);
    return MPPrecision{std::max(digits, 1), g_bf_digits};
}

std::vector<Byte> pack_bf_parameters()
{
    const std::size_t length = static_cast<std::size_t>(g_bf_length) + 2;
    std::vector<Byte> result(22 * length);
    std::size_t offset = 0;
    for (const BigFloat *value : parameter_bindings())
    {
        value->pack_bf(result.data() + offset, length);
        offset += length;
    }
    return result;
}

void unpack_bf_parameters(const std::vector<Byte> &parameters)
{
    const std::size_t length = parameters.size() / 22;
    if (parameters.size() % 22 != 0 || length <= 6 || length > static_cast<std::size_t>(MAX_BIGFLOAT_LENGTH) + 6)
    {
        throw std::invalid_argument("Incomplete arbitrary precision parameter payload");
    }
    // Two exponent bytes and four extra BigFloat mantissa bytes.
    const int bn_length = static_cast<int>(length) - 6;
    if (parameters.size() != bf_parameter_bytes(bn_length, g_fractal_type))
    {
        throw std::invalid_argument("Incomplete arbitrary precision parameter payload");
    }
    init_bf_length(bn_length);
    const Byte *cursor = parameters.data();
    for (BigFloat *value : parameter_bindings())
    {
        cursor = BigFloat::unpack_bf(*value, cursor, g_bf_length);
    }
}

void free_bf_vars()
{
    for (BigFloat *target : working_values())
    {
        *target = BigFloat{};
    }
    for (BigFloat *target : parameter_bindings())
    {
        *target = BigFloat{};
    }
    g_bf_math = BFMathType::NONE;
    g_bn_length = 0;
    g_int_length = 0;
    g_decimals = 0;
    g_bf_length = 0;
    g_bf_decimals = 0;
}

static int integer_length()
{
    if (g_user.bailout_value > 10)      // arbitrary value
    {
        // using 2 doesn't gain much and requires another test
        return 4;
    }
    else if (g_fractal_type == FractalType::MANDEL_Z_POWER //
        || g_fractal_type == FractalType::JULIA_Z_POWER    //
        || g_fractal_type == FractalType::DIVIDE_BROT5)
    {
        return 4;                                          // 2 leaves artifacts in the center of the lakes
    }
    // the bailout tests need greater dynamic range
    else if (g_bailout_test == Bailout::REAL //
        || g_bailout_test == Bailout::IMAG   //
        || g_bailout_test == Bailout::AND    //
        || g_bailout_test == Bailout::MANR)
    {
        return 2;
    }
    return 1;
}

/************************************************************************/
// initialize bignumber global variables
//   dec = decimal places after decimal point
void init_bf_dec(const int dec)
{
    const int decimals{g_bf_digits > 0 ? g_bf_digits : dec};
    const int int_length{integer_length()};
    // conservative estimate
    const int bn_length{int_length + static_cast<int>(decimals / LOG10_256) + 1}; // round up
    init_bf2(bn_length, int_length, decimals);
}

/************************************************************************/
// initialize bignumber global variables
//   bnl = bignumber length
//   intl = bytes for integer part (1, 2, or 4)
void init_bf_length(const int bnl)
{
    if (bnl <= 0 || bnl > MAX_BIGFLOAT_LENGTH)
    {
        throw PrecisionError(fmt::format("Requested bignumber length of {} is unsupported", bnl));
    }
    const int int_length{integer_length()};
    // conservative estimate
    const int decimals{static_cast<int>((bnl - int_length) * LOG10_256)};
    init_bf2(bnl, int_length, decimals);
}

} // namespace id::math
