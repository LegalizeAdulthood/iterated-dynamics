<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# SIMD Highway Plan

Use Google Highway to accelerate eligible calculation without changing
which pass selects pixels or moving rendering into UI.

## Status And Release 1.5 Boundary

The calculation/pass refactor is implemented.  Highway integration,
`simd=`, grid views, and SIMD kernels are not yet implemented.

[Release 1.5](release-1.5.md) includes Phase 1 slices 1-8, including their
sub-slices.  Slice 9 diagnostics are optional.  Slice 10 eligibility
expansion, Phase 2 IFS, and Phase 3 non-standard work remain deferred.
Existing numbering is retained so those release references stay valid.

Phase 1 supports only eligible double-precision Mandelbrot and Julia
calculation under `passes=1|2`.  Unsupported states retain the current
scalar implementation.  Default `passes=g` is not accelerated by this
phase.  Moving to worker threads, eliminating all globals, and completing
the broader [hexagonal boundary](hexagonal.md) are not prerequisites.

## Current Implementation

The relevant boundaries are:

- `StandardFractal` owns setup, timing, work-list handling, active pixel
  state, dispatch, and the selected perturbation strategy.
- `StandardPass` routes concrete pass objects through its non-virtual
  variant.  Pixel passes select coordinates and accept completed colors.
- `OnePass` and `TwoPass` share `ScanlinePass`.  It owns traversal,
  first-pass replication, second-pass skips, and suspension coordinates.
- `StandardFractal::calculate_pixel()` calls `fractals::calc_type()`
  through `g_dispatch`, then supplies a completed color to the pass.
  The scalar calculator also plots the primary pixel.
- `calc_mandelbrot_type()` delegates to
  `StandardFractal::calculate_mandelbrot_pixel()`.  Its orbit calculation
  is `mandelbrot_orbit()` in `engine/calmanfp.cpp`.
- `pixel_grid.cpp` privately owns four coordinate vectors; its public
  accessors use `g_col` and `g_row`.  No batch/grid-view API exists.
- `engine/pixel.h` owns pixel/span access.  UI controllers own input,
  show-dot presentation, sound servicing, and display waits.

The removed free-function pass dispatcher is not an integration point.
Do not restore it, add another calculation-dispatch global, or put
calculation callbacks back into the passes.

## Decisions

- Highway is a calculation implementation, not a new pass algorithm.
- Do not add `passes=h` or a SIMD alternative to the pass variant.
- Keep traversal, symmetry, work-list, and lifecycle decisions in their
  current owners.
- Select SIMD inside `StandardFractal` for an eligible scanline operation,
  after setup has resolved effective options and dispatch.
- Keep kernels independent of UI, drivers, mutable global bindings,
  keyboard polling, and sound servicing.
- Use bounded batches and retain existing useful interaction checkpoints.
  A vector or internal state transition is not a reason to poll input.
- Keep the current scalar path as the oracle and complete fallback.
- Do not require speculative platform workarounds or a new port framework
  just to introduce the first kernel.

`passes=p` is compatibility syntax selecting perturbation with
solid-guess traversal, not an independent perturbation pass.  It is outside
Phase 1.  Perturbation selection remains orthogonal to traversal and SIMD
must not silently replace an active perturbation strategy.

## User Option

Add the engine-owned execution policy:

```text
simd=auto
simd=off
simd=force
```

- `auto` is the default: choose Highway only when the effective state is
  eligible; otherwise retain scalar calculation.
- `off` always retains the existing scalar implementation.
- `force` requires the Highway path and reports a specific reason when
  it cannot be used.  It must not silently use the legacy calculator.

Highway's portable target is still a Highway implementation; `force`
does not promise a particular hardware instruction set.

Keep parsing in engine command handling through the existing
`CommandRouter`/`CommandProcessor` route, including `@` parameter sets.
Store requested policy with engine user settings, initialize/reset it
explicitly, and preserve it through history and PAR output.  This
execution hint does not need a new GIF metadata field.

A forced rejection returns through the existing calculation diagnostic
boundary; the engine does not open a dialog.  Define rejection before
committing output rather than discovering an unsupported option halfway
through a batch.

Optional diagnostics may later add `simd=stats` or equivalent opt-in
reporting.  Their absence must not prevent normal release 1.5 use.

## Phase 1: Escape-Time Calculation

### Eligibility

Use effective state after image and per-image setup, not just the
requested fractal name or original `FractalSpecific` entry.

The initial set requires:

- `type=mandel|julia`, effective `passes=1|2`, and the optimized
  `calc_mandelbrot_type` dispatch.
- Double math, valid populated coordinate grids, and no active
  perturbation.
- Modulus bailout; normal initialization for the selected type, including
  the existing Mandelbrot parameter offset and Julia constant.
- `periodicity=0`; periodicity support belongs to later expansion.
- Inside `ColorMethod::COLOR` or `ColorMethod::ITER`, with numerical
  inside color handled separately from the method.
- Outside `ColorMethod::ITER` and ordinary palette-index coloring.
- No inversion, distance estimator, decomposition, biomorph, potential,
  log mapping, finite attractors, or true-color output.
- No orbit overlay, orbit sound, tone recording, enabled showdot, or
  output behavior requiring per-orbit presentation.
- No quick-calculation reuse or unsupported legacy/debug overrides.

Validate parameters and dimensions accepted by the optimized reference;
do not add unproved assumptions about bailout, maximum iterations,
palette size, offsets, or resume positions.  Additional restrictions
must have a named reason and a test.

Reject arbitrary-precision, formula, general-kernel, and other pass
operations in `force`; fall back in `auto`.  A default render with
periodicity enabled is initially scalar even under `simd=auto`.

Evaluate policy again at an existing safe work boundary if effective
state changes, for example after orbit display is toggled.  Retain
already committed pixels and apply the normal fallback/rejection policy
without bypassing input handling or duplicating work.

### Coordinate And Parameter Data

Expose a small read-only view of the four vectors in `pixel_grid.cpp`:

```text
x = x0[col] + x1[row]
y = y0[row] + y1[col]
```

The project currently uses C++17.  Use a domain-specific pointer/count view
rather than `std::span` or a language-standard upgrade.  Preserve the
existing grid arithmetic and validate dimensions before accessing it.
Views are valid only until grid storage is cleared or rebuilt.

Build an immutable snapshot containing only supported calculation inputs:
type, initial offset or Julia constant, bailout, iteration limit,
inside method/color, and palette reduction parameters.
Do not copy the entire global state or have the kernel read globals
through a convenience wrapper.

The snapshot and selected coordinates are engine data.  They can later
be supplied by another controller or worker without changing the kernel.

### Scalar Oracle Before SIMD

First implement the proposed batch calculation in scalar C++.
Compare it against the current optimized pixel path, not an idealized
Mandelbrot pseudocode loop.

The current Julia path performs an initial recurrence before entering
the shared loop.  Counter decrement, bailout equality, iteration-limit
handling, and palette reduction must match exactly.  Preserve arithmetic
operation order; do not substitute fused multiply-add or fast-math
transformations that change supported output.

Produce completed colors and the iteration/counter facts required for
normal completion and status.  Coloring methods and numerical color
values are separate inputs.  Use nonnegative colors, not calculation
sentinels, in completed output buffers.

### Bounded Calculation And Completion

The current pass API retains one selected pixel until completion.  It
cannot supply an entire row to a vector kernel without an explicit
extension.  Add bounded scanline selection in `ScanlinePass`, routed
through `StandardPass`; do not duplicate traversal in the SIMD backend.

Selection must identify exactly the next permitted positions in the
active row/pass and retain them until completion.  It must respect
work-item bounds, offsets, resume starts, and two-pass parity.
Selection does not calculate, plot, or advance the committed cursor.

`StandardFractal` owns batch calculation and the returned colors.
Complete accepted results in traversal order through the pass's existing
completion semantics.  Preserve first-pass neighboring-pixel copies and
second-pass omissions.  Primary plotting still belongs to calculation,
not to the kernel's UI caller.

A bounded batch may stop before a row ends.  Do not calculate a full row
to completion merely because it is convenient for vector loads.
Keep lookahead bounded, retain any uncommitted results, and preserve
work-budget accounting based on actual iteration work.

Do not return to UI after every vector or batch unconditionally.
Accumulate useful work until the existing calculation checkpoint, then
yield.  Account for the last committed pixel's visible counters and
status; updating colors alone is insufficient.

On suspension, record the first uncommitted selection through the pass.
Normal resume may recompute uncommitted math, but must not skip or replay
committed pixel output.  Completion and failure use the existing render
lifecycle and engine timing.

### Output And Symmetry

The first implementation can commit colors with existing `g_plot`
behavior and pass completion.  Vectorized math does not require replacing
pixel-output ownership in the same slice.

`write_span()` is raw pixel output, not a symmetry/traversal operation.
`sym_fill_line()` assumes a uniform color and cannot safely mirror
arbitrary-color SIMD rows.  Do not use it as a general row helper or add
a wrapper that hides this restriction.

Any later span-output optimization must preserve reversed mirrored
coordinates, offsets, bounds, and two-pass replication, and must prove
equivalence with the existing ordered output.  It is not a prerequisite
for the first release 1.5 vector calculation path.

### Highway Build And Dispatch

The dependency is not currently in the manifest.  Add `highway` to the
application feature dependencies, using the existing vcpkg configuration.
The bundled port's usage specifies `find_package(hwy CONFIG REQUIRED)`
and target `hwy::hwy`; use that version's package rather than assuming
nightly documentation names match it.

Keep target-specific vector code in an engine translation unit and expose
only ordinary scalar data types at its boundary.  Highway's
[dynamic dispatch][highway-dispatch] selects a compiled implementation
for the running CPU.  Keep its required target macros/include machinery
local; do not expose Highway vector types or platform intrinsics to
passes, UI, or public calculation APIs.

Use portable targets/tail handling as required by the chosen Highway
version.  Do not add project-wide CPU flags, fast math, or unrelated
compiler-policy changes.  [Highway's correctness notes][highway-faq]
describe contraction and partial-vector hazards relevant to this work.

Retain existing builds on Windows, Linux, macOS, FreeBSD, OpenBSD, and
NetBSD.  Fix demonstrated configuration/compiler problems, not guessed
platform limitations.

### Validation

Automate the deterministic behavior:

- Parameter parsing, reset/history/PAR policy, and routing through `@`.
- Each eligibility rejection, including active perturbation and the
  distinction between requested and effective calculation state.
- Grid views versus current coordinates for rotated/skewed bounds and
  logical offsets.
- Scalar batch versus optimized scalar pixel colors and iteration facts.
- Highway versus scalar batch across executable CPU targets, short
  batches, partial vectors, and safe tails.
- Bailout equality, Julia initialization, interior points, iteration
  limits, parameter offsets, and palette reduction.
- Selection/completion order, two-pass parity and replication, symmetry,
  odd bounds, work-list transitions, and first uncommitted resume state.
- Controller suspension, menu return, and fallback at existing work
  boundaries, without asserting exact poll counts or elapsed times.
- Complete eligible images with `simd=off`, `auto`, and `force`,
  compared byte-for-byte within the same platform/build.

Use the existing scanline/pass/controller fixtures and image/autokey
harnesses rather than inventing parallel test infrastructure.
Test arguments must explicitly satisfy initial eligibility, including
`periodicity=0`; image tests disable sound but do not imply every other
gate is satisfied.

Also verify that ineligible/default states retain scalar results.
Use per-scenario or parameterized tests according to existing test rules.
Native manual testing is needed only for a changed interaction that
those automated paths cannot cover.

Measure kernel and end-to-end calculation performance in optimized builds.
Do not promise a speedup from lane count alone, change synchronization
cadence to improve a measurement, or add timing thresholds to correctness
tests.

## Phase 2: IFS Fractals

### Current Shape

`NonStandardFractal` selects and owns `IFS2D` or `IFS3D` according to
the loaded definition.  Their bounded `iterate()` methods remain in
`fractals/lorenz.cpp`.

`ui::IFS2DController` and `ui::StereoController` start and step the
selected calculation.  They own input and presentation, not transformation
selection or orbit math.  The former synchronous IFS entries are gone.

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

The SIMD kernel advances independent walkers.  The calculation object
owns their point budget and unbounded state; its UI controller continues
to drive bounded work without knowing Highway lane types.

This is deterministic if the lane RNG streams are deterministic.  It is not
byte-for-byte identical to the old scalar single-walker renderer.  The test
oracle for Phase 2 is a scalar batched walker that uses the same lane
states, the same RNG streams, and the same lane output order.

Keep logical walker count and stream assignment independent of the
runtime CPU vector width if results must be stable across Highway
targets.  Changing vector width must not silently select a different
rendering experiment.

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

Keep the loaded definition unchanged.  The SoA object is a separate
execution representation built by the SIMD path.

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

Phase 3 covers the calculation objects selected by `NonStandardFractal`
other than the IFS work in Phase 2.  It is not part of release 1.5.

Observed calculator families:

- bifurcation
- cellular
- dynamic 2D and Mandel cloud
- inverse Julia
- Julibrot
- orbit 2D
- orbit 3D
- plasma
- popcorn and popcornjul
- FrothyBasin
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
SIMD point generation, and ordered scalar plotting.  This preserves
agreement with the scalar batched reference when lanes hit the same pixel;
it does not establish equivalence with the old single-orbit ordering.

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
L-system expansion and turtle traversal now use explicit stacks, but
still are not a dense numeric loop.  FrothyBasin needs its own eligibility
and overlay policy and remains scalar here.  The TestPoint calculator
should not drive the SIMD plan.

## Phase 1 Implementation Slices

Implement these as small commits.  Required sub-slices remain within
their parent release item.  Run focused tests with `cmake --workflow temp`
and integrated validation with `cmake --workflow rt-default`, following
the repository test rules.  Markdown-only work needs neither workflow.

Temporary bridges must identify their removing slice and carry the
`// TODO: removed later` marker.  Do not leave unused transition APIs.

### 1. Option Plumbing

- Add `SimdMode` with `AUTO`, `OFF`, and `FORCE` to engine user policy.
- Add the engine-owned `simd` handler through current command routing.
- Define process/fractal reset defaults and retain the requested policy
  in history and PAR output without a new GIF extension.
- Document values, eligibility limits, and scalar fallback in help.
- Cover parsing, invalid values, `@` sources, defaults, history, and
  output round trips.
- Until activation, report a requested forced backend as unavailable;
  mark that temporary guard for replacement in slice 7.3.

No Highway dependency or automatic SIMD selection is introduced here.

### 2. Eligibility Report

- Add an engine-owned eligibility result with a named rejection reason.
- Inspect effective math, active dispatch, traversal, coloring, output,
  and presentation requirements after setup.
- Distinguish `off`, supported configuration, and backend availability.
- Cover each supported/rejected case without invoking UI.
- Keep all rendering scalar until activation.

The report describes calculation capability, not a new pass type.

### 3. Pixel Grid Views

- Expose C++17 read-only coordinate views with dimensions and lifetime
  tied to the existing grid storage.
- Keep scalar `dx_pixel()` and `dy_pixel()` behavior unchanged.
- Compare coordinates for ordinary, rotated/skewed, offset, and rebuilt
  grids; reject invalid view dimensions.

This is data access only.

### 4. Scalar Row Kernel

Split the original kernel slice into two reviewable foundations.

#### 4.1. Inputs And Scalar Semantics

- Define compact immutable supported inputs and completed-pixel facts.
- Match the optimized Mandelbrot/Julia initialization, bailout,
  iteration, and palette behavior using explicit coordinate inputs.
- Compare against the existing pixel calculator, including edge cases.
- Keep calculation, plotting, and pass advancement separate.

#### 4.2. Bounded Scalar Batch

- Calculate an explicit bounded set of selected row positions.
- Support short groups and explicit sample positions without assuming
  every column in a row is requested.
- Return colors and required completion/work-budget facts.
- Compare every result with the scalar semantic reference.
- Do not connect the new batch calculator to normal rendering yet.

This reference remains available for Highway comparison.

### 5. Highway Build Integration

- Add `highway` to application dependencies in `vcpkg.json`.
- Find `hwy` and link `hwy::hwy` privately to `libid` using the bundled
  port's package.
- Add an isolated dispatch smoke test without selecting it for rendering.
- Verify existing platform configurations and CPU baseline behavior.
- Do not raise the language standard or change global CPU/math flags.

No rendering behavior changes in this slice.

### 6. Highway Row Kernel

- Implement masked independent pixel lanes from the scalar batch model.
- Preserve arithmetic ordering and palette results.
- Handle short groups and tails without out-of-bounds loads or stores.
- Compare colors and completion facts with the scalar batch reference on
  executable Highway targets.
- Keep automatic rendering on the existing path until slice 7.3.

If target or tail support requires a separate preparatory commit, keep it
within this parent item rather than combining it with traversal changes.

### 7. One-Pass Auto Selection

The original integration item needs three focused sub-slices.

#### 7.1. Bounded One-Pass Selection

- Extend `ScanlinePass` with bounded selection of the next one-pass
  positions; route it through the existing `StandardPass` variant.
- Retain selection until completion; keep the committed cursor unchanged
  while calculation is pending.
- Preserve single-pixel behavior for existing consumers.
- Test bounds, resumed starts, repeated selection, partial completion,
  and suspension at the first uncommitted position.

No vector math or automatic backend selection belongs in this sub-slice.

#### 7.2. Engine Batch Completion

- Let `StandardFractal` own bounded batch results and ordered completion.
- Plot primary pixels through existing calculation output and advance
  the pass only for committed colors.
- Preserve iteration-based work budgets, current status, engine timing,
  and suspend/resume behavior.
- Validate using the scalar batch reference before enabling Highway.
- Do not add UI callbacks, per-batch input polls, or a general span
  symmetry helper.

#### 7.3. Activate The One-Pass Backend

- Route eligible `auto` and `force` one-pass work to Highway.
- Keep `off` on the legacy scalar path and preserve `auto` fallback.
- Replace the temporary unavailable-backend guard from slice 1.
- Check effective-state changes at safe boundaries before new selection.
- Compare complete images and resumed output with scalar controls,
  including symmetry and logical-screen offsets.
- Reuse controller/autokey tests for interruption and menu return.

SIMD does not change which layer owns input or output.

### 8. Two-Pass Auto Selection

Keep traversal preparation separate from backend activation.

#### 8.1. Two-Pass Batch Selection And Completion

- Extend bounded selection for the coarse first pass and sparse
  second-pass omissions.
- Preserve parity at odd bounds and resumed starts, neighboring-pixel
  replication, work-list deferral, and remaining pass state.
- Test selection and completion with explicit supplied colors.
- Retain the existing scalar renderer until activation.

#### 8.2. Activate The Two-Pass Backend

- Enable the same calculation policy and kernel for selected positions
  in both passes, without adding a second traversal implementation.
- Verify intermediate coarse-pass output as well as the final image.
- Cover interruption/resume in each pass, deferred work, offsets,
  symmetry, and scalar fallback.
- Verify that batching does not add synchronization at vector boundaries.
- Document the completed release 1.5 supported set.

Completion of slices 1-8, including sub-slices, is the Phase 1 release
requirement.  No IFS or other non-standard SIMD path is required for 1.5.

### 9. Diagnostics

Optional for release 1.5:

- Add opt-in statistics or debug reporting.
- Report target, selected backend, work counts, and fallback/rejection
  reasons without changing normal output.
- Keep presentation in existing UI diagnostic/status owners.

### 10. Expand Eligibility

Deferred beyond the initial release 1.5 set.
Add one feature at a time with a scalar oracle:

- Periodicity.
- Log mapping.
- Potential.
- Additional coloring methods.
- Burning Ship or other standard formulas.
- Distance estimation.
- Additional output or presentation support.

Do not combine all special cases into one kernel or silently widen
eligibility before the required state/output behavior is implemented.

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

- Compare fixed-seed Highway and scalar-batch 2D images within one
  platform/build; do not add cross-platform drift-sensitive golds.
- Keep existing scalar tests on the scalar path.
- Add tests for transform-index coloring and histogram coloring.

### 7. Scalar Batched IFS3D

- Implement scalar batched IFS3D walkers.
- Preserve the waste phase and min/max setup.
- Store projected lane results before plotting.
- Keep stereo modes rejected until their projection/view contract is
  supported explicitly.
- Add scalar batch reference tests.

### 8. Highway IFS3D Kernel

- Implement vector 3D affine transform.
- Implement vector view matrix transform.
- Implement vector perspective and screen projection where eligible.
- Reduce waste-phase min/max across lanes.
- Compare Highway output to the scalar batch reference.

### 9. IFS3D Image Tests

- Compare fixed-seed Highway and scalar-batch 3D images within one
  platform/build; keep current single-walker controls separate.
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
- Use current recurrence/sequence tests as the semantic oracle.
- Reject unsupported variants explicitly.
- Add tests that the extracted kernel matches the old path.

### 3. Highway Bifurcation Kernel

- Process multiple columns per Highway vector.
- Keep lane-local recurrence state.
- Store lane results to temporary arrays before plotting if needed.
- Compare SIMD output to the scalar column kernel.
- Extend semantic recurrence/sequence comparisons with appropriate
  floating-point tolerances; do not restore removed bifurcation golds.

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

- A conventional escape-time pseudocode loop can differ from the current
  optimized initialization, bailout equality, or iteration count.
- Floating-point contraction and reassociation can change colors.
  Same-build scalar/SIMD pixel equivalence is required for Phase 1;
  fall back or reject when it cannot be established.
- A math-only batch can still corrupt resume state, counters, symmetry,
  or two-pass output if completion does not preserve ownership.
- Full-row calculation or unconditional vector-boundary yielding can
  regress interaction or synchronization overhead.
- Cached eligibility or grid views can become invalid after a restart or
  effective-state change.
- Highway packaging/target dispatch must work with the pinned dependency
  and all current platforms, not just the developer's CPU.
- IFS and other walker batches can alter RNG streams, pixel ordering, and
  images.  Their compatibility policy is separate from release 1.5.
- Grid-mutating and dependent-trajectory algorithms do not become
  parallel merely by using vector types.

## Success Criteria

### Release 1.5

- Phase 1 slices 1-8 and their sub-slices are complete.
- Existing parameter files retain traversal and perturbation semantics.
- Eligible `passes=1|2` Mandelbrot and Julia work uses Highway under
  `auto`; `off` preserves the current scalar calculator.
- `force` takes the requested backend or reports why it cannot.
- Supported scalar and SIMD output agrees within the same platform/build,
  including intermediate two-pass state and resumed output.
- Ineligible states retain scalar behavior under `auto`.
- Calculation remains engine-owned and input remains UI-owned.
- Existing builds and tests stay green; optimized-build measurements
  demonstrate the useful performance of the supported path.

### Deferred Phases

- IFS SIMD matches its scalar batched reference under an explicitly
  accepted compatibility policy.
- Non-standard families are added individually with suitable semantic
  or image comparisons, not a generic SIMD renderer.
- Existing scalar behavior remains available throughout expansion.

[highway-dispatch]:
  https://google.github.io/highway/en/master/quick_reference.html
[highway-faq]: https://google.github.io/highway/en/master/faq.html
