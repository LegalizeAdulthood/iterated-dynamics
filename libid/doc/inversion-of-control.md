<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Engine Polling I/O Inversion Plan

Goal: invert rendering control so input and presentation code decide when
to ask for another bounded unit of work, while engine and fractal code own
calculation state, lifecycle, timing, setup, and cleanup.

This is true inversion of control for the existing polling renderer.  It
is not yet the later worker-thread or event-driven GUI design.  The
release 1.5 target is a clean directory boundary: only `libid/ui`
interacts directly with keyboard and mouse input, and engine code never
selects UI control paths.

Inversion is not achieved by moving a call to a new function, class,
callback table, adapter, or wrapper while the same synchronous control
flow remains hidden inside it.  A slice counts as inversion only when it
changes who owns a decision or return point:

- input polling and key meaning move outward to `libid/ui`;
- waiting and presentation pacing move outward to `libid/ui`;
- calculation work becomes explicit, bounded, and resumable;
- calculation code returns to its caller instead of polling input or
  waiting internally;
- application phase and command effect are represented separately.

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
    call bounded iterate() steps until the calculation object is done

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
one small public surface, such as `resume()`, `suspend()`, `done()`, and
`iterate()`, and delegates to the active variant alternative.

Empty pass placeholders may start in `StandardPass.h`, but they should not
grow there.  As each pass slice is implemented, move that placeholder into
its own pass header and implementation file.  The pass class owns the
state and operation methods.  `StandardPass` includes that header, stores
the concrete class in the variant, and routes calls to it.  Temporary
compatibility functions may remain only as call adapters around new
stateful implementations.  They are not evidence of inversion by
themselves.

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

The UI polling loop owns input policy.  It constructs or resumes
`StandardFractal`, polls input through the active UI context, asks the
object to suspend when work is interrupted, and calls bounded `iterate()`
steps until `done()` is true.

The standard pixel kernels are calculation details.  They should not
double as image-level UI entry points.  If the current names are kept for
compatibility during the refactor, the final shape should still separate:

- image-level UI polling loop
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
  initialized and driven from `StandardFractal`, and is selected
  separately from the active traversal mode.  `passes=p` is compatibility
  syntax that maps traversal to solid guessing for now.  Primary pixels
  are traversed by `StandardPass`, and glitch retry traversal is owned by
  `StandardFractal`.  `PertEngine` supplies perturbation reference and
  point calculations below the pass layer.

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
- A change to a file in `libid/engine` or `libid/fractals` must not add
  an include from `libid/ui`.
- Calculation code reports progress, completion, suspension state, and
  calculation decisions.  It does not interpret keys.
- Keep behavior first.  Pure call relocation is not a completed slice; the
  owning state, decision, or return point must become clearer.
- End each slice that changes calculation control flow with manual testing
  steps only for user interaction paths not covered by automated tests.

## Keyboard Handler Stack

The handler stack is UI input plumbing.  It supports the standard UI
polling loop; it is not the calculation-side architecture.

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

## Application Phase and Command Effects

The next architectural split is separating application phase from command
effect.  `MainState` currently does both jobs.  It was hacked out of the
old if/then/else control flow, so menu handlers now return values that
are partly application phase and partly "what must happen next".

Keep the existing `id_main` application-phase switch at first, but stop
making menu handlers report application phases.  Menu and input handlers
should report command effects.  A single boundary then maps command
effects onto the existing `MainState` transitions and local-loop behavior.

Input is the outer loop after control is inverted.  Keyboard, mouse,
command-file, command-line, and batch input are all command sources.  The
engine should construct into a default, well-defined render model, but no
image can be generated until a display target exists.  Selecting a video
mode supplies the image dimensions; after that, default parameters such
as `type=mandel` and `passes=g` are enough to begin rendering.

The control inputs should be separated from coarsest to finest:

- Application phase: no display target, configured idle, rendering, image
  complete, restoring or loading, and exiting.
- Fractal identity: changing `type=` rebuilds dispatch, resets
  type-specific state, and invalidates the image, but is not a full
  restart.  `<Insert>` is the full reset path and resets the active type
  back to `mandel`.
- Render invalidation: changing passes, bailout, corners, maxiter,
  inside/outside color modes, precision, or similar values requires pixel
  orbits to be recomputed.
- Presentation invalidation: changing the orbit window, palette or
  colormap, tab display, or other display-only state does not require
  pixel orbits to be recomputed.
- Local UI or no-op: menu navigation, help, cancelled dialogs, save,
  shell, and other actions that do not require an application phase
  change.

Current menu handlers communicate these effects through a mixture of
`MainState`, `g_calc_status`, and `MainContext::more_keys`.
`MainState::CONTINUE` is loop mechanics, not a semantic command effect.
`MainState::NOTHING` is overloaded: it can mean ignored, handled locally,
presentation changed, or render parameters changed through side effects.

The first safe implementation split is:

- Add a menu/input command-result enum.
- Convert `MenuHandler` callbacks to return that enum.
- Keep the existing `MainState` switch in `id_main`.
- Add one mapping point near `big_while_loop()` that translates command
  results into existing `MainState` transitions and local-loop behavior.
- Preserve the current `g_calc_status` and `MainContext::more_keys`
  side effects initially.

The audited command-result categories are:

- ignored
- handled locally
- presentation changed
- display target changed
- resumable render state changed
- render parameters changed
- fractal type changed
- interaction mode changed
- restore requested
- full reset requested
- exit requested

## Remaining Inversion-of-Control Slices

These slices use the existing pattern from `libid/ui/ant.cpp`,
`libid/ui/bifurcation.cpp`, `libid/ui/cellular.cpp`, and similar files:

```text
ui polling loop
    construct or resume renderer
    poll keyboard input for the active UI context
    call suspend() or exit when the UI decides to stop
    call bounded iterate() steps until the renderer reports no more work

calculation object
    own calculation state
    expose iterate(), done() when useful, resume(), and suspend()
    do not poll keyboard or mouse input
```

Before changing image calculation ownership again, split menu command
effects from application phase.  This is the next control-flow slice.  It
does not move rendering code.

### Slice 1: Menu Command Result Split

Work:

- Add a menu/input command-result enum for the audited categories above.
- Convert `MenuHandler` callbacks and shared menu helpers to return that
  enum instead of `MainState`.
- Keep the existing `MainState` switch in `id_main`.
- Add one mapping point near `big_while_loop()` that translates command
  results into existing `MainState` transitions and local-loop behavior.
- Preserve current `g_calc_status` and `MainContext::more_keys` side
  effects initially.
- Do not move rendering code in this slice.

Done when:

- Menu handlers no longer return `MainState`.
- `id_main` still owns the existing application-phase switch.
- `big_while_loop()` is the only place that maps menu command effects to
  `MainState`.
- `MainState::CONTINUE` is no longer part of the menu handler contract.

Manual testing:

- `video=F6 type=mandel`

After that, apply the same rule at the image-engine boundary.  Do not
create an `ImageCalculation` facade whose first `iterate()` call still
runs the old whole-image function to completion.  Do not move renderer
classification into `libid/ui`.  Do not add an interface, callback table,
or adapter merely to let the old control flow call through a new name.

The target is one active render operation selected from engine state and
dispatch data.  The operation owns render setup, bounded calculation
work, suspend/resume state, completion state, image timing, and cleanup.
Shared setup and teardown may use an implementation hierarchy, but the
base must own real common behavior rather than hide another synchronous
renderer call.

The input and presentation side starts, resumes, cancels, or paces the
active operation.  It does not classify standard versus Lyapunov versus
non-standard renderers, and it does not own orbit math, pass traversal,
or pixel plotting.

Before more image-boundary code changes, the next plan revision must name
the concrete render operation objects and the common setup/teardown they
own.  A valid implementation slice must make at least one decision or
return point explicit; pure relocation is not enough.

### Slice 2: Render Operation Boundary Audit

Work:

- Audit the current `calc_fract()`, `standard_fractal()`,
  `lyapunov_fractal()`, and `non_standard_fractal()` ownership split.
- Identify the concrete render operation object for each selected
  renderer.
- Identify setup and teardown code that belongs in shared render
  operation behavior.
- Identify setup and teardown code that belongs in a specific render
  operation.
- Revise the next implementation slices from that audit before making
  image-boundary code changes.

Done when:

- The plan names each render operation object.
- The plan states which code moves into shared setup/teardown and which
  code remains renderer-specific.
- No slice asks for a facade over the current whole-image call.

Manual testing:

- None; this is a plan-refinement slice.

### Slice 3: Standard Orbit Plot Pacing

Orbit plotting is engine work done from inside each pixel's orbit
iteration.  The UI layer must not plot or scrub orbit points.  After
inversion, preserve user-visible behavior rather than the exact old
keyboard polling cadence: orbit points should still appear during
calculation, `orbitdelay` should still pace visible points, and a
pending key should still wake that delay promptly.

The engine owns the plot and scrub operations.  The UI pacing loop owns
only the delay between engine-owned plot steps because the delay exists
to make plotted orbit points visible, not to advance fractal math.
The required inversion is to make engine orbit plotting incremental so
it can return to the UI pacing loop between plot steps.

The delay mechanism must follow the timing constraints in
[orbit-delay-timer.md](orbit-delay-timer.md).  Tiny values such as
`orbitdelay=1` need a high-resolution deadline, such as
`QueryPerformanceCounter()` on Win32.  The message or event queue is not
the timer; it is only the responsiveness mechanism that lets a pending
keypress wake the delay.

The current `plot_orbit()` name hides two different behaviors:

- Overlay orbit display draws a temporary orbit over an existing image.
  It saves covered pixels, uses the legacy `color == -1` convention, and
  later scrubs the overlay away.  This is the `showorbit=yes` behavior
  for standard escape-time rendering and FrothyBasin.
- Image-orbit plotting makes the orbit point part of the rendered image.
  It writes an explicit color and must not save or scrub covered pixels.
  Popcorn belongs here.

Popcorn is not a standard fractal renderer.  It uses screen pixels as
seed points, iterates a Popcorn map, and plots the produced orbit points
elsewhere on the screen.  It does not color the seed pixel by orbit
length, and it does not use StandardPass traversal.  Do not route
Popcorn presentation state through `StandardFractal`; make `Popcorn` the
calculation object that owns scan state, orbit state, and pending orbit
points.

The Popcorn conversion should follow review boundaries, not a guessed
slice count.  A slice is small enough when it preserves current behavior,
removes one false dependency, and leaves a reviewable intermediate state.

Work:

- Keep orbit plotting and overlay scrubbing in engine or fractal
  calculation objects.
- Reshape orbit plotting so the engine performs bounded incremental
  plot work and returns to the UI pacing loop.
- Remove the delayed `OrbitPlot::iterate()` compatibility path that
  hides waiting inside engine orbit plotting.
- Make engine calculation code record pending orbit pacing and yield to
  the UI pacing loop after plotting a visible orbit point.
- Let the UI pacing loop perform only message-aware delay pacing before
  resuming engine calculation work.
- Implement the delay through a driver or UI timing primitive that uses a
  high-resolution deadline for tiny delays and pumps the platform message
  or event queue enough to notice keypresses.
- On Win32, use `QueryPerformanceCounter()` for the delay deadline.
  `PeekMessage()` or a message-aware wait may be used to pump messages or
  wake on input, but must not be treated as the delay clock.
- Preserve the distinction between overlay orbit display and image-orbit
  plotting.

Done when:

- `engine/orbit.cpp` has no `wait_until()` call.
- Engine code does not perform delay loops around orbit plotting.
- UI code does not perform orbit plot or overlay scrub operations.
- Standard, FrothyBasin, and Popcorn keep orbit plotting in engine or
  fractal calculation objects and use UI only for pacing delay.
- `orbitdelay=1` uses a high-resolution deadline instead of scheduler
  sleep granularity, and pending key input wakes the delay promptly.

Manual testing:

- `type=mandel showorbit=yes orbitdelay=30`
- `type=frothybasin showorbit=yes orbitdelay=30`
- `type=popcorn orbitdelay=30`

### Slice 4: Sound Output Pacing

Work:

- Treat speaker sound as UI presentation, not engine rendering.
- Make driver speaker calls and sound delay pacing UI-owned instead of
  engine-owned.
- Let calculation and orbit code report tone requests without blocking to
  play them.
- Preserve sound-file output separately from speaker playback.

Done when:

- `write_sound()` no longer waits.
- `engine/sound.cpp` no longer includes `engine/wait_until.h`.
- Rendering an image does not need to wait in order to produce sound.

Manual testing:

- `@music.par/Music_Fractal-1`

### Slice 5: Wait-Until Cleanup

Work:

- Recheck all remaining `wait_until()`, `reset_wait_until()`, and
  `sleep_orbit_delay()` callers after the pacing slices.
- Remove the `showdot` compatibility fallback in
  `engine/calcfrac.cpp` after the standard pass types that cannot yet
  yield at a pixel are converted to incremental UI-owned delay pacing.
- Remove `wait_until()` if no non-UI caller remains.
- Move any remaining UI timing helper to `libid/ui`.

Done when:

- Code in `libid/engine` and `libid/fractals` does not depend on
  `engine/wait_until.h`.
- No rendering path reaches keyboard polling through a wait helper.

Manual testing:

- None unless a remaining interactive caller is changed.

The remaining interrupt slices are grouped by UI polling loop.  The
audited non-wait call sites all belong to standard rendering:
`SolidGuess`, `SOI`, one/two pass standard pixels, and sticky-orbit
traversal are driven from `libid/ui/standard_fractal.cpp`.

### Slice 6: StandardFractal Pixel Interrupts

Work:

- Make `libid/ui/standard_fractal.cpp` the only code that checks for
  interruption during standard pixel rendering.
- Reshape `StandardFractal::calculate_standard_pixel()`,
  `calc_mandelbrot_type()`, and the one/two pass loops so they return
  after bounded pixel work instead of checking `calc_interrupted()`.
- Move resume and work-list decisions that currently branch on
  `calc_interrupted()` into pass suspension state triggered by the UI
  polling loop.
- Keep orbit math and color selection unchanged.

Done when:

- Standard pixel code in `engine/calcfrac.cpp` does not call
  `calc_interrupted()`.
- `engine/one_or_two_pass.cpp` does not call `calc_interrupted()`.
- `ui/standard_fractal.cpp` polls between `StandardFractal` increments
  and calls `suspend()` when interruption is requested.

Manual testing:

- `type=mandel passes=1 maxiter=10000`
- `type=mandel passes=2 maxiter=10000`

### Slice 7: StandardFractal Solid Guess Interrupts

Work:

- Keep `libid/ui/standard_fractal.cpp` as the only owner of
  interruption for solid guessing.
- Convert `SolidGuess` row fill and symmetry repaint work into
  incremental state owned by `SolidGuess`.
- Return to `StandardFractal` after bounded repaint work instead of
  polling the driver inside `SolidGuess::guess_row()`.
- Preserve current solid-guess resume behavior through pass suspension
  state.

Done when:

- `engine/solid_guess.cpp` has no driver key polling.
- `SolidGuess` can yield during row fill and symmetry repaint without
  losing block state.
- The standard UI polling loop handles the interrupt decision.

Manual testing:

- `type=mandel passes=g maxiter=10000`

### Slice 8: StandardFractal SOI Interrupts

Work:

- Keep `libid/ui/standard_fractal.cpp` as the only owner of
  interruption for SOI.
- Convert the `SOI::rhombus_aux()` polling points into incremental SOI
  work checkpoints.
- Use SOI-owned state, including an explicit stack if needed, so
  recursive or scan state survives a UI-triggered suspension.
- Remove direct driver polling from SOI.

Done when:

- `engine/soi.cpp` has no driver key polling.
- SOI can yield from recursion and scan work without losing position.
- The standard UI polling loop handles the interrupt decision.

Manual testing:

- `type=mandel passes=s maxiter=10000`

### Slice 9: StandardFractal Sticky-Orbits Interrupts

Work:

- Keep `libid/ui/standard_fractal.cpp` as the only owner of
  interruption for `passes=o` sticky-orbit rendering.
- Make `StickyOrbits` own rectangle, line, and function traversal state
  and return after bounded orbit-point work.
- Change `plot_orbits2d()` so it plots the current point only; it should
  not poll the keyboard, mute the driver, allocate resume state, or
  return an interrupt result.
- Let `StickyOrbits` and `StandardFractal` record resume state when the
  UI polling loop suspends rendering.

Done when:

- `fractals/lorenz.cpp::plot_orbits2d()` has no driver key polling.
- `plot_orbits2d()` no longer owns interruption or resume setup.
- Sticky-orbit traversal is incremental and interruptible from the
  standard UI polling loop.

Manual testing:

- `type=lorenz passes=o maxiter=10000`

## Current Direct Input Leaks

Direct polling or key consumption exists outside `libid/ui` in:

- `libid/engine/calcfrac.cpp`: top-level renderer selection calls UI
  control paths, non-standard polling path, and direct standard-pixel
  fallback polling.
- `libid/engine/solid_guess.cpp`: interrupt checks during block repaint.
- `libid/engine/soi.cpp`: interrupt checks during recursive scan.
- `libid/engine/orbit.cpp`: shared `plot_orbit()` path owns screen orbit
  display and reaches input polling through `wait_until()`.
- `libid/engine/sound.cpp`: suppress tone while keys are pending.
- `libid/engine/wait_until.cpp`: delay loop wakes on pending key.
- `libid/fractals/lorenz.cpp`: orbit interrupt.

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

- Top-level image rendering is requested through `libid/ui`, not through
  an engine function that selects UI control paths.
- Standard escape-time rendering is driven by a UI polling loop around
  `StandardFractal`.
- `StandardFractal` owns standard renderer iteration, mode dispatch, and
  resume state.
- `libid/engine` and `libid/fractals` do not include headers from
  `libid/ui`.
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
