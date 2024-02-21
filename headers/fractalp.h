#pragma once

#include "big.h"
#include "helpdefs.h"

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

struct fractalspecificstuff
{
    char const  *name;                  // name of the fractal
                                        // (leading "*" supresses name display)
    char const  *param[4];              // name of the parameters
    double paramvalue[4];               // default parameter values
    help_labels helptext;               // helpdefs.h HT_xxxx, -1 for none
    help_labels helpformula;            // helpdefs.h HF_xxxx, -1 for none
    unsigned flags;                     // constraints, bits defined below
    float xmin;                         // default XMIN corner
    float xmax;                         // default XMAX corner
    float ymin;                         // default YMIN corner
    float ymax;                         // default YMAX corner
    int   isinteger;                    // 1 if integerfractal, 0 otherwise
    fractal_type tojulia;               // mandel-to-julia switch
    fractal_type tomandel;              // julia-to-mandel switch
    fractal_type tofloat;               // integer-to-floating switch
    symmetry_type symmetry;             // applicable symmetry logic
                                        //  0 = no symmetry
                                        // -1 = y-axis symmetry (If No Params)
                                        //  1 = y-axis symmetry
                                        // -2 = x-axis symmetry (No Parms)
                                        //  2 = x-axis symmetry
                                        // -3 = y-axis AND x-axis (No Parms)
                                        //  3 = y-axis AND x-axis symmetry
                                        // -4 = polar symmetry (No Parms)
                                        //  4 = polar symmetry
                                        //  5 = PI (sin/cos) symmetry
                                        //  6 = NEWTON (power) symmetry
                                        //
    int (*orbitcalc)();                 // function that calculates one orbit
    int (*per_pixel)();                 // once-per-pixel init
    bool (*per_image)();                // once-per-image setup
    int (*calctype)();                  // name of main fractal function
    int orbit_bailout;                  // usual bailout value for orbit calc
};

extern AlternateMath         g_alternate_math[];    // alternate math function pointers
extern fractalspecificstuff  g_fractal_specific[];
extern MOREPARAMS            g_more_fractal_params[];
extern int                   g_num_fractal_types;

extern bool typehasparm(fractal_type type, int parm, char *buf);
extern bool paramnotused(int);
