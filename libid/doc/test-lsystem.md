<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# L-System Testing Plan

Goal: add useful L-system tests without relying on image goldens.
Rendered line pixels are too sensitive to small floating-point differences,
because a tiny coordinate drift can move a rasterized line onto different
pixels.  Tests should compare L-system semantics before rasterization.

## Testing Layers

### Parser Tests

Validate the `.l` parser and file loading rules:

- Axiom is loaded.
- Angle is loaded and validated.
- Rules are loaded in source order.
- Bad syntax reports failure.
- Missing or invalid angle reports failure.
- Empty or malformed rules report failure.

These tests can start with file fixtures through `lsystem_load()`.  Direct
string-based parser tests can wait until parsing is split from file lookup.

### Traversal Tests

Drive small synthetic systems through `LSystem::start()` and
`LSystem::iterate()`:

- Rule expansion depth is honored.
- Branch push and pop order is preserved.
- Multiple matching rules, if supported, are applied in the current order.
- `done()` becomes true after traversal completes.
- `interrupted()` becomes true on stack overflow or key interruption.

These tests should not compare pixels.  They should compare command or
turtle events.

### Semantic Draw Trace

Add a test sink below turtle execution but above pixel plotting.  The
production sink calls `driver_draw_line()`.  The test sink records events:

- `line(x0, y0, x1, y1, color)`
- `move(x0, y0, x1, y1)`
- `turn(angle_index)`
- `scale(factor)`
- `push()`
- `pop()`
- `color(n)`

Coordinate checks should use a tolerance.  Event counts, event order,
colors, branch depth, and final turtle state can be exact where the values
are integer or symbolic.

### Summary Invariants

For larger built-in L-systems, avoid full trace goldens.  Check summaries:

- Line count.
- Move count.
- Branch count.
- Maximum branch depth.
- Final angle.
- Final color.
- Bounding box with tolerance.
- Total drawn length with tolerance.
- Command count.

This catches large regressions while allowing harmless math drift.

### Quantized Trace Hash

For medium systems, hash semantic events after quantizing coordinates to
a tolerant grid.  This gives compact regression coverage without comparing
final image pixels.

## Refactoring Slices

### Slice 1: Draw Sink Seam

Work:

- Add an optional L-system draw sink.
- Store the sink in `LSysTurtleState`.
- Replace direct `driver_draw_line()` calls in the draw commands with
  `sink.line(...)`.
- Use a production sink that forwards to `driver_draw_line()`.
- Add a test sink that records line events.

Done when:

- Existing rendering behavior is unchanged.
- A test can render a tiny L-system and inspect recorded line events.

### Slice 2: Semantic Event Trace

Work:

- Extend the sink with move, turn, scale, color, push, and pop events.
- Emit branch push and pop events from the draw stack.
- Emit turtle operation events from the existing command functions.
- Add test helpers for event counts, final state, and tolerant coordinate
  comparison.

Done when:

- Tests can verify expansion order, branch restore behavior, color changes,
  and approximate geometry without reading pixels.

### Slice 3: Summary Tests

Work:

- Add summary helpers over recorded events.
- Add tests for one or more built-in L-system definitions.
- Compare counts exactly and geometry with tolerance.

Done when:

- Larger L-system coverage exists without image goldens.
- Small floating-point drift does not fail the tests.

### Slice 4: Parser Isolation

Work:

- Split parsed L-system data into an `LSystemDefinition` object.
- Let tests construct definitions directly from strings.
- Keep file lookup and `lsystem_load()` as UI or loading concerns.

Done when:

- Parser tests and renderer tests can be written independently.
- Renderer tests no longer need file-search setup for tiny synthetic
  systems.

## Preferred Starting Point

Start with the draw sink seam.  It is the smallest useful refactor,
exercises the real parser and renderer, and fits the current `LSystem`
pimpl shape.  Parser isolation can come later if file fixtures become too
noisy.
