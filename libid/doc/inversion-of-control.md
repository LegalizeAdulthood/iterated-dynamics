<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Polling I/O Handler Stack Plan

Goal: route the existing polling keyboard I/O through a stack of UI-owned
key handlers, then move direct polling out of calculation code.

This is not yet the full inversion-of-control design.  It is the
intermediate step that preserves today's polling behavior while moving key
meaning into UI contexts.  After polling sites use the handler stack, the
engine can move to a worker thread and the GUI thread can dispatch key
events to the same stack.

This also moves the code toward hexagonal architecture.  The engine becomes
less coupled to input adapters, while `libid/ui` owns keyboard and mouse
interaction at the edge.

The GitHub discussion records the larger inversion-of-control path.  Those
items are kept in `Future Event-Driven Work` so the polling slices stay
mechanical and behavior-preserving.

## Rules

- The driver remains the production key queue while polling I/O exists.
- Only `libid/ui` directly interacts with keyboard and mouse input.
- `libid/ui` owns keyboard dispatch, key meaning, mouse subscriptions, and
  legacy input side effects.
- Calculation code may ask whether work should stop.
- Code outside `libid/ui` must not call `driver_get_key`,
  `driver_key_pressed`, `driver_wait_key_pressed`, or `driver_unget_key`.
- Code outside `libid/ui` must not use `MouseNotification`,
  `mouse_subscribe`, `mouse_unsubscribe`, `g_cursor_mouse_tracking`, or
  `g_look_at_mouse`.
- Keep behavior first.  Rename and reshape after the calls are moved.
- Keep each slice small enough to review by inspection and focused tests.
- End each slice that changes calculation control flow with manual testing
  steps.

## Existing Polling Source

Do not add a second production key queue.  The driver key queue remains the
source of pending keys while the program still polls.

The UI keyboard dispatcher polls the driver queue, reads available keys,
and offers them to the active handler stack.  This keeps the current driver
side effects in one place: window-event pumping, redraw handling, key
buffering, and mouse notification delivery.

Later event-driven work can dispatch GUI key events to the same handler
stack without changing the calculation-side contract.

## Keyboard Handler Stack

The driver owns queue mechanics while polling I/O exists.  Handlers decide
what a key means in a particular UI context.

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

The UI maintains a stack of active `KeyboardHandlerPtr` values.  When a key
is available, the UI dispatcher reads it from the driver queue and offers
it to the handler stack from top to bottom.  The first handler that returns
`true` stops propagation.  If no handler consumes the key, the key is
discarded.

Calculation code must not interpret keys.  It only calls:

```cpp
namespace id::ui
{
bool calc_interrupted();
}
```

`calc_interrupted()` pumps pending driver keys through the handler stack
and returns the current calculation-interrupt flag.  Each handler decides
which keys, if any, request interruption for its context.

`MainLoopKeyboardHandler` is the outer image UI context.  It sits below
more-specific handlers, consumes keys that reach it, records the key as
the next main-loop command, and requests calculation interruption.  This
preserves the current model: a key interrupts rendering, then the main
loop handles that key as a command.

Rules:

- Push and pop handlers with scoped ownership so stale handlers cannot
  remain on the stack.
- Reset calculation-interrupt state at the start of each calculation.
- Do not requeue keys as the normal handoff between contexts.  The
  interested handler should consume the key and record the command or state
  needed by its context.
- Orbit toggling is a handler policy: the orbit handler consumes `o` and
  `O`, toggles orbit display, and does not request interruption.
- The main-loop handler is the fallback render command handler.  It
  consumes keys not handled by more-specific contexts, records the next
  command, and requests interruption.

Caller rules:

- Non-UI code may include `ui/KeyboardHandler.h` only for
  calculation-interrupt checks.
- Non-UI code should not call raw `Driver` keyboard methods after a
  matching reusable handler mechanism exists.
- Direct `Driver` key calls stay in the UI dispatcher and driver classes.
- Direct mouse notification, subscription, and mouse-state manipulation
  stay in `libid/ui`.
- Tests that exercise polling use `MockDriver`; tests that exercise key
  meaning call handlers directly.
- Later event-driven work may replace driver polling with GUI event
  dispatch without touching calculation code.

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

## Slice 1: Add Main-Loop Keyboard Context

Work:

- Add `MainLoopKeyboardHandler` in `libid/ui`.
- Move main-loop command capture from
  `libid/ui/big_while_loop.cpp:753-772`.
- Push it while the image UI main loop is active.
- Make it consume keys that reach the main-loop context.
- Make it record the next command for the main loop.
- Make it request calculation interruption after recording a command.
- Reset calculation-interrupt state at calculation start.
- Keep existing direct polling call sites unchanged.

Done when:

- The main loop can retrieve the next command captured by the handler.
- Captured commands preserve existing resume/menu behavior.
- Calculation code still has no knowledge of which key interrupted work.
- Existing direct polling call sites are unchanged.

## Slice 2: Add Orbit Toggle Handler

Work:

- Add `OrbitToggleKeyboardHandler` in `libid/ui`.
- Move orbit key handling from `libid/ui/check_key.cpp:17-32`.
- Move Mandelbrot orbit handling from
  `libid/engine/calcfrac.cpp:1254-1266`.
- Make it consume `o` and `O`, toggle `g_show_orbit`, and leave
  calculation-interrupt state unchanged.
- Push it above the main-loop handler while rendering contexts support
  orbit toggling.
- Keep non-orbit key behavior owned by the main-loop handler.

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
- Leave all other key meaning to the active main-loop handler.
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

- Move the sound pending-key check behind a UI-owned polling query.
- Preserve the rule that pending input suppresses `driver_sound_on`.
- Keep this separate from `calc_interrupted()` because it is not a
  calculation interrupt.
- Do not add a second key queue; the query still reads the driver queue
  while polling I/O exists.
- Remove `misc/Driver.h` includes made unnecessary by the move.

Done when:

- `sound.cpp` has no direct key polling calls.
- Sound output is still skipped when input is pending.

## Slice 8: Move Wait-Until Key Wakeup

Work:

- Move the delay-loop key wakeup behind a UI-owned polling query.
- Keep sleep interval and wakeup behavior unchanged.
- Keep this separate from `calc_interrupted()` because it is not a
  calculation interrupt.
- Update `test_wait_until` to use `MockDriver` for the polling behavior.
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

- Convert `Frame`, `WinText`, and `Plot` to wxWidgets controls while the
  current polling code still pumps GUI events.
- Add menu-bar support, with blocking text screens still preventing normal
  menu interaction until they are migrated.
- Convert blocking text screens to modal dialogs incrementally.  Start with
  simple numeric input, then form-style prompts, then file-entry screens.
- After text screens are dialogs, remove the menu-bar blocking workaround.
- Replace polling-driven calculation with idle-processing callbacks first.
  Treat idle processing as a bridge to later worker-thread or distributed
  rendering, not as wasted work.
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
