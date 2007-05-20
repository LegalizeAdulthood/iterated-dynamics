#if !defined(FPU_H)
#define FPU_H

extern void cdecl FPUcplxmul(ComplexD *, ComplexD *, ComplexD *);
extern void cdecl FPUcplxdiv(ComplexD *, ComplexD *, ComplexD *);
extern void cdecl FPUsincos(double *, double *, double *);
extern void cdecl FPUsinhcosh(double *, double *, double *);
extern void cdecl FPUcplxlog(ComplexD *, ComplexD *);
extern void cdecl SinCos086(long, long *, long *);
extern void cdecl SinhCosh086(long, long *, long *);
extern long cdecl r16Mul(long, long);
extern long cdecl RegFloat2Fg(long, int);
extern long cdecl Exp086(long);
extern unsigned long cdecl ExpFudged(long, int);
extern long cdecl RegDivFloat(long, long);
extern long cdecl LogFudged(unsigned long, int);
extern long cdecl LogFloat14(unsigned long);
extern long cdecl RegFg2Float(long, int);
extern long cdecl RegSftFloat(long, int);
extern void cdecl FPUaptan387(double *, double *, double *);
extern void cdecl FPUcplxexp387(ComplexD *, ComplexD *);

#endif
