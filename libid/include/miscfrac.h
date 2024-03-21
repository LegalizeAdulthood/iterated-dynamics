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

bool CellularSetup();
int cellular();
int diffusion();
bool lya_setup();
int lyapunov();
int plasma();
int popcorn();
int test();
