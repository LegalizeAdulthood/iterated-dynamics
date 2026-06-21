<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Polling I/O Refactor Plan

Goal: move direct polling I/O out of calculation code and into `libid/ui`.
Keep the current polling behavior while doing it.

This plan is only for the polling refactor.  It does not convert text
screens to dialogs, add a tool stack, rewrite the event loop, or delete the
`Driver` key API.

The GitHub discussion also records the larger inversion-of-control path.
Those items are kept in `Future Event-Driven Work` so the polling slices
stay mechanical and behavior-preserving.

## Rules

- `libid/ui` owns keyboard polling, key buffering, and legacy key side
  effects.
- Calculation code may ask whether work should stop.
- Calculation code must not call `driver_get_key`,
  `driver_key_pressed`, `driver_wait_key_pressed`, or
  `driver_unget_key`.
- Keep behavior first.  Rename and reshape after the calls are moved.
- Keep each slice small enough to review by inspection and focused tests.

## Proposed Polling API

Add:

- `libid/include/ui/KeyboardInput.h`
- `libid/ui/KeyboardInput.cpp`
- `tests/libid/ui/test_KeyboardInput.cpp`

Keep key codes as `int`.  This is a boundary move, not a new input event
model.

Interface:

```cpp
namespace id::ui
{
class KeyboardInput
{
public:
    virtual ~KeyboardInput() = default;

    virtual int pending_key() = 0;
    virtual int read_key() = 0;
    virtual int wait_for_key(bool timeout) = 0;
    virtual void push_key(int key) = 0;
};
}
```

Semantics:

- `pending_key()` returns the pending key code, or zero.  It does not
  consume the key.
- `read_key()` consumes one key.  It may block, exactly as the driver does
  today.
- `wait_for_key()` waits for a key using the current timeout rule.
- `push_key()` requeues one key.

Production:

- Add `DriverKeyboardInput final : public KeyboardInput` in
  `ui/KeyboardInput.cpp`.
- `DriverKeyboardInput` forwards to `driver_key_pressed`,
  `driver_get_key`, `driver_wait_key_pressed`, and `driver_unget_key`.
- The production forwarding path must preserve current driver side
  effects: window-event pumping, redraw handling, key buffering, and mouse
  notification delivery.
- Keep the default instance in `ui/KeyboardInput.cpp`; do not add a
  second key buffer.

Accessors:

```cpp
namespace id::ui
{
extern KeyboardInput *g_keyboard_input;

int kb_pending_key();
int kb_read_key();
int kb_wait_for_key(bool timeout);
void kb_push_key(int key);
}
```

The `kb_xxx` free functions delegate to `g_keyboard_input`.  Production
points it at `DriverKeyboardInput`.  Tests may replace it with a fake
`KeyboardInput`.

Calculation code should use semantic helpers:

```cpp
namespace id::ui
{
bool calculation_interrupted();
bool calculation_interrupted_or_orbit_toggled();
bool input_pending();
}
```

Semantics:

- `calculation_interrupted()` returns `kb_pending_key() != 0`.
- `calculation_interrupted_or_orbit_toggled()` preserves the Mandelbrot
  hot-path rule: `o` or `O` is consumed and toggles `g_show_orbit`; any
  other pending key reports interruption and is left pending.
- `input_pending()` is for delay and sound code that must wake or avoid
  output when a key is pending, without consuming that key.

Caller rules:

- `libid/engine` and `libid/fractals` may include
  `ui/KeyboardInput.h`.
- `libid/engine` and `libid/fractals` must use only semantic helpers.
- Direct `Driver` key calls stay in `DriverKeyboardInput` and driver
  classes.
- New abstract interfaces are exposed through global pointers, like
  `g_driver`.  Use `g_keyboard_input` for `KeyboardInput` and
  `g_mouse_input` for mouse interaction.  Do not drag them through
  engine call chains.
- Tests install a fake `KeyboardInput`; most tests should not need
  `MockDriver`.
- Later event-driven work may replace the facade internals without
  touching calculation code.

## Non-UI Includes Of UI Headers

Scope: production code only.  Tests and vendored files are excluded.

This is the broader layering backlog for making `libid/ui` sit cleanly on
top of engine and support code.

### libid/engine

- `boundary_trace.cpp`: `ui/stop_msg.h`, `ui/video.h`.
- `calcfrac.cpp`: `ui/check_key.h`, `ui/diskvid.h`,
  `ui/find_special_colors.h`, `ui/frothy_basin.h`, `ui/stop_msg.h`,
  `ui/video.h`.
- `calc_frac_init.cpp`: `ui/stop_msg.h`, `ui/zoom.h`.
- `cmdfiles.cpp`: `ui/big_while_loop.h`, `ui/comments.h`,
  `ui/do_pause.h`, `ui/get_fract_type.h`, `ui/goodbye.h`,
  `ui/help_title.h`, `ui/history.h`, `ui/make_batch_file.h`,
  `ui/slideshw.h`, `ui/stereo.h`, `ui/stop_msg.h`.
- `fractalb.cpp`: `ui/goodbye.h`, `ui/stop_msg.h`.
- `jiim.cpp`: `ui/diskvid.h`, `ui/editpal.h`,
  `ui/find_special_colors.h`, `ui/frothy_basin.h`,
  `ui/get_a_number.h`, `ui/help.h`, `ui/id_keys.h`, `ui/mouse.h`,
  `ui/stop_msg.h`, `ui/temp_msg.h`, `ui/video.h`.
- `one_or_two_pass.cpp`: `ui/video.h`.
- `orbit.cpp`: `ui/video.h`.
- `solid_guess.cpp`: `ui/video.h`.
- `sound.cpp`: `ui/stop_msg.h`.
- `tesseral.cpp`: `ui/check_key.h`, `ui/video.h`.
- `video_mode.cpp`: `ui/id_keys.h`.

### libid/fractals

- `Ant.cpp`: `ui/video.h`.
- `Cellular.cpp`: `ui/thinking.h`, `ui/video.h`.
- `Diffusion.cpp`: `ui/video.h`.
- `fractalp.cpp`: `ui/ant.h`, `ui/bifurcation.h`,
  `ui/cellular.h`, `ui/diffusion.h`, `ui/dynamic2d.h`,
  `ui/frothy_basin.h`, `ui/inverse_julia.h`, `ui/orbit2d.h`,
  `ui/plasma.h`, `ui/standard_4d.h`, `ui/testpt.h`.
- `frasetup.cpp`: `ui/editpal.h`.
- `ifs.cpp`: `ui/stop_msg.h`.
- `julibrot.cpp`: `ui/get_3d_params.h`, `ui/starfield.h`,
  `ui/stop_msg.h`.
- `lorenz.cpp`: `ui/ifs2d.h`, `ui/ifs3d.h`, `ui/orbit3d.h`,
  `ui/stop_msg.h`, `ui/video.h`.
- `lsystem.cpp`: `ui/stop_msg.h`, `ui/thinking.h`.
- `lyapunov.cpp`: `ui/stop_msg.h`.
- `parser.cpp`: `ui/stop_msg.h`.
- `Plasma.cpp`: `ui/diskvid.h`, `ui/video.h`.

### libid/geometry

- `line3d.cpp`: `ui/big_while_loop.h`, `ui/diskvid.h`,
  `ui/stereo.h`, `ui/stop_msg.h`, `ui/video.h`.
- `plot3d.cpp`: `ui/diskvid.h`, `ui/video.h`.

### libid/io

- `decoder.cpp`: `ui/video.h`.
- `decode_info.cpp`: `ui/evolve.h`.
- `encoder.cpp`: `ui/big_while_loop.h`, `ui/diskvid.h`,
  `ui/evolve.h`, `ui/goodbye.h`, `ui/slideshw.h`,
  `ui/stop_msg.h`, `ui/temp_msg.h`, `ui/video.h`.
- `file_item.cpp`: `ui/stop_msg.h`.
- `gifview.cpp`: `ui/diskvid.h`, `ui/slideshw.h`,
  `ui/stereo.h`, `ui/video.h`.
- `loadfile.cpp`: `ui/big_while_loop.h`, `ui/get_3d_params.h`,
  `ui/get_video_mode.h`, `ui/make_batch_file.h`,
  `ui/stop_msg.h`.
- `loadmap.cpp`: `ui/stop_msg.h`.
- `save_timer.cpp`: `ui/big_while_loop.h`, `ui/read_ticker.h`.

### libid/math

- `biginit.cpp`: `ui/goodbye.h`, `ui/stop_msg.h`.

### libid/misc

- `memory.cpp`: `ui/diskvid.h`, `ui/goodbye.h`, `ui/stop_msg.h`.

### libid/include

- `engine/cmdfiles_test.h`: `ui/stop_msg.h`.
- `io/gif_extensions.h`: `ui/evolve.h`.
- `io/gif_file.h`: `ui/evolve.h`.

## Current Leaks

Direct polling or key consumption exists outside `libid/ui` in:

- `libid/engine/calcfrac.cpp`: Mandelbrot hot path, including the `o`
  orbit toggle.
- `libid/engine/jiim.cpp`: inverse-Julia key wait, drain, and requeue.
- `libid/engine/PertEngine.cpp`: interrupt check during reference pass.
- `libid/engine/solid_guess.cpp`: interrupt checks during block repaint.
- `libid/engine/soi.cpp`: interrupt checks during recursive scan.
- `libid/engine/sound.cpp`: suppress tone while keys are pending.
- `libid/engine/wait_until.cpp`: delay loop wakes on pending key.
- `libid/fractals/lorenz.cpp`: orbit interrupt and stereo save prompt.
- `libid/fractals/lsystem.cpp`: recursive draw interrupt.
- `libid/fractals/lyapunov.cpp`: per-pixel interrupt.

`ui/check_key` already owns one useful legacy rule: a pending key means
interrupt unless it is `o` or `O`, which toggles orbit display.

## Slice 1: Add A UI Polling Facade

Work:

- Add a small `ui/KeyboardInput` API around the current `Driver` key
  methods.
- Expose semantic operations: pending key, read key, wait key, push key.
- Add a calculation helper for "pending key means interrupt".
- Keep `ui/check_key` as the orbit-toggle helper.
- Add tests that verify the facade forwards to `MockDriver`.

Done when:

- New code can poll only through `libid/ui`.
- The facade preserves the current `Driver` call behavior.
- Tests cover pending, read, wait, and push operations.

## Slice 2: Move Mandelbrot Hot-Path Polling

Work:

- Move the direct key check in `engine/calcfrac.cpp` to `libid/ui`.
- Preserve the `o` and `O` orbit toggle exactly.
- Leave non-orbit keys pending and report interruption.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `calcfrac.cpp` has no direct key polling calls.
- Orbit-toggle and interruption behavior are covered by focused tests.

## Slice 3: Move Inverse-Julia Keyboard Polling

Work:

- Extract inverse-Julia key wait, drain, read, and requeue from
  `engine/jiim.cpp` into `libid/ui`.
- Keep calculation code receiving decisions, not raw key polling.
- Preserve pushed-back key behavior.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `jiim.cpp` has no direct key polling calls.
- The UI layer owns inverse-Julia blocking reads, drains, and requeue.
- Tests or manual checks cover inverse-Julia keyboard exit.

## Slice 4: Move Perturbation Interrupt Check

Work:

- Replace the reference-pass `driver_key_pressed` check in
  `engine/PertEngine.cpp` with a semantic UI helper.
- Keep the `-1` interruption result unchanged.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `PertEngine.cpp` has no direct key polling calls.
- Existing perturbation behavior is unchanged.

## Slice 5: Move Solid-Guess Interrupt Checks

Work:

- Replace the block-repaint `driver_key_pressed` checks in
  `engine/solid_guess.cpp` with a semantic UI helper.
- Keep return values and work-list resume behavior unchanged.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `solid_guess.cpp` has no direct key polling calls.
- Solid-guess interruption still resumes from the same work point.

## Slice 6: Move SOI Interrupt Checks

Work:

- Replace recursive scan `driver_key_pressed` checks in `engine/soi.cpp`
  with a semantic UI helper.
- Keep recursive unwind and `true` interruption results unchanged.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `soi.cpp` has no direct key polling calls.
- SOI interruption still exits through the same status paths.

## Slice 7: Move Sound Pending-Key Check

Work:

- Replace the pending-key suppression check in `engine/sound.cpp` with a
  semantic UI helper.
- Preserve the rule that pending input suppresses `driver_sound_on`.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `sound.cpp` has no direct key polling calls.
- Sound output is still skipped when input is pending.

## Slice 8: Move Wait-Until Key Wakeup

Work:

- Replace the delay-loop key wakeup in `engine/wait_until.cpp` with a
  semantic UI helper.
- Keep sleep interval and wakeup behavior unchanged.
- Update `test_wait_until` to use a fake `KeyboardInput`.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `wait_until.cpp` has no direct key polling calls.
- Existing wait timing tests pass against the UI keyboard seam.

## Slice 9: Move Lorenz Orbit Interrupt

Work:

- Replace the `plot_orbits2d` interrupt check in
  `fractals/lorenz.cpp` with a semantic UI helper.
- Keep `driver_mute`, resume allocation, and return value unchanged.
- Remove `misc/Driver.h` includes if the later Lorenz slice also no
  longer needs it.

Done when:

- The orbit interrupt location in `lorenz.cpp` has no direct key polling.
- Orbit plotting still allocates the same resume data on interruption.

## Slice 10: Move Lorenz Stereo Save Prompt

Work:

- Extract the photographer-mode save prompt loop in `fractals/lorenz.cpp`
  into `libid/ui`.
- Preserve repeated `s` or `S` save behavior.
- Keep calculation code receiving the decision to continue.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- The stereo save prompt location in `lorenz.cpp` has no direct key
  polling.
- Photographer-mode save and continue behavior is unchanged.

## Slice 11: Move L-System Interrupt Check

Work:

- Replace the recursive draw `driver_key_pressed` check in
  `fractals/lsystem.cpp` with a semantic UI helper.
- Keep counter rollback and null return behavior unchanged.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `lsystem.cpp` has no direct key polling calls.
- L-system interruption still unwinds through the same null return.

## Slice 12: Move Lyapunov Interrupt Check

Work:

- Replace the per-pixel `driver_key_pressed` check in
  `fractals/lyapunov.cpp` with a semantic UI helper.
- Keep the `-1` interruption result unchanged.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `lyapunov.cpp` has no direct key polling calls.
- Lyapunov interruption still reports `-1`.

## Slice 13: Add A Keyboard Boundary Check

Work:

- Add a grep-style check to the developer workflow or tests.
- Fail if `driver_get_key`, `driver_key_pressed`,
  `driver_wait_key_pressed`, or `driver_unget_key` appear under
  `libid/engine` or `libid/fractals`.
- Allow comments only by deleting or rewriting stale comments.

Done when:

- The boundary check is easy to run locally.
- The check fails on new calculation-layer keyboard polling calls.

## Slice 14: Move JIIM Mouse Handling

Work:

- Move `InverseJuliaMouseNotification` from `engine/jiim.cpp` to
  `libid/ui`.
- Move inverse-Julia mouse subscribe and unsubscribe calls to `libid/ui`.
- Move `g_cursor_mouse_tracking` and `g_look_at_mouse` manipulation to
  the UI code that owns the subscription.
- Add a small `MouseInput` abstract interface for inverse-Julia
  mouse/cursor state if needed.
- Expose the active mouse interface as `g_mouse_input`.
- Keep `jiim.cpp` receiving position updates and "mouse changed" decisions.
- Remove `ui/mouse.h` from `engine/jiim.cpp`.

Done when:

- `engine/jiim.cpp` has no direct `MouseNotification`,
  `mouse_subscribe`, `mouse_unsubscribe`, `g_cursor_mouse_tracking`, or
  `g_look_at_mouse` use.
- The only remaining inverse-Julia mouse subscription code is in
  `libid/ui`.
- Tests or manual checks cover inverse-Julia mouse movement and exit.

## Future Event-Driven Work

This plan leaves the following ideas out of scope for the current polling
slices.  They should be kept as future direction after direct polling calls
are moved behind `libid/ui`.

- Replace polling-driven calculation with idle-processing callbacks first.
  Treat idle processing as a bridge to later worker-thread or distributed
  rendering, not as wasted work.
- Plan worker-thread and UI-thread communication as separate follow-up
  work.  Do not mix thread ownership, cancellation, or result delivery into
  this polling-boundary refactor.
- Consider event callbacks that set calculation interrupt state instead of
  polling `KeyboardInput` directly.  Keep that as a later implementation
  choice, after the facade owns the boundary.
- Convert blocking text screens to modal dialogs incrementally.  Start with
  simple numeric input, then form-style prompts, then file-entry screens.
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

- `rg "driver_(get_key|key_pressed|wait_key_pressed|unget_key)" \
  libid/engine libid/fractals` returns no matches.
- `rg "MouseNotification|mouse_subscribe|mouse_unsubscribe" \
  libid/engine libid/fractals` returns no matches.
- `rg "g_look_at_mouse|g_cursor_mouse_tracking" \
  libid/engine libid/fractals` returns no matches.
- `libid/ui` is the only libid layer with direct polling I/O.
- Calculation code reports interruption or decisions without owning input.
- Current behavior is preserved while longer-term architecture stays out of
  scope.
