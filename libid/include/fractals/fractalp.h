// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/calcfrac.h"
#include "fractals/fractype.h"
#include "helpdefs.h"
#include "math/big.h"
#include "misc/id.h"

#include <complex>

using namespace id::misc;

namespace id::fractals
{

struct AlternateMath
{
    FractalType type;      // index in fractalname of the fractal
    math::BFMathType math; // kind of math used
    int (*orbit_calc)();   // function that calculates one orbit
    int (*per_pixel)();    // once-per-pixel init
    bool (*per_image)();   // once-per-image setup
};

struct MoreParams
{
    FractalType type;                        // index in fractalname of the fractal
    const char *param_names[MAX_PARAMS - 4]; // name of the parameters
    double params[MAX_PARAMS - 4];           // default parameter values
};

// bitmask values for FractalSpecific flags
enum class FractalFlags
{
    // clang-format off
    NONE        = 0x00'00'00,   // no flags
    NO_ZOOM     = 0x00'00'01,   // zoom box not allowed at all
    NO_GUESS    = 0x00'00'02,   // solid guessing not allowed
    NO_TRACE    = 0x00'00'04,   // boundary tracing not allowed
    NO_ROTATE   = 0x00'00'08,   // zoom box rotate/stretch not allowed
    NO_RESUME   = 0x00'00'10,   // can't interrupt and resume
    INF_CALC    = 0x00'00'20,   // this type calculates forever
    TRIG1       = 0x00'00'30,   // number of trig functions in formula
    TRIG2       = 0x00'00'40,   //
    TRIG3       = 0x00'00'80,   //
    TRIG4       = 0x00'01'00,   //
    PARAMS_3D   = 0x00'04'00,   // uses 3d parameters
    OK_JB       = 0x00'08'00,   // works with Julibrot
    MORE        = 0x00'10'00,   // more than 4 params
    BAIL_TEST   = 0x00'20'00,   // can use different bailout tests
    BF_MATH     = 0x00'40'00,   // supports arbitrary precision
    LD_MATH     = 0x00'80'00,   // supports long double
    PERTURB     = 0x01'00'00,   // supports perturbation
    // clang-format on
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
using PerturbationReferenceBF = void(const math::BFComplex &center, math::BFComplex &z);
using PerturbationPoint = void(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0);

struct FractalSpecific
{
    FractalType type;                       // type of the fractal
    const char *name;                       // name of the fractal
                                            // (leading "*" suppresses name display)
    const char *param_names[4];             // name of the parameters
    double params[4];                       // default parameter values
    help::HelpLabels help_text;             // helpdefs.h HT_xxxx or NONE
    help::HelpLabels help_formula;          // helpdefs.h HF_xxxx or NONE
    FractalFlags flags;                     // constraints, bits defined above
    float x_min;                            // default XMIN corner
    float x_max;                            // default XMAX corner
    float y_min;                            // default YMIN corner
    float y_max;                            // default YMAX corner
    FractalType to_julia;                   // mandel-to-julia switch
    FractalType to_mandel;                  // julia-to-mandel switch
    engine::SymmetryType symmetry;          // applicable symmetry logic
    int (*orbit_calc)();                    // function that calculates one orbit
    int (*per_pixel)();                     // once-per-pixel init
    bool (*per_image)();                    // once-per-image setup
    int (*calc_type)();                     // name of main fractal function
    int orbit_bailout;                      // usual bailout value for orbit calc
    PerturbationReference *pert_ref{};      // compute perturbation reference orbit
    PerturbationReferenceBF *pert_ref_bf{}; // compute BFComplex perturbation reference orbit
    PerturbationPoint *pert_pt{};           // compute point via perturbation
};

extern AlternateMath         g_alternate_math[];    // alternate math function pointers
extern FractalSpecific       g_fractal_specific[];
extern MoreParams            g_more_fractal_params[];
extern int                   g_num_fractal_types;
extern FractalSpecific      *g_cur_fractal_specific;

FractalSpecific *get_fractal_specific(FractalType type);

inline bool per_image()
{
    return get_fractal_specific(g_fractal_type)->per_image();
}
inline int per_pixel()
{
    return get_fractal_specific(g_fractal_type)->per_pixel();
}
inline int orbit_calc()
{
    return get_fractal_specific(g_fractal_type)->orbit_calc();
}

inline void set_fractal_type(FractalType value)
{
    g_fractal_type = value;
    g_cur_fractal_specific = get_fractal_specific(value);
}

} // namespace id::fractals
