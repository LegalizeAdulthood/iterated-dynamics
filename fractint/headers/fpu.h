#pragma once

extern int DivideOverflow;

extern void FPUcplxmul(ComplexD const *, ComplexD const *, ComplexD *);
extern void FPUcplxdiv(ComplexD *, ComplexD *, ComplexD *);
extern void FPUsincos(double *, double *, double *);
extern void FPUsinhcosh(double *, double *, double *);
extern void FPUcplxlog(ComplexD const *, ComplexD *);
extern void SinCos086(long, long *, long *);
extern void SinhCosh086(long, long *, long *);
extern long r16Mul(long, long);
extern long RegFloat2Fg(long, int);
extern long Exp086(long);
extern unsigned long ExpFudged(long, int);
extern long RegDivFloat(long, long);
extern long LogFudged(unsigned long, int);
extern long LogFloat14(unsigned long);
extern long RegFg2Float(long, int);
extern long RegSftFloat(long, int);
extern void FPUaptan387(double *, double *, double *);
extern void FPUcplxexp387(ComplexD const *, ComplexD *);
