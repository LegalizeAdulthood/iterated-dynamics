#pragma once

#include "big.h"

DComplex cmplxbntofloat(BNComplex *);
DComplex cmplxbftofloat(BFComplex *);
void comparevalues(char const *s, LDBL x, bn_t bnx);
void comparevaluesbf(char const *s, LDBL x, bf_t bfx);
void show_var_bf(char const *s, bf_t n);
void show_two_bf(char const *s1, bf_t t1, char const *s2, bf_t t2, int digits);
void bfcornerstofloat();
void showcornersdbl(char const *s);
bool MandelbnSetup();
int mandelbn_per_pixel();
int juliabn_per_pixel();
int JuliabnFractal();
int JuliaZpowerbnFractal();
BNComplex *cmplxlog_bn(BNComplex *t, BNComplex *s);
BNComplex *cplxmul_bn(BNComplex *t, BNComplex *x, BNComplex *y);
BNComplex *ComplexPower_bn(BNComplex *t, BNComplex *xx, BNComplex *yy);
bool MandelbfSetup();
int mandelbf_per_pixel();
int juliabf_per_pixel();
int JuliabfFractal();
int JuliaZpowerbfFractal();
BFComplex *cmplxlog_bf(BFComplex *t, BFComplex *s);
BFComplex *cplxmul_bf(BFComplex *t, BFComplex *x, BFComplex *y);
BFComplex *cplxdiv_bf(BFComplex *t, BFComplex *x, BFComplex *y);
BFComplex *ComplexPower_bf(BFComplex *t, BFComplex *xx, BFComplex *yy);
