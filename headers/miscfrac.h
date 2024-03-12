#pragma once

extern int Bifurcation();
extern int BifurcAddTrigPi();
extern int BifurcLambda();
extern int BifurcLambdaTrig();
extern int BifurcMay();
extern bool BifurcMaySetup();
extern int BifurcSetTrigPi();
extern int BifurcStewartTrig();
extern int BifurcVerhulstTrig();

extern int LongBifurcAddTrigPi();
extern int LongBifurcLambdaTrig();
extern int LongBifurcMay();
extern int LongBifurcSetTrigPi();
extern int LongBifurcStewartTrig();
extern int LongBifurcVerhulstTrig();

extern int calcfroth();
extern bool CellularSetup();
extern int cellular();
extern int diffusion();
extern void froth_cleanup();
extern int froth_per_orbit();
extern int froth_per_pixel();
extern bool froth_setup();
extern bool lya_setup();
extern int lyapunov();
extern int plasma();
extern int popcorn();
extern int test();
