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
    FractalType type;                  // index in fractalname of the fractal
    BFMathType math;                  // kind of math used
    int (*orbitcalc)();                 // function that calculates one orbit
    int (*per_pixel)();                 // once-per-pixel init
    bool (*per_image)();                // once-per-image setup
};

struct MoreParams
{
    FractalType type;                  // index in fractalname of the fractal
    char const *param[MAX_PARAMS-4];    // name of the parameters
    double   paramvalue[MAX_PARAMS-4];  // default parameter values
};

// bitmask values for fractalspecific flags
enum class FractalFlags
{
    NONE = 0,         // no flags
    NO_ZOOM = 1,      // zoom box not allowed at all
    NO_GUESS = 2,     // solid guessing not allowed
    NO_TRACE = 4,     // boundary tracing not allowed
    NO_ROTATE = 8,    // zoom box rotate/stretch not allowed
    NO_RESUME = 16,   // can't interrupt and resume
    INF_CALC = 32,    // this type calculates forever
    TRIG1 = 64,       // number of trig functions in formula
    TRIG2 = 128,      //
    TRIG3 = 192,      //
    TRIG4 = 256,      //
    PARAMS_3D = 1024, // uses 3d parameters
    OK_JB = 2048,     // works with Julibrot
    MORE = 4096,      // more than 4 params
    BAIL_TEST = 8192, // can use different bailout tests
    BF_MATH = 16384,  // supports arbitrary precision
    LD_MATH = 32768,  // supports long double
    PERTURB = 65536   // supports perturbation
};
inline int operator+(FractalFlags value)
{
    return static_cast<int>(value);
}
inline FractalFlags operator|(FractalFlags lhs, FractalFlags rhs)
{
    return static_cast<FractalFlags>(+lhs | +rhs);
}
inline FractalFlags operator&(FractalFlags lhs, FractalFlags rhs)
{
    return static_cast<FractalFlags>(+lhs & +rhs);
}
inline FractalFlags operator^(FractalFlags lhs, FractalFlags rhs)
{
    return static_cast<FractalFlags>(+lhs ^ +rhs);
}
inline FractalFlags operator~(FractalFlags lhs)
{
    return static_cast<FractalFlags>(~+lhs);
}
inline bool bit_set(FractalFlags value, FractalFlags bit)
{
    return (value & bit) == bit;
}
inline bool bit_clear(FractalFlags value, FractalFlags bit)
{
    return (value & bit) == FractalFlags::NONE;
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
    HelpLabels helptext;                   // helpdefs.h HT_xxxx or NONE
    HelpLabels helpformula;                // helpdefs.h HF_xxxx or NONE
    FractalFlags flags;                    // constraints, bits defined above
    float xmin;                             // default XMIN corner
    float xmax;                             // default XMAX corner
    float ymin;                             // default YMIN corner
    float ymax;                             // default YMAX corner
    int isinteger;                          // 1 if integer fractal, 0 otherwise
    FractalType tojulia;                   // mandel-to-julia switch
    FractalType tomandel;                  // julia-to-mandel switch
    FractalType tofloat;                   // integer-to-floating switch
    SymmetryType symmetry;                 // applicable symmetry logic
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
extern MoreParams            g_more_fractal_params[];
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
