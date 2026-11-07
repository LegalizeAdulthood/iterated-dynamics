// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::fractals
{

// Gaps in the numbering are due to the removal of integer fractal
// types that are migrated to their floating-point equivalent on
// image load.
//
// clang-format off
enum class FractalType
{
    NO_FRACTAL                  = -1,
    NEWT_BASIN                  = 2,
    MANDEL                      = 4,
    NEWTON                      = 5,
    JULIA                       = 6,
    PLASMA                      = 7,
    MANDEL_FN                   = 8,
    MAN_O_WAR                   = 9,
    TEST                        = 11,
    SQR_FN                      = 18,
    IFS                         = 26,
    IFS_3D                      = 27,
    FN_Z_SQR                    = 31,
    BIFURCATION                 = 32,
    FN_PLUS_FN                  = 33,
    FN_TIMES_FN                 = 35,
    SQR_1_OVER_FN               = 37,
    FN_MUL_Z_PLUS_Z             = 39,
    KAM                         = 40,
    KAM_3D                      = 42,
    MANDEL_FN_PLUS_Z_SQRD       = 47,
    JULIA_FN_PLUS_Z_SQRD        = 48,
    LAMBDA_FN                   = 49,
    MANDEL_Z_POWER              = 53,
    JULIA_Z_POWER               = 54,
    MAN_Z_TO_Z_PLUS_Z_PWR       = 55,
    JUL_Z_TO_Z_PLUS_Z_PWR       = 56,
    MANDEL_FN_PLUS_EXP          = 59,
    JULIA_FN_PLUS_EXP           = 60,
    POPCORN                     = 61,
    LORENZ                      = 63,
    COMPLEX_NEWTON              = 68,
    COMPLEX_BASIN               = 69,
    COMPLEX_MARKS_MAND          = 70,
    COMPLEX_MARKS_JUL           = 71,
    FORMULA                     = 73,
    SIERPINSKI                  = 74,
    LAMBDA                      = 75,
    BARNSLEY_M1                 = 76,
    BARNSLEY_J1                 = 77,
    BARNSLEY_M2                 = 78,
    BARNSLEY_J2                 = 79,
    BARNSLEY_M3                 = 80,
    BARNSLEY_J3                 = 81,
    MANDEL_LAMBDA               = 82,
    LORENZ_3D                   = 84,
    ROSSLER                     = 86,
    HENON                       = 88,
    PICKOVER                    = 89,
    GINGERBREAD                 = 90,
    DIFFUSION                   = 91,
    UNITY                       = 92,
    SPIDER                      = 93,
    TETRATE                     = 95,
    MAGNET_1M                   = 96,
    MAGNET_1J                   = 97,
    MAGNET_2M                   = 98,
    MAGNET_2J                   = 99,
    BIF_LAMBDA                  = 102,
    BIF_PLUS_SIN_PI             = 103,
    BIF_EQ_SIN_PI               = 104,
    POPCORN_JUL                 = 105,
    L_SYSTEM                    = 107,
    MAN_O_WAR_J                 = 108,
    FN_PLUS_FN_PIX              = 110,
    MARKS_MANDEL_PWR            = 112,
    TIMS_ERROR                  = 114,
    BIF_STEWART                 = 118,
    HOPALONG                    = 120,
    CIRCLE                      = 121,
    MARTIN                      = 122,
    LYAPUNOV                    = 123,
    LORENZ_3D1                  = 124,
    LORENZ_3D3                  = 125,
    LORENZ_3D4                  = 126,
    LAMBDA_FN_FN                = 128,
    JUL_FN_FN                   = 130,
    MAN_LAM_FN_FN               = 132,
    MAN_FN_FN                   = 134,
    BIF_MAY                     = 136,
    HALLEY                      = 138,
    DYNAMIC                     = 139,
    QUAT                        = 140,
    QUAT_JUL                    = 141,
    CELLULAR                    = 142,
    JULIBROT                    = 143,
    INVERSE_JULIA               = 145,
    MANDEL_CLOUD                = 146,
    PHOENIX                     = 148,
    MAND_PHOENIX                = 150,
    HYPER_CMPLX                 = 151,
    HYPER_CMPLX_J               = 152,
    FROTHY_BASIN                = 154,
    MANDEL4                     = 155,
    JULIA4                      = 156,
    MARKS_MANDEL                = 157,
    MARKS_JULIA                 = 158,
    ICON                        = 159,
    ICON_3D                     = 160,
    PHOENIX_CPLX                = 162,
    MAND_PHOENIX_CPLX           = 164,
    ANT                         = 165,
    CHIP                        = 166,
    QUADRUP_TWO                 = 167,
    THREEPLY                    = 168,
    VL                          = 169,
    ESCHER                      = 170,
    LATOO                       = 171,
    DIVIDE_BROT5                = 172,
    MANDELBROT_MIX4             = 173,
    BURNING_SHIP                = 174,
    MAX                         = 175, // maximum FractalType number
};
// clang-format on
constexpr int operator+(const FractalType rhs)
{
    return static_cast<int>(rhs);
}
inline bool operator==(const FractalType lhs, const FractalType rhs)
{
    return +lhs == +rhs;
}
inline bool operator!=(const FractalType lhs, const FractalType rhs)
{
    return +lhs != +rhs;
}
inline bool operator<(const FractalType lhs, const FractalType rhs)
{
    return +lhs < +rhs;
}
inline bool operator<=(const FractalType lhs, const FractalType rhs)
{
    return +lhs <= +rhs;
}
inline bool operator>(const FractalType lhs, const FractalType rhs)
{
    return +lhs > +rhs;
}
inline bool operator>=(const FractalType lhs, const FractalType rhs)
{
    return +lhs >= +rhs;
}

extern FractalType g_fractal_type;

} // namespace id::fractals
