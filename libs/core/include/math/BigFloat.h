// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/big.h>
#include <math/BigArena.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace id::math
{

class BigNum;

class BigFloat
{
public:
    // Default construction is null; numeric construction and math need a context.
    BigFloat() noexcept;
    explicit BigFloat(LDouble value);
    explicit BigFloat(std::string_view text);
    explicit BigFloat(const BigNum &value);
    explicit BigFloat(BigArena &arena);
    BigFloat(BigArena &arena, LDouble value);
    BigFloat(BigArena &arena, std::string_view text);
    BigFloat(BigArena &arena, const BigNum &value);
    BigFloat(const BigFloat &value);
    BigFloat(BigArena &arena, const BigFloat &value);
    BigFloat(BigFloat &&value) noexcept;
    BigFloat &operator=(const BigFloat &value);
    BigFloat &operator=(BigFloat &&value);
    BigFloat &operator=(LDouble value);

    // Tests for an allocated value, including numeric zero.
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_storage);
    }

    BigArena &arena() const noexcept
    {
        return m_storage.arena();
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

    // Borrowed buffers require a numeric value and expire on arena relocation.
    BigFloatPtr data() noexcept
    {
        return m_storage.data();
    }

    BigFloatConstPtr data() const noexcept
    {
        return m_storage.data();
    }

    // Adapters copy canonical mantissa/exponent bytes, without scalar conversion.
    static BigFloat from_legacy(BigFloatConstPtr source, std::size_t size);
    static BigFloat from_legacy(BigArena &arena, BigFloatConstPtr source, std::size_t size);
    void copy_to_legacy(BigFloatPtr destination, std::size_t size) const;
    void copy_to_legacy_num(BigNumPtr destination, std::size_t size) const;

private:
    explicit BigFloat(BigArena::Allocation storage);

    std::size_t encoded_size() const noexcept
    {
        return arena().value_size();
    }

    BigArena::Allocation m_storage;
};

BigFloat sqr(const BigFloat &value);
BigFloat log(const BigFloat &value);
BigFloat exp(const BigFloat &value);
std::pair<BigFloat, BigFloat> sin_cos(const BigFloat &value);
BigFloat atan2(const BigFloat &y, const BigFloat &x);

} // namespace id::math
