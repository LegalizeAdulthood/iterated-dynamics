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

A practical implementation is the current non-virtual `StandardPass`
router.  It holds a `std::variant` of concrete pass classes.
`StandardPass` exposes one small public surface, such as `reset()`,
`select()`, `iterate()`, `status()`, and `suspend()`, and delegates to
the active variant alternative.

Each concrete pass owns its state and operation methods in its own class:
`BoundaryTrace`, `Diffusion`, `OnePass`, `TwoPass`, `SolidGuess`,
`SynchronousOrbit`, `Tesseral`, and `StickyOrbits`.  `OnePass` and
`TwoPass` share `ScanlinePass`.  `StandardPass` includes the matching
pass headers, stores the concrete class in the variant, and routes calls
to it.  Old free-function pass traversal entry points have been removed;
do not reintroduce them as call adapters.

`StandardFractal` also owns the standard-mode dispatch currently selected
by `g_std_calc_mode`, including `SynchronousOrbit`, `Tesseral`,
`BoundaryTrace`, `SolidGuess`, `Diffusion`, `StickyOrbits`, and the
`OnePass` and `TwoPass` paths.

Perturbation is an alternate way to compute an escape-time orbit for a
pixel.  It belongs below `StandardPass` as an orbit strategy selected by
`StandardFractal`, separately from traversal.  `passes=p` remains
compatibility syntax that selects perturbation orbit calculation while
using the default standard traversal.

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
- Tesseral is a concrete `StandardPass` alternative.  `Tesseral` has
  `iterate()`, `done()`, and `suspend()`, and owns its box stack.
  Initial edge setup, later edge probes, and box filling run through
  bounded, resumable state.  Tesseral suspend records the active box and
  restarts initial edge setup if interruption happens during that phase.
- The `OnePass` and `TwoPass` paths share `ScanlinePass`; they preserve
  scan position in the concrete pass object and yield standard pixel work
  back to the UI.  They still communicate active pixel position through
  `g_row` and `g_col`.
- `BoundaryTrace` is a concrete `StandardPass` alternative.  It owns the
  outer scan cursor, outline tracing state, region filling state, and
  suspend path.
- `Diffusion` is a concrete `StandardPass` alternative.  It preserves the
  diffusion counter, but tile-local traversal state still lives on the
  stack inside `Diffusion::engine()`.
- `SolidGuess` is a concrete `StandardPass` alternative.  First-pass row
  traversal, remaining pass traversal, `guess_row()` block state, and
  fast row repaint state still live on the stack.  It still probes
  keyboard input while filling guessed rows.
- `SynchronousOrbit` owns an explicit rhombus frame stack and explicit
  leaf scan state.  It suspends through `StandardPass`.
- `StickyOrbits` is a concrete `StandardPass` alternative.  It owns
  rectangle, line, and function seed traversal state, current orbit
  progress, and current orbit color.  It suspends through
  `StandardPass`.
- Perturbation has a `PertEngine` owned by `StandardFractal`, is
  initialized and driven from `StandardFractal`, and is selected
  separately from the active traversal mode.  `passes=p` is compatibility
  syntax that selects perturbation without making perturbation a
  `StandardPass` traversal.  Primary pixels are traversed by
  `StandardPass`, and glitch retry traversal is owned by `StandardFractal`.
  `PertEngine` supplies perturbation reference and point calculations
  below the pass layer.

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

The menu command-result split keeps this boundary:

- `MenuHandler` callbacks return `CommandResult`.
- Keep the existing `MainState` switch in `id_main`.
- One mapping point near `big_while_loop()` translates command results
  into existing `MainState` transitions and local-loop behavior.
- The current `g_calc_status` and `MainContext::more_keys`
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

Apply the same rule at the image-engine boundary.  Do not create an
`ImageCalculation` facade whose first `iterate()` call still runs the old
whole-image function to completion.  Do not move renderer classification
into `libid/ui`.  Do not add an interface, callback table, or adapter
merely to let the old control flow call through a new name.

The target is one active render operation selected from engine state and
dispatch data.  The operation owns render setup, bounded calculation
work, suspend/resume state, completion state, image timing, and cleanup.
Shared setup and teardown may use an implementation hierarchy, but the
base must own real common behavior rather than hide another synchronous
renderer call.

The current common lifecycle behavior is `ImageCalculation::start()` and
`ImageCalculation::finish()` in `engine/calcfrac.cpp`.  That behavior is
real shared render setup and teardown, not a renderer facade.  It owns
true-color setup and teardown, parser setup, wait reset, log-map setup
and cleanup, orbit display defaults, inversion setup, resume cleanup,
calculation-time accounting, work-area cleanup, sound close, and disk
image cleanup.

The concrete render operation objects are:

- `engine::StandardFractal`: owns standard escape-time traversal,
  `StandardPass`, standard pixel state, perturbation, pass dispatch,
  standard orbit-plot requests, show-dot pacing requests, and standard
  suspend/resume state.
- `fractals::Lyapunov`: owns Lyapunov render state by containing an
  `engine::StandardFractal`; Lyapunov-specific per-image setup remains in
  `lyapunov_per_image()` below the dispatch layer.
- `engine::NonStandardFractal`: owns non-standard dispatch setup,
  symmetry defaults, plot defaults, `per_image()` invocation, calculation
  timing around the selected `CalcType`, and final status interpretation.
  It is still synchronous and must grow a real operation shape before it
  can participate in outer-loop control.

The input and presentation side starts, resumes, cancels, or paces the
active operation.  It does not classify standard versus Lyapunov versus
non-standard renderers, and it does not own orbit math, pass traversal,
or pixel plotting.

The current wrong boundary is `engine::calc_fract()` calling UI entry
points: `standard_fractal()`, `lyapunov_fractal()`, and
`non_standard_fractal()`.  Fixing that requires render operations that
return control after bounded work.  It is not fixed by moving those calls
behind another engine wrapper.

The near-term implementation slices continue closing specific
unbounded/waiting paths before top-level image rendering is inverted.

### Slice 1: Orbit Delay Timer Interface

The orbit-pacing inversion depends on a usable high-resolution delay
primitive.  Define the portable UI-owned surface first, without changing
ownership of orbit plotting or overlay scrubbing.

Work:

- Add a UI-owned orbit delay timer interface based on
  [orbit-delay-timer.md](orbit-delay-timer.md).
- Accept delay values in `orbitdelay` units: 100 us per unit.
- Keep input policy outside the timer.  The timer may receive a small
  callback that lets the current UI context pump or test input, but it
  must not decide what a key means.
- Use a hybrid policy: avoid scheduler oversleep near tiny deadlines,
  but avoid burning CPU for long delays.
- Define the common reset and wait API needed by the current standard
  orbit display path.
- Keep platform-specific clock and sleep details behind the timer API.
- Do not move orbit plotting, overlay scrubbing, sound output, or engine
  wait helpers in this slice.

Done when:

- The timer API exists in `libid/ui` or a lower platform-neutral layer
  callable from `libid/ui`.
- The API accepts a UI-owned input callback without depending on driver
  keyboard polling directly.
- Platform-specific timing code is isolated behind the API.

Manual testing:

- None.

### Slice 2: Win32 Orbit Delay Timer

Work:

- Implement the Win32 timer backend for the orbit delay timer API.
- Add this backend source to the CMake platform source selection for
  Win32.
- Do not use preprocessor `#ifdef` branches in timer code to select this
  backend.
- Use `QueryPerformanceCounter()` for deadline measurement instead of
  scheduler sleep granularity.
- Allow message pumping or message-aware waits to wake on input, but do
  not use the message queue as the delay clock.
- Preserve the shared hybrid sleep/spin policy.
- Build this backend only when CMake configures a Win32 target.

Done when:

- Win32 `orbitdelay=1` is measured against a
  `QueryPerformanceCounter()` deadline.
- CMake selects this backend at configure time for Win32 targets.
- Pending input can still wake the delay promptly.
- The backend does not interpret keys.

Manual testing:

- `type=mandel showorbit=yes orbitdelay=1`
- `type=mandel showorbit=yes orbitdelay=30`

### Slice 3: Linux Orbit Delay Timer

Work:

- Implement or validate the Linux timer backend for the orbit delay timer
  API.
- Add this backend source to the CMake platform source selection for
  Linux.
- Do not use preprocessor `#ifdef` branches in timer code to select this
  backend.
- Use a monotonic high-resolution deadline source such as
  `clock_gettime(CLOCK_MONOTONIC, ...)`.
- Use Linux sleep or yield mechanisms only while far enough from the
  deadline that scheduler latency is acceptable.
- Keep input pumping in the UI callback supplied to the timer.
- Build this backend only when CMake configures a Linux target.

Done when:

- Linux builds use a monotonic deadline clock for orbit delay pacing.
- CMake selects this backend at configure time for Linux targets.
- The implementation does not depend on Win32 APIs or assumptions.
- Pending input can still wake the delay promptly.

Manual testing:

- `type=mandel showorbit=yes orbitdelay=1`
- `type=mandel showorbit=yes orbitdelay=30`

### Slice 4: macOS Orbit Delay Timer

Work:

- Implement or validate the macOS timer backend for the orbit delay timer
  API.
- Add this backend source to the CMake platform source selection for
  macOS.
- Do not use preprocessor `#ifdef` branches in timer code to select this
  backend.
- Use a monotonic high-resolution deadline source, such as
  `std::chrono::steady_clock` when verified suitable, or Mach absolute
  time if explicit platform control is needed.
- Avoid assuming sub-millisecond scheduler sleeps are precise.
- Keep input pumping in the UI callback supplied to the timer.
- Build this backend only when CMake configures a macOS target.

Done when:

- macOS builds use a monotonic deadline clock for orbit delay pacing.
- CMake selects this backend at configure time for macOS targets.
- The implementation does not depend on Win32 or Linux-only APIs.
- Pending input can still wake the delay promptly.

Manual testing:

- `type=mandel showorbit=yes orbitdelay=1`
- `type=mandel showorbit=yes orbitdelay=30`

### Slice 5: BSD Orbit Delay Timer

Work:

- Implement or validate the BSD timer backend for the orbit delay timer
  API.
- Add this backend source to the CMake platform source selection for BSD
  targets.
- Do not use preprocessor `#ifdef` branches in timer code to select this
  backend.
- Use portable BSD timing first, such as
  `clock_gettime(CLOCK_MONOTONIC, ...)`.
- Avoid relying on BSD-specific fast clock variants unless guarded and
  optional.
- Verify the implementation across FreeBSD, OpenBSD, and NetBSD CI.
- Keep input pumping in the UI callback supplied to the timer.
- Build this backend only when CMake configures a BSD target.

Done when:

- FreeBSD, OpenBSD, and NetBSD builds use a monotonic deadline clock for
  orbit delay pacing.
- CMake selects this backend at configure time for BSD targets.
- The implementation does not depend on Win32, Linux-only, or
  single-BSD APIs.
- Pending input can still wake the delay promptly.

Manual testing:

- `type=mandel showorbit=yes orbitdelay=1`
- `type=mandel showorbit=yes orbitdelay=30`

### Slice 6: Wire Orbit Delay Timer

Work:

- Replace the current sleep-based delay in
  `libid/ui/standard_orbit_plot.cpp` with the portable orbit delay timer
  API.
- Keep the existing orbit plotting and overlay scrubbing ownership.
- Pass the current UI keyboard pumping callback into the timer.
- Do not move sound output or engine wait helpers in this slice.

Done when:

- The standard UI orbit display path uses the timer primitive.
- `engine/wait_until.cpp` remains unchanged for later cleanup.
- Standard orbit delay behavior is otherwise unchanged.

Manual testing:

- `type=mandel showorbit=yes orbitdelay=1`
- `type=mandel showorbit=yes orbitdelay=30`

### Slice 7: Standard Orbit Plot Pacing

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

The delay mechanism comes from the portable orbit delay timer primitive.
This slice changes orbit plotting ownership; it must not also invent
timing policy.

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
- Use the existing orbit delay timer primitive for pacing.
- Preserve the distinction between overlay orbit display and image-orbit
  plotting.

Done when:

- `engine/orbit.cpp` has no `wait_until()` call.
- Engine code does not perform delay loops around orbit plotting.
- UI code does not perform orbit plot or overlay scrub operations.
- Standard, FrothyBasin, and Popcorn keep orbit plotting in engine or
  fractal calculation objects and use UI only for pacing delay.
- Standard orbit plotting uses the orbit delay timer primitive, and
  pending key input wakes the delay promptly.

Manual testing:

- `type=mandel showorbit=yes orbitdelay=30`
- `type=frothybasin showorbit=yes orbitdelay=30`
- `type=popcorn orbitdelay=30`

### Slice 8: Sound Output Pacing

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

### Slice 9: Wait-Until Cleanup

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

The remaining direct interrupt checks are grouped by UI polling loop.
After the pass-state slices, the audited non-wait call sites that should
remain are the standard pixel and scanline checks driven from
`libid/ui/standard_fractal.cpp`.

### Slice 10: StandardFractal Pixel Interrupts

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
- `engine/ScanlinePass.cpp` does not call `calc_interrupted()`.
- `ui/standard_fractal.cpp` polls between `StandardFractal` increments
  and calls `suspend()` when interruption is requested.

Manual testing:

- `type=mandel passes=1 maxiter=10000`
- `type=mandel passes=2 maxiter=10000`

### Slice 11: Perturbation Traversal Ownership

Work:

- Remove pass-era assumptions from `PertEngine`: perturbation must not
  own the order in which primary pixels are evaluated.
- Treat perturbation as an orbit calculation strategy used by the active
  `StandardPass` traversal, not as a traversal or pass algorithm.
- Replace the current deferred-pixel model with an interface that always
  computes and returns the color for the requested pixel.
- Make `PertEngine` maintain a list of reference points instead of a
  single active reference point for the whole image.
- When a pixel has too much error for the current reference point, select
  a closer existing reference point and retry that same pixel.
- If the current reference point is already the closest reference, create
  a new reference point at the current pixel, add it to the list, and
  retry that same pixel.
- Use that current-pixel reference insertion as the first implementation;
  leave more elaborate reference-selection heuristics for later
  experiments.
- Make BoundaryTrace compatible with perturbation by ensuring every
  evaluated pixel gets a stable memoized color before BoundaryTrace uses
  the image as its orbit-result cache.
- Remove the temporary BoundaryTrace perturbation exclusion once
  perturbation always computes the requested pixel immediately.

Done when:

- `passes=p` still selects perturbation orbit calculation as compatibility
  syntax without making perturbation a `StandardPass` traversal.
- `PertEngine` does not decide primary pixel order.
- `PertEngine::calculate_pixel()` returns a stable color for the requested
  pixel, selecting or adding reference points internally as needed.
- Glitch handling does not leave BoundaryTrace-visible pixels uncolored
  or indistinguishable from uncomputed background pixels.
- BoundaryTrace can use perturbation without crashing or recomputing
  already-computed pixel orbits.

Manual testing:

- Deep zoom with `type=mandel passes=b`
- `type=mandel passes=p`

### Slice 12: Perturbation Pass-Switch Tests

Work:

- Add tests after perturbation no longer owns traversal order.
- Cover switching standard pass algorithms while arbitrary precision
  selects perturbation automatically.
- Include the case that currently fails manually: start from a
  BoundaryTrace render where perturbation is temporarily disabled, switch
  to solid guessing, and verify perturbation is used correctly for the
  new traversal.
- Ensure pass switching resets or preserves only the perturbation state
  that is valid for the new traversal.
- Include a `perturbation=no` comparison case to prove failures are in
  perturbation/pass interaction, not in arbitrary precision rendering.
- Prefer image or autokey coverage over source-grep tests.

Done when:

- Switching from `passes=b` to `passes=g` under arbitrary precision
  renders a correct image with perturbation enabled.
- The same scenario with `perturbation=no` still renders correctly.
- No stale reference, retry, or traversal state from the previous pass
  affects the next pass algorithm.

Manual testing:

- Deep zoom with `type=mandel passes=b`, then switch to `passes=g`
- Deep zoom with `type=mandel passes=b perturbation=no`, then switch to
  `passes=g`

## Current Direct Input Leaks

Direct polling or key-sensitive waiting exists outside `libid/ui` in:

- `libid/engine/calcfrac.cpp`: standard pixel calculation still calls
  `calc_interrupted()`, and show-dot fallback pacing still uses
  engine-side delay helpers.
- `libid/engine/ScanlinePass.cpp`: one-pass and two-pass traversal still
  calls `calc_interrupted()` while deciding whether to suspend work.
- `libid/engine/orbit.cpp`: shared orbit plotting still reaches input
  polling indirectly through `wait_until()`.
- `libid/engine/sound.cpp`: speaker output still waits through
  `wait_until()` and suppresses speaker tones through an input-pending
  callback.
- `libid/engine/wait_until.cpp`: delay loop wakes on pending key.
- `libid/engine/cmdfiles.cpp`: comments document indirect startup and
  error-message paths that may call UI key-waiting code through
  `init_msg()`.

The remaining slices cover the first five items.  The `cmdfiles.cpp`
case is not a direct polling call, but it marks a remaining mixed
engine/UI boundary to keep visible.

## Keyboard Polling Still Outside `id_main`

The current release 1.5 boundary is narrower than the final
event-driven design.  For release 1.5, the key rule is that only
`libid/ui` talks directly to keyboard and mouse I/O.  The stricter
future target is that `id_main` or the top application loop is the only
code that polls the driver key queue.

To reach the stricter target, split keyboard dispatch from keyboard
polling:

- `KeyboardHandler.cpp` should hold the handler stack and interruption
  state, and dispatch keys that are passed to it.
- `KeyboardHandler.cpp` should not call `driver_key_pressed()` or
  `driver_get_key()`.
- The top application loop should read driver keys and feed them to the
  handler stack.
- Render controllers should become resumable phases that do bounded work
  when the top loop asks them to advance.

Simple render controllers that already wrap calculation objects but
still poll the driver queue directly:

- `libid/ui/bifurcation.cpp`
- `libid/ui/cellular.cpp`
- `libid/ui/dynamic2d.cpp`
- `libid/ui/ifs2d.cpp`
- `libid/ui/ifs3d.cpp`
- `libid/ui/lsystem.cpp`
- `libid/ui/orbit2d.cpp`
- `libid/ui/orbit3d.cpp`
- `libid/ui/plasma.cpp`
- `libid/ui/standard_4d.cpp`
- `libid/ui/testpt.cpp`

Controllers with bespoke interaction or partial handler-stack use that
still read or write the driver queue directly:

- `libid/ui/ant.cpp`
- `libid/ui/inverse_julia.cpp`
- `libid/ui/non_standard_fractal.cpp`
- `libid/ui/stereo.cpp`

Handler-based render controllers that still rely on
`KeyboardHandler.cpp` polling internally:

- `libid/ui/diffusion.cpp`
- `libid/ui/frothy_basin.cpp`
- `libid/ui/lyapunov.cpp`
- `libid/ui/popcorn.cpp`
- `libid/ui/standard_fractal.cpp`
- `libid/ui/standard_orbit_plot.cpp`

Blocking text screens, modal prompts, and utility UI paths also poll
directly.  They can remain for the release 1.5 directory boundary, but
must become application phases or dialogs before `id_main` can be the
only keyboard poller:

- `libid/ui/big_while_loop.cpp`
- `libid/ui/do_pause.cpp`
- `libid/ui/editpal.cpp`
- `libid/ui/file_get_window.cpp`
- `libid/ui/full_screen_choice.cpp`
- `libid/ui/full_screen_prompt.cpp`
- `libid/ui/get_key_no_help.cpp`
- `libid/ui/help.cpp`
- `libid/ui/intro.cpp`
- `libid/ui/main_menu.cpp`
- `libid/ui/main_menu_switch.cpp`
- `libid/ui/rotate.cpp`
- `libid/ui/stop_msg.cpp`
- `libid/ui/temp_msg.cpp`
- `libid/ui/zoom.cpp`

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
