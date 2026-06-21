# Calc-Type Dispatch

`g_cur_fractal_specific->calc_type` is static table metadata.
`g_dispatch.calc_type()` is the runtime pixel calculation function.

They are not equivalent.

## Divergence Cases

### Mandel And Julia Optimization

`mandel_per_image()` and `julia_per_image()` may replace dispatch
`calc_type` with `calc_mandelbrot_type`.

- table value: `standard_fractal_type`
- runtime value: `calc_mandelbrot_type`
- condition: `use_calc_mandelbrot()` returns true
- files: `libid/fractals/frasetup.cpp`

Affected types:

- `type=mandel`
- `type=julia`

### Mandel And Julia Fallback

The same setup paths may force dispatch `calc_type` back to
`standard_fractal_type` when the optimized Mandelbrot engine cannot be
used.

This does not create pointer divergence from the table, but it shows that
`g_dispatch.calc_type()` is deliberately runtime state selected by setup
code.

### Showdot Wrapper

When `showdot` is active, `perform_work_list()` wraps the current
runtime calculator.

- saved runtime value: `s_calc_type_tmp`
- runtime value: `calc_type_show_dot`
- table value: unchanged
- file: `libid/engine/calcfrac.cpp`

The wrapper later calls `s_calc_type_tmp()`.

### Dispatch Seed

`set_fractal_type()` seeds function dispatch from static table metadata
and invalidates runtime `calc_type`:

- `g_fractal_type`
- `g_cur_fractal_specific`
- `g_dispatch`

Calculation setup calls `g_dispatch.init_calc_type()` from the table
default before per-image setup may override dispatch for the current
calculation.

### Alternate Math

`BF_MATH` alternate math swaps:

- `orbit_calc`
- `per_pixel`
- `per_image`

It does not swap `calc_type`.  In this case `g_dispatch.calc_type()` and
`g_cur_fractal_specific->calc_type` may still match while lower-level
math functions differ.

## Implication

Use `g_cur_fractal_specific->calc_type` only for static type capability
tests.  Use `g_dispatch.calc_type()` after calculation setup when the
actual runtime pixel calculator is needed.

## Implementation Slices

Goal: make `g_fractal_specific` immutable by moving runtime-selected
function pointers into explicit calculation dispatch state.

## Remaining Slices After Dispatch

These slices assume `FractalSpecific` is already const and runtime-selected
functions already live in dispatch state.

### Slice 1: Test Showdot Wrapping

Work:

- Test that showdot saves the current runtime calculator.
- Test that the wrapper calls the saved calculator.
- Test that disabling showdot restores the prior dispatch state.

Done when:

- Showdot never writes table metadata.
- Tests cover showdot with an optimized Mandelbrot runtime calculator.
- Nested setup and teardown leave dispatch unchanged except for the
  wrapper.

### Slice 2: Test Alternate Math Dispatch

Work:

- Add tests for `BF_MATH` alternate dispatch.
- Verify alternate math swaps `orbit_calc`, `per_pixel`, and `per_image`.
- Verify alternate math does not change static `calc_type`.
- Verify no alternate entry leaves dispatch partially updated.

Done when:

- Alternate math behavior is asserted through dispatch.
- `calc_type` table metadata remains unchanged.
- Fallback to `BFMathType::NONE` is tested.

### Slice 3: Add Boundary Checks

Work:

- Add a grep-style check or unit test for forbidden writes and runtime
  reads.
- Reject writes to `FractalSpecific` function pointers.
- Reject runtime calls through `g_cur_fractal_specific->orbit_calc`,
  `per_pixel`, `per_image`, or `calc_type`.

Done when:

- The check is easy to run locally.
- The check fails on new direct runtime table use.
- Allowed static metadata reads stay explicit.
