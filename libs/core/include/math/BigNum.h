// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/big.h>
#include <math/BigArena.h>

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
    BigNum &operator=(LDouble value);

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

    bool operator==(const BigNum &right) const;
    bool operator!=(const BigNum &right) const;
    bool operator<(const BigNum &right) const;
    bool operator<=(const BigNum &right) const;
    bool operator>(const BigNum &right) const;
    bool operator>=(const BigNum &right) const;

    LDouble to_float() const;
    std::string to_string(int digits = 0) const;

    // Borrowed buffers require a numeric value and expire on arena relocation.
    BigNumPtr data() noexcept
    {
        return m_storage.data();
    }

    BigNumConstPtr data() const noexcept
    {
        return m_storage.data();
    }

    // Adapters copy encoded values without machine floating-point conversion.
    static BigNum from_legacy(BigNumConstPtr source, std::size_t size);
    static BigNum from_legacy(BigArena &arena, BigNumConstPtr source, std::size_t size);
    void copy_to_legacy(BigNumPtr destination, std::size_t size) const;
    void copy_to_legacy_float(BigFloatPtr destination, std::size_t size) const;

private:
    explicit BigNum(BigArena::Allocation storage);

    std::size_t encoded_size() const noexcept
    {
        return arena().value_size();
    }
    // Signed little-endian fixed point: g_int_length high bytes are integral.
    // The remaining g_bn_length - g_int_length bytes are fractional.
    BigArena::Allocation m_storage;
};

BigNum sqr(const BigNum &value);
void square_bn(BigNum &result, BigNum &source);
void mult_bn(BigNum &result, BigNum &left, BigNum &right);

} // namespace id::math
