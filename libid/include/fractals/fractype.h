// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// Gaps in the numbering are due to the removal of integer fractal
// types that are migrated to their floating-point equivalent on
// image load.
//
// clang-format off
enum class FractalType
{
    NO_FRACTAL                  = -1,
    NEWT_BASIN                  = 2,
    MANDEL_FP                   = 4,
    NEWTON                      = 5,
    JULIA_FP                    = 6,
    PLASMA                      = 7,
    MANDEL_TRIG_FP              = 8,
    MAN_O_WAR_FP                = 9,
    TEST                        = 11,
    SQR_TRIG_FP                 = 18,
    IFS                         = 26,
    IFS_3D                      = 27,
    TRIG_SQR_FP                 = 31,
    BIFURCATION                 = 32,
    TRIG_PLUS_TRIG_FP           = 33,
    TRIG_X_TRIG_FP              = 35,
    SQR_1_OVER_TRIG_FP          = 37,
    Z_X_TRIG_PLUS_Z_FP          = 39,
    KAM_FP                      = 40,
    KAM_3D_FP                   = 42,
    MAN_TRIG_PLUS_Z_SQRD_FP     = 47,
    JUL_TRIG_PLUS_Z_SQRD_FP     = 48,
    LAMBDA_TRIG_FP              = 49,
    MANDEL_Z_POWER_FP           = 53,
    JULIA_Z_POWER_FP            = 54,
    MAN_Z_TO_Z_PLUS_Z_PWR_FP    = 55,
    JUL_Z_TO_Z_PLUS_Z_PWR_FP    = 56,
    MAN_TRIG_PLUS_EXP_FP        = 59,
    JUL_TRIG_PLUS_EXP_FP        = 60,
    POPCORN_FP                  = 61,
    LORENZ_FP                   = 63,
    COMPLEX_NEWTON              = 68,
    COMPLEX_BASIN               = 69,
    COMPLEX_MARKS_MAND          = 70,
    COMPLEX_MARKS_JUL           = 71,
    FORMULA_FP                  = 73,
    SIERPINSKI_FP               = 74,
    LAMBDA_FP                   = 75,
    BARNSLEY_M1_FP              = 76,
    BARNSLEY_J1_FP              = 77,
    BARNSLEY_M2_FP              = 78,
    BARNSLEY_J2_FP              = 79,
    BARNSLEY_M3_FP              = 80,
    BARNSLEY_J3_FP              = 81,
    MANDEL_LAMBDA_FP            = 82,
    LORENZ_3D_FP                = 84,
    ROSSLER_FP                  = 86,
    HENON_FP                    = 88,
    PICKOVER_FP                 = 89,
    GINGERBREAD_FP              = 90,
    DIFFUSION                   = 91,
    UNITY_FP                    = 92,
    SPIDER_FP                   = 93,
    TETRATE_FP                  = 95,
    MAGNET_1M                   = 96,
    MAGNET_1J                   = 97,
    MAGNET_2M                   = 98,
    MAGNET_2J                   = 99,
    BIF_LAMBDA                  = 102,
    BIF_PLUS_SIN_PI             = 103,
    BIF_EQ_SIN_PI               = 104,
    POPCORN_JUL_FP              = 105,
    L_SYSTEM                    = 107,
    MAN_O_WAR_J_FP              = 108,
    FN_PLUS_FN_PIX_FP           = 110,
    MARKS_MANDEL_PWR_FP         = 112,
    TIMS_ERROR_FP               = 114,
    BIF_STEWART                 = 118,
    HOPALONG_FP                 = 120,
    CIRCLE_FP                   = 121,
    MARTIN_FP                   = 122,
    LYAPUNOV                    = 123,
    LORENZ_3D1_FP               = 124,
    LORENZ_3D3_FP               = 125,
    LORENZ_3D4_FP               = 126,
    LAMBDA_FN_FN_FP             = 128,
    JUL_FN_FN_FP                = 130,
    MAN_LAM_FN_FN_FP            = 132,
    MAN_FN_FN_FP                = 134,
    BIF_MAY                     = 136,
    HALLEY                      = 138,
    DYNAMIC_FP                  = 139,
    QUAT_FP                     = 140,
    QUAT_JUL_FP                 = 141,
    CELLULAR                    = 142,
    JULIBROT_FP                 = 143,
    INVERSE_JULIA_FP            = 145,
    MANDEL_CLOUD                = 146,
    PHOENIX_FP                  = 148,
    MAND_PHOENIX_FP             = 150,
    HYPER_CMPLX_FP              = 151,
    HYPER_CMPLX_J_FP            = 152,
    FROTH_FP                    = 154,
    MANDEL4_FP                  = 155,
    JULIA4_FP                   = 156,
    MARKS_MANDEL_FP             = 157,
    MARKS_JULIA_FP              = 158,
    ICON                        = 159,
    ICON_3D                     = 160,
    PHOENIX_FP_CPLX             = 162,
    MAND_PHOENIX_FP_CPLX        = 164,
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
    MAX                         = 175,
};
// clang-format on
constexpr int operator+(FractalType rhs)
{
    return static_cast<int>(rhs);
}
inline bool operator==(FractalType lhs, FractalType rhs)
{
    return +lhs == +rhs;
}
inline bool operator!=(FractalType lhs, FractalType rhs)
{
    return +lhs != +rhs;
}
inline bool operator<(FractalType lhs, FractalType rhs)
{
    return +lhs < +rhs;
}
inline bool operator<=(FractalType lhs, FractalType rhs)
{
    return +lhs <= +rhs;
}
inline bool operator>(FractalType lhs, FractalType rhs)
{
    return +lhs > +rhs;
}
inline bool operator>=(FractalType lhs, FractalType rhs)
{
    return +lhs >= +rhs;
}

extern FractalType g_fractal_type;
