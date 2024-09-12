#pragma once

#include "cmplx.h"

constexpr double ID_INFINITY{1.0e+300};

void FPUcplxmul(DComplex const *x, DComplex const *y, DComplex *z); // z = x * y
void FPUcplxdiv(DComplex const *x, DComplex const *y, DComplex *z); // z = x / y
void FPUcplxlog(DComplex const *x, DComplex *z);                    // z = log(x)
void FPUcplxexp(const DComplex *x, DComplex *z);                    // z = e^x
void sin_cos(double const *angle, double *sine, double *cosine);
void sinh_cosh(double const *angle, double *sine, double *cosine);

void sin_cos(long angle, long *sine, long *cosine);
void sinh_cosh(long angle, long *sine, long *cosine);
long r16Mul(long, long);
long RegFloat2Fg(long, int);
long Exp086(long);
unsigned long ExpFudged(long, int);
long RegDivFloat(long, long);
long LogFudged(unsigned long, int);
long LogFloat14(unsigned long);
long RegFg2Float(long, int);
long RegSftFloat(long, int);
