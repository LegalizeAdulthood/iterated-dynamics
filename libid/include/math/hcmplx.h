// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::math
{

template <typename T>
struct HyperComplex
{
    T x;
    T y;
    T z;
    T t;
};

using DHyperComplex = HyperComplex<double>;
using LHyperComplex = HyperComplex<long>;

void hcmplx_mult(const DHyperComplex *arg1, const DHyperComplex *arg2, DHyperComplex *out);
void hcmplx_sqr(const DHyperComplex *arg, DHyperComplex *out);
int hcmplx_inv(const DHyperComplex *arg, DHyperComplex *out);
void hcmplx_add(const DHyperComplex *arg1, const DHyperComplex *arg2, DHyperComplex *out);
void hcmplx_sub(const DHyperComplex *arg1, const DHyperComplex *arg2, DHyperComplex *out);
void hcmplx_minus(const DHyperComplex *arg1, DHyperComplex *out);
void hcmplx_trig0(const DHyperComplex *h, DHyperComplex *out);

} // namespace id::math
