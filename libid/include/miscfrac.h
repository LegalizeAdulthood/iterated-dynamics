// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

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

int diffusion();
bool lya_setup();
int lyapunov();
int plasma();
int popcorn();
int test();

bool init_perturbation(int);
int perturbation();
