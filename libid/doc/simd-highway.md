# SIMD Highway Plan

This document describes the first implementation plan for using Google
Highway in the standard escape-time renderer.

The goal is not to replace every pass algorithm.  The goal is to add a
SIMD implementation for the ordinary row scan when the current render state
is compatible with vector execution.  Existing scalar code remains the
complete behavior oracle and handles every unsupported case.

## Decisions

- Highway is an implementation detail, not a traversal mode.
- Do not add `passes=h`.
- Keep `passes=1`, `passes=2`, `passes=g`, `passes=b`, and the other pass
  modes focused on traversal strategy.
- Add a `simd=` option for diagnostics and test control.
- Use SIMD automatically when the render state is eligible.
- Fall back to scalar code unless the user explicitly forced SIMD.
- Keep the first SIMD path narrow and easy to prove.
- Refactor existing pass algorithms later, after the row kernel is trusted.

## User Option

Add a command option:

```text
simd=auto
simd=off
simd=force
```

Meanings:

- `auto`: default.  Use Highway when eligible, otherwise use scalar code.
- `off`: force scalar code.  This is useful for comparison tests.
- `force`: require Highway.  If the current render state is ineligible,
  stop or report a clear ineligibility reason.

Possible later value:

```text
simd=stats
```

`stats` can mean `auto` plus diagnostics: selected Highway target,
eligibility result, fallback reason, row counts, and scalar tail counts.

## High-Level Shape

The normal pass dispatcher remains responsible for traversal.  SIMD is
selected inside the ordinary one-pass and two-pass path:

```cpp
case CalcMode::ONE_PASS:
case CalcMode::TWO_PASS:
    if (can_use_highway_pass())
    {
        highway_pass();
    }
    else
    {
        one_or_two_pass();
    }
    break;
```

In practice this selection may live in the default case that currently
calls `one_or_two_pass()`, but the behavior should match the shape above.

`highway_pass()` is a row/span implementation.  It does not call
`g_calc_type()` per pixel.  It gathers the render constants once, uses the
pixel grid arrays as input, writes a span of color indexes, then passes the
span to the existing row output path.

## Why Not `passes=h`

The existing `passes=` option chooses a traversal algorithm:

- `1`: one-pass sequential scan
- `2`: two-pass sequential scan
- `g`: solid guessing
- `b`: boundary trace
- `t`: tesseral
- `d`: diffusion
- `o`: orbit pass
- `p`: perturbation

Highway does not define a new traversal.  It accelerates the pixel math
used by a traversal.  Making it a pass mode would mix two different
concepts and would make existing parameter files opt in to an
implementation detail.

`simd=` is the correct control surface because it expresses implementation
selection and gives tests a stable way to force scalar or SIMD behavior.

## Initial Eligibility

Start with a deliberately small set of supported states.

Eligible:

- standard escape-time render
- `passes=1` or `passes=2`
- double precision math only
- Mandelbrot or Julia optimized path
- pixel grid arrays allocated and filled
- no inversion
- no distance estimator
- no orbit display
- no orbit sound
- no show-dot wrapper
- no potential coloring
- no finite attractor handling
- no periodicity checking in the first slice
- simple inside color
- simple outside iteration color
- no bignum or bigfloat alternate math
- no formula interpreter

Everything else falls back to scalar code in `simd=auto` and `simd=off`.
In `simd=force`, every rejected condition should produce a specific reason.

The first SIMD path should target the same behavior as the optimized
Mandelbrot/Julia calculator, not the full `standard_fractal_type()` feature
set.

## Data Model

The current pixel grid is already close to the needed structure of arrays:

```text
x = grid_x0[col] + grid_x1[row]
y = grid_y0[row] + grid_y1[col]
```

Expose read-only spans or pointer/size views from `pixel_grid.cpp`:

```cpp
struct PixelGridSpans
{
    std::span<const double> x0;
    std::span<const double> y0;
    std::span<const double> x1;
    std::span<const double> y1;
};
```

If `std::span` is not available under the current C++ standard/library
constraints, use a tiny view type:

```cpp
template <typename T>
struct SpanView
{
    const T *data;
    std::size_t size;
};
```

The SIMD kernel should not call `dx_pixel()` or `dy_pixel()`.  Those
functions depend on global `g_col` and `g_row`, which is the scalar shape
the SIMD path is avoiding.

## Render Constants

Create a compact immutable parameter object before entering the SIMD pass:

```cpp
struct EscapeSimdParams
{
    bool julia;
    double param_x;
    double param_y;
    double bailout;
    long max_iterations;
    int colors;
    int and_color;
    int inside_color;
};
```

Add fields only when a supported coloring or formula needs them.  Do not
copy the entire global render state into this object.  The object is the
contract for what the SIMD kernel actually supports.

## Row Kernel

The first kernel should be row-oriented:

```cpp
void render_escape_row_highway(
    const EscapeSimdParams &params,
    const PixelGridSpans &grid,
    int row,
    int first_col,
    int last_col,
    Byte *colors);
```

The output buffer contains one color index per column from `first_col` to
`last_col`, inclusive.

The Highway loop owns lane state:

```text
cx      pixel real coordinate
cy      pixel imaginary coordinate
zx      current orbit real value
zy      current orbit imaginary value
iter    iteration count
active  lanes still iterating
```

Mandelbrot initialization:

```text
cx = x
cy = y
zx = x + param_x
zy = y + param_y
```

Julia initialization:

```text
cx = param_x
cy = param_y
zx = x
zy = y
```

Core loop:

```text
while any(active) and iter < max_iterations:
    x2 = zx * zx
    y2 = zy * zy
    mag = x2 + y2
    escaped = active and mag >= bailout
    record iter for newly escaped lanes
    active = active and not escaped
    zy = 2 * zx * zy + cy
    zx = x2 - y2 + cx
    iter += active
```

The exact initialization must be checked against
`calc_mandelbrot_type()` and `mandelbrot_orbit()` before coding.  Preserve
current off-by-one iteration semantics even if the loop above changes
shape.

## Color Mapping

Keep first-slice color mapping simple and explicit:

- escaped lanes use the escape iteration, with the existing zero adjustment
  if needed
- inside lanes use the configured inside color or `max_iterations`,
  matching the current optimized Mandelbrot/Julia path
- apply the same modulo/`g_and_color` behavior used by
  `calc_mandelbrot_type()`

Do not support log map, potential, decomposition, distance estimator, or
special outside colors in the first slice.  Each one can be added later as
a small post-process or as a separate kernel path.

## Output

The SIMD pass should write spans, not individual pixels.

Preferred path:

1. Render a row into a temporary `std::vector<Byte>` or fixed row buffer.
2. Use the existing symmetry-aware row output helper.
3. Preserve two-pass behavior by copying coarse-pass pixels the same way
   the scalar pass does, but at span granularity where practical.

If the existing row helper is private to `calcfrac.cpp`, either move it to
a small internal engine module or add a narrow wrapper.  Do not duplicate
the full symmetry logic in the SIMD pass.

## Highway Integration

Add the vcpkg dependency:

```json
"highway"
```

Find and link it in `libid/CMakeLists.txt`.  The exact target name should
be verified from the vcpkg package, but Highway commonly exports
`hwy::hwy`.

Keep Highway code isolated in its own translation unit, for example:

```text
engine/highway_escape.cpp
include/engine/highway_escape.h
```

Use Highway dynamic dispatch in that file:

```cpp
#define HWY_TARGET_INCLUDE "engine/highway_escape.cpp"
#include "hwy/foreach_target.h"
#include "hwy/highway.h"

HWY_BEFORE_NAMESPACE();
namespace id::engine::HWY_NAMESPACE
{
// target-specific implementation
}
HWY_AFTER_NAMESPACE();
```

The non-target wrapper calls `HWY_DYNAMIC_DISPATCH(...)`.  Do not expose
Highway vector types in public headers.  Pass scalar pointers, counts, and
plain parameter structs across the dispatch boundary.

## Testing Strategy

Tests need to compare scalar and SIMD behavior directly.

Add command parsing tests:

- `simd=auto`
- `simd=off`
- `simd=force`
- invalid `simd=` value

Add eligibility tests:

- Mandelbrot eligible
- Julia eligible
- inversion rejected
- distance estimator rejected
- bignum or bigfloat rejected
- formula rejected
- show orbit rejected

Add row-kernel tests:

- fixed grid, fixed params, known Mandelbrot colors
- odd width row
- width smaller than the lane count
- exact lane-count width
- scalar tail width
- max-iteration interior points
- immediate bailout points

Add image tests:

- `type=mandel passes=1 simd=off`
- `type=mandel passes=1 simd=force`
- `type=mandel passes=2 simd=off`
- `type=mandel passes=2 simd=force`
- `type=julia passes=1 simd=off`
- `type=julia passes=1 simd=force`

The SIMD and scalar images should match byte-for-byte for the supported
first-slice feature set.

## Implementation Slices

### Slice 1: Option Plumbing

- Add `SimdMode {AUTO, OFF, FORCE}`.
- Add global/user state for the selected mode.
- Parse `simd=auto|off|force`.
- Save and restore the option in parameter output and history if
  appropriate.
- Add unit tests for parsing.

No Highway dependency is needed in this slice.

### Slice 2: Eligibility Report

- Add `can_use_highway_pass()`.
- Return a small result object, not just `bool`.
- Include an enum reason for the first rejected condition.
- Honor `simd=off`.
- Make `simd=force` report rejection clearly.
- Add unit tests for eligible and rejected states.

This slice still calls scalar rendering only.

### Slice 3: Pixel Grid Views

- Expose read-only grid views from `pixel_grid`.
- Keep `dx_pixel()` and `dy_pixel()` unchanged for scalar users.
- Add tests that grid views match `dx_pixel()` and `dy_pixel()` for
  selected rows and columns.

This slice is data access only.

### Slice 4: Scalar Row Kernel

- Implement the new row-kernel interface in scalar C++ first.
- Do not use Highway yet.
- Match `calc_mandelbrot_type()` for the initial supported feature set.
- Compare row output against the existing scalar pixel path in unit tests.

This de-risks semantics before adding SIMD.

### Slice 5: Highway Build Integration

- Add `highway` to `vcpkg.json`.
- Add `find_package` and link target in `libid/CMakeLists.txt`.
- Add an empty or scalar-equivalent Highway dispatch function.
- Verify all platforms still configure and build.

No behavior change should occur in this slice.

### Slice 6: Highway Row Kernel

- Implement the masked lane loop with Highway.
- Handle partial final vectors.
- Keep scalar row kernel available for comparison tests.
- Add direct unit tests for odd widths and tails.
- Keep the pass dispatcher using scalar output until the row kernel is
  proven.

### Slice 7: One-Pass Auto Selection

- Route `passes=1 simd=auto` through Highway when eligible.
- Route `passes=1 simd=off` through scalar code.
- Route `passes=1 simd=force` through Highway or report rejection.
- Add image comparison tests.

### Slice 8: Two-Pass Auto Selection

- Add the same selection for `passes=2`.
- Preserve the first-pass pixel replication behavior.
- Add image comparison tests for both passes.

### Slice 9: Diagnostics

- Add optional `simd=stats` or debug logging.
- Report selected Highway target, rows rendered, scalar fallbacks, and
  rejection reason.
- Keep diagnostics out of normal output unless explicitly requested.

### Slice 10: Expand Eligibility

Add features one at a time, with scalar/SIMD comparison tests for each:

- periodicity
- log map
- potential
- simple outside methods
- burning ship
- distance estimator
- selected non-Mandel standard fractals

Do not merge all special cases into one large kernel.  Prefer small kernels
or post-processing steps with clear eligibility.

## Risks

- Iteration counts may differ by one if the optimized scalar path semantics
  are not matched exactly.
- Floating-point differences can appear if fused multiply-add changes the
  arithmetic.  Tests should define whether byte-for-byte output is required
  for the first slice.
- Highway dynamic dispatch adds build-system complexity.
- Some current behavior depends on global side effects such as
  `g_old_color_iter`, keyboard check counters, and orbit globals.  The
  first SIMD path should reject states needing those effects.
- Symmetry and two-pass replication can corrupt output if row-span
  boundaries are wrong.

## Success Criteria

- Existing parameter files keep their `passes=` meaning.
- `simd=auto` accelerates eligible Mandelbrot and Julia renders.
- `simd=off` gives the old scalar result.
- `simd=force` is reliable for tests and reports clear rejection reasons.
- First-slice scalar and SIMD image outputs match for supported states.
- Unsupported states continue to render through existing scalar engines.
