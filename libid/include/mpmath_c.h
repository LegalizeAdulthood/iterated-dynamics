#pragma once

#include "mpmath.h"

// Mark Peterson's expanded floating point operators. If
// the operation results in an overflow (result < 2**(2**14), or division
// by zero) the global 'g_mp_overflow' is set to one.
MP *MPmul(MP, MP);
MP *MPdiv386(MP, MP);
MP *MPadd(MP, MP);
int MPcmp386(MP, MP);
MP *d2MP(double);// Convert double to type MP
double *MP2d386(MP);
MP *fg2MP(long, int);// Convert fudged to type MP

MP *MPsub(MP, MP);
MP *MPabs(MP);
MPC MPCsqr(MPC);
inline MP MPCmod(MPC x)
{
    return *MPadd(*MPmul(x.x, x.x), *MPmul(x.y, x.y));
}
MPC MPCmul(MPC, MPC);
MPC MPCdiv(MPC, MPC);
MPC MPCadd(MPC, MPC);
MPC MPCsub(MPC, MPC);
MPC MPCpow(MPC, int);
int MPCcmp(MPC, MPC);
DComplex MPC2cmplx(MPC);
MPC cmplx2MPC(DComplex);
inline MP *MPdiv(MP x, MP y)
{
    return MPdiv386(x, y);
}
inline int MPcmp(MP x, MP y)
{
    return MPcmp386(x, y);
}


DComplex ComplexPower(DComplex, DComplex);
void SetupLogTable();
long logtablecalc(long);
long ExpFloat14(long);
void Arcsinz(DComplex z, DComplex *rz);
void Arccosz(DComplex z, DComplex *rz);
void Arcsinhz(DComplex z, DComplex *rz);
void Arccoshz(DComplex z, DComplex *rz);
void Arctanhz(DComplex z, DComplex *rz);
void Arctanz(DComplex z, DComplex *rz);
