// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::misc
{

enum class DebugFlags
{
    NONE                                = 0,
    BENCHMARK_TIMER                     = 1,
    HISTORY_DUMP_JSON                   = 2,
    FORCE_FLOAT_PERSPECTIVE             = 22,
    FORCE_DISK_RESTORE_NOT_SAVE         = 50,
    FORCE_STANDARD_FRACTAL              = 90,
    FORCE_REAL_POPCORN                  = 96,
    WRITE_FORMULA_DEBUG_INFORMATION     = 98,
    ALLOW_INIT_COMMANDS_ANYTIME         = 110,
    BENCHMARK_ENCODER                   = 200,
    PREVENT_MIIM                        = 300,
    SHOW_FORMULA_INFO_AFTER_COMPILE     = 324,
    FORCE_MEMORY_FROM_DISK              = 420,
    FORCE_MEMORY_FROM_MEMORY            = 422,
    FORCE_BOUNDARY_TRACE_ERROR          = 470,
    FORCE_SOLID_GUESS_ERROR             = 472,
    FORCE_PRECISION_0_DIGITS            = 700,
    FORCE_PRECISION_20_DIGITS           = 720,
    FORCE_LONG_DOUBLE_PARAM_OUTPUT      = 750,
    ALLOW_LARGE_COLORMAP_CHANGES        = 910,
    FORCE_LOSSLESS_COLORMAP             = 920,
    ALLOW_NEWTON_MP_TYPE                = 1010,
    MANDELBROT_MIX4_FLIP_SIGN           = 1012,
    FORCE_SMALLER_BIT_SHIFT             = 1234,
    SHOW_FLOAT_FLAG                     = 2224,
    FORCE_ARBITRARY_PRECISION_MATH      = 3200,
    PREVENT_ARBITRARY_PRECISION_MATH    = 3400,
    PREVENT_PLASMA_RANDOM               = 3600,
    ALLOW_NEGATIVE_CROSS_PRODUCT        = 4010,
    FORCE_SCALED_SOUND_FORMULA          = 4030,
    FORCE_DISK_MIN_CACHE                = 4200,
    FORCE_COMPLEX_POWER                 = 6000,
    DISPLAY_MEMORY_STATISTICS           = 10000,
};

inline int operator+(const DebugFlags val)
{
    return static_cast<int>(val);
}

inline DebugFlags operator~(const DebugFlags val)
{
    return static_cast<DebugFlags>(~+val);
}

inline DebugFlags operator&(const DebugFlags lhs, const DebugFlags rhs)
{
    return static_cast<DebugFlags>(+lhs & +rhs);
}

inline DebugFlags &operator&=(DebugFlags &lhs, const DebugFlags rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

extern DebugFlags           g_debug_flag;

} // namespace id::misc
