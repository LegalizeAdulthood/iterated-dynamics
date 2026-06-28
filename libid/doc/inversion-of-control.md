<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Standard Fractal Polling I/O Plan

Goal: move keyboard and mouse interaction out of engine calculation code
and into `libid/ui`, preserving the current polling behavior.

This is not the full event-driven inversion-of-control design.  It is the
intermediate polling refactor for release 1.5.  The immediate target is a
clean directory boundary: only `libid/ui` interacts directly with keyboard
and mouse input.

The standard escape-time renderer should follow the same shape as the
calculation types that have already been split into UI coordination code
and calculation objects.

## Existing Shape

These files already show the desired structure:

- `libid/ui/ant.cpp` drives `libid/fractals/Ant.cpp`.
- `libid/ui/bifurcation.cpp` drives
  `libid/fractals/Bifurcation.cpp`.
- `libid/ui/cellular.cpp` drives `libid/fractals/Cellular.cpp`.
- `libid/ui/diffusion.cpp` drives `libid/fractals/Diffusion.cpp`.
- `libid/ui/dynamic2d.cpp` drives `libid/fractals/lorenz.cpp`.
- `libid/ui/ifs2d.cpp` drives `libid/fractals/lorenz.cpp`.
- `libid/ui/ifs3d.cpp` drives `libid/fractals/lorenz.cpp`.
- `libid/ui/orbit2d.cpp` drives `libid/fractals/lorenz.cpp`.
- `libid/ui/orbit3d.cpp` drives `libid/fractals/lorenz.cpp`.
- `libid/ui/plasma.cpp` drives `libid/fractals/Plasma.cpp`.
- `libid/ui/standard_4d.cpp` drives
  `libid/fractals/julibrot.cpp`.
- `libid/ui/testpt.cpp` drives `libid/fractals/TestPoint.cpp`.

The common structure is:

```text
libid/ui/<type>.cpp
    construct or resume the calculation object
    poll keyboard input for this UI context
    suspend or exit when the UI context decides work should stop
    call iterate() until the calculation object is done

libid/fractals/<Type>.cpp or libid/engine/<Type>.cpp
    own calculation state
    expose resume(), suspend(), done(), and iterate()
    do not poll keyboard or mouse input
```

`libid/ui/inverse_julia.cpp` is close, but JIIM still has engine-side
keyboard and mouse ownership.

## Target Shape

Add a standard escape-time calculation object:

```cpp
namespace id::engine
{
class StandardFractal
{
public:
    void resume();
    void suspend();
    bool done() const;
    void iterate();
};
}
```

`StandardFractal` owns the standard renderer lifecycle.  That includes the
work list, the active work-list item, the active `StandardPass`, and the
active pixel's orbit state when a pixel needs to yield before it is
finished.

The active `StandardPass` owns pass-specific traversal and fill state.
That includes active row and column, subdivision state, block state, scan
stack, and decisions about whether a pixel needs real orbit work or can be
filled from surrounding pixels.

The pass algorithm does not compute the orbit.  It advances traversal and
fill state, then sets up the state needed for the next call to the
standard orbit function.  `StandardFractal` calls the correct orbit
function after the pass has prepared the current pixel state.  The chosen
function depends on the selected fractal type and orbit algorithm.  During
this refactor, that communication may remain indirect through existing
globals such as `g_row`, `g_col`, `g_color`, and `g_plot`.  Removing those
globals is separate work.

A practical implementation is a non-virtual `StandardPass` router that
holds a `std::variant` of concrete pass classes.  `StandardPass` exposes
one small interface, such as `resume()`, `suspend()`, `done()`, and
`iterate()`, and delegates to the active variant alternative.

Empty pass placeholders may start in `StandardPass.h`, but they should not
grow there.  As each pass slice is implemented, move that placeholder into
its own pass header and implementation file.  The pass class owns the
state and operation methods.  `StandardPass` includes that header, stores
the concrete class in the variant, and routes calls to it.  Thin
compatibility wrappers for the old free functions may remain during the
transition.

`StandardFractal` also owns the standard-mode dispatch currently selected
by `g_std_calc_mode`, including synchronous orbit, Tesseral, boundary
trace, solid guessing, diffusion scan, orbit mode, and the one-pass and
two-pass paths.

Perturbation is not a standard pass.  GitHub issue #180 records that the
current implementation computes the full image during fractal setup, which
is the wrong layer.  Perturbation is an alternate way to compute an
escape-time orbit for a pixel.  It belongs below the pass layer as a
standard orbit strategy.  `passes=p` should remain as compatibility
syntax, but should select perturbation orbit calculation while using the
default standard traversal.  For traversal purposes, `passes=p` is
synonymous with the default solid-guessing path.

The UI wrapper owns input policy.  It constructs or resumes
`StandardFractal`, polls input through the active UI context, asks the
object to suspend when work is interrupted, and calls `iterate()` until
`done()` is true.

The standard pixel kernels are calculation details.  They should not
double as image-level UI entry points.  If the current names are kept for
compatibility during the refactor, the final shape should still separate:

- image-level UI wrapper
- standard calculation object
- per-pixel escape-time helpers
- per-orbit helper functions

## Current Audit Snapshot

The current code is already part-way through this plan:

- `libid/ui/standard_fractal.cpp` already owns standard-render keyboard
  polling through the handler stack.
- `StandardFractal::run_current_work_item_mode()` is already a thin
  explicit `CalcMode` dispatch.
- Tesseral is closest to the target shape.  `StandardFractal` owns a
  `Tesseral`, and `Tesseral` has `iterate()`, `done()`, and `suspend()`.
- The one-pass and two-pass paths preserve scan position in the
  `StandardPass` object and yield standard pixel work back to the UI.
  They still communicate active pixel position through `g_row` and
  `g_col`.
- Boundary trace is still a monolithic function with static trail state
  and work-list resume on pixel interruption.
- Diffusion scan traversal state is owned by the `StandardPass`
  diffusion class.
- Solid guessing has a concrete `SolidGuess` class, but it is stack-local
  inside `solid_guess()` and still probes keyboard input.
- SOI still uses recursive control flow plus direct key polling, but its
  calculation state is owned by the `StandardPass` SOI class.
- Orbit mode traversal is owned by the `StandardPass` sticky-orbits
  class; interruption still happens in the orbit plotting helper.
- Perturbation has a `PertEngine` owned by `StandardFractal`, is
  initialized and driven from `StandardFractal`, yields to the UI between
  point chunks, and is selected separately from the active traversal mode.
  `passes=p` is compatibility syntax that maps traversal to solid
  guessing for now.  `PertEngine` still owns full-frame traversal and
  should be moved below the pass layer per issue #180.

## Input Rules

- The driver remains the production key queue while polling I/O exists.
- Do not add a second production key queue.
- `libid/ui` owns keyboard dispatch, key meaning, mouse subscriptions, and
  legacy input side effects.
- Code outside `libid/ui` must not call `driver_get_key`,
  `driver_key_pressed`, `driver_wait_key_pressed`, or `driver_unget_key`.
- Code outside `libid/ui` must not use `MouseNotification`,
  `mouse_subscribe`, `mouse_unsubscribe`, `g_cursor_mouse_tracking`, or
  `g_look_at_mouse`.
- Calculation code reports progress, completion, suspension state, and
  calculation decisions.  It does not interpret keys.
- Keep behavior first.  Rename and reshape after calls are moved.
- End each slice that changes calculation control flow with manual testing
  steps only for user interaction paths not covered by automated tests.

## Keyboard Handler Stack

The handler stack is UI input plumbing.  It supports the standard UI
wrapper; it is not the calculation-side architecture.

Handlers decide what a key means in a UI context:

```cpp
namespace id::ui
{
class KeyboardHandler
{
public:
    virtual ~KeyboardHandler() = default;

    virtual bool handle_key(int key) = 0;
};

using KeyboardHandlerPtr = std::shared_ptr<KeyboardHandler>;
}
```

`handle_key()` returns `true` only when the handler consumed the key.  It
does not return an interrupt result.  Handlers that want calculation to
yield set UI-owned interruption state.

`MainLoopKeyboardHandler` is the outer image UI context.  It consumes keys
that reach it, records the next main-loop command, and requests
calculation interruption.

`OrbitToggleKeyboardHandler` is a more-specific render context.  It
consumes `o` and `O`, toggles orbit display, and does not request
calculation interruption.

The UI dispatcher polls the driver queue, reads available keys, and offers
them to the active handler stack from top to bottom.  The first handler
that returns `true` stops propagation.  If no handler consumes the key,
the key is discarded.

## Remaining Inversion-of-Control Slices

These slices use the existing pattern from `libid/ui/ant.cpp`,
`libid/ui/bifurcation.cpp`, `libid/ui/cellular.cpp`, and similar files:

```text
ui wrapper
    construct or resume renderer
    poll keyboard input for the active UI context
    call suspend() or exit when the UI decides to stop
    call iterate() until the renderer reports no more work

calculation object
    own calculation state
    expose iterate(), done() when useful, resume(), and suspend()
    do not poll keyboard or mouse input
```

Do not introduce a new progress-result enum unless an individual renderer
needs more state than the existing `iterate()` and `done()` pattern can
express.

The next slices are split into two phases.  First, finish the
`StandardPass` ownership boundary by moving pass state out of globals,
file statics, stack locals, and recursive frames into the variant
alternatives.  These ownership slices should preserve synchronous
behavior and existing polling.  After pass state is owned, later slices
can change control flow and remove direct input from calculation code.

### Slice 1: Perturbation Pixel Strategy

Work:

- Expose the perturbation point calculator as a standard orbit strategy.
- Have `StandardFractal::calculate_standard_pixel()` use perturbation
  when perturbation is active and the fractal type supports it.
- Keep communication through existing pixel globals during this refactor.
- Leave glitch retry ownership in `PertEngine` until the next slice.

Done when:

- Standard pass traversal chooses the pixels.
- Perturbation supplies the per-pixel orbit calculation.
- `passes=p` traverses like the default standard path.

Manual testing:

- Render `type=mandel passes=p`.
- Interrupt and resume while pixels are being computed.

### Slice 2: Perturbation Glitch Work Items

Work:

- Represent perturbation glitch retries as standard renderer work, not a
  private full-frame loop.
- Use the standard work list or an equivalent renderer-owned queue for
  pixels or regions that need recomputation.
- Keep the glitch-divergence threshold behavior unchanged.
- Consider a later `perturbation=epsilon` parameter only after this
  ownership boundary is correct.

Done when:

- Glitch points that need recomputation are owned by the standard
  renderer state.
- `PertEngine` no longer owns image-level traversal.
- Perturbation is an orbit strategy below the pass layer.

Manual testing:

- Render a perturbation image that produces glitch retries.
- Interrupt and resume during glitch retry work.

### Slice 3: LSystem Renderer

Work:

- Add a `fractals::LSystem` calculation object for the renderer state in
  `libid/fractals/lsystem.cpp`.
- Add `libid/ui/lsystem.cpp` to construct or resume `LSystem`, poll
  keyboard input, call `suspend()`, and drive `iterate()`.
- Convert the recursive turtle draw path to resumable command-stack state
  owned by `LSystem`.
- Remove the key probe from `draw_lsys()`.

Done when:

- `lsystem.cpp` has no direct keyboard polling.
- L-system rendering follows the same UI/calculation split as Ant and
  Bifurcation.

Manual testing:

- Render one L-system image and interrupt it.
- Resume the interrupted render if resume is supported for the selected
  L-system.

### Slice 4: Lyapunov Renderer

Work:

- Add a `fractals::Lyapunov` calculation object for per-pixel Lyapunov
  state.
- Add `libid/ui/lyapunov.cpp` to own keyboard polling and drive
  `Lyapunov::iterate()`.
- Stop treating Lyapunov keyboard ownership as standard-engine detail even
  if Lyapunov still shares standard pixel-grid setup.
- Remove the key probe from `lyapunov_type()`.

Done when:

- `lyapunov.cpp` has no direct keyboard polling.
- Lyapunov rendering has an explicit UI wrapper and calculation object.

Manual testing:

- Render one Lyapunov image and interrupt it.

### Slice 5: Lorenz Photographer Mode

Work:

- Add `LorenzStereoKeyboardHandler` in `libid/ui`.
- Move the photographer-mode `s` / `S` save loop from
  `libid/fractals/lorenz.cpp:1553-1561` into the UI code that owns the
  stereo workflow.
- Preserve repeated `s` / `S` saves before the second image is rendered.

Done when:

- Lorenz calculation code does not consume photographer-mode keys.
- Photographer-mode save and continue behavior is unchanged.

Manual testing:

- Exercise photographer mode.
- Press `s` repeatedly before rendering the second image.

### Slice 6: Non-Interrupt Pending-Key Utilities

Work:

- Treat `engine/sound.cpp` and `engine/wait_until.cpp` separately from
  render interruption.
- Preserve the rule that pending input suppresses tone playback.
- Preserve the rule that delay loops wake when input is pending.
- Move the direct keyboard polling behind UI-owned timing or sound
  coordination without making calculation code ask whether rendering
  should stop.

Done when:

- `engine/sound.cpp` has no direct key polling calls.
- `engine/wait_until.cpp` has no direct key polling calls.
- Sound and wait behavior are unchanged.

Manual testing:

- Confirm sound is still suppressed when input is pending.
- Confirm delay loops still wake when input is pending.

## Current Direct Input Leaks

Direct polling or key consumption exists outside `libid/ui` in:

- `libid/engine/calcfrac.cpp`: non-standard wrapper polling and direct
  standard-pixel fallback polling.
- `libid/engine/solid_guess.cpp`: interrupt checks during block repaint.
- `libid/engine/soi.cpp`: interrupt checks during recursive scan.
- `libid/engine/sound.cpp`: suppress tone while keys are pending.
- `libid/engine/wait_until.cpp`: delay loop wakes on pending key.
- `libid/fractals/lorenz.cpp`: orbit interrupt and stereo save prompt.
- `libid/fractals/lsystem.cpp`: recursive draw interrupt.
- `libid/fractals/lyapunov.cpp`: per-pixel interrupt.

## Future Event-Driven Work

This plan leaves the following ideas out of scope for the current polling
slices.  They should be kept as future direction after direct polling
calls are moved behind `libid/ui`.

- Convert `Frame`, `WinText`, and `Plot` to wxWidgets controls while the
  current polling code still pumps GUI events.
- Add menu-bar support, with blocking text screens still preventing normal
  menu interaction until they are migrated.
- Convert blocking text screens to modal dialogs incrementally.  Start
  with simple numeric input, then form-style prompts, then file-entry
  screens.
- After text screens are dialogs, remove the menu-bar blocking workaround.
- Replace polling-driven calculation with idle-processing callbacks first.
  Treat idle processing as a bridge to later worker-thread or distributed
  rendering.
- Plan worker-thread and UI-thread communication as separate follow-up
  work.  Do not mix thread ownership, cancellation, or result delivery
  into this polling-boundary refactor.
- Replace driver polling in the UI dispatcher with GUI-thread event
  dispatch to the same handler stack.
- Let event callbacks set calculation-interrupt state through the active
  handlers once the engine runs on a worker thread.
- Build a tool interface for image interaction.  A tool should receive
  mouse, keyboard, and modifier-key events, not only `MouseNotification`
  calls.
- Support a tool stack with a permanent default zoom tool.  Events should
  go to the top tool, with optional fall-through to lower tools.
- Allow multiple zoom tools over time, such as current zoom-box behavior,
  XaoS-style continuous zoom, or ManpWIN-style controls.
- Preserve accurate zoom-box area selection.  It is a required
  interaction, not an artifact to discard when adding other zoom modes.
- After blocking screens and tools own input, remove the `Driver` keyboard
  API, custom message-pumping path, and custom event-loop workarounds.

## Exit Criteria

- Standard escape-time rendering is driven by a UI wrapper around
  `StandardFractal`.
- `StandardFractal` owns standard renderer iteration, mode dispatch, and
  resume state.
- `rg "driver_(get_key|key_pressed|wait_key_pressed|unget_key)" \
  libid/engine libid/fractals libid/geometry libid/io libid/math \
  libid/misc` returns no matches.
- `rg "MouseNotification|mouse_subscribe|mouse_unsubscribe" \
  libid/engine libid/fractals libid/geometry libid/io libid/math \
  libid/misc` returns no matches.
- `rg "g_look_at_mouse|g_cursor_mouse_tracking" \
  libid/engine libid/fractals libid/geometry libid/io libid/math \
  libid/misc` returns no matches.
- `libid/ui` is the only libid layer with direct keyboard and mouse I/O.
- Calculation code reports interruption or decisions without owning input.
- Current behavior is preserved while longer-term architecture stays out
  of scope.
