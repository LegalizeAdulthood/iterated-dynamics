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

#endif
