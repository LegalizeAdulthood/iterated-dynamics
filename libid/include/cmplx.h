// various complex number definitions

#pragma once

#include "port.h"

namespace id
{

template <typename T>
struct HyperComplex
{
    T x;
    T y;
    T z;
    T t;
};

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

} // namespace id

using DHyperComplex = id::HyperComplex<double>;
using LHyperComplex = id::HyperComplex<long>;

using DComplex = id::Complex<double>;
using LDComplex = id::Complex<LDBL>;
using LComplex = id::Complex<long>;
