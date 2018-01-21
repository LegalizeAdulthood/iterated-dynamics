#pragma once
#if !defined(MPMATH_C_H)
#define MPMATH_C_H

extern MP *MPmul086(MP, MP);
extern MP *MPdiv086(MP, MP);
extern MP *MPadd086(MP, MP);
extern int MPcmp086(MP, MP);
extern MP *d2MP086(double);
extern double *MP2d086(MP);
extern MP *fg2MP086(long, int);
extern MP *MPmul386(MP, MP);
extern MP *MPdiv386(MP, MP);
extern MP *MPadd386(MP, MP);
extern int MPcmp386(MP, MP);
extern MP *d2MP386(double);
extern double *MP2d386(MP);
extern MP *fg2MP386(long, int);
extern double *MP2d(MP);
extern int MPcmp(MP, MP);
extern MP *MPmul(MP, MP);
extern MP *MPadd(MP, MP);
extern MP *MPdiv(MP, MP);
extern MP *d2MP(double);   // Convert double to type MP
extern MP *fg2MP(long, int);  // Convert fudged to type MP

extern MP *MPsub(MP, MP);
extern MP *MPsub086(MP, MP);
extern MP *MPsub386(MP, MP);
extern MP *MPabs(MP);
extern MPC MPCsqr(MPC);
extern MP MPCmod(MPC);
extern MPC MPCmul(MPC, MPC);
extern MPC MPCdiv(MPC, MPC);
extern MPC MPCadd(MPC, MPC);
extern MPC MPCsub(MPC, MPC);
extern MPC MPCpow(MPC, int);
extern int MPCcmp(MPC, MPC);
extern DComplex MPC2cmplx(MPC);
extern MPC cmplx2MPC(DComplex);
extern void setMPfunctions();
extern DComplex ComplexPower(DComplex, DComplex);
extern void SetupLogTable();
extern long logtablecalc(long);
extern long ExpFloat14(long);
extern bool ComplexNewtonSetup();
extern int ComplexNewton();
extern int ComplexBasin();
extern int GausianNumber(int, int);
extern void Arcsinz(DComplex z, DComplex *rz);
extern void Arccosz(DComplex z, DComplex *rz);
extern void Arcsinhz(DComplex z, DComplex *rz);
extern void Arccoshz(DComplex z, DComplex *rz);
extern void Arctanhz(DComplex z, DComplex *rz);
extern void Arctanz(DComplex z, DComplex *rz);

#endif
