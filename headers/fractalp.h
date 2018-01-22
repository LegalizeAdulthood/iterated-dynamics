#pragma once
#if !defined(FRACTALP_H)
#define FRACTALP_H

#include "big.h"

struct AlternateMath
{
    fractal_type type;                  // index in fractalname of the fractal
    bf_math_type math;                  // kind of math used
    int (*orbitcalc)();                 // function that calculates one orbit
    int (*per_pixel)();                 // once-per-pixel init
    bool (*per_image)();                // once-per-image setup
};

struct MOREPARAMS
{
    fractal_type type;                      // index in fractalname of the fractal
    char const *param[MAX_PARAMS-4];     // name of the parameters
    double   paramvalue[MAX_PARAMS-4];   // default parameter values
};

extern AlternateMath         g_alternate_math[];    // alternate math function pointers
extern fractalspecificstuff  g_fractal_specific[];
extern MOREPARAMS            g_more_fractal_params[];
extern int                   g_num_fractal_types;

extern bool typehasparm(fractal_type type, int parm, char *buf);
extern bool paramnotused(int);

#endif
