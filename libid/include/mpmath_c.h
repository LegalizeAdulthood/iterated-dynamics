// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "mpmath.h"

// Mark Peterson's expanded floating point operators. If
// the operation results in an overflow (result < 2**(2**14), or division
// by zero) the global 'g_mp_overflow' is set to one.
MP *mp_mul(MP, MP);
MP *mp_div(MP, MP);
MP *mp_add(MP, MP);
int mp_cmp(MP, MP);
MP *d_to_mp(double);// Convert double to type MP
double *mp_to_d(MP);
MP *fg_to_mp(long, int);// Convert fudged to type MP

inline MP *mp_sub(MP x, MP y)
{
    y.Exp ^= 0x8000;
    return mp_add(x, y);
}

MP *mp_abs(MP);
MPC mpc_sqr(MPC);
inline MP mpc_mod(MPC x)
{
    return *mp_add(*mp_mul(x.x, x.x), *mp_mul(x.y, x.y));
}
MPC mpc_mul(MPC, MPC);
MPC mpc_div(MPC, MPC);
MPC mpc_add(MPC, MPC);
MPC mpc_sub(MPC, MPC);
MPC mpc_pow(MPC, int);
int mpc_cmp(MPC, MPC);
DComplex mpc_to_cmplx(MPC);
MPC cmplx_to_mpc(DComplex);

DComplex complex_power(DComplex, DComplex);
void setup_log_table();
long log_table_calc(long);
long exp_float14(long);
void asin_z(DComplex z, DComplex *rz);
void acos_z(DComplex z, DComplex *rz);
void asinh_z(DComplex z, DComplex *rz);
void acosh_z(DComplex z, DComplex *rz);
void atanh_z(DComplex z, DComplex *rz);
void atan_z(DComplex z, DComplex *rz);
