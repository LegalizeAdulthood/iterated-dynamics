#pragma once

#include "cmplx.h"

constexpr double ID_INFINITY{1.0e+300};

void FPUcplxmul(DComplex const *x, DComplex const *y, DComplex *z);
void FPUcplxdiv(DComplex const *x, DComplex const *y, DComplex *z);
void FPUsincos(double const *Angle, double *Sin, double *Cos);
void FPUsinhcosh(double const *Angle, double *Sin, double *Cos);
void FPUcplxlog(DComplex const *x, DComplex *z);
void SinCos086(long x, long *sinx, long *cosx);
void SinhCosh086(long x, long *sinx, long *cosx);
long r16Mul(long, long);
long RegFloat2Fg(long, int);
long Exp086(long);
unsigned long ExpFudged(long, int);
long RegDivFloat(long, long);
long LogFudged(unsigned long, int);
long LogFloat14(unsigned long);
long RegFg2Float(long, int);
long RegSftFloat(long, int);
