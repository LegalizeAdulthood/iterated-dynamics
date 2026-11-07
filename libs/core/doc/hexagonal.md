<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Ports And Adapters

## Purpose

Move incrementally toward hexagonal architecture: calculation and its
data must not depend on interactive presentation or platform input.
This document records the architectural direction and migration approach;
it is not yet a set of implementation slices.

The implementation is the source of truth.  The completed input work is
described in [inversion-of-control.md](inversion-of-control.md).

## Current Boundary

Keyboard and mouse polling have been removed from calculation code.
Calculation objects advance incrementally under UI controllers rather
than controlling input themselves.

That does not complete the architectural separation.  Some lower-level
code still includes UI headers and accesses UI-owned state.  The broad
driver interface also exposes input and presentation capabilities to
clients that only need output.  Removing runtime input polling is not
the same as removing source-level dependencies on UI responsibilities.

## Dependency Direction

An arrow means "depends on":

```text
engine -> geometry
ui -> engine
ui -> io
io -> engine
```

The intended responsibilities are:

- `geometry` provides geometric operations without engine or UI state.
- `engine` owns calculation, rendering state, and engine data without
  depending on UI or file formats.
- `io` translates persisted representations to and from engine data.
- `ui` coordinates configuration, rendering, loading, saving, and input.

This graph does not permit `engine -> io`, which would introduce a cycle
with `io -> engine`.  Where calculation needs an external capability,
define an appropriate port below UI and supply its implementation from
above.  Do not move a dependency into a callback merely to hide it.

Shared data belongs below UI when its meaning is independent of user
interaction.  UI coordination remains above it.  Evolution metadata is
an example: I/O should serialize data without reaching into interactive
evolution controls.  Its final ownership needs an explicit decision,
not just relocation of a header.

## Input And Calculation Boundaries

`libs/core/ui` owns keyboard dispatch, key meaning, mouse subscriptions,
and input side effects.  While polling I/O remains, the driver owns the
production key queue; do not add a second queue to decouple calculation.
Code outside UI must not call `driver_get_key`, `driver_key_pressed`,
`driver_wait_key_pressed`, or `driver_unget_key`.  Mouse notification,
subscription, and cursor-tracking facilities such as `MouseNotification`,
`mouse_subscribe`, `mouse_unsubscribe`, `g_cursor_mouse_tracking`, and
`g_look_at_mouse` also belong exclusively in UI.

Calculation reports progress, completion, suspension state, and
calculation decisions; it does not interpret keys.  This boundary applies
to setup, sizing, completion, errors, and cleanup as well as incremental
work.  Indirect calls through diagnostics, thinking displays, dispatch
pointers, cursor helpers, or storage setup must not reintroduce input or
modal UI.  Changes to engine or fractal code must not add UI includes.

Image plotting and pixel access belong in calculation or storage code.
The UI owns presentation waits and coordination, not rendering algorithms.
Moving rendering into UI to remove an upward dependency does not resolve
the architectural boundary.  Misplaced backend work should instead move
below UI.

Numerical sound submission follows the same output-port principle, with
its non-blocking contract and UI-owned playback coordination defined in
[sound.md](sound.md).

## Remaining Upward Dependencies

The current starting inventory is:

- `geometry/line3d.cpp` registers cleanup through UI-owned
  `g_out_line_cleanup` declared in `ui/big_while_loop.h`.
- `io/ImageSave.cpp` reads evolution state to construct GIF metadata and
  includes `ui/big_while_loop.h` and `ui/evolve.h`.
- `io/loadfile.cpp` restores evolution state and reads
  `g_make_parameter_file` through UI declarations.
- `io/load_config.cpp` parses video key names and updates UI-owned
  video key bindings.
- `io/save_timer.cpp` depends on the UI ticker and `g_finish_row` for
  autosave coordination.
- `io/encoder.cpp` still includes `ui/big_while_loop.h` and `ui/evolve.h`;
  check whether these includes are unused before planning ownership work.

These dependencies do not themselves establish that input polling
remains.  They establish that UI is not yet wholly a layer above the
other components.  Audit actual symbol use as well as includes.

## Driver Interface Segregation

The driver acts as a God Class for existing input and output ports.
It groups keyboard input, mouse input, pixel output, palette operations,
sound, timing, text screens, and window management.  Clients depend on
more capabilities than they use, violating the Interface Segregation
Principle (ISP).

Separate capabilities into focused ports based on actual consumers:

- Calculation code depends on the rendering output capabilities it uses.
- UI depends on input, window, and presentation capabilities.
- I/O depends on engine data and the image access it actually needs.
- Platform adapters implement the capabilities they support.

These are responsibility groups, not a predetermined interface list.
Audit driver methods and callers before selecting port boundaries.
One class can implement several ports; ISP does not require a separate
implementation object for every interface.

The existing driver can remain the implementation bridge during
migration.  A narrow port must restrict what its clients can access,
not merely forward an equally broad driver API under a new name.

The [sound output port](sound.md) is an existing example: engine-facing
samples and operations are separate from the interactive driver adapter.
Do not replace that completed separation with a new parallel mechanism.

## Miscellaneous Code

`misc` is a collection of responsibilities, not an architectural layer.
Classify its contents individually:

- Rendering concepts belong in `engine`.
- Geometric operations belong in `geometry`.
- Persistence belongs in `io`.
- Interaction and application coordination belong in `ui`.
- Genuinely general utilities may remain in a low-level support area
  without depending on the higher layers.

Do not move the entire directory or create a generic shared layer that
simply preserves the existing dependencies.

## Incremental Migration

Use two related tracks.  Neither requires replacing the whole driver or
changing platform implementations at once.

### Remove Upward Dependencies

For each remaining dependency on UI:

1. Identify the specific state or operation the lower layer needs.
2. Place engine data and lifecycle responsibilities below UI.
3. Keep interaction and coordination in UI.
4. Remove the upward dependency and validate preserved behavior.

Give each responsibility its own small, cohesive, reviewable slice.
Do not combine unrelated changes because they use the same UI header.
Deleting an include alone does not resolve ownership or hidden coupling.

### Split Driver Capabilities

For each capability used by calculation code:

1. Identify consumers and the operations they require.
2. Extract a narrow port at the appropriate lower layer.
3. Adapt the existing driver to implement the port.
4. Migrate consumers in small slices.
5. Remove superseded driver methods after their last caller migrates.

Preserve behavior while changing boundaries.  Do not simultaneously
redesign platform backends, output semantics, or application threading.
Temporary adapters are migration tools, not the final architecture.

Preserve the existing synchronization cadence.  An incremental state
transition is not automatically an input-dispatch, display-refresh, or
cross-thread synchronization point.  Bounded calculation may process many
orbit points or pixels before returning to its controller.  Making every
internal step synchronize would penalize local, threaded, and future
remote rendering without improving the ownership boundary.

## Next Planning Step

Inventory driver capabilities and callers against the dependency graph.
Combine that inventory with the remaining upward dependencies to decide
port ownership and migration order before writing implementation slices.

Audit globals, callback ownership, and reachable capabilities as well as
headers.  No UI includes is necessary, but not sufficient, for the desired
boundary.  Calculation clients must not retain access to input through a
broad driver interface.

Centralizing UI polling in the application loop and moving rendering to
a worker thread are separate follow-ups.  This boundary work prepares
for them without making them prerequisites for each small refactoring.
