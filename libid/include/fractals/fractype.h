// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// These MUST match the corresponding indices into the FractalSpecific array
// clang-format off
enum class FractalType
{
    NO_FRACTAL                  = -1,
    MANDEL                      = 0,
    JULIA                       = 1,
    NEWT_BASIN                  = 2,
    LAMBDA                      = 3,
    MANDEL_FP                   = 4,
    NEWTON                      = 5,
    JULIA_FP                    = 6,
    PLASMA                      = 7,
    LAMBDA_SINE                 = 8, // obsolete
    MANDEL_TRIG_FP              = 8,
    LAMBDA_COS                  = 9, // obsolete
    MAN_O_WAR_FP                = 9,
    LAMBDA_EXP                  = 10, // obsolete
    MAN_O_WAR                   = 10,
    TEST                        = 11,
    SIERPINSKI                  = 12,
    BARNSLEY_M1                 = 13,
    BARNSLEY_J1                 = 14,
    BARNSLEY_M2                 = 15,
    BARNSLEY_J2                 = 16,
    MANDEL_SINE                 = 17, // obsolete
    SQR_TRIG                    = 17,
    MANDEL_COS                  = 18, // obsolete
    SQR_TRIG_FP                 = 18,
    MANDEL_EXP                  = 19, // obsolete
    TRIG_PLUS_TRIG              = 19,
    MANDEL_LAMBDA               = 20,
    MARKS_MANDEL                = 21,
    MARKS_JULIA                 = 22,
    UNITY                       = 23,
    MANDEL4                     = 24,
    JULIA4                      = 25,
    IFS                         = 26,
    IFS_3D                      = 27,
    BARNSLEY_M3                 = 28,
    BARNSLEY_J3                 = 29,
    DEM_M                       = 30, // obsolete
    TRIG_SQR                    = 30,
    DEM_J                       = 31, // obsolete
    TRIG_SQR_FP                 = 31,
    BIFURCATION                 = 32,
    MANDEL_SINH                 = 33, // obsolete
    TRIG_PLUS_TRIG_FP           = 33,
    LAMBDA_SINH                 = 34, // obsolete
    TRIG_X_TRIG                 = 34,
    MANDEL_COSH                 = 35, // obsolete
    TRIG_X_TRIG_FP              = 35,
    LAMBDA_COSH                 = 36, // obsolete
    SQR_1_OVER_TRIG             = 36,
    MANDEL_SINE_L               = 37, // obsolete
    SQR_1_OVER_TRIG_FP          = 37,
    LAMBDA_SINE_L               = 38, // obsolete
    Z_X_TRIG_PLUS_Z             = 38,
    MANDEL_COS_L                = 39, // obsolete
    Z_X_TRIG_PLUS_Z_FP          = 39,
    LAMBDA_COS_L                = 40, // obsolete
    KAM_FP                      = 40,
    MANDEL_SINH_L               = 41, // obsolete
    KAM                         = 41,
    LAMBDA_SINH_L               = 42, // obsolete
    KAM_3D_FP                   = 42,
    MANDEL_COSH_L               = 43, // obsolete
    KAM_3D                      = 43,
    LAMBDA_COSH_L               = 44, // obsolete
    LAMBDA_TRIG                 = 44,
    MAN_TRIG_PLUS_Z_SQRD_L      = 45,
    JUL_TRIG_PLUS_Z_SQRD_L      = 46,
    MAN_TRIG_PLUS_Z_SQRD_FP     = 47,
    JUL_TRIG_PLUS_Z_SQRD_FP     = 48,
    MANDEL_EXP_L                = 49, // obsolete
    LAMBDA_TRIG_FP              = 49,
    LAMBDA_EXP_L                = 50, // obsolete
    MANDEL_TRIG                 = 50,
    MANDEL_Z_POWER_L            = 51,
    JULIA_Z_POWER_L             = 52,
    MANDEL_Z_POWER_FP           = 53,
    JULIA_Z_POWER_FP            = 54,
    MAN_Z_TO_Z_PLUS_Z_PWR_FP    = 55,
    JUL_Z_TO_Z_PLUS_Z_PWR_FP    = 56,
    MAN_TRIG_PLUS_EXP_L         = 57,
    JUL_TRIG_PLUS_EXP_L         = 58,
    MAN_TRIG_PLUS_EXP_FP        = 59,
    JUL_TRIG_PLUS_EXP_FP        = 60,
    POPCORN_FP                  = 61,
    POPCORN_L                   = 62,
    LORENZ_FP                   = 63,
    LORENZ_L                    = 64,
    LORENZ_3D_L                 = 65,
    NEWTON_MP                   = 66,
    NEWT_BASIN_MP               = 67,
    COMPLEX_NEWTON              = 68,
    COMPLEX_BASIN               = 69,
    COMPLEX_MARKS_MAND          = 70,
    COMPLEX_MARKS_JUL           = 71,
    FORMULA                     = 72,
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
    JULIBROT                    = 83,
    LORENZ_3D_FP                = 84,
    ROSSLER_L                   = 85,
    ROSSLER_FP                  = 86,
    HENON_L                     = 87,
    HENON_FP                    = 88,
    PICKOVER_FP                 = 89,
    GINGERBREAD_FP              = 90,
    DIFFUSION                   = 91,
    UNITY_FP                    = 92,
    SPIDER_FP                   = 93,
    SPIDER                      = 94,
    TETRATE_FP                  = 95,
    MAGNET_1M                   = 96,
    MAGNET_1J                   = 97,
    MAGNET_2M                   = 98,
    MAGNET_2J                   = 99,
    BIFURCATION_L               = 100,
    BIF_LAMBDA_L                = 101,
    BIF_LAMBDA                  = 102,
    BIF_PLUS_SIN_PI             = 103,
    BIF_EQ_SIN_PI               = 104,
    POPCORN_JUL_FP              = 105,
    POPCORN_JUL_L               = 106,
    L_SYSTEM                    = 107,
    MAN_O_WAR_J_FP              = 108,
    MAN_O_WAR_J                 = 109,
    FN_PLUS_FN_PIX_FP           = 110,
    FN_PLUS_FN_PIX_LONG         = 111,
    MARKS_MANDEL_PWR_FP         = 112,
    MARKS_MANDEL_PWR            = 113,
    TIMS_ERROR_FP               = 114,
    TIMS_ERROR                  = 115,
    BIF_EQ_SIN_PI_L             = 116,
    BIF_PLUS_SIN_PI_L           = 117,
    BIF_STEWART                 = 118,
    BIF_STEWART_L               = 119,
    HOPALONG_FP                 = 120,
    CIRCLE_FP                   = 121,
    MARTIN_FP                   = 122,
    LYAPUNOV                    = 123,
    LORENZ_3D1_FP               = 124,
    LORENZ_3D3_FP               = 125,
    LORENZ_3D4_FP               = 126,
    LAMBDA_FN_FN_L              = 127,
    LAMBDA_FN_FN_FP             = 128,
    JUL_FN_FN_L                 = 129,
    JUL_FN_FN_FP                = 130,
    MAN_LAM_FN_FN_L             = 131,
    MAN_LAM_FN_FN_FP            = 132,
    MAN_FN_FN_L                 = 133,
    MAN_FN_FN_FP                = 134,
    BIF_MAY_L                   = 135,
    BIF_MAY                     = 136,
    HALLEY_MP                   = 137,
    HALLEY                      = 138,
    DYNAMIC_FP                  = 139,
    QUAT_FP                     = 140,
    QUAT_JUL_FP                 = 141,
    CELLULAR                    = 142,
    JULIBROT_FP                 = 143,
    INVERSE_JULIA               = 144,
    INVERSE_JULIA_FP            = 145,
    MANDEL_CLOUD                = 146,
    PHOENIX                     = 147,
    PHOENIX_FP                  = 148,
    MAND_PHOENIX                = 149,
    MAND_PHOENIX_FP             = 150,
    HYPER_CMPLX_FP              = 151,
    HYPER_CMPLX_J_FP            = 152,
    FROTH                       = 153,
    FROTH_FP                    = 154,
    MANDEL4_FP                  = 155,
    JULIA4_FP                   = 156,
    MARKS_MANDEL_FP             = 157,
    MARKS_JULIA_FP              = 158,
    ICON                        = 159,
    ICON_3D                     = 160,
    PHOENIX_CPLX                = 161,
    PHOENIX_FP_CPLX             = 162,
    MAND_PHOENIX_CPLX           = 163,
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
inline int operator+(FractalType rhs)
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
