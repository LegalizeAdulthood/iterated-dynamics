#pragma once

enum class debug_flags
{
    none                                = 0,
    benchmark_timer                     = 1,
    force_float_perspective             = 22,
    force_disk_restore_not_save         = 50,
    prevent_287_math                    = 72,
    force_standard_fractal              = 90,
    force_ld_check                      = 94,
    force_real_popcorn                  = 96,
    write_formula_debug_information     = 98,
    allow_init_commands_anytime         = 110,
    benchmark_encoder                   = 200,
    prevent_miim                        = 300,
    prevent_formula_optimizer           = 322,
    show_formula_info_after_compile     = 324,
    force_memory_from_disk              = 420,
    force_memory_from_memory            = 422,
    force_boundary_trace_error          = 470,
    force_solid_guess_error             = 472,
    force_precision_0_digits            = 700,
    force_precision_20_digits           = 720,
    force_long_double_param_output      = 750,
    allow_large_colormap_changes        = 910,
    force_lossless_colormap             = 920,
    allow_mp_newton_type                = 1010,
    mandelbrot_mix4_flip_sign           = 1012,
    force_smaller_bitshift              = 1234,
    show_float_flag                     = 2224,
    force_arbitrary_precision_math      = 3200,
    prevent_arbitrary_precision_math    = 3400,
    use_soi_long_double                 = 3444,
    prevent_plasma_random               = 3600,
    prevent_coordinate_grid             = 3800,
    allow_negative_cross_product        = 4010,
    force_scaled_sound_formula          = 4030,
    force_disk_min_cache                = 4200,
    force_complex_power                 = 6000,
    prevent_386_math                    = 8088,
    display_memory_statistics           = 10000
};

inline int operator+(debug_flags val)
{
    return static_cast<int>(val);
}

inline debug_flags operator~(debug_flags val)
{
    return static_cast<debug_flags>(~+val);
}

inline debug_flags operator&(debug_flags lhs, debug_flags rhs)
{
    return static_cast<debug_flags>(+lhs & +rhs);
}

inline debug_flags &operator&=(debug_flags &lhs, debug_flags rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

extern debug_flags           g_debug_flag;
