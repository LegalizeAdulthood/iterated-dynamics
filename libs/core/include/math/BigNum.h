// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/big.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace id::math
{

class BigFloat;

class BigNum
{
public:
    // Callers establish a compatible legacy context for construction and math.
    BigNum();
    explicit BigNum(LDouble value);
    explicit BigNum(std::string_view text);
    explicit BigNum(const BigFloat &value);
    BigNum &operator=(LDouble value);

    BigNum operator-() const;
    BigNum &operator+=(const BigNum &right);
    BigNum &operator-=(const BigNum &right);
    BigNum &operator*=(const BigNum &right);
    BigNum &operator/=(const BigNum &right);

    BigNum operator+(const BigNum &right) const;
    BigNum operator-(const BigNum &right) const;
    BigNum operator*(const BigNum &right) const;
    BigNum operator/(const BigNum &right) const;
    BigNum sqr() const;

    bool operator==(const BigNum &right) const;
    bool operator!=(const BigNum &right) const;
    bool operator<(const BigNum &right) const;
    bool operator<=(const BigNum &right) const;
    bool operator>(const BigNum &right) const;
    bool operator>=(const BigNum &right) const;

    std::string to_string(int digits = 0) const;

    // Adapters copy encoded values without machine floating-point conversion.
    static BigNum from_legacy(BigNumConstPtr source, std::size_t size);
    void copy_to_legacy(BigNumPtr destination, std::size_t size) const;
    void copy_to_legacy_float(BigFloatPtr destination, std::size_t size) const;

private:
    // Signed little-endian fixed point: g_int_length high bytes are integral.
    // The remaining g_bn_length - g_int_length bytes are fractional.
    std::vector<Byte> m_storage;
};

BigNum sqr(const BigNum &value);

} // namespace id::math
