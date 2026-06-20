// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/calcfrac.h"
#include "fractals/fractype.h"
#include "helpdefs.h"
#include "math/big.h"

#include <complex>

using namespace id::misc;

namespace id::fractals
{

using OrbitCalc = int (*)();
using OrbitCalc3D = int (*)(double *x, double *y, double *z);
using PerPixel = int (*)();
using PerImage = bool (*)();
using CalcType = int (*)();

struct AlternateMath
{
    FractalType type;      // index in fractalname of the fractal
    math::BFMathType math; // kind of math used
    OrbitCalc orbit_calc;  // function that calculates one orbit
    PerPixel per_pixel;    // once-per-pixel init
    PerImage per_image;    // once-per-image setup
};

struct MoreParams
{
    FractalType type;                                // index in fractalname of the fractal
    const char *param_names[engine::MAX_PARAMS - 4]; // name of the parameters
    double params[engine::MAX_PARAMS - 4];           // default parameter values
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

inline int operator+(const FractalFlags value)
{
    return static_cast<int>(value);
}
inline FractalFlags operator|(const FractalFlags lhs, const FractalFlags rhs)
{
    return static_cast<FractalFlags>(+lhs | +rhs);
}
inline FractalFlags operator&(const FractalFlags lhs, const FractalFlags rhs)
{
    return static_cast<FractalFlags>(+lhs & +rhs);
}
inline FractalFlags operator^(const FractalFlags lhs, const FractalFlags rhs)
{
    return static_cast<FractalFlags>(+lhs ^ +rhs);
}
inline FractalFlags operator~(const FractalFlags lhs)
{
    return static_cast<FractalFlags>(~+lhs);
}
inline bool bit_set(const FractalFlags value, const FractalFlags bit)
{
    return (value & bit) == bit;
}
inline bool bit_clear(const FractalFlags value, const FractalFlags bit)
{
    return (value & bit) == FractalFlags::NONE;
}

using PerturbationReference = void (*)(const std::complex<double> &center, std::complex<double> &z);
using PerturbationReferenceBF = void (*)(const math::BFComplex &center, math::BFComplex &z);
using PerturbationPoint = void (*)(
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
    OrbitCalc orbit_calc;                   // function that calculates one orbit
    PerPixel per_pixel;                     // once-per-pixel init
    PerImage per_image;                     // once-per-image setup
    CalcType calc_type;                     // name of main fractal function
    int orbit_bailout;                      // usual bailout value for orbit calc
    PerturbationReference pert_ref{};       // compute perturbation reference orbit
    PerturbationReferenceBF pert_ref_bf{};  // compute BFComplex perturbation reference orbit
    PerturbationPoint pert_pt{};            // compute point via perturbation
};

const FractalSpecific *get_fractal_specific(FractalType type);

class FractalDispatch
{
public:
    FractalDispatch() = default;

    FractalDispatch(const FractalDispatch &rhs) = default;

    FractalDispatch(OrbitCalc orbit, PerPixel pixel, PerImage image, CalcType calc) :
        m_orbit_calc(orbit),
        m_per_pixel(pixel),
        m_per_image(image),
        m_calc_type(calc)
    {
    }

    FractalDispatch(const FractalSpecific &specific) :
        m_orbit_calc(specific.orbit_calc),
        m_per_pixel(specific.per_pixel),
        m_per_image(specific.per_image),
        m_calc_type(specific.calc_type)
    {
    }

    FractalDispatch(FractalType type) :
        FractalDispatch(*get_fractal_specific(type))
    {
    }

    // clang-format off
    OrbitCalc orbit_calc() const            { return m_orbit_calc; }
    void set_orbit_calc(OrbitCalc value)    { m_orbit_calc = value; }
    PerPixel per_pixel() const              { return m_per_pixel; }
    void set_per_pixel(PerPixel value)      { m_per_pixel = value; }
    PerImage per_image() const              { return m_per_image; }
    void set_per_image(PerImage value)      { m_per_image = value; }
    CalcType calc_type() const              { return m_calc_type; }
    void set_calc_type(CalcType value)      { m_calc_type = value; }
    // clang-format on

    void init_calc_type(const FractalSpecific &specific)
    {
        m_calc_type = specific.calc_type;
    }

    void set_current_alternate_math(const AlternateMath &value)
    {
        m_orbit_calc = value.orbit_calc;
        m_per_pixel = value.per_pixel;
        m_per_image = value.per_image;
    }

private:
    OrbitCalc m_orbit_calc{};
    PerPixel m_per_pixel{};
    PerImage m_per_image{};
    CalcType m_calc_type{};
};

extern const AlternateMath   g_alternate_math[];    // alternate math function pointers
extern const FractalSpecific g_fractal_specific[];
extern const MoreParams      g_more_fractal_params[];
extern const int             g_num_fractal_types;
extern const FractalSpecific *g_cur_fractal_specific;
extern FractalDispatch       g_dispatch;

inline bool per_image()
{
    return g_dispatch.per_image()();
}
inline int per_pixel()
{
    return g_dispatch.per_pixel()();
}
inline int orbit_calc()
{
    return g_dispatch.orbit_calc()();
}
inline int calc_type()
{
    return g_dispatch.calc_type()();
}

inline void set_fractal_type(const FractalType value)
{
    g_fractal_type = value;
    g_cur_fractal_specific = get_fractal_specific(value);
    g_dispatch = FractalDispatch(*g_cur_fractal_specific);
}

} // namespace id::fractals
