#pragma once

#include "mpmath.h"

MP *MPmul386(MP, MP);
MP *MPdiv386(MP, MP);
MP *MPadd386(MP, MP);
int MPcmp386(MP, MP);
MP *d2MP386(double);// Convert double to type MP
double *MP2d386(MP);
MP *fg2MP386(long, int);// Convert fudged to type MP

MP *MPsub386(MP, MP);
MP *MPabs(MP);
MPC MPCsqr(MPC);
inline MP MPCmod(MPC x)
{
    return *MPadd386(*MPmul386(x.x, x.x), *MPmul386(x.y, x.y));
}
MPC MPCmul(MPC, MPC);
MPC MPCdiv(MPC, MPC);
MPC MPCadd(MPC, MPC);
MPC MPCsub(MPC, MPC);
MPC MPCpow(MPC, int);
int MPCcmp(MPC, MPC);
DComplex MPC2cmplx(MPC);
MPC cmplx2MPC(DComplex);
inline MP *MPadd(MP x, MP y)
{
    return MPadd386(x, y);
}
inline MP *MPsub(MP x, MP y)
{
    return MPsub386(x, y);
}
inline MP *fg2MP(long x, int fg)
{
    return fg2MP386(x, fg);
}
inline MP *d2MP(double x)
{
    return d2MP386(x);
}
inline MP *MPmul(MP x, MP y)
{
    return MPmul386(x, y);
}
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
