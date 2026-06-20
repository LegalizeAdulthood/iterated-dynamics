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

## Implementation Slices

Goal: make `g_fractal_specific` immutable by moving runtime-selected
function pointers into explicit calculation dispatch state.

### Slice 1: Make FractalSpecific Const

Work:

- Change `g_fractal_specific` to a const array.
- Change `g_cur_fractal_specific` and `get_fractal_specific()` to return
  const pointers.
- Update tests and helpers that save or inspect the current type.
- Add a test that renders or setup paths do not modify table entries.

Done when:

- The compiler rejects writes to `FractalSpecific` entries.
- Runtime behavior still changes through dispatch.
- The static table is immutable metadata.

### Slice 2: Retire Compatibility State

Work:

- Fold `g_calc_type` into dispatch, or make it a thin alias of dispatch
  state.
- Remove helper wrappers that exist only for the transition.
- Update comments to describe static metadata versus runtime dispatch.

Done when:

- The code has one runtime calculator source of truth.
- Static capability checks cannot accidentally observe runtime dispatch.
- This document matches the final architecture.

## Remaining Slices After Dispatch

These slices assume `FractalSpecific` is already const and runtime-selected
functions already live in dispatch state.

### Slice 3: Classify Calc-Type Reads

Work:

- Audit every read of `calc_type`.
- Mark each read as static capability metadata or runtime calculator
  dispatch.
- Replace runtime reads with the dispatch accessor.
- Keep table reads only where static type capability is being tested.

Done when:

- Runtime code does not inspect `FractalSpecific::calc_type`.
- Static checks are named or commented as static metadata checks.
- Tests still cover non-standard and standard dispatch decisions.

### Slice 4: Guard Dispatch Lifetime

Work:

- Make `set_fractal_type()` seed or invalidate dispatch explicitly.
- Add an assertion or status check before runtime dispatch is used.
- Prevent a dispatch selected for one type from being reused after a type
  change.

Done when:

- Changing type cannot leave a stale runtime calculator active.
- Tests cover type change before setup and type change followed by setup.
- Runtime dispatch access fails clearly if setup has not prepared it.

### Slice 5: Test Mandel And Julia Selection

Work:

- Add tests for Mandelbrot and Julia optimized selection.
- Verify the const table remains at the static default calculator.
- Verify dispatch selects `calc_mandelbrot_type` only when eligible.
- Verify dispatch falls back to `standard_fractal_type` when ineligible.

Done when:

- Tests cover both optimized and fallback paths.
- The tests compare table metadata and dispatch state separately.
- Changing the static table cannot hide a runtime selection bug.

### Slice 6: Test Showdot Wrapping

Work:

- Move showdot wrapping to dispatch state if any compatibility path
  remains.
- Test that showdot saves the current runtime calculator.
- Test that the wrapper calls the saved calculator.
- Test that disabling showdot restores the prior dispatch state.

Done when:

- Showdot never writes table metadata.
- Tests cover showdot with an optimized Mandelbrot runtime calculator.
- Nested setup and teardown leave dispatch unchanged except for the
  wrapper.

### Slice 7: Test Alternate Math Dispatch

Work:

- Add tests for `BF_MATH` alternate dispatch.
- Verify alternate math swaps `orbit_calc`, `per_pixel`, and `per_image`.
- Verify alternate math does not change static `calc_type`.
- Verify no alternate entry leaves dispatch partially updated.

Done when:

- Alternate math behavior is asserted through dispatch.
- `calc_type` table metadata remains unchanged.
- Fallback to `BFMathType::NONE` is tested.

### Slice 8: Add Boundary Checks

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

### Slice 9: Remove Compatibility Names

Work:

- Remove `g_calc_type` if dispatch can be the only runtime calculator
  source.
- Otherwise rename it to make aliasing explicit and temporary.
- Remove transition helpers that duplicate dispatch access.
- Update docs and comments to name dispatch as the runtime mechanism.

Done when:

- There is one runtime calculator source of truth.
- `FractalSpecific::calc_type` is only static metadata.
- The release notes can describe the calc-type fix without caveats.
