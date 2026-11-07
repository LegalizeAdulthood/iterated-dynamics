<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# L-System Testing Plan

## Goal

Extend L-system coverage without cross-platform image goldens.  Small
floating-point differences can move rasterized lines onto different
pixels.  Prefer exact symbolic checks and tolerant geometry checks before
rasterization.  The implementation is the source of truth; existing tests
already cover much of the original plan.

## Current Implementation

- [lsystem.h](../include/fractals/lsystem.h) exposes the pimpl-based
  `LSystem`: `start()`, `iterate()`, `sizing()`, `done()`, `interrupted()`,
  and `suspend()`.
- [lsystem.cpp](../fractals/lsystem.cpp) owns explicit sizing and drawing
  stacks.  Sizing advances by at most 256 traversal steps per `iterate()`;
  drawing advances one traversal step, which need not draw a line.
- `LSysTurtleState`, command arrays, and branch snapshots remain private.
  Definition text, loaded state, and trigonometric tables still have
  file-scope storage; this is not an independently owned definition model.
- `lsystem_load()` locates and parses a named file entry.  It throws a
  diagnostic on failure, clears partial definition state, and leaves
  `lsystem_loaded()` false.  Starting without a loaded definition throws
  `std::logic_error`.  Empty rule replacements are valid, not malformed.
- [NonStandardFractal](../engine/NonStandardFractal.cpp) selects and owns
  the L-system calculation and manages its image lifecycle.
- [LSystemController](../../ui/LSystemController.cpp) advances that
  calculation, polls input, and manages the thinking display and cleanup.
  The calculation does not poll keys or display thinking messages.
- Drawing commands `lsys_draw_d()` and `lsys_draw_f()` convert turtle
  endpoints to integers and call `driver_draw_line()`.  No floating-point
  line sink or semantic event trace exists yet.

Do not interpret `interrupted()` as "a key was detected."  Direct
`LSystem::suspend()` sets it without completing traversal, and some drawing
failure paths also set it.  Sizing overflow can instead end calculation
without setting it.  Controller suspension finalizes the selected
calculation as non-resumable; it does not save traversal stacks for later
resumption.

## Existing Coverage

### Calculation And Loading

[Fractal tests](../../../tests/libs/core/fractals/test_lsystem.cpp) already
use `MockDriver::draw_line()` to record ordered integer endpoints and
colors. Coordinates allow a one-pixel difference; colors and line counts
are checked exactly.  These are line-output tests, not raster image
goldens.

The parameterized `TestLSystemRender.rendersDefinitionAtOrder` covers:

- Fitting both axes and centering a degenerate axis.
- Nested branches and branches inside expanded rules.
- Restoring reverse direction, real angle, and scale after a branch.
- Including unplotted movement in sizing bounds.
- Expansion orders zero, one, and two.
- Matching-rule source order and empty replacements.
- Deep expansion using explicit stacks rather than recursive calls.

Additional fixtures cover bounded sizing, the first drawing increment,
completion, overflow, suspension, cached definitions, and equivalence
between direct `LSystem` work and `NonStandardFractal` selection.  Selected
calculation tests forbid driver keyboard and text-screen calls.

Loading coverage includes missing files and entries, missing axioms,
invalid or missing angles, reserved symbols, extra text, malformed and
unknown commands, unterminated entries, failed reload cleanup, rejection
of starts after load failure, and recovery with a valid definition.

Do not add these scenarios again merely to satisfy the old parser and
traversal testing sections.

### UI Controller

[Controller tests](../../../tests/libs/ui/test_lsystem.cpp) cover:

- Thinking display lifetime, spinner updates, and closure before drawing.
- Sizing and drawing interruption with the command left pending.
- Non-resumable status and definition cleanup after suspension.
- Input, display, and plotting exceptions with cleanup.
- Invalid controller lifecycle calls and iteration after completion.
- Stepwise equivalence with the underlying calculation.
- Controller destruction closing presentation without prematurely
  finishing the calculation.

Keep input and presentation assertions here rather than adding them to
the turtle execution tests.

### Application Integration

The existing autokey tests provide interaction coverage:

- [lsystem-sizing](../../../tests/autokey/lsystem_sizing_test.cmake)
  interrupts high-order `Koch1` sizing, saves the interrupted result,
  changes the order, and completes a new calculation.
- [lsystem-validation](
  ../../../tests/autokey/lsystem_validation_test.cmake)
  exercises invalid selection, correction, and cancellation.  It also
  checks batch failure diagnostics and the absence of saved batch output.

Their checker programs validate metadata, blank versus nonblank output,
and, for validation recovery, unchanged output within the same run.
They do not compare L-system pixels against cross-platform gold images.
They complement semantic tests rather than replacing them.

## Remaining Test Strategy

Use existing line capture when integer endpoints and colors are enough.
The remaining geometry gap is visibility before integer conversion, not
the ability to substitute a driver.  A new seam must expose calculation
output without introducing UI callbacks, input polling, or timing policy.
Keep plotting in the calculation layer, consistent with
[the ports-and-adapters direction](hexagonal.md).

For small synthetic systems, compare ordered floating-point segments and
symbolic operations.  Choose explicit absolute and relative tolerances
appropriate to the coordinate scale.  Do not make line counts, colors,
operation order, or branch depth approximate.

For larger definitions, prefer exact counts and tolerant summaries over
full trace goldens.  Useful summaries include drawn length, bounds,
branch depth, final turtle position, angle, and color.  Distinguish sizing
traversal from drawing traversal so events are not counted twice.  Moves
affect sizing bounds even when they do not draw a segment.

A quantized trace hash remains optional, not the starting point.  Small
drift near a quantization boundary can still change the hash; tolerance
comparisons and structural summaries give better failure diagnostics.

## Remaining Implementation Slices

### Slice 1: Floating-Point Line Output

Work:

- Add a narrow way to capture line endpoints before integer conversion in
  `lsys_draw_d()` and `lsys_draw_f()`.
- Preserve the existing production conversion and `driver_draw_line()`
  output.  Avoid adding a second integer-only draw sink: the mock driver
  already supplies that seam.
- Keep any injected output dependency owned by the calculation instance,
  not a new mutable global or UI-owned turtle callback.
- Add a tiny synthetic definition test that compares floating-point
  segments with tolerance and colors exactly.

Done when a test observes unrounded endpoints while existing line-output
tests continue to verify unchanged production behavior.

### Slice 2: Targeted Turtle Semantics

Split into separately reviewable sub-slices:

1. Add capture of unplotted movement and final turtle geometry.  Test
   fixed-angle and real-angle movement without relying on fitted integer
   line endpoints.
2. Expose branch push and restore observations from the explicit stacks.
   Verify saved position, direction, scale, and color with focused cases.
3. Add direct coverage for color commands, reversal, turns, and scale
   factors where current line-output cases do not already distinguish
   the behavior.  Observe command effects, not UI presentation.

Use a minimal observation mechanism sufficient for each test.  Do not
require an event protocol for every internal operation just to duplicate
coverage already provided by existing rendered segments.  Keep sizing
and drawing observations distinguishable.

Done when new tests identify previously unobserved turtle behavior with
exact symbolic checks and tolerant geometry checks.

### Slice 3: Larger-System Summaries

Work:

- Add summary helpers over the observations from earlier slices.
- Select representative definitions from
  [id.l](../../../home/lsystem/id.l), including curved geometry and
  branching, after inspecting their commands and existing coverage.
- Add focused tests for uncovered counts, branch depth, bounds, drawn
  length, or final state.  Use independently derived expectations for
  tractable systems rather than recording implementation output blindly.
- Reuse `Koch1` lifecycle and sizing coverage; do not add another generic
  nonblank-image smoke test.

Done when larger-system checks catch semantic or geometric regressions
without depending on rasterized pixels.

### Slice 4: Definition And Parser Isolation

This remains separate from the existing parser validation coverage.
Split into small steps:

1. Extract an owned `LSystemDefinition` containing axiom, angle, and rules.
   Preserve current rule ordering, empty replacements, diagnostics, and
   named-entry loading behavior.
2. Separate definition parsing from file lookup.  Keep loading as an I/O
   concern without moving engine parsing or geometry into UI.
3. Allow calculation tests to supply definitions directly.  Retain file
   integration tests for lookup and load failure behavior; migrate only
   tests whose subject is independent of file lookup.

Do not claim full reentrancy from a definition object alone: shared
trigonometric tables and remaining global configuration need separate
ownership decisions if multiple calculations are required.

Done when parser and turtle tests can use independent definitions
without file-search setup, while file loading retains its existing
behavior.

## Validation

- Each test case covers one scenario.  Extract repeated setup and teardown
  into fixtures; parameterize cases that differ only by inputs.
- Reuse existing test support.  Place shared helper tests under
  `tests/libs/core/test`, not under production `misc` test directories.
- Run new and affected tests through the `temp` workflow while developing
  a slice; use `rt-default` for full workflow validation as required.
- These slices need no repeated manual checks of automated loading,
  thinking-display, interruption, or image-completion scenarios.  Add a
  manual check only for a traced interaction gap not covered by those
  tests.
