// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/big.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace id::math
{

class BigNum;

class BigFloat
{
public:
    // Callers establish a compatible legacy context for construction and math.
    BigFloat();
    explicit BigFloat(LDouble value);
    explicit BigFloat(std::string_view text);
    explicit BigFloat(const BigNum &value);
    BigFloat &operator=(LDouble value);

    BigFloat operator-() const;
    BigFloat &operator+=(const BigFloat &right);
    BigFloat &operator-=(const BigFloat &right);
    BigFloat &operator*=(const BigFloat &right);
    BigFloat &operator/=(const BigFloat &right);

    BigFloat operator+(const BigFloat &right) const;
    BigFloat operator-(const BigFloat &right) const;
    BigFloat operator*(const BigFloat &right) const;
    BigFloat operator/(const BigFloat &right) const;
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

    std::string to_string(int digits = 0) const;

    // Adapters copy canonical mantissa/exponent bytes, without scalar conversion.
    static BigFloat from_legacy(BigFloatConstPtr source, std::size_t size);
    void copy_to_legacy(BigFloatPtr destination, std::size_t size) const;
    void copy_to_legacy_num(BigNumPtr destination, std::size_t size) const;

private:
    explicit BigFloat(std::vector<Byte> storage);

    std::vector<Byte> m_storage;
};

BigFloat sqr(const BigFloat &value);
BigFloat log(const BigFloat &value);
BigFloat exp(const BigFloat &value);
std::pair<BigFloat, BigFloat> sin_cos(const BigFloat &value);
BigFloat atan2(const BigFloat &y, const BigFloat &x);

} // namespace id::math
