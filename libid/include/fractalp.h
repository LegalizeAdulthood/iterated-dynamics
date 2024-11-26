// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "big.h"
#include "calcfrac.h"
#include "fractype.h"
#include "helpdefs.h"
#include "id.h"

#include <complex>

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
    fractal_type type;                  // index in fractalname of the fractal
    char const *param[MAX_PARAMS-4];    // name of the parameters
    double   paramvalue[MAX_PARAMS-4];  // default parameter values
};

// bitmask values for fractalspecific flags
enum class fractal_flags
{
    NONE = 0,        // no flags
    NOZOOM = 1,      // zoombox not allowed at all
    NOGUESS = 2,     // solid guessing not allowed
    NOTRACE = 4,     // boundary tracing not allowed
    NOROTATE = 8,    // zoombox rotate/stretch not allowed
    NORESUME = 16,   // can't interrupt and resume
    INFCALC = 32,    // this type calculates forever
    TRIG1 = 64,      // number of trig functions in formula
    TRIG2 = 128,     //
    TRIG3 = 192,     //
    TRIG4 = 256,     //
    PARMS3D = 1024,  // uses 3d parameters
    OKJB = 2048,     // works with Julibrot
    MORE = 4096,     // more than 4 parms
    BAILTEST = 8192, // can use different bailout tests
    BF_MATH = 16384, // supports arbitrary precision
    LD_MATH = 32768, // supports long double
    PERTURB = 65536  // supports perturbation
};
inline int operator+(fractal_flags value)
{
    return static_cast<int>(value);
}
inline fractal_flags operator|(fractal_flags lhs, fractal_flags rhs)
{
    return static_cast<fractal_flags>(+lhs | +rhs);
}
inline fractal_flags operator&(fractal_flags lhs, fractal_flags rhs)
{
    return static_cast<fractal_flags>(+lhs & +rhs);
}
inline fractal_flags operator^(fractal_flags lhs, fractal_flags rhs)
{
    return static_cast<fractal_flags>(+lhs ^ +rhs);
}
inline fractal_flags operator~(fractal_flags lhs)
{
    return static_cast<fractal_flags>(~+lhs);
}
inline bool bit_set(fractal_flags value, fractal_flags bit)
{
    return (value & bit) == bit;
}
inline bool bit_clear(fractal_flags value, fractal_flags bit)
{
    return (value & bit) == fractal_flags::NONE;
}

using PerturbationReference = void(const std::complex<double> &center, std::complex<double> &z);
using PerturbationReferenceBF = void(const BFComplex &center, BFComplex &z);
using PerturbationPoint = void(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0);

struct FractalSpecific
{
    char const *name;                       // name of the fractal
                                            // (leading "*" suppresses name display)
    char const *param[4];                   // name of the parameters
    double paramvalue[4];                   // default parameter values
    help_labels helptext;                   // helpdefs.h HT_xxxx or NONE
    help_labels helpformula;                // helpdefs.h HF_xxxx or NONE
    fractal_flags flags;                    // constraints, bits defined above
    float xmin;                             // default XMIN corner
    float xmax;                             // default XMAX corner
    float ymin;                             // default YMIN corner
    float ymax;                             // default YMAX corner
    int isinteger;                          // 1 if integer fractal, 0 otherwise
    fractal_type tojulia;                   // mandel-to-julia switch
    fractal_type tomandel;                  // julia-to-mandel switch
    fractal_type tofloat;                   // integer-to-floating switch
    symmetry_type symmetry;                 // applicable symmetry logic
    int (*orbitcalc)();                     // function that calculates one orbit
    int (*per_pixel)();                     // once-per-pixel init
    bool (*per_image)();                    // once-per-image setup
    int (*calctype)();                      // name of main fractal function
    int orbit_bailout;                      // usual bailout value for orbit calc
    PerturbationReference *pert_ref{};      // compute perturbation reference orbit
    PerturbationReferenceBF *pert_ref_bf{}; // compute BFComplex perturbation reference orbit
    PerturbationPoint *pert_pt{};           // compute point via perturbation
};

extern AlternateMath         g_alternate_math[];    // alternate math function pointers
extern FractalSpecific       g_fractal_specific[];
extern MOREPARAMS            g_more_fractal_params[];
extern int                   g_num_fractal_types;

inline bool per_image()
{
    return g_fractal_specific[+g_fractal_type].per_image();
}
inline int per_pixel()
{
    return g_fractal_specific[+g_fractal_type].per_pixel();
}
inline int orbit_calc()
{
    return g_fractal_specific[+g_fractal_type].orbitcalc();
}
