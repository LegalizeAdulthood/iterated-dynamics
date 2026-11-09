// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// various complex number definitions

#pragma once

#include <math/fixed_pt.h>
#include <math/sqr.h>

#include <utility>

namespace id::math
{

template <typename T>
struct Complex
{
    T x{};
    T y{};

    Complex operator*(const Complex &rhs) const;
    Complex operator/(const Complex &rhs) const;
    Complex &operator*=(const Complex &rhs);
    Complex &operator/=(const Complex &rhs);

    Complex &operator+=(const Complex &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Complex &operator-=(const Complex &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Complex &operator*=(T rhs)
    {
        x *= rhs;
        y *= rhs;
        return *this;
    }
};

template <typename T>
Complex<T> Complex<T>::operator*(const Complex &rhs) const
{
    T real = x * rhs.x;
    const T imag_product = y * rhs.y;
    real = real - imag_product;
    T imag = x * rhs.y;
    const T cross = y * rhs.x;
    imag = imag + cross;
    return {std::move(real), std::move(imag)};
}

template <typename T>
Complex<T> Complex<T>::operator/(const Complex &rhs) const
{
    const T real_squared = sqr(rhs.x);
    const T imag_squared = sqr(rhs.y);
    const T denominator = real_squared + imag_squared;
    if (denominator == T{0})
    {
        g_overflow = true;
        return {T{0}, T{0}};
    }
    T real = x * rhs.x;
    const T imag_product = y * rhs.y;
    real = (real + imag_product) / denominator;
    T imag = y * rhs.x;
    const T cross = x * rhs.y;
    imag = (imag - cross) / denominator;
    return {std::move(real), std::move(imag)};
}

template <typename T>
Complex<T> &Complex<T>::operator*=(const Complex &rhs)
{
    *this = *this * rhs;
    return *this;
}

template <typename T>
Complex<T> &Complex<T>::operator/=(const Complex &rhs)
{
    *this = *this / rhs;
    return *this;
}

template <typename T>
Complex<T> operator+(const Complex<T> &val)
{
    Complex<T> result;
    result = val;
    return result;
}

template <typename T>
Complex<T> operator-(const Complex<T> &val)
{
    return {-val.x, -val.y};
}

template <typename T>
Complex<T> operator+(const Complex<T> &lhs, const Complex<T> &rhs)
{
    Complex<T> result;
    result = lhs;
    result += rhs;
    return result;
}

template <typename T>
Complex<T> operator-(const Complex<T> &lhs, const Complex<T> &rhs)
{
    Complex<T> result;
    result = lhs;
    result -= rhs;
    return result;
}

template <typename T>
Complex<T> operator*(const Complex<T> &lhs, double rhs)
{
    Complex<T> result;
    result = lhs;
    result *= T(rhs);
    return result;
}

template <typename T>
Complex<T> operator*(double lhs, const Complex<T> &rhs)
{
    Complex<T> result;
    result = rhs;
    result *= T(lhs);
    return result;
}

template <typename T, typename U = T>
bool operator==(const Complex<T> &lhs, const Complex<U> &rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}
template <typename T, typename U = T>
bool operator!=(const Complex<T> &lhs, const Complex<U> &rhs)
{
    return !(lhs == rhs);
}

template <typename T>
auto mag_squared(const Complex<T> &z)
{
    const auto real_squared = sqr(z.x);
    const auto imag_squared = sqr(z.y);
    return real_squared + imag_squared;
}

using DComplex = Complex<double>;

DComplex sqrt(double x, double y);
inline DComplex sqrt(const DComplex &z)
{
    return sqrt(z.x, z.y);
}
DComplex pow(const DComplex &xx, const DComplex &yy);

void asin(const DComplex &z, DComplex &rz);
void acos(const DComplex &z, DComplex &rz);
void asinh(const DComplex &z, DComplex &rz);
void acosh(const DComplex &z, DComplex &rz);
void atanh(const DComplex &z, DComplex &rz);
void atan(const DComplex &z, DComplex &rz);

} // namespace id::math
