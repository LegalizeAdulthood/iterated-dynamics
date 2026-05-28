# `g_calc_type` Divergence

`g_cur_fractal_specific->calc_type` is static table metadata.
`g_calc_type` is the runtime pixel calculation function.

They are not equivalent.

## Divergence Cases

### Mandel And Julia Optimization

`mandel_per_image()` and `julia_per_image()` may replace the static table
value with `calc_mandelbrot_type`.

- table value: `standard_fractal_type`
- runtime value: `calc_mandelbrot_type`
- condition: `use_calc_mandelbrot()` returns true
- files: `libid/fractals/frasetup.cpp`

Affected types:

- `type=mandel`
- `type=julia`

### Mandel And Julia Fallback

The same setup paths may force `g_calc_type` back to
`standard_fractal_type` when the optimized Mandelbrot engine cannot be
used.

This does not create pointer divergence from the table, but it shows that
`g_calc_type` is deliberately runtime state selected by setup code.

### Showdot Wrapper

When `showdot` is active, `perform_work_list()` wraps the current runtime
calculator.

- saved runtime value: `s_calc_type_tmp`
- runtime value: `calc_type_show_dot`
- table value: unchanged
- file: `libid/engine/calcfrac.cpp`

The wrapper later calls `s_calc_type_tmp()`.

### Stale Runtime Value Before Setup

`set_fractal_type()` changes only:

- `g_fractal_type`
- `g_cur_fractal_specific`

It does not update `g_calc_type`.  Between a type change and the next
calculation setup, `g_calc_type` can still name the previous image's
runtime calculator.

### Alternate Math

`BF_MATH` alternate math swaps:

- `orbit_calc`
- `per_pixel`
- `per_image`

It does not swap `calc_type`.  In this case `g_calc_type` and
`g_cur_fractal_specific->calc_type` may still match while lower-level math
functions differ.

## Implication

Use `g_cur_fractal_specific->calc_type` only for static type capability
tests.  Use `g_calc_type` only after calculation setup when the actual
runtime pixel calculator is needed.
