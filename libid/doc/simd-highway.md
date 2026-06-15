# SIMD Highway Plan

This document describes the implementation plan for using Google Highway in
selected fractal renderers.

Phase 1 covers standard escape-time fractals.  The goal is not to replace
every pass algorithm.  The goal is to add a SIMD implementation for the
ordinary row scan when the current render state is compatible with vector
execution.

Phase 2 covers 2D and 3D IFS fractal types.  IFS rendering is not a row
scan.  The scalar renderer advances one random orbit at a time, so the SIMD
shape is a batch of independent walkers.

Phase 3 covers non-standard fractal types that do not render through
`calc_standard_fractal()`.  These renderers have different dependency
shapes, so each needs its own eligibility and test oracle.

Existing scalar code remains the complete behavior oracle for unsupported
states.  Where a SIMD algorithm deliberately changes the execution shape,
the plan calls that out and adds a scalar batch reference for tests.

## Decisions

- Highway is an implementation detail, not a traversal mode.
- Do not add `passes=h`.
- Keep `passes=1`, `passes=2`, `passes=g`, `passes=b`, and the other pass
  modes focused on traversal strategy.
- Add a `simd=` option for diagnostics and test control.
- Use SIMD automatically when the render state is eligible.
- Fall back to scalar code unless the user explicitly forced SIMD.
- Keep each SIMD path narrow and easy to prove.
- Refactor existing pass algorithms later, after the row kernel is trusted.
- Treat escape-time fractals as Phase 1.
- Treat 2D and 3D IFS fractal types as Phase 2.
- Treat other non-standard fractal renderers as Phase 3.

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

## Phases

Phase 1 is the ordinary escape-time row renderer.  It should be
byte-for-byte compatible with the existing scalar path for every supported
state.

Phase 2 is the IFS renderer.  A single IFS orbit is sequential, so the SIMD
algorithm uses multiple independent walkers in lanes.  This is
deterministic, but it is not the same pixel sequence as the current scalar
single-walker implementation.  Phase 2 therefore needs its own scalar batch
reference and should start as `simd=force` only until the compatibility
policy is accepted.

Phase 3 is the set of remaining non-standard fractal renderers.  Some are
good SIMD targets because they have independent columns, pixels, or walker
states.  Others are poor targets because they are dominated by ordered grid
mutation, recursive drawing, or one long dependent trajectory.

## Phase 1: Escape-Time Fractals

### High-Level Shape

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

### Why Not `passes=h`

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

### Initial Eligibility

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

### Data Model

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

### Render Constants

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

### Row Kernel

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

### Color Mapping

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

### Output

The SIMD pass should write spans, not individual pixels.

Preferred path:

1. Render a row into a temporary `std::vector<Byte>` or fixed row buffer.
2. Use the existing symmetry-aware row output helper.
3. Preserve two-pass behavior by copying coarse-pass pixels the same way
   the scalar pass does, but at span granularity where practical.

If the existing row helper is private to `calcfrac.cpp`, either move it to
a small internal engine module or add a narrow wrapper.  Do not duplicate
the full symmetry logic in the SIMD pass.

### Highway Integration

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

### Testing Strategy

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

## Phase 2: IFS Fractals

### Current Shape

The 2D and 3D IFS entry points are:

- `ifs2d()`
- `ifs3d_calc()`
- `IFS2D::iterate()`
- `IFS3D::iterate()`

Both scalar algorithms advance one orbit point per call:

1. Generate a random number with `random_unit()`.
2. Select an affine transform by cumulative probability.
3. Apply that affine transform to the previous orbit point.
4. Convert the new point to screen coordinates.
5. Plot the point, either by transform index or by incrementing the
   current pixel color.

That dependency chain means the next point depends on the current point.
Widening the loop over time would not expose simple lane parallelism.

### SIMD Shape

Use SIMD lanes as independent IFS walkers:

```text
lane 0: walker 0 point n -> point n + 1
lane 1: walker 1 point n -> point n + 1
lane 2: walker 2 point n -> point n + 1
...
```

Each lane owns:

```text
x, y       current 2D point
x, y, z    current 3D point
rng        random stream state
k          selected transform index
active     lane still inside sane bounds
```

The SIMD kernel advances all lanes by one IFS step.  The pass driver calls
the kernel until the configured point budget is exhausted or every lane is
unbounded.

This is deterministic if the lane RNG streams are deterministic.  It is not
byte-for-byte identical to the old scalar single-walker renderer.  The test
oracle for Phase 2 is a scalar batched walker that uses the same lane
states, the same RNG streams, and the same lane output order.

### Phase 2 Eligibility

Start narrow.

Eligible:

- `type=ifs`
- 2D IFS definitions loaded from `g_ifs_definition`
- 3D IFS definitions loaded from `g_ifs_definition`
- no orbit save file in the first slice
- no real-time stereo glasses in the first 3D slice
- deterministic per-lane RNG streams
- scalar ordered plotting after SIMD point generation
- transform-index coloring
- histogram coloring only when plotting remains scalar and lane ordered

Rejected at first:

- raw orbit save output
- real-time stereo plotting
- direct vector scatter to the display buffer
- shared global `random_unit()` inside the SIMD kernel
- any mode that requires exact scalar single-walker output

`simd=off` keeps the existing scalar IFS algorithm.  Initial Phase 2 should
require `simd=force` for IFS.  After tests and image policy are settled,
`simd=auto` can choose the batched IFS path.

### IFS Data Model

Convert `g_ifs_definition` into structure-of-arrays data before entering
the SIMD loop.

2D transform fields:

```text
a, b, c, d, e, f
probability_cdf
```

3D transform fields:

```text
m00, m01, m02, tx
m10, m11, m12, ty
m20, m21, m22, tz
probability_cdf
```

Keep the original vector as the file-format representation.  The SoA object
is an execution representation built by the SIMD path.

### RNG

Do not call the global scalar RNG from inside the Highway kernel.  Add a
small explicit RNG state for SIMD IFS walkers.

Requirements:

- deterministic from the existing seed
- one independent stream per lane
- scalar batch and SIMD batch use the same RNG state transition
- tests can reproduce the same lane random values exactly

The first implementation can seed lane streams by consuming the existing
scalar RNG during setup, then use an explicit per-lane generator from that
point onward.

### Transform Selection

For each lane, generate `r` in `[0, 1)`.  Select the first transform whose
CDF is greater than or equal to `r`.

Highway shape:

```text
selected = last transform
for each transform i:
    take_i = not chosen and r <= cdf[i]
    selected = if take_i then i else selected
    chosen = chosen or take_i
```

Use the selected transform masks to choose coefficients with
`IfThenElse`.  This avoids gather in the first implementation and is fine
for the small number of IFS transforms normally used.

### 2D Kernel

The 2D SIMD step computes:

```text
new_x = a[k] * x + b[k] * y + e[k]
new_y = c[k] * x + d[k] * y + f[k]
col = cvt.a * new_x + cvt.b * new_y + cvt.e
row = cvt.c * new_x + cvt.d * new_y + cvt.f
```

Store `row`, `col`, and `k` into lane-order temporary arrays.  Then plot
with existing scalar functions in lane order.

Scalar ordered plotting keeps histogram coloring correct when multiple
lanes hit the same pixel in one batch.

### 3D Kernel

The 3D SIMD step computes:

```text
new_x = m00[k] * x + m01[k] * y + m02[k] * z + tx[k]
new_y = m10[k] * x + m11[k] * y + m12[k] * z + ty[k]
new_z = m20[k] * x + m21[k] * y + m22[k] * z + tz[k]
```

Then apply the view matrix and screen projection in SIMD:

```text
view = double_mat * orbit
if perspective:
    view = perspective(view)
col = cvt.a * view.x + cvt.b * view.y + cvt.e + g_xx_adjust
row = cvt.c * view.x + cvt.d * view.y + cvt.f + g_yy_adjust
```

The current 3D path has a waste phase that finds min and max values before
plotting.  Phase 2 can handle this in two steps:

1. Batch the waste phase and reduce lane min/max values into scalar
   min/max values.
2. Build the final view matrix and batch the plotting phase.

Start without real-time stereo.  Add stereo later by running the same
projection for both view matrices and scalar plotting both images in lane
order.

### IFS Output

Do not start with vector scatter.  Store lane results to temporary arrays
and plot them with existing scalar `g_plot`, `get_color`, and
color-sticking logic.

This first output strategy preserves existing display side effects and
avoids undefined ordering when two lanes target the same pixel.

Later, a tiled histogram buffer can reduce display reads and writes:

- accumulate hits in a private tile or image-sized buffer
- resolve saturated colors after the batch
- scatter only when the ordering policy is proven acceptable

### IFS Testing Strategy

Add a scalar batch reference first.  Compare SIMD against that reference,
not against the old scalar single-walker orbit.

Add tests for:

- deterministic lane RNG setup
- 2D transform selection by probability
- 3D transform selection by probability
- 2D scalar batch versus old scalar for one-lane mode
- 2D scalar batch versus SIMD batch
- 3D scalar batch versus SIMD batch
- repeated pixel hits with histogram coloring
- transform-index coloring
- unbounded lane handling
- `simd=force` IFS image tests with fixed seeds

Keep existing scalar IFS image tests on `simd=off` or default scalar until
the project chooses to rebaseline IFS images for batched SIMD.

## Phase 3: Other Non-Standard Fractals

### Scope

Phase 3 covers fractal types whose calculator is not
`calc_standard_fractal()` and which are not already covered by Phase 2 IFS.

Observed calculator families:

- bifurcation
- cellular
- dynamic 2D and Mandel cloud
- inverse Julia
- Julibrot
- orbit 2D
- orbit 3D
- plasma
- popcorn
- ant
- diffusion
- L-system
- test

Do not try to put these behind one generic SIMD loop.  The useful SIMD
shape differs by family.  Add one narrow SIMD renderer at a time, each with
its own scalar reference and eligibility report.

### Priority Implementation List

Priority 1: bifurcation.

Bifurcation renders mostly independent columns.  SIMD can process multiple
columns at once while each lane advances its own scalar recurrence.

Start with the simplest logistic path and reject periodicity, trig-heavy
variants, and special plotting modes.  Then add variants one at a time.
This is the best Phase 3 first target because the dependency shape is clear
and output order is naturally column-local.

Priority 2: cellular.

Cellular has a row-to-row dependency, but cells within the next row depend
only on a local window from the previous row.  SIMD can process adjacent
columns by loading overlapping windows and producing one next-row span.

The current rolling sum is scalar-shaped.  Refactor to a row kernel first,
then replace the window update with vector loads, shifts, or prefix sums.
Keep boundary cells scalar until the central span is proven.

Priority 3: Julibrot.

Julibrot is an escape-time renderer outside the standard path.  Screen
pixels are independent, and each pixel scans through a depth line.  SIMD
can run across adjacent pixels or across depth samples for one pixel.

Start with monocular rendering, simple projection, and the same color
semantics as the scalar path.  Reject stereo, unusual orbit modes, and any
state that requires scalar side effects in the first slice.

Priority 4: popcorn.

Popcorn starts independent orbits from screen points, then plots orbit
points into the image.  SIMD can advance several orbits as independent
walkers, but plotting must remain scalar and lane ordered at first.

Use the same model as IFS: explicit lane state, scalar batch reference,
SIMD point generation, and ordered scalar plotting.  This avoids image
differences when two lanes hit the same pixel.

Priority 5: dynamic 2D and Mandel cloud.

These are also walker-style renderers.  SIMD lanes can hold independent
starting points or independent random seeds.  They should reuse the IFS and
popcorn batching infrastructure where possible.

Start only after the batched walker policy is stable.  The main decision is
whether `simd=auto` may choose a deterministic batched output that is not
the old single-walker sequence.

Priority 6: chaotic orbit systems.

This covers Lorenz, orbit 2D, and orbit 3D families where the historical
renderer follows one dependent trajectory.

The useful SIMD mode is not to accelerate one trajectory.  Instead, trace
the original starting point plus a small epsilon grid around that starting
point:

```text
lane 0: original starting point
lane 1: start + (-eps, 0)
lane 2: start + ( eps, 0)
lane 3: start + (0, -eps)
lane 4: start + (0,  eps)
...
```

For 3D systems, extend the grid with z offsets.  Each lane advances its own
trajectory with the same equation and the same time step.  The image then
shows sensitivity to initial conditions directly: nearby starting points
separate over time.

This is an alternate visualization mode, not the old scalar single-orbit
sequence.  It needs a scalar epsilon-grid reference, lane-ordered plotting,
and explicit color policy for each trajectory.  Start behind `simd=force`
until the output policy is accepted.

Priority 7: inverse Julia.

Inverse Julia has random-walk and queue-like behavior.  The random-walk
case may batch like IFS.  Queue modes are more order-sensitive and should
remain scalar until their ordering rules are explicit.

Treat inverse Julia as a later walker-family target, not as an escape-time
row target.

Priority 8: plasma.

Plasma has recursive subdivision and random displacement.  SIMD may help
with level or wavefront midpoint updates, but the recursive structure and
random sequence make it less direct than bifurcation, cellular, or
Julibrot.

Keep it low priority unless profiling shows plasma is important.

Defer:

- ant
- diffusion
- L-system
- test

Ant and diffusion mutate a shared grid with strong order dependencies.
L-system rendering is recursive string and turtle work, not a dense numeric
loop.  The test calculator should not drive the SIMD plan.

## Phase 1 Implementation Slices

### 1. Option Plumbing

- Add `SimdMode {AUTO, OFF, FORCE}`.
- Add global/user state for the selected mode.
- Parse `simd=auto|off|force`.
- Save and restore the option in parameter output and history if
  appropriate.
- Add unit tests for parsing.

No Highway dependency is needed in this slice.

### 2. Eligibility Report

- Add `can_use_highway_pass()`.
- Return a small result object, not just `bool`.
- Include an enum reason for the first rejected condition.
- Honor `simd=off`.
- Make `simd=force` report rejection clearly.
- Add unit tests for eligible and rejected states.

This slice still calls scalar rendering only.

### 3. Pixel Grid Views

- Expose read-only grid views from `pixel_grid`.
- Keep `dx_pixel()` and `dy_pixel()` unchanged for scalar users.
- Add tests that grid views match `dx_pixel()` and `dy_pixel()` for
  selected rows and columns.

This slice is data access only.

### 4. Scalar Row Kernel

- Implement the new row-kernel interface in scalar C++ first.
- Do not use Highway yet.
- Match `calc_mandelbrot_type()` for the initial supported feature set.
- Compare row output against the existing scalar pixel path in unit tests.

This de-risks semantics before adding SIMD.

### 5. Highway Build Integration

- Add `highway` to `vcpkg.json`.
- Add `find_package` and link target in `libid/CMakeLists.txt`.
- Add an empty or scalar-equivalent Highway dispatch function.
- Verify all platforms still configure and build.

No behavior change should occur in this slice.

### 6. Highway Row Kernel

- Implement the masked lane loop with Highway.
- Handle partial final vectors.
- Keep scalar row kernel available for comparison tests.
- Add direct unit tests for odd widths and tails.
- Keep the pass dispatcher using scalar output until the row kernel is
  proven.

### 7. One-Pass Auto Selection

- Route `passes=1 simd=auto` through Highway when eligible.
- Route `passes=1 simd=off` through scalar code.
- Route `passes=1 simd=force` through Highway or report rejection.
- Add image comparison tests.

### 8. Two-Pass Auto Selection

- Add the same selection for `passes=2`.
- Preserve the first-pass pixel replication behavior.
- Add image comparison tests for both passes.

### 9. Diagnostics

- Add optional `simd=stats` or debug logging.
- Report selected Highway target, rows rendered, scalar fallbacks, and
  rejection reason.
- Keep diagnostics out of normal output unless explicitly requested.

### 10. Expand Eligibility

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

## Phase 2 Implementation Slices

### 1. IFS Policy And Eligibility

- Add IFS-specific SIMD eligibility.
- Require `simd=force` for IFS at first.
- Report why IFS SIMD is rejected.
- Document that the batched IFS renderer is deterministic but not the old
  scalar single-walker sequence.
- Keep `simd=off` on the existing scalar IFS path.

### 2. IFS Execution Data

- Build a SoA execution object from `g_ifs_definition`.
- Include 2D coefficient arrays and CDF arrays.
- Include 3D coefficient arrays and CDF arrays.
- Add validation for row sizes and probability totals.
- Add tests that the SoA object matches the original definition.

### 3. Explicit Lane RNG

- Add an explicit RNG state type for IFS SIMD lanes.
- Seed lane states deterministically from the current random seed.
- Implement scalar and Highway-compatible random generation from that
  state.
- Add tests for reproducible lane random values.

### 4. Scalar Batched IFS2D

- Implement a scalar batched IFS2D walker.
- Use the same lane state layout planned for Highway.
- Store lane `row`, `col`, and transform index arrays.
- Plot lane results in scalar lane order.
- Add tests for one-lane equivalence with the old scalar orbit where
  practical.

### 5. Highway IFS2D Kernel

- Implement vector transform selection.
- Implement vector 2D affine transform.
- Implement vector screen coordinate conversion.
- Keep plotting scalar and lane ordered.
- Compare Highway output to the scalar batch reference.

### 6. IFS2D Image Tests

- Add fixed-seed `simd=force` image tests for 2D IFS.
- Keep existing scalar tests on the scalar path.
- Add tests for transform-index coloring and histogram coloring.

### 7. Scalar Batched IFS3D

- Implement scalar batched IFS3D walkers.
- Preserve the waste phase and min/max setup.
- Store projected lane results before plotting.
- Keep real-time stereo rejected.
- Add scalar batch reference tests.

### 8. Highway IFS3D Kernel

- Implement vector 3D affine transform.
- Implement vector view matrix transform.
- Implement vector perspective and screen projection where eligible.
- Reduce waste-phase min/max across lanes.
- Compare Highway output to the scalar batch reference.

### 9. IFS3D Image Tests

- Add fixed-seed `simd=force` image tests for 3D IFS.
- Cover non-perspective and perspective projection if both are supported.
- Keep stereo rejected until a separate slice supports it.

### 10. IFS Auto Selection

- Decide whether batched IFS output is acceptable for `simd=auto`.
- If accepted, allow `simd=auto` for eligible IFS states.
- If not accepted, keep IFS SIMD behind `simd=force`.
- Update diagnostics to report IFS walker count and fallback reasons.

## Phase 3 Implementation Slices

### 1. Non-Standard Registry

- List non-standard fractal calculators and their SIMD family.
- Add a family-level eligibility report.
- Keep all Phase 3 families scalar by default.
- Make `simd=force` report the family-specific rejection reason.
- Add tests for classification and rejection.

### 2. Bifurcation Scalar Column Kernel

- Extract a scalar column kernel for the simplest bifurcation path.
- Keep old rendering behavior as the oracle.
- Reject unsupported variants explicitly.
- Add tests that the extracted kernel matches the old path.

### 3. Highway Bifurcation Kernel

- Process multiple columns per Highway vector.
- Keep lane-local recurrence state.
- Store lane results to temporary arrays before plotting if needed.
- Compare SIMD output to the scalar column kernel.
- Add fixed image tests for the supported bifurcation subset.

### 4. Cellular Row Kernel

- Extract a scalar next-row kernel.
- Define central vector span and scalar boundary handling.
- Replace scalar rolling sums with a vector-friendly window model.
- Add tests for boundaries, narrow widths, and odd widths.

### 5. Highway Cellular Kernel

- Implement the central row span with Highway.
- Keep boundary cells scalar.
- Compare the full image against the scalar row kernel.
- Add image tests for the supported cellular rule set.

### 6. Julibrot Scalar Pixel Kernel

- Extract a scalar pixel or depth-line kernel.
- Choose the first SIMD axis: adjacent pixels or depth samples.
- Reject stereo and complex side-effect modes.
- Add tests against the current Julibrot output.

### 7. Highway Julibrot Kernel

- Implement the selected vector axis with Highway.
- Preserve scalar color semantics.
- Add tail handling for odd widths or depth counts.
- Compare supported images byte-for-byte with scalar output.

### 8. Walker Shared Infrastructure

- Generalize explicit lane RNG and lane state from Phase 2.
- Add scalar batched references for walker-style renderers.
- Keep plotting scalar and lane ordered.
- Use this for popcorn, dynamic 2D, Mandel cloud, and inverse Julia.

### 9. Popcorn And Dynamic Walkers

- Add scalar batched popcorn.
- Add Highway popcorn point generation.
- Add scalar batched dynamic 2D and Mandel cloud when policy is clear.
- Compare SIMD output to the scalar batched references.
- Keep `simd=auto` disabled until output policy is accepted.

### 10. Chaotic Orbit Epsilon Grid

- Add a scalar epsilon-grid reference for Lorenz, orbit 2D, and orbit 3D.
- Seed one lane with the original starting point.
- Seed the remaining lanes with small offsets around the starting point.
- Advance each lane as an independent trajectory.
- Plot lane results in scalar lane order with distinct trajectory colors.
- Keep the mode behind `simd=force` until output policy is accepted.
- Add fixed image tests showing sensitivity to initial conditions.

### 11. Later Or Deferred Families

- Revisit inverse Julia random-walk batching.
- Revisit plasma only with profiler evidence or a clear wavefront model.
- Leave ant, diffusion, L-system, and test calculators scalar.

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
- IFS SIMD changes the execution shape from one walker to many walkers.
  This is deterministic but not byte-for-byte compatible with the old
  scalar single-walker output.
- IFS histogram coloring depends on pixel update order.  The first SIMD
  path must plot generated points in scalar lane order.
- IFS direct scatter can produce different results when lanes collide on
  one pixel.  Scatter should wait until the ordering policy is explicit.
- IFS 3D setup has a waste phase that computes min/max values.  SIMD must
  reduce those values consistently before plotting.
- Phase 3 renderers do not share one SIMD shape.  Treating them as one
  generic backend would hide eligibility bugs.
- Walker-style Phase 3 renderers can change pixel update order just like
  IFS.  Keep ordered scalar plotting until the output policy is explicit.
- Grid-mutating renderers such as ant and diffusion have strong ordering
  dependencies and may not repay SIMD work.

## Success Criteria

- Existing parameter files keep their `passes=` meaning.
- `simd=auto` accelerates eligible Mandelbrot and Julia renders.
- `simd=off` gives the old scalar result.
- `simd=force` is reliable for tests and reports clear rejection reasons.
- First-slice scalar and SIMD image outputs match for supported states.
- Phase 2 SIMD IFS matches the scalar batched IFS reference.
- Phase 2 keeps existing scalar IFS output available with `simd=off`.
- Phase 3 accelerates at least bifurcation, cellular, and Julibrot through
  family-specific SIMD paths.
- Unsupported states continue to render through existing scalar engines.
