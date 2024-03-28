// various complex number defs
#pragma once

#include "port.h"

namespace id
{

template <typename T>
struct hyper_complex
{
    T x;
    T y;
    T z;
    T t;
};

template <typename T>
struct complex
{
    T x;
    T y;

    complex &operator+=(const complex &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    complex &operator-=(const complex &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    complex &operator*=(T rhs)
    {
        x *= rhs;
        y *= rhs;
        return *this;
    }
};

template <typename T>
complex<T> operator+(const complex<T> &val)
{
    return val;
}

template <typename T>
complex<T> operator-(const complex<T> &val)
{
    return {-val.x, -val.y};
}

template <typename T>
complex<T> operator+(const complex<T> &lhs, const complex<T> &rhs)
{
    complex<T> result = lhs;
    result += rhs;
    return result;
}

template <typename T>
complex<T> operator-(const complex<T> &lhs, const complex<T> &rhs)
{
    complex<T> result = lhs;
    result -= rhs;
    return result;
}

template <typename T>
complex<T> operator*(const complex<T> &lhs, double rhs)
{
    complex<T> result = lhs;
    result *= rhs;
    return result;
}

template <typename T>
complex<T> operator*(double lhs, const complex<T> &rhs)
{
    complex<T> result = rhs;
    result *= lhs;
    return result;
}

}

using DHyperComplex = id::hyper_complex<double>;
using LHyperComplex = id::hyper_complex<long>;

using DComplex = id::complex<double>;
using LDComplex = id::complex<LDBL>;
using LComplex = id::complex<long>;
