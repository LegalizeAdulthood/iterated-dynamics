// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// These MUST match the corresponding indices into the fractalspecific array
enum class fractal_type
{
    NOFRACTAL                   = -1,
    MANDEL                      = 0,
    JULIA                       = 1,
    NEWTBASIN                   = 2,
    LAMBDA                      = 3,
    MANDELFP                    = 4,
    NEWTON                      = 5,
    JULIAFP                     = 6,
    PLASMA                      = 7,
    LAMBDASINE                  = 8, // obsolete
    MANDELTRIGFP                = 8,
    LAMBDACOS                   = 9, // obsolete
    MANOWARFP                   = 9,
    LAMBDAEXP                   = 10, // obsolete
    MANOWAR                     = 10,
    TEST                        = 11,
    SIERPINSKI                  = 12,
    BARNSLEYM1                  = 13,
    BARNSLEYJ1                  = 14,
    BARNSLEYM2                  = 15,
    BARNSLEYJ2                  = 16,
    MANDELSINE                  = 17, // obsolete
    SQRTRIG                     = 17,
    MANDELCOS                   = 18, // obsolete
    SQRTRIGFP                   = 18,
    MANDELEXP                   = 19, // obsolete
    TRIGPLUSTRIG                = 19,
    MANDELLAMBDA                = 20,
    MARKSMANDEL                 = 21,
    MARKSJULIA                  = 22,
    UNITY                       = 23,
    MANDEL4                     = 24,
    JULIA4                      = 25,
    IFS                         = 26,
    IFS3D                       = 27,
    BARNSLEYM3                  = 28,
    BARNSLEYJ3                  = 29,
    DEMM                        = 30, // obsolete
    TRIGSQR                     = 30,
    DEMJ                        = 31, // obsolete
    TRIGSQRFP                   = 31,
    BIFURCATION                 = 32,
    MANDELSINH                  = 33, // obsolete
    TRIGPLUSTRIGFP              = 33,
    LAMBDASINH                  = 34, // obsolete
    TRIGXTRIG                   = 34,
    MANDELCOSH                  = 35, // obsolete
    TRIGXTRIGFP                 = 35,
    LAMBDACOSH                  = 36, // obsolete
    SQR1OVERTRIG                = 36,
    LMANDELSINE                 = 37, // obsolete
    SQR1OVERTRIGFP              = 37,
    LLAMBDASINE                 = 38, // obsolete
    ZXTRIGPLUSZ                 = 38,
    LMANDELCOS                  = 39, // obsolete
    ZXTRIGPLUSZFP               = 39,
    LLAMBDACOS                  = 40, // obsolete
    KAMFP                       = 40,
    LMANDELSINH                 = 41, // obsolete
    KAM                         = 41,
    LLAMBDASINH                 = 42, // obsolete
    KAM3DFP                     = 42,
    LMANDELCOSH                 = 43, // obsolete
    KAM3D                       = 43,
    LLAMBDACOSH                 = 44, // obsolete
    LAMBDATRIG                  = 44,
    LMANTRIGPLUSZSQRD           = 45,
    LJULTRIGPLUSZSQRD           = 46,
    FPMANTRIGPLUSZSQRD          = 47,
    FPJULTRIGPLUSZSQRD          = 48,
    LMANDELEXP                  = 49, // obsolete
    LAMBDATRIGFP                = 49,
    LLAMBDAEXP                  = 50, // obsolete
    MANDELTRIG                  = 50,
    LMANDELZPOWER               = 51,
    LJULIAZPOWER                = 52,
    FPMANDELZPOWER              = 53,
    FPJULIAZPOWER               = 54,
    FPMANZTOZPLUSZPWR           = 55,
    FPJULZTOZPLUSZPWR           = 56,
    LMANTRIGPLUSEXP             = 57,
    LJULTRIGPLUSEXP             = 58,
    FPMANTRIGPLUSEXP            = 59,
    FPJULTRIGPLUSEXP            = 60,
    FPPOPCORN                   = 61,
    LPOPCORN                    = 62,
    FPLORENZ                    = 63,
    LLORENZ                     = 64,
    LLORENZ3D                   = 65,
    MPNEWTON                    = 66,
    MPNEWTBASIN                 = 67,
    COMPLEXNEWTON               = 68,
    COMPLEXBASIN                = 69,
    COMPLEXMARKSMAND            = 70,
    COMPLEXMARKSJUL             = 71,
    FORMULA                     = 72,
    FFORMULA                    = 73,
    SIERPINSKIFP                = 74,
    LAMBDAFP                    = 75,
    BARNSLEYM1FP                = 76,
    BARNSLEYJ1FP                = 77,
    BARNSLEYM2FP                = 78,
    BARNSLEYJ2FP                = 79,
    BARNSLEYM3FP                = 80,
    BARNSLEYJ3FP                = 81,
    MANDELLAMBDAFP              = 82,
    JULIBROT                    = 83,
    FPLORENZ3D                  = 84,
    LROSSLER                    = 85,
    FPROSSLER                   = 86,
    LHENON                      = 87,
    FPHENON                     = 88,
    FPPICKOVER                  = 89,
    FPGINGERBREAD               = 90,
    DIFFUSION                   = 91,
    UNITYFP                     = 92,
    SPIDERFP                    = 93,
    SPIDER                      = 94,
    TETRATEFP                   = 95,
    MAGNET1M                    = 96,
    MAGNET1J                    = 97,
    MAGNET2M                    = 98,
    MAGNET2J                    = 99,
    LBIFURCATION                = 100,
    LBIFLAMBDA                  = 101,
    BIFLAMBDA                   = 102,
    BIFADSINPI                  = 103,
    BIFEQSINPI                  = 104,
    FPPOPCORNJUL                = 105,
    LPOPCORNJUL                 = 106,
    LSYSTEM                     = 107,
    MANOWARJFP                  = 108,
    MANOWARJ                    = 109,
    FNPLUSFNPIXFP               = 110,
    FNPLUSFNPIXLONG             = 111,
    MARKSMANDELPWRFP            = 112,
    MARKSMANDELPWR              = 113,
    TIMSERRORFP                 = 114,
    TIMSERROR                   = 115,
    LBIFEQSINPI                 = 116,
    LBIFADSINPI                 = 117,
    BIFSTEWART                  = 118,
    LBIFSTEWART                 = 119,
    FPHOPALONG                  = 120,
    FPCIRCLE                    = 121,
    FPMARTIN                    = 122,
    LYAPUNOV                    = 123,
    FPLORENZ3D1                 = 124,
    FPLORENZ3D3                 = 125,
    FPLORENZ3D4                 = 126,
    LLAMBDAFNFN                 = 127,
    FPLAMBDAFNFN                = 128,
    LJULFNFN                    = 129,
    FPJULFNFN                   = 130,
    LMANLAMFNFN                 = 131,
    FPMANLAMFNFN                = 132,
    LMANFNFN                    = 133,
    FPMANFNFN                   = 134,
    LBIFMAY                     = 135,
    BIFMAY                      = 136,
    MPHALLEY                    = 137,
    HALLEY                      = 138,
    DYNAMICFP                   = 139,
    QUATFP                      = 140,
    QUATJULFP                   = 141,
    CELLULAR                    = 142,
    JULIBROTFP                  = 143,
    INVERSEJULIA                = 144,
    INVERSEJULIAFP              = 145,
    MANDELCLOUD                 = 146,
    PHOENIX                     = 147,
    PHOENIXFP                   = 148,
    MANDPHOENIX                 = 149,
    MANDPHOENIXFP               = 150,
    HYPERCMPLXFP                = 151,
    HYPERCMPLXJFP               = 152,
    FROTH                       = 153,
    FROTHFP                     = 154,
    MANDEL4FP                   = 155,
    JULIA4FP                    = 156,
    MARKSMANDELFP               = 157,
    MARKSJULIAFP                = 158,
    ICON                        = 159,
    ICON3D                      = 160,
    PHOENIXCPLX                 = 161,
    PHOENIXFPCPLX               = 162,
    MANDPHOENIXCPLX             = 163,
    MANDPHOENIXFPCPLX           = 164,
    ANT                         = 165,
    CHIP                        = 166,
    QUADRUPTWO                  = 167,
    THREEPLY                    = 168,
    VL                          = 169,
    ESCHER                      = 170,
    LATOO                       = 171,
    DIVIDE_BROT5                = 172,
    MANDELBROTMIX4              = 173
};

inline int operator+(fractal_type rhs)
{
    return static_cast<int>(rhs);
}
inline bool operator==(fractal_type lhs, fractal_type rhs)
{
    return +lhs == +rhs;
}
inline bool operator!=(fractal_type lhs, fractal_type rhs)
{
    return +lhs != +rhs;
}
inline bool operator<(fractal_type lhs, fractal_type rhs)
{
    return +lhs < +rhs;
}
inline bool operator<=(fractal_type lhs, fractal_type rhs)
{
    return +lhs <= +rhs;
}
inline bool operator>(fractal_type lhs, fractal_type rhs)
{
    return +lhs > +rhs;
}
inline bool operator>=(fractal_type lhs, fractal_type rhs)
{
    return +lhs >= +rhs;
}

extern fractal_type g_fractal_type;
