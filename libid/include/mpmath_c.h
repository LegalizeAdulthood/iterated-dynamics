#pragma once

MP *MPmul086(MP, MP);
MP *MPdiv086(MP, MP);
MP *MPadd086(MP, MP);
int MPcmp086(MP, MP);
MP *d2MP086(double);
double *MP2d086(MP);
MP *fg2MP086(long, int);
MP *MPmul386(MP, MP);
MP *MPdiv386(MP, MP);
MP *MPadd386(MP, MP);
int MPcmp386(MP, MP);
MP *d2MP386(double);
double *MP2d386(MP);
MP *fg2MP386(long, int);
double *MP2d(MP);
int MPcmp(MP, MP);
MP *MPmul(MP, MP);
MP *MPadd(MP, MP);
MP *MPdiv(MP, MP);
MP *d2MP(double);   // Convert double to type MP
MP *fg2MP(long, int);  // Convert fudged to type MP

MP *MPsub(MP, MP);
MP *MPsub086(MP, MP);
MP *MPsub386(MP, MP);
MP *MPabs(MP);
MPC MPCsqr(MPC);
MP MPCmod(MPC);
MPC MPCmul(MPC, MPC);
MPC MPCdiv(MPC, MPC);
MPC MPCadd(MPC, MPC);
MPC MPCsub(MPC, MPC);
MPC MPCpow(MPC, int);
int MPCcmp(MPC, MPC);
DComplex MPC2cmplx(MPC);
MPC cmplx2MPC(DComplex);
void setMPfunctions();
DComplex ComplexPower(DComplex, DComplex);
void SetupLogTable();
long logtablecalc(long);
long ExpFloat14(long);
bool ComplexNewtonSetup();
int ComplexNewton();
int ComplexBasin();
int GausianNumber(int, int);
void Arcsinz(DComplex z, DComplex *rz);
void Arccosz(DComplex z, DComplex *rz);
void Arcsinhz(DComplex z, DComplex *rz);
void Arccoshz(DComplex z, DComplex *rz);
void Arctanhz(DComplex z, DComplex *rz);
void Arctanz(DComplex z, DComplex *rz);
