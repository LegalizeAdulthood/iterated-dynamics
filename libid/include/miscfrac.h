#pragma once

extern bool g_cellular_next_screen;

int Bifurcation();
int BifurcAddTrigPi();
int BifurcLambda();
int BifurcLambdaTrig();
int BifurcMay();
bool BifurcMaySetup();
int BifurcSetTrigPi();
int BifurcStewartTrig();
int BifurcVerhulstTrig();

int LongBifurcAddTrigPi();
int LongBifurcLambdaTrig();
int LongBifurcMay();
int LongBifurcSetTrigPi();
int LongBifurcStewartTrig();
int LongBifurcVerhulstTrig();

int calcfroth();
bool CellularSetup();
int cellular();
int diffusion();
void froth_cleanup();
int froth_per_orbit();
int froth_per_pixel();
bool froth_setup();
bool lya_setup();
int lyapunov();
int plasma();
int popcorn();
int test();
