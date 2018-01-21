#pragma once
#if !defined(CALCFRAC_H)
#define CALCFRAC_H

#include "big.h"

extern long multiply(long x, long y, int n);
extern long divide(long x, long y, int n);
extern int calcfract();
extern int calcmand();
extern int calcmandfp();
extern int standard_fractal();
extern int test();
extern int plasma();
extern int diffusion();
extern int Bifurcation();
extern int BifurcLambda();
extern int BifurcSetTrigPi();
extern int LongBifurcSetTrigPi();
extern int BifurcAddTrigPi();
extern int LongBifurcAddTrigPi();
extern int BifurcMay();
extern bool BifurcMaySetup();
extern int LongBifurcMay();
extern int BifurcLambdaTrig();
extern int LongBifurcLambdaTrig();
extern int BifurcVerhulstTrig();
extern int LongBifurcVerhulstTrig();
extern int BifurcStewartTrig();
extern int LongBifurcStewartTrig();
extern int popcorn();
extern int lyapunov();
extern bool lya_setup();
extern int cellular();
extern bool CellularSetup();
extern int calcfroth();
extern int froth_per_pixel();
extern int froth_per_orbit();
extern bool froth_setup();
extern int logtable_in_extra_ok();
extern int find_alternate_math(fractal_type type, bf_math_type math);

#endif
