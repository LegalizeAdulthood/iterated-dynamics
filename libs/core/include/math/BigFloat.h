// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/cmplx.h>
#include <math/MPMath.h>
#include <misc/sized_types.h>

#include <config/port.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace id::math
{

class MPConversions;

class BigFloat
{
public:
    // Default construction is null; numeric construction uses current precision.
    BigFloat() noexcept;
    explicit BigFloat(LDouble value);
    explicit BigFloat(std::string_view text);
    BigFloat(const BigFloat &value);
    BigFloat(BigFloat &&value) noexcept;
    BigFloat &operator=(const BigFloat &value);
    BigFloat &operator=(BigFloat &&value);
    BigFloat &operator=(LDouble value);
    void clear();
    void load_pi();

    // Tests for a value, including numeric zero.
    explicit operator bool() const noexcept
    {
        return m_value.has_value();
    }

    BigFloat operator-() const;
    BigFloat &operator+=(const BigFloat &right);
    BigFloat &operator-=(const BigFloat &right);
    BigFloat &operator*=(const BigFloat &right);
    BigFloat &operator/=(const BigFloat &right);
    BigFloat &operator/=(misc::U16 right);

    BigFloat operator+(const BigFloat &right) const;
    BigFloat operator-(const BigFloat &right) const;
    BigFloat operator*(const BigFloat &right) const;
    BigFloat operator*(misc::U16 right) const;
    BigFloat operator/(const BigFloat &right) const;
    BigFloat abs() const;
    BigFloat sqr() const;
    BigFloat half() const;
    BigFloat log() const;
    BigFloat exp() const;
    std::pair<BigFloat, BigFloat> sin_cos() const;
    BigFloat atan2(const BigFloat &x) const;

    bool operator==(const BigFloat &right) const;
    bool operator!=(const BigFloat &right) const;
    bool operator<(const BigFloat &right) const;
    bool operator<=(const BigFloat &right) const;
    bool operator>(const BigFloat &right) const;
    bool operator>=(const BigFloat &right) const;

    int sign() const;
    bool is_zero() const;

    LDouble to_float() const;
    std::string to_string(int digits = 0) const;
    std::string to_scientific_string(int digits = 0) const;
    void hex_dump() const;

    void set_precision(const MPPrecision &precision);

    void pack_bf(Byte *record, std::size_t length) const;
    static const Byte *unpack_bf(BigFloat &destination, const Byte *source, int stored_length);

private:
    friend class MPConversions;
    friend Complex<BigFloat> log(const Complex<BigFloat> &value);
    friend Complex<BigFloat> pow(const Complex<BigFloat> &base, const Complex<BigFloat> &exponent);

    static BigFloat from_value(MPReal value);

    std::optional<MPReal> m_value;
};

BigFloat abs(const BigFloat &value);
BigFloat sqr(const BigFloat &value);
BigFloat log(const BigFloat &value);
BigFloat exp(const BigFloat &value);
std::pair<BigFloat, BigFloat> sin_cos(const BigFloat &value);
BigFloat atan2(const BigFloat &y, const BigFloat &x);

using BFComplex = Complex<BigFloat>;

extern BigFloat g_bf_pi;
extern BigFloat g_bf_tmp;

// g_bf_length+2
extern BigFloat g_delta_x_bf;
extern BigFloat g_delta_y_bf;
extern BigFloat g_delta2_x_bf;
extern BigFloat g_delta2_y_bf;
extern BigFloat g_close_enough_bf;
extern BigFloat g_tmp_sqr_x_bf;
extern BigFloat g_tmp_sqr_y_bf;
extern BFComplex g_param_z_bf;
extern BFComplex g_saved_z_bf;
extern BFComplex g_old_z_bf;
extern BFComplex g_new_z_bf;

// g_bf_length+2
extern BigFloat g_bf_x_min;
extern BigFloat g_bf_x_max;
extern BigFloat g_bf_y_min;
extern BigFloat g_bf_y_max;
extern BigFloat g_bf_x_3rd;
extern BigFloat g_bf_y_3rd;
extern BigFloat g_bf_save_x_min;
extern BigFloat g_bf_save_x_max;
extern BigFloat g_bf_save_y_min;
extern BigFloat g_bf_save_y_max;
extern BigFloat g_bf_save_x_3rd;
extern BigFloat g_bf_save_y_3rd;
extern BigFloat g_bf_params[10]; // (g_bf_length+2)*10

} // namespace id::math
