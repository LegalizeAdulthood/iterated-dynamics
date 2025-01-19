// SPDX-License-Identifier: GPL-3.0-only
//
// various complex number definitions

#pragma once

#include "math/sqr.h"

namespace id
{

template <typename T>
struct Complex
{
    T x;
    T y;

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
Complex<T> operator+(const Complex<T> &val)
{
    return val;
}

template <typename T>
Complex<T> operator-(const Complex<T> &val)
{
    return {-val.x, -val.y};
}

template <typename T>
Complex<T> operator+(const Complex<T> &lhs, const Complex<T> &rhs)
{
    Complex<T> result = lhs;
    result += rhs;
    return result;
}

template <typename T>
Complex<T> operator-(const Complex<T> &lhs, const Complex<T> &rhs)
{
    Complex<T> result = lhs;
    result -= rhs;
    return result;
}

template <typename T>
Complex<T> operator*(const Complex<T> &lhs, double rhs)
{
    Complex<T> result = lhs;
    result *= rhs;
    return result;
}

template <typename T>
Complex<T> operator*(double lhs, const Complex<T> &rhs)
{
    Complex<T> result = rhs;
    result *= lhs;
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
double modulus(const Complex<T> &z)
{
    return sqr(z.x) + sqr(z.y);
}

} // namespace id

using DComplex = id::Complex<double>;
using LComplex = id::Complex<long>;

LComplex complex_sqrt_long(long, long);
DComplex complex_sqrt_float(double x, double y);
inline DComplex complex_sqrt_float(const DComplex &z)
{
    return complex_sqrt_float(z.x, z.y);
}
DComplex complex_power(DComplex xx, DComplex yy);

void asin_z(DComplex z, DComplex *rz);
void acos_z(DComplex z, DComplex *rz);
void asinh_z(DComplex z, DComplex *rz);
void acosh_z(DComplex z, DComplex *rz);
void atanh_z(DComplex z, DComplex *rz);
void atan_z(DComplex z, DComplex *rz);
