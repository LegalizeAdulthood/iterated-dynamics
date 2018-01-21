#pragma once
#if !defined(JB_H)
#define JB_H

extern bool JulibrotSetup();
extern bool JulibrotfpSetup();
extern int jb_per_pixel();
extern int jbfp_per_pixel();
extern int zline(long, long);
extern int zlinefp(double, double);
extern int Std4dFractal();
extern int Std4dfpFractal();

#endif
