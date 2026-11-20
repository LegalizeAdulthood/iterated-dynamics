// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/BigArena.h>
#include <math/cmplx.h>
#include <misc/sized_types.h>

#include <config/port.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace id::math
{

class BigFloat;
class BigNum
{
public:
    // Default construction is null; numeric construction and math need a context.
    BigNum() noexcept;
    explicit BigNum(LDouble value);
    explicit BigNum(std::string_view text);
    explicit BigNum(const BigFloat &value);
    explicit BigNum(BigArena &arena);
    BigNum(BigArena &arena, LDouble value);
    BigNum(BigArena &arena, std::string_view text);
    BigNum(BigArena &arena, const BigFloat &value);
    BigNum(const BigNum &value);
    BigNum(BigArena &arena, const BigNum &value);
    BigNum(BigNum &&value) noexcept;
    BigNum &operator=(const BigNum &value);
    BigNum &operator=(BigNum &&value);
    BigNum &operator=(const BigFloat &value);
    BigNum &operator=(LDouble value);
    void clear();

    // Tests for an allocated value, including numeric zero.
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_storage);
    }

    BigArena &arena() const noexcept
    {
        return m_storage.arena();
    }

    BigNum operator-() const;
    BigNum &operator+=(const BigNum &right);
    BigNum &operator-=(const BigNum &right);
    BigNum &operator*=(const BigNum &right);
    BigNum &operator/=(const BigNum &right);
    BigNum &operator/=(misc::U16 right);

    BigNum operator+(const BigNum &right) const;
    BigNum operator-(const BigNum &right) const;
    BigNum operator*(const BigNum &right) const;
    BigNum operator*(misc::U16 right) const;
    BigNum operator/(const BigNum &right) const;
    BigNum sqr() const;
    BigNum abs() const;
    BigNum half() const;

    bool operator==(const BigNum &right) const;
    bool operator!=(const BigNum &right) const;
    bool operator<(const BigNum &right) const;
    bool operator<=(const BigNum &right) const;
    bool operator>(const BigNum &right) const;
    bool operator>=(const BigNum &right) const;
    bool is_negative() const;

    LDouble to_float() const;
    BigFloat to_big_float(BigArena &arena) const;
    std::string to_string(int digits = 0) const;
    void hex_dump() const;

private:
    explicit BigNum(BigArena::Allocation storage);

    Byte *data() noexcept;
    const Byte *data() const noexcept;

    std::size_t encoded_size() const noexcept
    {
        return arena().value_size();
    }
    // Signed little-endian fixed point: g_int_length high bytes are integral.
    // The remaining g_bn_length - g_int_length bytes are fractional.
    BigArena::Allocation m_storage;
};

BigNum sqr(const BigNum &value);

using BNComplex = Complex<BigNum>;

// Kernel workspaces; active lengths may be shortened.
extern BigNum g_bn_tmp2; // g_bn_length
extern BigNum g_bn_tmp3; // g_bn_length

extern BigNum g_bn_tmp;  // g_bn_length

// g_bn_length
extern BigNum g_x_min_bn;
extern BigNum g_x_max_bn;
extern BigNum g_y_min_bn;
extern BigNum g_y_max_bn;
extern BigNum g_x_3rd_bn;
extern BigNum g_y_3rd_bn;
extern BigNum g_delta_x_bn;
extern BigNum g_delta_y_bn;
extern BigNum g_delta2_x_bn;
extern BigNum g_delta2_y_bn;
extern BigNum g_close_enough_bn;
// g_bn_length
extern BigNum g_tmp_sqr_x_bn;
extern BigNum g_tmp_sqr_y_bn;
// Canonical orbit values.
extern BNComplex g_old_z_bn;
extern BNComplex g_param_z_bn;
extern BNComplex g_saved_z_bn;
extern BNComplex g_new_z_bn;

} // namespace id::math
