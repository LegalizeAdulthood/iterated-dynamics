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
keyboard and mouse ownership.  `libid/ui/frothy_basin.cpp` is not this
pattern; it is still a UI-side per-pixel helper that calls `check_key()`.

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

`StandardFractal` owns the standard renderer's iteration state.  That
includes the current standard pass, work list position, active rectangle,
active row and column, and the active pixel's orbit state when a pixel
needs to yield before it is finished.

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
  steps.

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
that returns `true` stops propagation.  If no handler consumes the key, the
key is discarded.

## Standard Fractal Slices

Line ranges are current anchors.  They may drift as slices land.

### Slice 1: Introduce StandardFractal

Work:

- Add `StandardFractal` in `libid/engine`.
- Move the high-level standard pass state from
  `libid/engine/calcfrac.cpp:808-873` into `StandardFractal`.
- Preserve the current one-pass, two-pass, solid-guess, and three-pass
  sequencing.
- Expose `resume()`, `suspend()`, `done()`, and `iterate()`.
- Keep existing keyboard polling locations unchanged in this slice.
- Keep `calc_fract()` behavior unchanged except that standard rendering is
  now driven through `StandardFractal`.

Done when:

- Standard rendering still completes through the existing engine path.
- `StandardFractal` owns the high-level standard pass phase state.
- Existing direct polling sites are unchanged.

Manual testing:

- Render a normal Mandelbrot image.
- Render with `passes=1`, `passes=2`, and solid guessing.
- Start a render, interrupt it, and resume it.

### Slice 2: Move Work-List State Into StandardFractal

Work:

- Move work-list setup, resume, pop, and completion state from
  `libid/engine/calcfrac.cpp:938-1228` into `StandardFractal`.
- Make `iterate()` advance a bounded unit of work instead of running the
  whole work list.
- Make `done()` report whether all standard phases and work-list items are
  complete.
- Make `suspend()` write the same resume data currently written by
  `perform_work_list()`.
- Keep existing keyboard polling locations unchanged in this slice.

Done when:

- Work-list ownership is in `StandardFractal`.
- Resume data is unchanged.
- Existing standard images still render identically.

Manual testing:

- Render one-pass and two-pass Mandelbrot images.
- Interrupt and resume each mode.
- Render a standard image that uses potential output.

### Slice 3: Add the UI Standard Wrapper

Work:

- Add `libid/ui/standard_fractal.cpp` and a matching header.
- Drive `StandardFractal` from the UI wrapper using the same structure as
  `ant`, `bifurcation`, `cellular`, `dynamic2d`, and `testpt`.
- Poll input in the UI wrapper between calls to `StandardFractal::iterate`.
- On interruption, call `StandardFractal::suspend()` and return control to
  the outer UI.
- Keep direct key polling inside `libid/ui` for this wrapper.
- Move the standard image-level entry point out of engine code as far as
  the current dispatch model allows.

Done when:

- The standard renderer has a UI coordination file.
- The UI wrapper owns the standard render polling loop.
- `StandardFractal` owns calculation progress and resume state.

Manual testing:

- Press a normal command key while standard rendering is active.
- Confirm rendering returns to the main loop and the command is handled.
- Confirm resume still continues the interrupted image.

### Slice 4: Use Standard Render Keyboard Handlers

Work:

- Add or reuse `MainLoopKeyboardHandler` for the outer image UI context.
- Add `OrbitToggleKeyboardHandler` for `o` and `O`.
- Push the main-loop handler while the image UI loop is active.
- Push the orbit handler while standard rendering supports orbit toggling.
- Replace raw driver polling in `libid/ui/standard_fractal.cpp` with the
  UI dispatcher and handler stack.
- Reset UI-owned calculation interruption state at standard render start.

Done when:

- The standard UI wrapper does not interpret raw driver keys directly.
- `o` and `O` toggle orbit display without interrupting calculation.
- Other keys interrupt rendering and are recorded for the main loop.

Manual testing:

- Press `o` during a Mandelbrot render and confirm orbit display toggles
  without ending the render.
- Press a normal command key during a Mandelbrot render and confirm the
  render yields to the main loop.

### Slice 5: Remove Mandelbrot Hot-Path Polling

Work:

- Move key handling out of
  `libid/engine/calcfrac.cpp:1239-1319`.
- Preserve the current keyboard check cadence as an iteration budget owned
  by `StandardFractal` or the standard UI wrapper.
- Keep orbit toggling owned by `OrbitToggleKeyboardHandler`.
- Keep all non-orbit key meaning owned by `MainLoopKeyboardHandler`.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- The Mandelbrot fast pixel path has no direct keyboard polling.
- The Mandelbrot fast pixel path does not interpret key values.
- Standard rendering still yields at the same practical cadence.

Manual testing:

- Render a fast Mandelbrot image and interrupt it.
- Toggle orbit display with `o` during the render.
- Resume the interrupted render.

### Slice 6: Make Standard Pixels Yieldable

Work:

- Move active pixel state from
  `libid/engine/calcfrac.cpp:1323-2030` into `StandardFractal`.
- Replace the `check_key()` calls at
  `libid/engine/calcfrac.cpp:1477` and
  `libid/engine/calcfrac.cpp:1996` by returning from
  `StandardFractal::iterate()`.
- Make a long-running pixel advance in bounded orbit chunks.
- Keep color, potential, distance-estimator, sound, and orbit-save side
  effects unchanged.
- Preserve the current bailout and periodicity behavior.

Done when:

- The standard pixel path has no direct keyboard polling.
- Long-running standard pixels can yield back to the UI wrapper.
- Pixel-local state survives across calls to `iterate()`.

Manual testing:

- Render a deep or high-iteration Mandelbrot image and interrupt it.
- Toggle orbit display during a slow render.
- Resume and confirm the image completes correctly.

### Slice 7: Finish Standard Calcfrac Cleanup

Work:

- Remove standard-render polling from `libid/engine/calcfrac.cpp`.
- Remove standard-render dependency on `ui/check_key.h`.
- Keep `calc_fract()` responsible for calculation setup and finish only if
  that remains the least disruptive route.
- Prefer moving image-level UI decisions to `libid/ui` when the call graph
  permits it without broad churn.
- Keep per-pixel helpers as calculation helpers, not UI entry points.

Done when:

- Standard rendering input ownership is in `libid/ui`.
- `StandardFractal` owns standard iteration state.
- `calcfrac.cpp` no longer calls direct keyboard polling for standard
  rendering.

Manual testing:

- Render standard Mandelbrot, Julia, formula, and Newton examples.
- Interrupt and resume at least one one-pass and one two-pass image.
- Verify orbit toggling during standard rendering.

## JIIM Slices

### Slice 8: Move Inverse-Julia Keyboard Context

Work:

- Add `InverseJuliaKeyboardHandler` for the modal JIIM context.
- Move key interpretation from `libid/engine/jiim.cpp:655-869`.
- Move key drain polling from `libid/engine/jiim.cpp:1136-1147`.
- Move final key requeue from `libid/engine/jiim.cpp:1335-1336`.
- Push the handler while inverse-Julia is active and pop it on exit.
- Keep cursor movement, save, exit, and requeue behavior unchanged.

Done when:

- `jiim.cpp` has no direct key polling calls.
- The active JIIM handler owns inverse-Julia key meaning.

Manual testing:

- Enter inverse-Julia mode.
- Move the cursor with the keyboard.
- Save from the modal context.
- Exit and confirm the following UI command behavior is unchanged.

### Slice 9: Move JIIM Mouse Handling

Work:

- Move `InverseJuliaMouseNotification` from `engine/jiim.cpp` to
  `libid/ui`.
- Move inverse-Julia mouse subscribe and unsubscribe calls to `libid/ui`.
- Move `g_cursor_mouse_tracking` and `g_look_at_mouse` manipulation to the
  UI code that owns the subscription.
- Keep `jiim.cpp` receiving position updates and mouse-change decisions.
- Remove `ui/mouse.h` from `engine/jiim.cpp`.

Done when:

- `engine/jiim.cpp` has no direct `MouseNotification`,
  `mouse_subscribe`, `mouse_unsubscribe`, `g_cursor_mouse_tracking`, or
  `g_look_at_mouse` use.
- The only inverse-Julia mouse subscription code is in `libid/ui`.

Manual testing:

- Enter inverse-Julia mode.
- Move the mouse and confirm the cursor updates.
- Exit inverse-Julia mode and confirm mouse tracking state is restored.

## Remaining Polling Cleanup

These slices happen after the standard renderer has the UI wrapper shape.
They should use the same rule: move input ownership to `libid/ui`; leave
calculation code with state, decisions, and return values.

### Slice 10: Move Pure Render Interrupt Probes

Work:

- Replace direct pending-key interrupt probes with UI-owned interruption
  checks.
- Cover `engine/PertEngine.cpp`, `engine/solid_guess.cpp`,
  `engine/soi.cpp`, `fractals/lsystem.cpp`, and
  `fractals/lyapunov.cpp`.
- Preserve each local return value and resume behavior.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- Those files have no direct key polling calls.
- Calculation code only reports whether it should stop or suspend.

Manual testing:

- Render one example for each changed path.
- Interrupt each render and confirm the outer UI resumes control.

### Slice 11: Move Lorenz UI Prompts

Work:

- Move the `plot_orbits2d` interrupt behavior from
  `libid/fractals/lorenz.cpp` into the owning UI wrapper.
- Add `LorenzStereoSaveKeyboardHandler` for the photographer-mode save
  loop.
- Move the save prompt loop from
  `libid/fractals/lorenz.cpp:1553-1561`.
- Preserve repeated `s` or `S` save behavior.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- Lorenz calculation code has no direct key polling.
- Photographer-mode save and continue behavior is unchanged.

Manual testing:

- Render a Lorenz orbit and interrupt it.
- Exercise photographer-mode save with repeated `s` or `S`.

### Slice 12: Move Non-Interrupt Pending-Key Queries

Work:

- Move the sound pending-key check behind a UI-owned polling query.
- Preserve the rule that pending input suppresses `driver_sound_on`.
- Move the delay-loop key wakeup behind a UI-owned polling query.
- Keep sleep interval and wakeup behavior unchanged.
- Keep these separate from calculation interruption.

Done when:

- `engine/sound.cpp` has no direct key polling calls.
- `engine/wait_until.cpp` has no direct key polling calls.
- Sound and wait behavior are unchanged.

Manual testing:

- Confirm sound is still suppressed when input is pending.
- Confirm delay loops still wake when input is pending.

## Current Direct Input Leaks

Direct polling or key consumption exists outside `libid/ui` in:

- `libid/engine/calcfrac.cpp`: standard renderer polling, including the
  Mandelbrot hot-path orbit toggle.
- `libid/engine/jiim.cpp`: inverse-Julia key wait, drain, and requeue.
- `libid/engine/PertEngine.cpp`: interrupt check during reference pass.
- `libid/engine/solid_guess.cpp`: interrupt checks during block repaint.
- `libid/engine/soi.cpp`: interrupt checks during recursive scan.
- `libid/engine/sound.cpp`: suppress tone while keys are pending.
- `libid/engine/wait_until.cpp`: delay loop wakes on pending key.
- `libid/fractals/lorenz.cpp`: orbit interrupt and stereo save prompt.
- `libid/fractals/lsystem.cpp`: recursive draw interrupt.
- `libid/fractals/lyapunov.cpp`: per-pixel interrupt.

Direct mouse ownership exists outside `libid/ui` in:

- `libid/engine/jiim.cpp`: inverse-Julia mouse notification and mouse
  tracking state.

## Future Event-Driven Work

This plan leaves the following ideas out of scope for the current polling
slices.  They should be kept as future direction after direct polling calls
are moved behind `libid/ui`.

- Convert `Frame`, `WinText`, and `Plot` to wxWidgets controls while the
  current polling code still pumps GUI events.
- Add menu-bar support, with blocking text screens still preventing normal
  menu interaction until they are migrated.
- Convert blocking text screens to modal dialogs incrementally.  Start with
  simple numeric input, then form-style prompts, then file-entry screens.
- After text screens are dialogs, remove the menu-bar blocking workaround.
- Replace polling-driven calculation with idle-processing callbacks first.
  Treat idle processing as a bridge to later worker-thread or distributed
  rendering.
- Plan worker-thread and UI-thread communication as separate follow-up
  work.  Do not mix thread ownership, cancellation, or result delivery into
  this polling-boundary refactor.
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
- `StandardFractal` owns standard renderer iteration and resume state.
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
- Current behavior is preserved while longer-term architecture stays out of
  scope.
