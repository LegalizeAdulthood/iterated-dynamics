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

- Add `DriverKeyboardInput : public KeyboardInput` in
  `ui/KeyboardInput.cpp`.
- `DriverKeyboardInput` forwards to `driver_key_pressed`,
  `driver_get_key`, `driver_wait_key_pressed`, and `driver_unget_key`.
- The production forwarding path must preserve current driver side
  effects: window-event pumping, redraw handling, key buffering, and mouse
  notification delivery.
- Keep the default instance in `ui/KeyboardInput.cpp`; do not add a
  second key buffer.

Access:

```cpp
namespace id::ui
{
extern KeyboardInput *g_kb_input;
}
```

Production points `g_kb_input` at `DriverKeyboardInput`.  Tests may
replace it with a fake `KeyboardInput`.

The initial slice only introduces this generic input facade.  It must not
move existing polling call sites or introduce per-site semantic helpers.
Later slices should first identify a reusable polling shape, then move a
set of matching call sites to that mechanism.

## Keyboard Handler Stack

`KeyboardInput` owns queue mechanics.  It does not decide what a key means
in a particular UI context.

Add a separate handler interface for key meaning:

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
yield set calculation-interrupt state owned by the UI.

The UI maintains a stack of active `KeyboardHandlerPtr` values.  When a
key is available, the UI reads it from `KeyboardInput` and offers it to
the handler stack from top to bottom.  The first handler that returns
`true` stops propagation.  If no handler consumes the key, the key is
discarded.

Calculation code must not interpret keys.  It only calls:

```cpp
namespace id::ui
{
bool calc_interrupted();
}
```

`calc_interrupted()` pumps pending keys through the handler stack and
returns the current calculation-interrupt flag.  Each handler decides
which keys, if any, request interruption for its context.

Rules:

- Push and pop handlers with scoped ownership so stale handlers cannot
  remain on the stack.
- Reset calculation-interrupt state at the start of each calculation.
- Do not use `push_key()` as the normal handoff between contexts.  The
  interested handler should consume the key and record the command or
  state needed by its context.
- Orbit toggling is a handler policy: the orbit handler consumes `o` and
  `O`, toggles orbit display, and does not request interruption.
- The image-render handler consumes keys that should return control to
  the outer UI, records the command for that UI, and requests
  interruption.

Caller rules:

- `libid/engine` and `libid/fractals` may include
  `ui/KeyboardInput.h`.
- `libid/engine` and `libid/fractals` should not call raw `Driver`
  keyboard methods after a matching reusable mechanism exists.
- Direct `Driver` key calls stay in `DriverKeyboardInput` and driver
  classes.
- New abstract interfaces are exposed through global pointers, like
  `g_driver`.  Use `g_kb_input` for `KeyboardInput` and
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

## Slice 1: Add Image-Render Keyboard Context

Work:

- Add `ImageRenderKeyboardHandler` in `libid/ui`.
- Move command capture from `libid/ui/big_while_loop.cpp:753-772`.
- Push it while an image calculation is active.
- Make it consume keys that should return control to the outer UI.
- Make it record the consumed command for the outer UI instead of using
  `driver_unget_key`.
- Make it request calculation interruption after recording such a command.
- Reset calculation-interrupt state at calculation start.

Done when:

- The outer UI can retrieve a command captured during rendering.
- Captured commands preserve existing resume/menu behavior.
- Calculation code still has no knowledge of which key interrupted work.

## Slice 2: Add Orbit Toggle Handler

Work:

- Add `OrbitToggleKeyboardHandler` in `libid/ui`.
- Move orbit key handling from `libid/ui/check_key.cpp:17-32`.
- Move Mandelbrot orbit handling from
  `libid/engine/calcfrac.cpp:1254-1266`.
- Make it consume `o` and `O`, toggle `g_show_orbit`, and leave
  calculation-interrupt state unchanged.
- Push it above the image-render handler while rendering contexts support
  orbit toggling.
- Keep non-orbit key behavior owned by the image-render handler.

Done when:

- `o` and `O` toggle orbit display without interrupting calculation.
- Other keys still return control to the outer UI.
- Focused tests cover handler ordering for orbit keys and other keys.

## Slice 3: Route Existing Check-Key Path Through Handlers

Work:

- Change `ui/check_key` to call `calc_interrupted()`.
- Preserve current callers and return values.
- Remove direct orbit-toggle behavior from `check_key`; it belongs to the
  orbit handler.

Done when:

- Existing `check_key()` callers still receive a boolean interruption
  result.
- Orbit toggling is handled only by the orbit handler.
- The existing `check_key()` render paths preserve resume behavior.

## Slice 4: Move Mandelbrot Hot-Path Polling

Work:

- Replace the Mandelbrot hot-path direct key check with
  `calc_interrupted()`.
- Preserve the special `o` and `O` behavior through the orbit handler.
- Leave all other key meaning to the active image-render handler.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `calcfrac.cpp` has no direct Mandelbrot hot-path key polling.
- The hot path does not interpret key values.
- Orbit-toggle and interruption behavior are covered by focused tests.

## Slice 5: Move Pure Render Interrupt Probes

Work:

- Replace direct pending-key interrupt probes with `calc_interrupted()`.
- Cover `engine/PertEngine.cpp`, `engine/solid_guess.cpp`,
  `engine/soi.cpp`, `fractals/lsystem.cpp`, and
  `fractals/lyapunov.cpp`.
- Preserve each local return value and resume behavior.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- Those files have no direct key polling calls.
- Calculation code only asks whether calculation was interrupted.
- Existing interruption return paths are unchanged.

## Slice 6: Move Lorenz Orbit Interrupt

Work:

- Replace the `plot_orbits2d` interrupt check in
  `fractals/lorenz.cpp` with `calc_interrupted()`.
- Keep `driver_mute`, resume allocation, and return value unchanged.
- Remove `misc/Driver.h` includes if the later Lorenz slice also no
  longer needs it.

Done when:

- The orbit interrupt location in `lorenz.cpp` has no direct key polling.
- Orbit plotting still allocates the same resume data on interruption.

## Slice 7: Move Sound Pending-Key Check

Work:

- Move the sound pending-key check behind generic `KeyboardInput`
  polling.
- Preserve the rule that pending input suppresses `driver_sound_on`.
- Keep this separate from `calc_interrupted()` because it is not a
  calculation interrupt.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `sound.cpp` has no direct key polling calls.
- Sound output is still skipped when input is pending.

## Slice 8: Move Wait-Until Key Wakeup

Work:

- Move the delay-loop key wakeup behind generic `KeyboardInput` polling.
- Keep sleep interval and wakeup behavior unchanged.
- Keep this separate from `calc_interrupted()` because it is not a
  calculation interrupt.
- Update `test_wait_until` to use a fake `KeyboardInput`.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `wait_until.cpp` has no direct key polling calls.
- Existing wait timing tests pass against the UI keyboard seam.

## Slice 9: Move Inverse-Julia Keyboard Context

Work:

- Add `InverseJuliaKeyboardHandler` for the modal JIIM context.
- Move key interpretation from `libid/engine/jiim.cpp:655-869`.
- Move key drain polling from `libid/engine/jiim.cpp:1136-1147`.
- Move final key requeue from `libid/engine/jiim.cpp:1335-1336`.
- Push it while inverse-Julia is active and pop it on exit.
- Move key meaning out of `engine/jiim.cpp` into that handler.
- Keep cursor movement, save, exit, and requeue behavior unchanged.
- Avoid adding per-call-site keyboard helper functions.

Done when:

- `jiim.cpp` has no direct key polling calls.
- The active JIIM handler owns inverse-Julia key meaning.
- Tests or manual checks cover inverse-Julia keyboard exit.

## Slice 10: Move Lorenz Stereo Save Prompt

Work:

- Add `LorenzStereoSaveKeyboardHandler` for the photographer-mode save
  loop.
- Move the save prompt loop from
  `libid/fractals/lorenz.cpp:1553-1561`.
- Preserve repeated `s` or `S` save behavior.
- Keep calculation code receiving the decision to continue.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- The stereo save prompt location in `lorenz.cpp` has no direct key
  polling.
- Photographer-mode save and continue behavior is unchanged.

## Slice 11: Move JIIM Mouse Handling

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
