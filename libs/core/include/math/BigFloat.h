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

} // namespace id::math
