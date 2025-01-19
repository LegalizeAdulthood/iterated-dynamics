// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

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

}

using DHyperComplex = id::HyperComplex<double>;
using LHyperComplex = id::HyperComplex<long>;

void hcmplx_mult(DHyperComplex *arg1, DHyperComplex *arg2, DHyperComplex *out);
void hcmplx_sqr(DHyperComplex *arg, DHyperComplex *out);
int hcmplx_inv(DHyperComplex *arg, DHyperComplex *out);
void hcmplx_add(DHyperComplex *arg1, DHyperComplex *arg2, DHyperComplex *out);
void hcmplx_sub(DHyperComplex *arg1, DHyperComplex *arg2, DHyperComplex *out);
void hcmplx_minus(DHyperComplex *arg1, DHyperComplex *out);
void hcmplx_trig0(DHyperComplex *, DHyperComplex *);
