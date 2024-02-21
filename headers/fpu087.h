#pragma once

// fpu087 -- assembler file prototypes
extern void FPUcplxmul(DComplex const *x, DComplex const *y, DComplex *z);
extern void FPUcplxdiv(DComplex const *x, DComplex const *y, DComplex *z);
extern void FPUsincos(double const *Angle, double *Sin, double *Cos);
extern void FPUsinhcosh(double const *Angle, double *Sin, double *Cos);
extern void FPUcplxlog(DComplex const *x, DComplex *z);
extern void SinCos086(long x, long *sinx, long *cosx);
extern void SinhCosh086(long x, long *sinx, long *cosx);
extern long r16Mul(long, long);
extern long RegFloat2Fg(long, int);
extern long Exp086(long);
extern unsigned long ExpFudged(long, int);
extern long RegDivFloat(long, long);
extern long LogFudged(unsigned long, int);
extern long LogFloat14(unsigned long);
extern long RegFg2Float(long, int);
extern long RegSftFloat(long, int);
