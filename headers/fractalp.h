#pragma once
#if !defined(FRACTALP_H)
#define FRACTALP_H

extern AlternateMath         g_alternate_math[];    // alternate math function pointers
extern fractalspecificstuff  g_fractal_specific[];
extern MOREPARAMS            g_more_fractal_params[];
extern int                   g_num_fractal_types;

extern bool typehasparm(fractal_type type, int parm, char *buf);
extern bool paramnotused(int);

#endif
