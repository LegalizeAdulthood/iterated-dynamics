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

These files show the calculation-object and UI-controller structure.
The audit below records the remaining reverse calls inside that structure:

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

Cellular still calls the polling `thinking()` UI helper during initial
generation.  L-system drawing is incremental, but sizing still runs
recursively inside `start()` and polls through `thinking()`.

JIIM still owns the outer interaction loop, cursor, and UI keyboard and
mouse scopes in `libid/engine/jiim.cpp`.  Extracting its input helpers into
`libid/ui/inverse_julia.cpp` has not inverted that control flow.

## Target Shape

The standard escape-time calculation object already has this lifecycle:

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
  suspend path.  Invalid options still invoke a modal `stop_msg()` from
  pass initialization.
- `Diffusion` owns its counter, tile traversal, and fill state.  Its
  explicit phases advance in bounded counter chunks and suspend through
  `StandardPass`.
- `SolidGuess` owns first-pass row traversal, remaining pass traversal,
  `guess_row()` block state, and row repaint state.  Its explicit phases
  yield at row boundaries without polling keyboard input.  Preserve the
  restored interaction cadence when changing these paths.
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
- `CommandResult` already separates menu command effects from the
  `MainState` application-phase switch.  Some effects still travel through
  `g_calc_status` and `MainContext::more_keys` side effects.
- `engine_timer` and benchmarking machinery have been removed.  Explicit
  engine timing still owns total image calculation time, including
  suspension and completion accounting through `g_calc_time`.
- Show-dot delay is UI-owned.  Marker save and restore state and a
  temporary display bridge still live in the standard engine.  The
  right-edge hang restriction and saved-state preservation across pixel
  yields are compatibility requirements for later cleanup.

The fresh reverse-call audit also found these live paths:

- `LSystem::start()` calls recursive sizing, which calls `thinking()`;
  `Cellular::iterate()` also calls `thinking()`.  That UI helper polls the
  driver queue.  Both types also end the thinking display from calculation
  code.
- `InverseJulia::iterate()` calls `process_inverse_julia_keyboard()`;
  `InverseJulia::process()` owns the handler scope, mouse scope, and loop.
- `calc_fract()` calls UI render controllers directly.
  `NonStandardFractal::iterate()`, `standalone_per_image()`, and
  `cellular_per_image()` also reach UI render loops through dispatch
  function pointers stored in `FractalSpecific`.
- `ifs_type()` selects UI renderers, while `orbit3d_type()` and the IFS3D
  path pass UI render functions to `funny_glasses_call()`.  That function
  also invokes photographer prompts between stereo images.
- Validation, parsing, memory, and precision failures can invoke modal
  `stop_msg()` or application shutdown through `goodbye()`.
- Pixel, span, and disk-storage functions used by calculation code live
  under `libid/ui`.  Disk storage mixes backend work with progress text
  and modal errors.  Pixel access itself remains calculation work.
- Image loading, image saving, and command parsing still select UI
  prompts, display status, or exit the application from non-UI modules.

The numbered slices below cover these paths explicitly.  A controller
listing, a future-direction paragraph, or a generic no-UI-includes rule
does not count as an implementation slice for a reverse call.

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
- Apply that rule to setup, sizing, completion, errors, and cleanup as
  well as `iterate()`.  Audit indirect calls through `thinking()`, modal
  diagnostics, dispatch pointers, cursor helpers, and storage setup.
- Keep image plotting and pixel access in calculation or storage code.
  Extract misplaced backend work from `libid/ui`; do not move rendering
  algorithms into UI to eliminate an include.
- Calculation may submit numerical data to the application-supplied
  `SoundOutput` port.  Its implementations must not poll input or wait
  inside a submission.  The UI coordinates playback backpressure with
  bounded calculation; this is not a callback into UI control flow.
- Keep behavior first.  Pure call relocation is not a completed slice; the
  owning state, decision, or return point must become clearer.
- Preserve the existing synchronization cadence.  A new resumable state
  must not force input dispatch or display refresh for every orbit point,
  pixel, or internal state transition.
- Validate changed calculation control flow with automated tests first.
  Require manual testing only for a specific changed interaction or native
  display behavior that those tests do not exercise.  Missing coverage of
  deterministic state transitions calls for a new automated regression.
- Trace each changed interaction before giving manual instructions.  Give
  arguments, actions, expected observations, and the automated coverage
  gap.  Reuse image tests instead of manually checking rendered results.

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

The menu command-result split has been implemented.  `MenuHandler`
callbacks return `CommandResult`; the boundary near `big_while_loop()`
maps those effects onto the existing `MainState` application-phase switch
and local-loop behavior.  Keep that distinction as the outer application
loop is inverted.

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

Current handlers communicate command effects through `CommandResult`,
with remaining `g_calc_status` and `MainContext::more_keys` side effects.
`MainState::CONTINUE` remains loop mechanics, not a semantic command
effect.  Preserve the explicit command-effect mapping as the outer
application loop learns to drive render increments.

The menu command-result split keeps this boundary:

- `MenuHandler` callbacks return `CommandResult`.
- Keep the existing `MainState` switch in `id_main`.
- One mapping point near `big_while_loop()` translates command results
  into existing `MainState` transitions and local-loop behavior.
- Retain the current `g_calc_status` and `MainContext::more_keys` side
  effects until a focused slice replaces each with an explicit effect.

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

An output port is a different boundary.  `SoundOutput` accepts calculated
data, like pixel output, without calling a controller or hiding a wait.
Do not reject that design merely because its interface is virtual.

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
  standard orbit-plot requests, the temporary show-dot display bridge,
  and standard suspend/resume state.
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

The indirect paths also belong to that boundary.  `FractalSpecific`
contains UI renderer pointers, `standalone_per_image()` and
`cellular_per_image()` invoke those pointers, and IFS and 3D front ends
call UI controllers from `fractals/lorenz.cpp`.  A top-level-only change
does not eliminate those nested UI calls.

The implementation order closes calculation-side input and presentation
paths first, then extracts misplaced backend helpers, and finally makes
the outer UI controller drive a selected engine operation.  Retain the
wait, interrupt, and perturbation work after the sound-port slices;
controller-specific audit follow-ups remain after that work.

## [Sound Output Port](sound.md)

The separate design artifact defines `SoundOutput`, numerical samples,
session ownership, backpressure, and coordinated interactive playback.
Calculation submits directly to an output sink.  Do not thread plotted
coordinates upward through pass and renderer results solely for sound,
or add sound-notification mailboxes to calculation objects.

Follow the [dependency-injection contract](sound.md#dependency-injection):
`SoundOutputPtr` aliases `std::shared_ptr<SoundOutput>`.  The application
installs one shared instance in `g_sound`; calculation calls that
active port directly and UI services the same instance.  Do not thread
sound parameters through calculation call chains, add constructor
injection to every calculation object, or introduce per-pass sound fields
and forwarding functions.  Tests replace and restore this same binding.

Interactive plotting and audio proceed at the slower rate.  The engine
returns when output cannot accept the next coordinated unit; UI owns
waiting and input.  Silent output and recording without device playback
do not acquire playback waits.  Preserve normal synchronization cadence
when the sink is ready.

The UI services non-blocking `SoundOutput::update()` during rendering,
playback waits, and completion.  The implementation releases due voices;
calculation does not call soundoff or service playback.  Normal release
preserves other voices and release tails, whereas explicit `mute()`
immediately silences all voices and resets playback state.  Normal
completion drains release tails rather than muting them away.

No multimedia library has been selected.  Implement the port first with
one driver-backed implementation that optionally records each accepted
tone under `OSF_MIDI` before attempting playback.  These sequential
behaviors are not two port implementations or a composite.  Private file
helpers need not implement `SoundOutput`.

Do not promise audible continuous sound on drivers that currently return
`false`.  Preserve optional recording regardless of driver capability.
Future audio-file-only output, new audio libraries, WAV encoding, and
changed sound mappings are separate work.

## Orbit Display And Delay Policy

Orbit plotting is engine work done from inside each pixel's orbit
iteration.  The UI layer must not plot or scrub orbit points.  The
inversion work is to remove keyboard and mouse polling buried inside
orbit display mechanics by making the engine return after bounded
plot or scrub work.  The UI controller owns input polling and any
presentation delay between those engine increments.

Do not settle `orbitdelay` display semantics as part of the mechanical
inversion slices.  The old DOS delay model may not fit a windowed
application, and exact FRACTINT microsecond compatibility is not
necessarily the right goal.  Revisit the entire policy before adding
new display-delay implementation slices.

The policy review must cover every relevant overlay or orbit-inspection
use:

- standard escape-time overlay display from `showorbit=yes`;
- FrothyBasin overlay display;
- Lorenz and other 2D or 3D orbit plotting paths;
- any remaining fractal type that uses `g_orbit_delay`, `plot_orbit`,
  visible point pacing, or sound output timing.

Questions to answer:

- Is `orbitdelay` still a compatibility parameter, a UI presentation
  cadence, or a legacy value that should be mapped to modern behavior?
- What should `orbitdelay=0`, small positive values, and large values
  mean for windowed display?
- Should visible orbit display use per-point delay, frame-rate-limited
  sampled presentation, single-step mode, or a separate animation UI?
- Should batch or headless rendering ignore visible orbit delay?
- What future sound-duration controls or `orbitdelay` compatibility
  mapping should replace the interim driver adapter's duration setting?

Done when:

- the chosen policy is documented before implementation;
- automated tests specify the chosen timing and state behavior; manual
  arguments and observations cover only remaining native display or input
  gaps, not every affected fractal type;
- new implementation slices separate engine plot work from UI polling,
  without mixing in unresolved delay-policy decisions.

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
calculation object that owns scan state, orbit state, and image point
plotting.

Popcorn and PopcornJul are not part of the standard overlay-orbit
policy.  They hijacked the old orbit plotting machinery to draw image
pixels, but they are not showing temporary escape-time orbit overlays.
Their work item is a dependency cleanup: remove the false dependency on
`OrbitPlot` without treating Popcorn as an orbit-inspection feature.

The running orbit overlay may eventually be superseded by an orbit tool
window with an interactive cursor in the main image.  That tool would
let the UI request orbit calculation for a selected image point, then
animate, pause, single-step, or play sound independently of image
rendering.  In that future shape, `showorbit=yes` can become
compatibility syntax that opens or enables the orbit tool instead of
forcing rendering to draw, delay, and scrub temporary overlay points.

That direction reinforces the current inversion boundary: image
rendering should not own input polling or UI pacing, and the UI should
not own fractal orbit math.  The engine supplies bounded orbit work; the
UI decides when to request it, how to present it, and how to handle user
input while it is visible.

The Popcorn conversion should follow review boundaries, not a guessed
slice count.  A slice is small enough when it preserves current behavior,
removes one false dependency, and leaves a reviewable intermediate state.

## Current Presentation Leaks

The remaining UI-concept leaks outside `libid/ui` are:

- `libid/engine/OrbitPlot.cpp`: owns sound mapping and `write_sound()`
  calls.
- `libid/include/engine/StandardFractal.h` and
  `libid/engine/StandardFractal.cpp`: expose the temporary show-dot
  display bridge that will be removed when show-dot presentation moves
  fully into the UI controller.
- `libid/engine/calcfrac.cpp`: owns show-dot marker save and restore
  state.  Standard calculation also writes sound time markers and closes
  sound output.
- `libid/engine/sound.cpp`: owns speaker/file output, calls
  `driver_sound_on()`, waits through `wait_until()`, and accepts an
  input-pending callback.
- `libid/engine/StickyOrbits.cpp` and `libid/fractals/lorenz.cpp`:
  call `write_sound()` directly from rendering iteration.
- `libid/fractals/lorenz.cpp`: calls `driver_mute()` from orbit
  renderer constructors, destructors, `done()`, and `suspend()`.
- `libid/io/gifview.cpp`: `put_sound_line()` plots a whole decoded line
  while calling `sleep_orbit_delay()` and `write_sound()`;
  `sound_line()` also calls `driver_mute()`.  Decoder and file traversal
  must return incrementally before UI can coordinate this output.
- `libid/engine/spindac.cpp`: includes palette rotation delay via
  `driver_delay()`.
- `libid/engine/wait_until.cpp`: implements a key-sensitive delay loop.
- `libid/fractals/Cellular.cpp`: starts, updates, and ends the thinking
  screen from generation, error, and suspend paths.
- `libid/fractals/lsystem.cpp`: recursive sizing owns the thinking screen
  and decides whether a pending key aborts sizing.
- `libid/engine/jiim.cpp`: owns cursor state, coordinate messages, the
  interaction loop, UI handler and mouse scopes, and calls into UI input
  processing and key requeue helpers.
- `libid/fractals/lorenz.cpp`: IFS and 3D front ends invoke UI renderers;
  stereo orchestration invokes photographer prompts between images.
- Calculation setup, fractal loaders, formula parsing, precision math,
  debug displays, and memory helpers invoke `stop_msg()` and sometimes
  `goodbye()`.  These are input and application-control leaks even when
  they are reachable only on failure or in debug builds.
- Calculation code calls pixel/span functions in `ui/video.cpp` and
  storage functions in `ui/diskvid.cpp`.  Storage also displays progress
  text and modal errors.  Backend extraction must retain plotting below
  the UI layer while returning status and failures to UI owners.
- `libid/io/loadfile.cpp`, `libid/io/encoder.cpp`, and
  `libid/engine/cmdfiles.cpp`: select prompts, display status, make
  continue/cancel decisions, or shut down the application from non-UI
  coordination code.

These are not all keyboard polling sites, but they are presentation
ownership leaks.  The engine and fractal classes may expose bounded
rendering facts such as "overlay point was drawn" or "scrub restored
pixels"; the UI controller decides whether that causes display delay or
input handling.  Sound samples go directly to `SoundOutput`; they are
not another reason to add UI-consumed flags to calculation objects.

Every presentation-cleanup slice must move behavior to a named UI
controller, output adapter, or appropriately owned backend before
removing the engine-side leak.  Do not merely delete delay, pacing,
sound, mute, or input behavior.  The sound-port slices preserve
`OSF_MIDI` and driver delegation, not the old DOS polling/wait mechanism.

When a slice adds a temporary interface, method, member, or helper only
as a bridge to a later slice, mark the added declaration or definition
with this comment immediately before it:

```cpp
// TODO: removed later
```

The later slice must remove the marked wart.  Permanent interfaces must
not use this marker.

Each slice must fit a focused commit.  Split prerequisite state changes
from ownership changes when either needs independent review.  Do not
combine all diagnostics, all renderer families, or all backend extraction
into one cleanup commit.  Every temporary bridge must name the later
slice that removes it in the plan.

Validation has two parts: existing image or semantic tests protect
calculation, while focused failure and interaction tests protect changed
control flow.  A grep-based test is not a substitute for exercising a
callback path.  Source searches remain audit tools, not repository tests.

## Validation Audit

Existing coverage does not imply coverage of live interaction:

- `tests/cmake/AddImageTest.cmake` runs images with `batch=yes` and
  `sound=off`.  Pass, cellular, orbit, IFS, and restoration fixtures
  protect output, not calculation input, transient markers, or playback.
  `ImageTest.rotate` tests geometric rotation, not palette cycling.
- `tests/autokey/CMakeLists.txt` defines only parameter/MIG generation,
  3D-look Targa output, and random-dot stereogram scripts.  None exercises
  standard-render menu interruption, palette cycling, JIIM, show-dot, or
  the stereo photographer prompt.
- Keyboard-handler tests cover dispatch, interruption flags, and key
  requeueing, not a rendering controller's suspension and resumption.
  Thinking-screen tests cover text and spinner updates, not cellular or
  L-system phase transitions.  Add controller tests for those transitions.
- Perturbation unit tests cover tolerance values, not deep-zoom rendering
  or switching passes.  The perturbation slices must add those automated
  regressions; existing images do not suffice.
- `TestStereo` covers random-line generation, not PHOTO-mode decisions.
  Add prompt/save/continue/cancel coverage in the stereo controller slice.
- Show-dot dispatch and parameter tests do not cover marker restoration.
  Add lifecycle tests; final images cannot verify transient presentation.
- Mock drivers and timer callbacks do not exercise a real window's event
  queue, physical mouse tracking, or visible refresh cadence.  Current
  Win32 and X11 drivers do not support continuous tone playback; require
  fake-port, mock-driver, and tone-log tests, not audible manual checks.

The validation requirements in each slice include tests still to be
implemented, not a claim that those tests already exist.  In particular,
automate menu results, preparation-screen lifetime, pass switching, and
failure cleanup rather than assigning them unspecified manual checks.

Only four slices retain required manual checks: palette cycling, JIIM
cursor presentation, show-dot presentation, and native input during busy
rendering.  Their instructions below identify the uncovered observation.
If a slice adds automation for that same observation, remove the manual
requirement.  Do not repeat it for an unchanged caller or each fractal.

## Implementation Slices

### 2. Driver-Backed SoundOutput

Source: sound mapping, file recording, and device output in
`engine/sound.cpp`; new `ui/DriverSoundOutput.h` and matching
implementation.

Work:

- Implement one concrete `SoundOutput` with instance-owned file and
  playback state.  Preserve the sequential output operation: optionally
  log the accepted tone, then attempt driver playback.
- Keep source-specific mapping in this implementation, including overlay
  debug scaling and restored-image palette indices.  Map each accepted
  sample once for both recording and playback.
- Preserve `OSF_MIDI`/`orbitsave=s`, checked save paths, lazy
  `sound001.txt` creation, integer records, and `time=...` records.
  A private file helper is permitted, not another port implementation.
- Check readiness before acceptance so refused samples leave no record
  or device side effect.  Report file errors without modal UI.
- Delegate device operations to `driver_sound_on()`, `driver_sound_off()`,
  and `driver_mute()` without choosing a multimedia dependency.
- In UI-serviced `update()`, release due voices with
  `driver_sound_off()`.  Reserve `driver_mute()` for explicit immediate
  silence; do not use it for normal note completion.
- Keep configuration and interim tone duration in the implementation.
  Track readiness with instance-owned time state.
- Return promptly from submission, update, and completion operations.
  Do not call `wait_until()`, poll keys, or hide a playback loop.
- Make repeated updates idempotent for released voices, and clear stale
  deadlines on mute before later playback.  Normal completion waits for
  release progression through updates, not by blocking or muting.
- Treat a driver refusing continuous sound as immediately ready; do not
  impose fake audio delays, suppress recording, or reject the sample.
- Leave production producers on the legacy path until their migration.

Done when:

- Optional recording and playback are sequential behaviors of one output
  implementation, without `LegacySoundFileOutput` or a device/file
  composite port.
- Driver acceptance and refusal work without input I/O or a hidden wait;
  requested recording survives either device outcome.
- Normal release, immediate silence, and final drain have distinct driver
  calls and testable lifecycle behavior.

Validation: use a mock driver, controlled clock, and temporary files for
recording enabled/disabled with driver acceptance/refusal.  Verify log
before playback, retry without duplication, naming, boundary records,
file failures, release, mute, and closure.  No real speaker or sleep is
required; production control flow is unchanged in this slice.

### 3. UI Sound Session Coordination

Source: `ui/sound.cpp` and the UI owner of the supplied `SoundOutput`.

Work:

- Configure the driver-backed implementation, including optional
  `OSF_MIDI` recording, from application settings.  Select the no-op sink
  only when neither playback nor recording is requested.
- Install the selected `SoundOutputPtr` in `g_sound` and retain
  shared ownership for the render operation, including nested
  controllers.  Producers use the binding directly; do not pass the
  pointer down dispatch or calculation call chains.
- Do not create `InteractiveSoundOutput`, compose separate file/device
  sinks, or add a speculative file-only mode for the legacy tone log.
- Have the UI controller call `update()` during rendering, playback
  waits, and completion, including when no new sample is submitted.
  Service the supplied implementation directly.
- Provide readiness waiting, explicit mute on interruption, and normal
  completion using the existing keyboard context.  Drain normal voice
  releases and tails instead of muting to finish.  Keep plotting below UI.
- Do not add producer-specific upward result plumbing, unbounded queues,
  a new input queue, or a replacement sound library.

Done when:

- A controller can advance bounded work at the slower of calculation and
  playback rates while continuing input handling between increments.
- Unsupported device output still permits optional tone recording
  without playback waits.  Batch mode does not implicitly enable logging.

Validation: exercise the supplied sink, optional recording with no device
playback, servicing without new samples, interrupted waiting, and final
release-tail completion with fakes.  Producer integration follows in the
next slices; no multimedia library is selected here.

### 4. Standard And FrothyBasin Overlay Sound Port

Source: `OrbitPlot`, standard overlay continuation in `calcfrac.cpp`,
and `FrothyBasin::flush_overlay_orbit_point()`; UI owners:
`ui/standard_fractal.cpp` and `ui/frothy_basin.cpp`.

Work:

- Submit numerical coordinates through the injected active binding at
  the overlay producer, without a constructor parameter or sound member.
  Move frequency formulas out of `OrbitPlot` into adapter mapping; do
  not bubble coordinates to a UI sound handler.
- Handle refusal before committing a plotted point.  Preserve the
  candidate and return through the existing calculation continuation;
  retry without repeating orbit math, plotting, or sample acceptance.
- Integrate both overlay consumers together because they share
  `OrbitPlot`.  Cover synchronous compatibility paths as well as the
  normal pending-overlay path; no path may spin on output readiness.
- Replace the standard pixel's `sound_time_write()` with the port's
  orbit-boundary operation so the migrated file path has one owner.
- Let each controller service `update()` and coordinate readiness,
  normal completion, and explicit mute through its session.  Preserve
  show-dot fixes and leave overlay-delay policy unchanged.

Done when:

- Standard and FrothyBasin overlay output uses the port directly, without
  legacy sound calls, sound-mode checks, or frequency mapping in plotting.
- Neither consumer loses sound-file output or plots ahead of a rejected
  sample.  Unconverted producers retain their legacy path separately.

Validation: record sample/plot order, clipped points, boundary records,
refusal/retry, and suspension for both consumers.  Add controller tests
for overlay toggling and menu interruption/resumption with a fake port;
existing handler tests alone do not cover these paths.  Reuse image
tests for pixels.  No audible or duplicate manual rendering check.

### 5. Sticky Orbit Sound Port

Source: `StickyOrbits::advance_orbit()` and its `StandardPass` route;
UI owner: `ui/standard_fractal.cpp`.

Work:

- Replace `write_sound(g_base_hertz)` with sample submission at the
  producer.  Preserve the initial constant-frequency adapter mapping.
- Retain an unaccepted point before plotting, then return to the existing
  controller.  Keep orbit count, seed state, and traversal resumable.
- Preserve the current 2,048-iteration chunk when output remains ready.
  Do not make every sample require key dispatch or a pass result field.

Done when:

- Sticky orbits use the same sound session without direct sound I/O or
  waiting.  Backpressure cannot skip or duplicate a plotted point.

Validation: exercise rectangle/line traversal, refusal across chunk and
seed boundaries, suspension, and tone recording when the driver refuses
playback.  Add fake-port controller tests for interruption/resumption;
existing image tests protect rendered results.  No manual test required.

### 6. Orbit2D Sound Port

Source: `Orbit2D` in `fractals/lorenz.cpp`; UI owner: `ui/orbit2d.cpp`.

Work:

- Submit numerical orbit values through the injected active binding.
  Do not add a sound constructor parameter or member to `Orbit2D`.
- Move `m_sound_var` selection and frequency mapping to the adapter.
  Preserve projection, connected-line plotting, and raw orbit-save data.
- Retain a pending output point on refusal.  Remove mute from `done()`
  and `suspend()`; UI services normal release/completion with `update()`
  and requests mute explicitly on interruption.

Done when:

- `Orbit2D` neither calls legacy sound/driver functions nor waits for
  playback.  Resumption preserves sample and plot ordering.

Validation: test pending-point continuation, completion, mute on menu
interruption, and file records with the controller and a fake port.
Reuse orbit image and data tests for output.  No manual test required.

### 7. Orbit3D Sound Port

Source: `Orbit3D` in `fractals/lorenz.cpp`; UI owner: `ui/orbit3d.cpp`.

Work:

- Submit transformed numerical orbit values to the supplied port instead
  of computing tones or calling `write_sound()`.
- Preserve projection and stereo eye ordering when a sample is refused;
  do not recompute an orbit or draw one eye twice on continuation.
- Move mute decisions to UI.  Service normal release/completion with
  `update()` instead of muting on completion; mute on interruption.
  Do not combine this with the later stereo-photographer refactor.

Done when:

- `Orbit3D` outputs through the port with correct stereo continuation
  and no device calls or playback waiting in calculation code.

Validation: cover transformed sample values, eye ordering, refusal,
completion, and interruption with fake-port controller tests.  Use
existing image coverage for output; automate stereo sequencing in its
own slice.  No manual test required.

### 8. Dynamic Orbit Sound Port

Source: `Dynamic2D` in `fractals/lorenz.cpp`; UI owner:
`ui/dynamic2d.cpp`.

Work:

- Submit numerical samples directly and move sound mapping to the sink.
- Preserve its loop, seed, iteration, and pending-point state across
  backpressure without increasing normal synchronization cadence.
- Remove mute from `done()` and `suspend()`.  UI services normal release
  with `update()` and uses explicit mute for interruption, not completion.

Done when:

- Dynamic rendering has no `m_sound_var`, legacy sound calls, driver
  mute, or playback wait inside its calculation loop.

Validation: cover refusal before and after seed changes and its existing
iteration boundary with fake-port controller tests, including suspension
and resumption.  Reuse dynamic orbit-data and image tests for output.
No manual test required.

### 9. Restored Image Sound Traversal State

Source: `put_sound_line()` and `sound_line()` in `io/gifview.cpp`, and
decoder line ownership where `g_out_line` selects sound restoration.

Work:

- Represent the active decoded line and column as resumable output state
  rather than assuming the callback can plot and sound the whole line.
  Name the calculation object `RestoredImageOutput` and its files after
  the class.
- Own any decoded data needed after the callback returns.  Preserve
  width wrapping and row accounting without retaining borrowed buffers.
- Establish a bounded return boundary before port integration.  Mark a
  temporary synchronous drain for removal in Restored Image Sound
  Controller, after its decoder and file-lifetime prerequisites.

Done when:

- Restored-image output can retain an uncommitted pixel between calls
  without decoding or plotting it twice.

Validation: cover wrapped lines, buffer lifetime, partial progress, and
normal restored output.  This prerequisite needs no manual image test.

### 10. GIF Decoder State Encapsulation

Source: `decoder()` and LZW bit, dictionary, stack, and line state in
`io/decoder.cpp`; new `io/GifDecoder.h` and `io/GifDecoder.cpp`.

Work:

- Move decoder locals and mutable static state into `GifDecoder`, with
  explicit initialization and lifetime.
- Preserve the synchronous caller for this isolated change.  Mark its
  drain as temporary, removed by Restored Image Sound Controller.
- Keep prefix expansion, clear/end codes, skip factors, and malformed
  input behavior unchanged; do not add audio or UI decisions here.

Done when:

- Decoder continuation can retain all state without its old stack frame.

Validation: reuse codec tests and cover independent decoder instances,
clear/end codes, and cleanup after malformed input.  No manual test.

### 11. GIF Decoder Incremental Lines

Source: line completion and pending prefix expansion in the encapsulated
GIF decoder.

Work:

- Return a decoded line to the caller instead of invoking `g_out_line`
  from inside a whole-image decode loop.
- Preserve a partially expanded code across a line boundary and keep the
  line buffer valid until its consumer has finished with it.
- Retain the marked synchronous compatibility drain for existing callers
  until Restored Image Sound Controller removes it from the sound path.

Done when:

- A caller can stop between lines without losing dictionary, bitstream,
  or expansion state.  Decoding itself never waits for sound output.

Validation: test code expansions crossing line boundaries, skipped rows,
end-of-image handling, and equivalent decoded pixels.  No manual test.

### 12. GIF Restore File Lifecycle

Source: `gif_view()` in `io/gifview.cpp`, especially file, GIF block, byte
buffer, and image-data state surrounding its `decoder()` call; new
`io/GifRestore.h` and `io/GifRestore.cpp`.

Work:

- Encapsulate file traversal in `GifRestore` and own the byte buffer
  across bounded decode calls.  Separate setup, decoding, and cleanup.
- Keep file/format decisions and data below UI.  Make completion and
  failure return to the caller without draining all remaining lines.
- Preserve the old synchronous entry for other restore consumers as a
  marked bridge; Image Loading UI Decisions removes that remaining path.

Done when:

- Sound restoration can retain a decoder and input file while returning
  to UI between output units, without nesting the old `gif_view()` loop.

Validation: cover normal end-of-file, GIF extensions, truncated input,
buffer lifetime, and cleanup; reuse codec images.  No manual image test.

### 13. Restored Image Sound Controller

Source: restored-image pixel, decoder, and file traversal prerequisites;
UI owner: the sound-restoration branch in `ui/big_while_loop.cpp`.

Work:

- Submit pixel-value samples directly from the bounded I/O operation.
  Keep palette-index frequency mapping in the adapter.
- Have UI advance that operation only as playback permits and handle
  interruption outside the decoder/output callback.  Service `update()`
  while rendering and waiting; finish with normal release, not mute.
- Remove temporary pixel/decoder drains from the sound path plus
  `sleep_orbit_delay()`, `write_sound()`, and direct mute calls.  Keep
  marked compatibility paths for other restore consumers separate.

Done when:

- Sound restoration cannot wait for playback inside line decoding or
  pixel output, and refused pixels remain uncommitted until accepted.

Validation: cover restoration refusal, line continuation, interruption,
and legacy file records with a fake port and decoder/controller tests.
Exercise sound-restore selection through its UI entry as well as the
traversal object.  Do not repeat image-restoration tests manually.

### 14. Sound Session Lifecycle Ownership

Source: `ImageCalculation::finish()`, remaining sound-file lifecycle
calls, render controllers, and both `calc_fract()` call sites in
`ui/big_while_loop.cpp`, including evolution rendering.

Work:

- Consolidate output lifetime at the UI render-operation owner.  Keep
  nested controllers from opening or closing a second sound file.
- Retain the same `SoundOutputPtr` used by the active binding throughout
  the operation.  Shared ownership does not replace explicit completion,
  and no producer-specific ownership or parameter plumbing is needed.
- Keep servicing `update()` while `finish()` reports pending playback or
  release tails.  Normal completion must not invoke mute to force a drain.
- Mute promptly on interruption, cancelling pending audible playback and
  stale release deadlines while preserving accepted file records.  Then
  finish or retain the session as its owner decides.
- Remove `close_sound()` from common engine teardown after all producers
  use the port.  Keep image timing and calculation cleanup in the engine.
- Preserve legacy orbit-boundary records without putting wall-clock
  reads or sound-file policy back into pixel calculation.

Done when:

- Completion, cancellation, failure, and evolution transitions close each
  sound session correctly without an engine-owned file or modal error.

Validation: exercise nested controllers, multiple evolution cells, late
playback/release-tail completion, errors, and accepted-once boundary
records.  Verify completion without mute and interruption without stale
release of later voices through fake-port controller tests.  No manual
test required; unsupported drivers cannot verify audible voice behavior.

### 15. Remove Legacy Sound Control Paths

Source: output routines in `engine/sound.cpp`, `SoundInputPendingFn`,
`init_sound_input()`, and any temporary production migration bridges.

Work:

- Remove unused `write_sound()`, `sound_open()`, `sound_time_write()`,
  `close_sound()`, and the input-pending callback after port migration.
- Remove legacy sound waits and polling, not the driver-backed output
  implementation or its optional recording.
  Keep parameter compatibility and completion-buzzer settings in scope.
- Remove temporary bridge declarations and TODO markers from the sound
  output paths.  Marked non-sound GIF compatibility drains remain for
  Image Loading UI Decisions.  Do not add a global forwarding API around
  `SoundOutput`.
- Recheck every migrated producer and failure path for direct driver
  sound calls, input polling, and indirect playback waits.

Done when:

- Calculation uses the port; the concrete implementation owns optional
  recording and driver output; UI owns coordination and input.
  `OSF_MIDI` still works.
- No calculation sound path depends on `engine/wait_until.h` or a
  multimedia library selected speculatively for this refactor.

Validation: replace obsolete input-suppression tests with port/adapter
contract tests and run the full `rt-default` workflow.  Rendering and
legacy file output must remain valid with unsupported driver sound.

### 16. Palette Spin Delay Boundary

Work:

- Split palette rotation state changes from presentation delay in
  `libid/engine/spindac.cpp`.
- Add palette cycling delay to UI callers such as rotate, menus, and
  palette editing.
- After UI callers own that delay, remove `driver_delay()` for palette
  cycling from `libid/engine/spindac.cpp`.
- Preserve existing palette index and DAC update behavior.

Done when:

- `libid/engine/spindac.cpp` does not call `driver_delay()`.
- UI callers own palette animation timing.

Validation: add mock-driver tests for palette index changes, DAC writes,
and UI delay selection.  A plain palette refresh must not acquire a
cycling delay.  Existing image tests do not test palette animation.

Manual testing, for visible cadence and physical input only:

- Arguments: `video=F6 type=mandel`.
- After rendering, press `+` to cycle the palette.  Use Down/Up to slow
  down/speed up, Space to pause/resume, and Esc to exit.  Verify visible
  cadence changes and responsive input.  Do not compare image pixels.
- The palette editor calls the same `rotate()` loop; do not repeat this
  test there unless its entry or return path changes in the slice.

### 17. Wait-Until Cleanup

Work:

- Recheck all remaining `wait_until()`, `reset_wait_until()`, and
  `sleep_orbit_delay()` callers after presentation ownership has moved to
  UI controllers.
- Remove `wait_until()` if no non-UI caller remains.
- Move any remaining UI timing helper to `libid/ui`.

Done when:

- Code in `libid/engine` and `libid/fractals` does not depend on
  `engine/wait_until.h`.
- No rendering path reaches keyboard polling through a wait helper.

Validation: retain timer and controller coverage for any remaining
callers.  Pure helper removal needs no manual test; a newly discovered
caller must have its actual behavior covered before removal.

The remaining direct interrupt checks are grouped by UI polling loop.
After the pass-state slices, the audited non-wait call sites that should
remain are the standard pixel and scanline checks driven from
`libid/ui/standard_fractal.cpp`.

### 18. StandardFractal Pixel Interrupts

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

Validation: retain one/two-pass image tests and add injected-key
controller or autokey regressions for suspension, `x` followed by Esc,
and resumption without skipped or repeated pixels in both passes.
Existing handler tests do not cover the controller path.  These are
deterministic transitions; no additional manual rendering check.

### 19. Perturbation Traversal Ownership

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

Validation: add a deterministic deep-zoom BoundaryTrace regression that
exercises reference failure and replacement, completes without a hang,
and checks computed colors and traversal ownership.  Existing tolerance
tests and ordinary perturbation images do not cover this case.  Use an
automated engine or batch-render test, not a manual completion check.

### 20. Perturbation Pass-Switch Tests

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

Validation: add controller or autokey coverage for the actual deep-zoom
`passes=b` to `passes=g` transition, asserting the selected perturbation
strategy and rendered result.  Include the `perturbation=no` control.
Separate renders do not prove transition correctness.  These new tests
replace the two manual pass-switch checks; no duplicate manual test.

### 21. Cellular Preparation Presentation

Source: `Cellular::iterate()` and `Cellular::suspend()` in
`libid/fractals/Cellular.cpp`; UI owner: `libid/ui/cellular.cpp`.

Work:

- Let the calculation object expose its initial-generation phase as
  calculation state, without thinking-screen flags or UI callbacks.
- Have the existing UI controller own `thinking()` and `thinking_end()`,
  including interruption, phase completion, and exception cleanup.
- Keep each generation's calculation and pixel output in `Cellular`.

Done when:

- `Cellular` never invokes a UI presentation or input helper.
- The UI controller polls during preparation and ends its thinking screen
  on every exit path.

Validation: retain cellular image coverage; exercise preparation-phase
interruption and error cleanup in new controller tests.  Verify thinking
screen entry, updates, and exactly-once cleanup on completion,
interruption, and failure.  Existing thinking-helper tests do not cover
phase ownership; extend automation instead of requiring a manual
preparation-screen check.

### 22. L-System Sizing Traversal State

Source: `find_size()`, `lsys_find_scale()`, and `LSystem::Impl` in
`libid/fractals/lsystem.cpp`.

Work:

- Replace sizing recursion with an explicit frame stack owned by
  `LSystem::Impl`, following the existing drawing-stack pattern.
- Preserve rule order, branch turtle save/restore, extrema, overflow, and
  failure behavior.  Keep sizing synchronous for this isolated change.
- Mark any synchronous drain retained for the next slice as temporary.

Done when:

- Sizing traversal survives between calls without depending on recursive
  local state.  The next slice can advance it in bounded chunks.

Validation: compare sizing extrema, scale, and turtle semantics, using the
approach in [test-lsystem.md](test-lsystem.md).  Do not require image golds
for floating-point line rasterization.  No new manual interaction test.

### 23. L-System Sizing UI Controller

Source: `LSystem::Impl::start()`, `find_scale()`, and sizing traversal;
UI owner: `libid/ui/lsystem.cpp`.

Work:

- Make sizing an explicit calculation phase advanced by bounded
  `iterate()` calls.  `start()` must not expand the whole sizing tree.
- Let the UI controller own the thinking display and interruption for
  sizing as well as drawing.  Keep sizing math in `LSystem`.
- Remove sizing's `thinking()` and `thinking_end()` calls and the
  temporary synchronous drain from the preceding slice.

Done when:

- Setup, sizing, and drawing return to the UI controller without polling
  input or interpreting pending keys inside calculation code.

Validation: cover sizing suspension and completion semantically.  Add
controller tests for thinking-screen entry, updates, cancellation, cleanup,
and transition to drawing, using a controlled sizing workload.  Existing
thinking-helper tests do not cover this integration.  No manual test.

### 24. JIIM Calculation And Tool State

Source: `InverseJulia`, `s_cursor`, window state, and coordinate display
state in `libid/engine/jiim.cpp`.

Work:

- Separate inverse-orbit calculation and queue state from cursor,
  window-layout, coordinate-message, last-key, and interaction state.
- Refactor locally first so the calculation increment does not need a
  `CrossHairCursor`, keyboard context, or mouse subscription.
- Mark the local tool-state holder as temporary; the next slice moves
  its ownership and the interaction loop into UI.

Done when:

- One calculation increment can run with explicit calculation inputs and
  no cursor or key-processing work mixed into it.

Validation: add state tests for inverse-orbit increments and independent
tool state, including continuation and selected coordinates.  This local
refactor does not change the UI loop.  The next slice's native cursor
check covers the ownership change; no separate manual test here.

### 25. JIIM UI Interaction Controller

Source: `InverseJulia::process()`, `iterate()`, `finish()`,
`process_inverse_julia_keyboard()`, and `requeue_inverse_julia_key()`.
UI owner: `InverseJuliaController` in `libid/ui/inverse_julia.cpp`.

Work:

- Move the outer loop, tool-state holder, cursor presentation, coordinate
  messages, handler lifetime, and mouse scope into the UI controller.
- Keep `InverseJuliaKeyboardHandler` and its keyboard context in UI.
  The controller applies selected coordinates to calculation state.
- Let UI poll or wait for input, call bounded engine increments, and
  decide when to finish and requeue a main-loop key.
- Remove calculation-to-UI input callbacks and the temporary holder left
  by the preceding slice.

Done when:

- `jiim.cpp` owns inverse-orbit work and queues, not an interaction loop.
- No engine method invokes UI key processing, cursor waits, mouse scopes,
  or key requeue helpers.

Validation: add controller tests for key and mouse notifications,
exact-point entry, selection, window toggles, handler lifetime, and return
to the main loop.  No existing JIIM controller tests cover these paths.

Manual testing, for native cursor tracking and transient display only:

- Arguments: `video=F6 type=mandel`.
- After rendering, press Space for the Julia picker.  Move the mouse
  without pressing keys, then use arrow keys.  Verify the crosshair tracks
  both smoothly and blinks.  Press `x`, then Esc to close the options
  screen; verify the restored Mandelbrot image has no stale cursor pixels.
- On the Mandelbrot image, use `o` for the orbit window and move the
  cursor.  Press `o` again to close it; verify cursor placement and display
  restoration.  Mock notifications do not cover native mouse delivery or
  visible cursor restoration; do not recheck orbit image correctness.

### 26. Standard Pixel Boundary

Source: `StandardFractal`, `StandardPass`, and the show-dot wrapper in
`libid/engine/calcfrac.cpp`.

Work:

- Expose a UI-neutral boundary between selecting a pixel for orbit work
  and computing it.  Preserve the pass's selected pixel and continuation
  state when returning to the controller.
- Keep that boundary common to standard traversal; do not add show-dot
  logic or feature flags to individual pass algorithms.
- Preserve complete pixel results and calculation order.  Presentation
  must not pretend a completed pixel is incomplete or use its color as
  a UI request.
- Keep ordinary rendering's interaction cadence.  Split pass-protocol
  prerequisites into focused commits if the common boundary cannot be
  introduced without changing several independent traversal mechanisms.

Done when:

- UI can observe the selected pixel around calculation without show-dot
  methods, consume flags, or a UI callback from a calculation function.

Validation: cover selection and continuation across the standard pass
algorithms with existing images and focused state tests.  No manual
image test duplicates automated pass coverage.

### 27. UI Show-Dot Marker Ownership

Source: `ShowDotState`, `show_dot_region()`, and save/restore operations
in `engine/calcfrac.cpp`; UI owner: `ShowDotController` used by
`libid/ui/standard_fractal.cpp`.

Work:

- Move marker geometry, saved pixels, drawing, restoring, and delay
  decisions into the UI controller around the neutral pixel boundary.
- Restore the marker before calculation or pass work can use its pixels
  as image data.  Keep symmetry handling and incomplete-calculation
  continuation correct across suspension and menu-driven recalculation.
- Preserve current delay behavior; do not settle `orbitdelay` policy here.
- Keep the right-edge hang fix until the old wrapper is removed.  Never
  widen its yield eligibility as a shortcut to support more passes.

Done when:

- Show-dot state and presentation decisions are UI-owned, and all
  standard traversal uses the same marker lifecycle.

Validation: add controller tests for marker save/restore order, symmetry,
right-edge continuation, menu cancellation, and parameter-driven restart.
Existing show-dot tests cover dispatch and parsing, not marker lifecycle.

Manual testing, for transient native marker presentation only:

- Arguments: `video=F6 type=mandel passes=1 showdot=b/20 orbitdelay=20`.
- Watch the marker reach the right edge and continue onto the next row.
  During rendering, press `x`, then Esc; verify no stranded marker.  Open
  `x` again, increase Maximum Iterations, and accept; verify the moving
  marker leaves no trails.  Do not wait for or compare the completed image.
- Explicit one-pass mode exercises the current show-dot yield path.  The
  old values 20 and 139 use the same show-dot delay branch; repeating the
  interaction at both values adds no coverage.  This check concerns live
  presentation, which final-image comparisons cannot observe.

### 28. Remove Show-Dot Transitional Wrappers

Source: `calc_type_show_dot()`, `wrap_show_dot_calc_type()`, standard
show-dot setup/cleanup, and the temporary `StandardFractal` display API.

Work:

- Remove show-dot dispatch wrapping and the temporary display bridge once
  the UI controller owns its complete lifecycle.
- Replace wrapper-specific tests with UI-controller save/restore and
  suspension tests, retaining startup parameter coverage.
- Remove all temporary show-dot declarations and their TODO comments.

Done when:

- Engine code has no show-dot marker state, display mailbox, or dispatch
  wrapper.  Parameter compatibility remains intact.

Validation: exercise actual marker lifecycle and interrupted/restarted
calculation.  Do not add source-grep tests or repeat image tests manually.

### 29. Standard Render Validation Diagnostics

Source: validation and allocation diagnostics in `engine/calcfrac.cpp`,
`engine/calc_frac_init.cpp`, and `BoundaryTrace::initialize()`.
UI owner: the standard-render controller.

Work:

- Return validation failures or recoverable warnings to the controller
  using calculation results or exceptions, rather than modal messages.
- Let UI display the diagnostic and decide whether to continue, retry,
  or return to parameter entry.  Retain engine-owned constraint checks.
- Replace show-dot buffer precondition message/exit paths with
  `std::logic_error` if those paths remain after show-dot extraction.

Done when:

- Standard setup and pass initialization cannot enter `stop_msg()`.

Validation: cover invalid pass options and setup failure paths with a
mock UI owner; preserve batch diagnostics and calculation status.

### 30. IFS Definition Diagnostics

Source: `ifs_load()` and shared file-entry lookup; UI owners:
`ui/get_file_entry.cpp` and the IFS render controller.

Work:

- Return malformed, empty, oversized, or unavailable definition failures
  as loader diagnostics without invoking `stop_msg()`.
- Let each UI entry point display its loader failure once and decide
  whether to retry selection or abandon rendering.
- Keep parsing and transform validation independent of the display.

Done when:

- IFS definition loading cannot wait for keyboard input, including through
  shared file-entry error helpers.

Validation: exercise malformed and missing definitions through loader and
UI-entry tests.  No manual rendering test duplicates existing IFS images.

### 31. L-System Definition Diagnostics

Source: definition parsing in `fractals/lsystem.cpp` and file-entry
lookup; UI owners: `ui/lsystem.cpp` and `ui/get_file_entry.cpp`.

Work:

- Return parser and lookup diagnostics to the UI owners instead of
  displaying modal errors from L-system code.
- Preserve rule-loading failure state and ensure sizing never begins
  after an invalid definition.

Done when:

- L-system loading, sizing, and drawing cannot invoke UI error handling.

Validation: reuse the parser-test direction in
[test-lsystem.md](test-lsystem.md), including retry after invalid input.

### 32. Formula Parser Diagnostics

Source: file reading, prescan, compilation, and error reporting in
`libid/fractals/parser.cpp`.

Work:

- Preserve diagnostic text and parse locations as parser results or
  errors.  Remove direct `stop_msg()` calls from those operations.
- Let formula selection and render-setup UI owners decide how to display
  errors and whether another definition is requested.
- Keep parser cleanup correct when parsing stops early.

Done when:

- Formula parsing and compilation can run without a UI driver and cannot
  open a modal message from calculation setup.

Validation: exercise malformed input, missing files, failed compilation,
and successful retry through existing parser and UI-entry tests.

### 33. Fractal Setup Validation Diagnostics

Source: `lyapunov_per_image()` and `julibrot_per_image()`.
UI owners: the Lyapunov and standard-4D controllers.

Work:

- Return unsupported inside-method and video-mode diagnostics to the UI
  owners rather than invoking `stop_msg()` from per-image setup.
- Preserve setup success/failure meaning and avoid starting calculation
  after validation failed.

Done when:

- These calculation setup routines cannot wait for input or choose an
  error-screen interaction.

Validation: cover both invalid configurations and the successful setup
paths.  Normal image coverage needs no duplicate manual checks.

### 34. Palette Definition Diagnostics

Source: color-map loading and validation in `libid/io/loadmap.cpp`;
UI owners: palette selection and render-setup controllers.

Work:

- Return missing-file and malformed-map diagnostics without `stop_msg()`
  from loader code, including loads initiated during Julibrot setup.
- Let UI decide whether to retry, retain the prior palette, or abandon
  the requested operation.
- Preserve parsing, compatibility, and palette data below UI.

Done when:

- Palette loading cannot enter a modal screen from calculation setup.

Validation: cover missing and malformed maps, retained prior state, and
successful retry through loader and UI-choice tests.

### 35. 3D Output Diagnostics

Source: Targa output failures and interactive debug diagnostics in
`libid/geometry/line3d.cpp`; UI owners: 3D restore and stereo controllers.

Work:

- Return rendering and output-file diagnostics to the requesting owner.
- Remove modal `stop_msg()` calls from geometry and file-output helpers,
  including debug-only paths.
- Keep projection, line plotting, file output, and cleanup in calculation
  or I/O code; UI owns display, continuation, and abandonment decisions.

Done when:

- 3D transformation and output cannot invoke a UI prompt while advancing
  geometry or writing an image.

Validation: cover output failure and cleanup plus existing 3D rendering
tests.  No new manual rendering comparison is needed.

### 36. Arbitrary-Precision Failure Ownership

Source: precision allocation and `alloc_stack()` in `math/biginit.cpp`.
UI owner: the controller requesting precision setup or rendering.

Work:

- Report unsupported precision and exhausted arena failures without
  `stop_msg()` or `goodbye()` in math code.
- Keep precision constraints, arena state, and cleanup in math code; let
  the UI owner choose parameter correction or application exit.
- Preserve batch error reporting and exit status through the outer owner.

Done when:

- Arbitrary-precision math can fail without polling input or terminating
  the application from the math layer.

Validation: exercise allocation-limit and stack-limit failures, cleanup,
and a subsequent valid precision request.

### 37. Precision Debug Display Ownership

Source: `show_var_bn()`, `show_corners_*()`, and related debug displays
in `libid/engine/fractalb.cpp`.

Work:

- Keep formatting or collection of numeric debug data independent of UI.
- Move interactive display and cancellation/exit handling into the UI
  owner that requests the debug inspection.
- Audit debug-only call paths as well as release paths.

Done when:

- Debug math and calculation helpers no longer call modal UI or
  `goodbye()`.  Debug builds satisfy the same dependency boundary.

Validation: exercise collected diagnostic data and UI handling without
requiring a manual pixel comparison.

### 38. Memory Failure And Recovery Ownership

Source: allocation, move validation, disk-memory recovery, debug messages,
and shutdown calls in `libid/misc/memory.cpp`.

Work:

- Return failure information from memory operations without modal
  `stop_msg()`, buzzer output, or `goodbye()`.
- Let requesting UI controllers own any retry/cancel choice and resume
  or abandon the operation after the choice.
- Keep allocation and resource cleanup below UI.  Do not replace direct
  prompts with callbacks that still block inside a memory operation.

Done when:

- Memory operations and their failure paths never require an input queue
  or select application shutdown.

Validation: exercise allocation failure, invalid operations, retry,
cancellation, and cleanup through focused failure-path tests.

### 39. Pixel Access Layer Ownership

Source: `get_color()`, pixel/span operations, and their backend selection
in `libid/ui/video.cpp`, plus non-UI callers and headers.

Work:

- Move image pixel access and storage routing into an appropriate engine
  or I/O module, following existing backend behavior.
- Leave display-target activation and UI commands with their UI owners.
- Update callers in passes, fractal classes, geometry, and codecs without
  moving their plotting algorithms into UI.

Done when:

- Calculation code does not include `ui/video.h` for pixel access.
- Pixel access is calculation/storage work, not a UI callback.

Validation: use existing image and codec tests.  This is a dependency
correction, not by itself inversion of an input loop; no new interface
or callback table is required merely to relocate the backend.

### 40. Disk Storage Backend State

Source: cache, allocation, disk pixel access, Targa backing, and memory
queue storage in `libid/ui/diskvid.cpp`.

Work:

- Separate backend storage state and operations from disk-video status
  screens and modal failure decisions.
- Put backend operations in an I/O-owned module used by calculation and
  UI alike.  Preserve potential, plasma, Targa, and MIIM storage behavior.
- Return storage progress and failures as data.  Mark any remaining
  presentation bridge for removal in the next slice.

Done when:

- The backing-store lifetime and cache state are owned below UI.

Validation: use existing disk-video, memory, potential, and image tests;
add focused failure coverage only where the split changes recovery.

### 41. Disk Storage UI Decisions

Source: `pot_start_disk()`, `common_start_disk()`, `dvid_status()`, and
storage setup callers in StandardFractal, Plasma, JIIM, and image I/O.
UI owner: disk-video presentation and the requesting controller.

Work:

- Let UI own disk-video progress text and error decisions around backend
  setup and bounded render work.
- Remove backend calls into status screens and modal errors, including
  indirect input through failed allocation.
- Remove temporary bridges from the preceding slice.  Preserve storage
  cleanup and headless/batch operation without requiring a text screen.

Done when:

- A calculation-side storage operation cannot call UI or wait for input.
- Non-UI code no longer needs `ui/diskvid.h` for backend operations.

Validation: exercise setup failure, cancellation, and normal storage
completion with injected storage failures and mock UI decisions.  Existing
disk-video tests cover normal memory/disk selection, not failure prompts;
add those cases.  No manual storage or rendering check required.

### 42. Image Loading UI Decisions

Source: `read_overlay()` and related coordination in `io/loadfile.cpp`;
UI owner: the existing image-load/restore path in `ui/big_while_loop.cpp`.

Work:

- Separate metadata reading and validation from video-mode selection,
  3D-parameter prompts, and UI history decisions.
- Return the loaded requirements to the UI owner, which gathers choices
  before starting or resuming decoder/render operations.
- Preserve cancelled-load cleanup, batch defaults, and prior-image state.
- Drive the bounded GIF restoration objects from the sound prerequisites
  for other restore consumers, then remove their marked synchronous
  compatibility drains.  Split distinct consumer conversions if needed.

Done when:

- Image-loading code does not invoke `get_video_mode()`,
  `get_3d_params()`, or modal diagnostics from inside decoding setup.

Validation: cover cancelled selection, invalid metadata, and successful
load through existing codec tests and focused UI-choice tests.

### 43. Image Saving UI Decisions

Source: save coordination around `encoder()` in `io/encoder.cpp`;
UI owners: manual save and autosave paths in `ui/big_while_loop.cpp`.

Work:

- Return write results and partial-save state to the UI owner before
  asking whether to retain, replace, or discard partial output.
- Let UI own saved/interrupted messages, completion sounds, and any
  save-then-exit decision.
- Keep encoding, file replacement mechanics, and resource cleanup in I/O;
  the selected UI decision determines when those mechanics run.

Done when:

- Encoding and save-file operations cannot open prompts or terminate the
  application.  Existing partial-file retention choices remain available.

Validation: exercise partial saves for new and existing files, both user
choices, write failures, and autosave completion.  Use codec tests for
file contents instead of manual image comparisons.

### 44. Command Parsing Diagnostics

Source: `arg_error()`, `init_msg()`, `s_stop_msg`, and `s_goodbye` in
`engine/cmdfiles.cpp`; UI owners: startup and command-entry controllers.

Work:

- Collect parse errors and startup warnings as data for the caller.
- Let UI choose startup presentation, deferred pause, retry, and exit.
  Preserve batch logs and exit status without modal interaction.
- Remove production UI calls hidden behind test-hook callbacks; do not
  replace them with another parser-to-UI callback interface.

Done when:

- Command parsing cannot call `stop_msg()`, `do_pause()`, or `goodbye()`.

Validation: preserve command-parser tests; exercise malformed startup
arguments, interactive command errors, and batch failure behavior.

### 45. Command Actions And Application Effects

Source: help-document generation, parameter-file generation, comments,
and application-exit branches in `engine/cmdfiles.cpp`.

Work:

- Make parsing report requested actions and settings to its outer owner.
- Let UI/application coordination run help/status screens, batch-file
  generation, and exit decisions after parsing returns.
- Keep content generation and parameter interpretation below UI where
  appropriate; classify data-only helpers separately from UI decisions.

Done when:

- A command parser no longer selects a screen or exits the application
  while interpreting an argument.

Validation: retain generated-output tests and cover command effects at
their caller.  Do not conflate this with the completed `CommandResult`
split for interactive menu handlers.

### 46. Common Engine Render Lifecycle

Source: `ImageCalculation::start()` and `finish()`, `StandardFractal`,
`Lyapunov`, and `NonStandardFractal`.

Work:

- Integrate common calculation setup, image timing, suspension, and
  cleanup into the selected calculation object's lifecycle.
- Use shared implementation only for real common setup and teardown.
  Retain `g_calc_time` ownership in engine code and account for the whole
  image exactly once.
- Expose initialization, bounded iteration, completion, suspension, and
  finish operations without invoking a UI controller inside them.

Done when:

- Standard and Lyapunov rendering have complete engine-owned lifecycles
  that an outer UI controller can advance directly.
- Common lifecycle operations do not hide a whole-image render call.

Validation: cover normal completion, suspension/resume, failure cleanup,
and calculation-time accounting.  Existing images protect render results.

### 47. Standalone Engine Operation Selection

Source: `NonStandardFractal`, `FractalDispatch`, `FractalSpecific`, and
the already incremental standalone calculation classes.

Work:

- Select a concrete calculation object from engine state and dispatch
  data.  Keep fractal classification and type-specific initialization
  below UI; UI receives the already-selected operation.
- Bind existing standalone calculation lifecycles into that selection
  using simple explicit routing consistent with `StandardPass`.
- Convert any remaining whole-render operation into bounded calculation
  phases before binding it.  Split that family's conversion into its own
  prerequisite slice if it is more than a mechanical binding.
- Do not create an operation whose `iterate()` calls a UI renderer.

Done when:

- A selected standalone operation performs calculation increments without
  invoking a UI loop through its dispatch entry.

Validation: drive selected operations to completion and suspension in
engine tests; reuse existing standalone image and semantic coverage.

### 48. Remove Per-Image Renderer Reentry

Source: `standalone_per_image()` in `fractals/frasetup.cpp`,
`cellular_per_image()`, full-renderer entries in `fractals/fractalp.cpp`,
and `NonStandardFractal::iterate()` in `engine/calcfrac.cpp`.

Work:

- Replace full-renderer UI function pointers with selection of the
  standalone calculation operations prepared in the preceding slice.
- Make per-image routines perform setup only.  Remove nested full-render
  calls and their timer brackets; preserve actual setup-result meaning.
- Keep the fractal table const and runtime dispatch changes explicit.
- Remove corresponding UI includes and transitional renderer-call
  bridges once their calculation operation owns the work.

Done when:

- Neither `per_image()` nor its setup callbacks can enter a UI render
  loop through a saved function pointer.

Validation: cover cellular and other standalone selection through the
real dispatch path, including setup failure and resume behavior.

### 49. IFS Front-End Calculation Selection

Source: `ifs_type()` and the `ifs3d()` front end in
`libid/fractals/lorenz.cpp`; UI owners: `ui/ifs2d.cpp` and `ui/ifs3d.cpp`.

Work:

- Keep IFS loading, dimension selection, transforms, and calculation
  state in engine or fractal code.
- Expose the selected IFS calculation object to the UI controller, which
  advances its existing `iterate()` operation.
- Remove `ifs_type()` calling `ifs2d()` or `ifs3d()` as UI functions from
  fractal code.  Do not move dimension-based renderer classification
  into the UI controller.

Done when:

- IFS front-end calculation code never invokes a UI render loop.

Validation: use existing 2D and 3D IFS rendering tests and add operation
selection tests.  This selection change needs no manual image comparison.

### 50. Stereo Rendering UI Sequence

Source: `funny_glasses_call()`, `orbit3d_type()`, and stereo IFS setup in
`fractals/lorenz.cpp`; UI owner: `StereoController` in `ui/stereo.cpp`.

Work:

- Make eye selection, projection setup, and rendering engine operations
  that can be started and incrementally driven for each eye.
- Let the UI controller sequence rendering and photographer prompts,
  including save, continue, cancellation, and final display restoration.
- Remove engine calls to `lorenz_stereo_photographer_prompt()` and the
  second-image `stop_msg()`, plus callbacks to UI render functions.
- Do not replace `funny_glasses_call()` with another synchronous wrapper
  that invokes whole UI render loops.

Done when:

- Stereo calculation never waits for a photographer's response or chooses
  a UI controller.  Each eye's rendering remains calculation work.

Validation: retain stereo image coverage and add mock-controller or
autokey tests through PHOTO-mode entry for the between-eye prompt, save,
continue, cancel, and display restoration.  Current random-line tests and
3D-look autokey coverage do not exercise that prompt.  These deterministic
UI decisions require new automated coverage, not a manual checklist.

### 51. Orbit Renderer Operation Selection

Source: orbit, dynamic, IFS, inverse-Julia, and standard-4D renderer
entries in `fractals/fractalp.cpp` and front ends in `fractals/lorenz.cpp`.

Work:

- Bind these remaining renderer families to engine-selected calculation
  operations using the earlier IFS and stereo separation.
- Keep projection, dimension selection, orbit setup, and calculation
  state in their calculation objects; controllers own input and prompts.
- Remove UI renderer function pointers from the remaining table entries
  and remove `ui/*` includes from `fractals/fractalp.cpp` and calculation
  front ends.
- Split any family needing new calculation-state work into a separate
  prerequisite; do not hide it in a generic UI-callback operation.

Done when:

- Calculation dispatch can select every renderer without naming a UI
  renderer function or calling back into a controller.

Validation: cover selected operation identity, setup, suspension, and
completion; reuse orbit, IFS, and standard-4D rendering tests.

### 52. Outer UI Render Operation Control

Source: `calc_fract()`, the render branches in `ui/big_while_loop.cpp`,
and standard, Lyapunov, and non-standard UI controllers.

Work:

- Have the outer UI render context obtain the engine-selected operation,
  initialize it, poll input, advance bounded work, and finish or suspend.
- Retain application-phase handling and command-effect mapping at their
  existing boundary.  Preserve render completion, save, and exit in batch
  mode, including user interruption while batch rendering.
- Remove engine calls to `standard_fractal()`, `lyapunov_fractal()`, and
  `non_standard_fractal()`, then remove obsolete `calc_fract()` dispatch
  scaffolding.  Do not duplicate renderer classification in UI.
- Reuse UI contexts for presentation while the engine operation owns
  calculation state, timing, and resource cleanup.

Done when:

- Rendering begins from UI/application coordination; engine code never
  selects or invokes a UI controller.
- No new wrapper retains the old nested engine/UI render call chain.

Validation: run the full `rt-default` workflow and cover batch completion,
save, exit, and interruption.  Add controller or autokey tests for menu
cancellation, parameter changes, and presentation toggles through the
outer application phases.  Existing autokey scripts do not cover these
transitions; do not substitute a manual retest of every menu or renderer.

Manual testing, for physical input during CPU-bound rendering only:

- Arguments:
  `video=F6 type=mandel passes=1 maxiter=1000000 periodicity=0`.
- While rendering, press `x` physically; verify the options screen opens
  responsively.  Press Esc and verify rendering resumes and the window
  continues repainting.  Do not wait for completion or compare pixels.
- Mock-driver and autokey tests bypass physical input delivery.  Unlike
  the show-dot check, this path has no marker delay pumping window events
  that could hide a missing outer-loop event pump.

### 53. Remaining Data-Only UI Dependencies

Source: remaining `ui/*` includes in calculation code after the concrete
control-flow and backend ownership slices.

Work:

- Classify each remaining dependency as data, calculation, backend work,
  or actual UI control.  Move shared constants, types, and pure helpers
  to their owning non-UI module when calculation requires them.
- Include palette-analysis helpers, projection/palette constants, video
  metadata, and UI-context flags still used by calculation setup.
- Keep actual UI decisions with controllers.  Any newly discovered
  callback needs its own focused inversion slice, not a renamed include
  or a forwarding function that hides the dependency.

Done when:

- Engine and fractal headers and implementations no longer need UI
  headers, including through data-only dependencies.

Validation: build the affected configurations and reuse existing data and
image tests.  This dependency correction does not introduce a new input
mechanism or a source-grep test.

## Current Direct And Indirect Input Leaks

Calculation code can still reach polling or key-sensitive waiting through
these direct and indirect paths:

- `libid/engine/calcfrac.cpp`: standard pixel calculation still calls
  `calc_interrupted()`.
- `libid/engine/ScanlinePass.cpp`: one-pass and two-pass traversal still
  calls `calc_interrupted()` while deciding whether to suspend work.
- `libid/engine/sound.cpp`: speaker output still waits through
  `wait_until()` and suppresses speaker tones through an input-pending
  callback.
- `libid/io/gifview.cpp`: restored-image output reaches that legacy
  sound wait from within synchronous decoder callbacks.  Its separate
  pixel delay also needs a bounded return to the UI owner.
- `libid/engine/wait_until.cpp`: delay loop wakes on pending key.
- `libid/fractals/Cellular.cpp`: `iterate()` calls `thinking()`, which
  polls the driver queue.  Completion, failure, and suspension also call
  UI screen cleanup.
- `libid/fractals/lsystem.cpp`: recursive `find_size()` calls
  `thinking()` from inside `start()` and interprets pending input as
  sizing failure.  UI polling around drawing does not cover this phase.
- `libid/engine/jiim.cpp`: `iterate()` calls
  `process_inverse_julia_keyboard()`, which may wait through the cursor.
  `process()` owns the keyboard handler, mouse scope, and interaction loop;
  `finish()` requeues a key through a UI helper.
- `libid/fractals/lorenz.cpp`: `funny_glasses_call()` enters a blocking
  photographer prompt and second-image modal message.  IFS and orbit
  front ends also invoke complete UI render loops.
- `libid/engine/calcfrac.cpp`, `fractals/frasetup.cpp`,
  `fractals/Cellular.cpp`, and renderer table entries in
  `fractals/fractalp.cpp`: reach UI render loops through direct calls or
  dispatch pointers, including from per-image setup.
- Calculation validation, definition loaders, formula parsing, math
  allocation, debug displays, and memory operations: `stop_msg()` polls
  and waits inside these callers, and some failures invoke `goodbye()`.
- Storage setup and disk-video progress can invoke modal failure screens
  from calculation code through `ui/diskvid.cpp` and memory allocation.
- `libid/io/loadfile.cpp` and `io/encoder.cpp`: enter mode/parameter
  selection or partial-save prompts from loading and saving operations.
- `libid/engine/cmdfiles.cpp`: `arg_error()` calls UI through
  `s_stop_msg` and `s_goodbye`; `init_msg()` displays startup errors and
  schedules `do_pause()`.  Other command branches generate UI output or
  exit.  These are live behaviors, not merely comments about old code.

The numbered slices cover all these ownership gaps.  The source audit
must trace UI helper and dispatch-pointer calls as well as raw driver
calls; an empty raw-key search alone does not establish inversion.

The `OrbitPlot` wait loop and delay state have been removed.  Its sound
presentation state remains tracked under current presentation leaks.

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
- Calculation setup, sizing, iteration, suspension, completion, and error
  handling cannot reach UI polling indirectly through `thinking()`,
  cursor waits, modal messages, or renderer dispatch pointers.
- Calculation and backend operations return failures and warnings to UI
  owners without selecting prompts or calling `goodbye()`.
- JIIM and stereo interaction are driven from UI controllers around
  bounded calculation operations.
- Pixel plotting, image storage, and calculation timing stay below UI;
  their backend operations do not own input or presentation decisions.
- Calculation may submit numerical samples directly to `SoundOutput`.
  The application injects one `SoundOutputPtr` through the active output
  binding, not through calculation call-chain parameters or per-pass
  fields; UI services that same instance.
  One concrete implementation preserves optional `OSF_MIDI` recording
  followed by driver output, without key polling or playback waits inside
  the port.  Interactive UI advances plotting and audio together at the
  slower rate; driver refusal does not delay or suppress recording.
- UI services non-blocking `update()` while rendering, waiting, and
  completing.  Normal voice release and release-tail completion do not
  use mute; interruption explicitly silences and resets playback.
- Show-dot marker state and all temporary display bridge methods have
  been removed from calculation code.
- Audit source calls and reachable dispatch targets in both release and
  debug configurations.  Ignore comment-only search matches; do not add
  grep-based tests that merely enforce source spelling.
- Calculation code reports interruption or decisions without owning input.
- Current behavior is preserved while longer-term architecture stays out
  of scope.
