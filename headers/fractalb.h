#pragma once

extern DComplex cmplxbntofloat(BNComplex *);
extern DComplex cmplxbftofloat(BFComplex *);
extern void comparevalues(char const *s, LDBL x, bn_t bnx);
extern void comparevaluesbf(char const *s, LDBL x, bf_t bfx);
extern void show_var_bf(char const *s, bf_t n);
extern void show_two_bf(char const *s1, bf_t t1, char const *s2, bf_t t2, int digits);
extern void bfcornerstofloat();
extern void showcornersdbl(char const *s);
extern bool MandelbnSetup();
extern int mandelbn_per_pixel();
extern int juliabn_per_pixel();
extern int JuliabnFractal();
extern int JuliaZpowerbnFractal();
extern BNComplex *cmplxlog_bn(BNComplex *t, BNComplex *s);
extern BNComplex *cplxmul_bn(BNComplex *t, BNComplex *x, BNComplex *y);
extern BNComplex *ComplexPower_bn(BNComplex *t, BNComplex *xx, BNComplex *yy);
extern bool MandelbfSetup();
extern int mandelbf_per_pixel();
extern int juliabf_per_pixel();
extern int JuliabfFractal();
extern int JuliaZpowerbfFractal();
extern BFComplex *cmplxlog_bf(BFComplex *t, BFComplex *s);
extern BFComplex *cplxmul_bf(BFComplex *t, BFComplex *x, BFComplex *y);
extern BFComplex *ComplexPower_bf(BFComplex *t, BFComplex *xx, BFComplex *yy);
