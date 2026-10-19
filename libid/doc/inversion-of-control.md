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
- `Orbit2D`, `Orbit3D`, and `Dynamic2D` submit numerical samples through
  `g_sound`.  Their UI controllers service playback, finish normal
  output, and mute on interruption.  Pending points retain calculation
  state without changing ready-output polling cadence.

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

## GIF File Operation Policy

Treat reading or writing a GIF as one atomic, run-to-completion I/O
operation.  Old DOS keyboard polling compensated for slow LZW work; it is
not a behavior to preserve when replacing the codecs with giflib.

- Remove direct and indirect keyboard polling, pending-input callbacks,
  and user-interruption results from GIF load/save during the conversion.
  Pending keys remain for the outer input loop to handle later.
- Use giflib's scanline APIs for both reading and writing.  The UI owner
  coordinates scanline feedback while completing the entire load/save
  command, without polling or dispatching keyboard input between lines.
  Atomic means non-interruptible by input, not feedback-free.
- During loading, process each decoded scanline for pixel output and
  optional sound before reading the next.  Keep per-pixel sound mapping
  and coordinated plotting; do not replace it with one tone per line.
  No full-image buffer, second traversal, or post-close playback stage
  is required.
- During saving, update on-screen progress after each completed scanline
  and call `driver_flush()` before writing the next.  This is a display
  flush, not a per-scanline disk flush.  Preserve progress restoration
  without modifying pixels that have not yet been saved.
- Keep codec work, pixel traversal, and pixel output below UI.  The UI
  owner services sound through `g_sound`, handles playback backpressure,
  and draws/flushes save feedback between scanlines.  The codecs do not
  poll input, service audio, or invoke UI feedback callbacks.
- Keep mode selection, overwrite decisions, busy-state setup/cleanup,
  error reporting, and completion notifications around file I/O.  Neither
  a codec nor scanline feedback may enter a modal UI or pump input during
  the operation.  Sound servicing may pace loading without making it
  keyboard-interruptible.
- Preserve I/O-error handling, resource cleanup, and temporary-file
  replacement rules.  Atomic here describes control flow, not new
  transactional filesystem guarantees.  Remove keyboard-interrupted
  save/keep-partial choices, not actual read/write failure reporting.
- Preserve saving an incompletely rendered image and its resume metadata.
  An interrupted calculation is not an interrupted GIF write.

The giflib replacement slices must establish this boundary, not leave
codec polling for the later general load/save UI-ownership cleanup.
Scanline feedback is part of the run-to-completion command; it is not a
reason to restore the old keyboard polling.

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

- `libid/include/engine/StandardFractal.h` and
  `libid/engine/StandardFractal.cpp`: expose the temporary show-dot
  display bridge that will be removed when show-dot presentation moves
  fully into the UI controller.
- `libid/engine/calcfrac.cpp`: owns show-dot marker save and restore
  state and closes legacy sound output for unconverted producers.
- `libid/engine/sound.cpp`: owns speaker/file output, calls
  `driver_sound_on()`, waits through `wait_until()`, and accepts an
  input-pending callback.
- `libid/io/gifview.cpp`: `sound_line()` still drains a whole decoded
  line with `sleep_orbit_delay()`, `write_sound()`, and `driver_mute()`.
  `RestoredImageOutput` owns the decoded bytes and cursor and plots one
  pixel per step.  `GifDecoder` returns one decoded line per call and
  retains pending expansion state.  `GifRestore` owns the file and
  advances by format block, extension sub-block, or line without output
  callbacks.  The marked `gif_view()` bridge still drains it through
  legacy callbacks.  `GifDecoder::decode()` is now test-only and is
  removed by GIF Custom Decoder Removal, before the sound-controller
  slice.  The giflib conversion replaces this polling-oriented file
  traversal with atomic, scanline-based loading.  The UI owner processes
  each decoded line for pixel/sound feedback without keyboard polling
  before requesting the next line.
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

### 12.1.1. Shared GIF Extension Compatibility

Source: `io/gif_file.h`, `io/gif_file.cpp`, their existing extension tests,
and metadata compatibility cases in `tests/libid/io/test_loadfile.cpp`.

Work:

- Make the existing `get_*_info()` and `put_*_info()` functions the shared
  payload implementation for reading and writing blocks 001 and 003-007.
  Retain their field serialization, byte order, and existing tests.
- Extend the existing readers only where the production loader supports
  valid older records that they cannot yet read.  Preserve versioned
  defaults and optional-block detection; do not silently accept arbitrary
  truncation or maintain a separate legacy field decoder.
- Add the missing resume-data read/write helpers for `fractint002`
  alongside the existing helpers.  Treat its payload as opaque bytes;
  do not invent a portable resume format in this conversion.
- Reuse `add_gif_extension()` and giflib's `ExtensionBlocks` representation
  for application identifiers and sub-block splitting.  Define ownership
  and cleanup for collected blocks; no separate payload serializer or
  replacement extension container is needed.
- Add only missing legacy-record and resume-data cases.  Keep the
  existing field-level round-trip tests rather than duplicating them.

Done when:

- The existing extension helpers support the metadata compatibility
  required by production reading and writing, including resume data.

Validation: run existing extension tests and the added compatibility and
resume-data cases.  No manual test; these are deterministic format checks.

### 12.2. GIF Restore giflib Backend

Source: `io/GifRestore.h`, `io/GifRestore.cpp`, and the already linked
giflib dependency; shared extension handling in `io/gif_file.cpp`.
The production entry point reads the file as one atomic operation.

Work:

- Replace manual file parsing and custom decompression inside
  `GifRestore` with an owned giflib handle and its sequential API.
  Do not use `DGifSlurp()` or require a full-raster buffer or second pass
  for sound-on-restore feedback.
- Use record and image-header reads, `DGifGetLine()`, and extension
  sub-block reads internally.  Expose decoded scanlines to the UI owner,
  which processes feedback and continues until the file is read and
  closed before returning to input handling.  Keep ownership, completion,
  failures, and cleanup explicit and below UI; retain scanline access,
  not LZW implementation steps or keyboard-driven decoder suspension.
- Remove keyboard checks and user-cancellation returns from the entire
  load path, including indirect line/output callbacks.  Do not replace
  them with controller polling between rows or giflib records.
- Feed each decoded scanline to the existing pixel-output operation,
  including sound and image-transformation consumers.  Let the UI owner
  coordinate feedback outside the codec, without checking input.  Finish
  the current line before reusing its buffer or reading the next; do not
  defer sound until file closure or add a full-image replay stage.
- Where extension data is needed, collect giflib extension blocks for
  the existing `get_*_info()` readers.  Do not add field decoding to
  `GifRestore` or copy the metadata parser from `loadfile.cpp`.  Metadata
  discovery before restoration is migrated separately in 12.2.1.
- Preserve metadata, line-buffer lifetime, image boundaries, and
  `gif_view()` caller-visible success/failure behavior.  A synchronous
  whole-file entry is intentional, not a temporary drain to remove for
  inversion.  UI-owned scanline feedback does not change that command
  boundary.  Remove TODO markers promising keyboard-driven codec steps.
- Apply subsampling after decompression in `GifRestore`, separately
  from giflib.  Decode discarded data to advance the stream correctly.
  Retain both oversized-image fitting and reduced-view-window loading;
  their eventual wxWidgets replacement is not part of this change.
- Preserve 16-bit potential color/low-byte line grouping before applying
  skip factors.  giflib accepts requested pixel counts shorter than a
  stored row, so this does not require custom LZW handling.
- Let giflib consume compressed image termination, including the empty
  data sub-block.  Remove the corresponding manual byte read.  Account
  for its final pixel read flushing remaining compressed sub-blocks;
  do not promise a strict byte budget inside a library call.
- Preserve current palette selection, MIG placement, and non-interlaced
  restore behavior.  Do not combine new GIF rendering features with the
  decoder replacement.
- Review malformed-input differences explicitly.  The custom decoder
  accepts minimum LZW code size 9; the bundled giflib rejects values
  above 8.  Do not reproduce this or other fallback behavior solely to
  preserve synthetic tests.  Establish a real-file requirement before
  adding any compatibility workaround, and document intentional changes
  in the affected tests.
- Keep genuine success and failure reporting compatible with callers;
  translate giflib results without moving diagnostic UI into the parser.
  Leave the unused custom decoder for the following removal slice.
- Test a complete load with pending input using the mock driver: no
  keyboard operation is called, no key is consumed, and decoded output
  is complete.  Cover indirect output paths as well as the codec itself.

Done when:

- Production GIF reading uses giflib atomically, without direct or
  indirect keyboard polling, and retains the two reduction features.
- Pixel output and optional sound consume each scanline during loading,
  before the next is read; no full-image replay buffer is required.

Validation: run the existing restore compatibility, focused failure and
lifetime tests, and the full `rt-default` workflow.  No manual test.

### 12.2.1. Production GIF Metadata Reader Integration

Source: `find_fractal_info()`, `load_ext_blk()`, and `skip_ext_blk()` in
`io/loadfile.cpp`; the shared extension readers in `io/gif_file.cpp`.

Work:

- Retain the `find_fractal_info()` caller contract while replacing its
  raw extension parsing and packed-structure decoding with the existing
  `get_*_info()` readers, including the resume-data helper from 12.1.1.
- Use giflib record reads to collect metadata before pixel restoration.
  Skip compressed image sub-blocks without decoding or buffering the
  raster; do not use `DGifSlurp()` merely to obtain metadata.  Run this
  metadata operation to completion without keyboard polling as well.
- Map the decoded values into the existing load results, preserving
  optional-block presence, plain-GIF defaults, older-record defaults,
  dimensions, palette requirements, and existing version migrations.
  Keep UI choices and applying loaded engine settings outside the codec.
- Share extension collection with the restore backend where needed;
  neither path may implement another field decoder.  Keep file and
  extension-buffer ownership explicit and release them on every exit.
- Remove the replaced raw parsing helpers when unused.  Retain byte-order
  helpers still used by the old writer until its conversion; do not
  preserve a second metadata reader as a runtime fallback or test oracle.

Done when:

- Production metadata loading and the existing giflib utilities use the
  same extension readers without requiring full raster decompression.

Validation: run extension, metadata-loader, and restore compatibility
tests, including older Fractint fixtures, and the full `rt-default`
workflow.  No manual test.

### 12.3. GIF Custom Decoder Removal

Source: `io/GifDecoder.cpp`, `io/GifDecoder.h`,
`tests/libid/io/test_GifDecoder.cpp`, and their CMake registrations.

Work:

- Remove `GifDecoder`, its test-only synchronous `decode()` drain, and
  the custom bit reader, dictionary, and prefix-expansion implementation.
  Remove their source and test registrations from CMake.
- Retain or migrate externally observable compatibility cases into
  `TestGifRestore`, reusing its existing coverage.  Remove tests whose
  only purpose is checking implementation details of the old LZW code.
- Do not keep an unused decoder facade or fallback merely to satisfy
  old tests.  Retain `g_out_line` and its declarations while the
  non-polling `gif_view()` consumers still need them; remove obsolete
  public step/drain interfaces that have no remaining production use.
- Remove stale custom-decoder references from the remaining plan.  Keep
  sound-controller work and the general image-loading UI split separate.

Done when:

- No custom GIF decompression implementation or test-only compatibility
  entry remains; restore behavior is covered at the giflib-backed
  `GifRestore` boundary.

Validation: run focused restore tests and the full `rt-default` workflow.
No manual test.

### 12.4. GIF Writer Raster Compatibility Coverage

Source: `tests/libid/io/test_encoder.cpp`, existing save/image fixtures,
and raster, palette, and descriptor output in `io/encoder.cpp`.

Work:

- Exercise the current production save path with deterministic pixel
  sources.  Reuse existing image coverage and add only missing cases.
- Identify old keyboard-interruption cases for replacement in 12.7,
  where pending input must not stop encoding or consume a key.  Add the
  changed-behavior assertions with that conversion, not as failing tests
  against the current backend.  Keep genuine I/O-error coverage here.
- Check decoded pixel indices, logical screen and image dimensions,
  GIF89a version, background index, color resolution, and aspect byte.
- Cover 2-, 4-, 16-, and 256-color palette selection, including fixed
  fallback palettes and the active DAC palette where applicable.
- Cover unsigned dimensions above 32767 and existing GIF size limits.
  Check rejection of an unrepresentable doubled potential width.
- Cover 16-bit potential output with distinct color and low-byte data:
  each stored row has double width, with colors followed by low bytes.
- Compare decoded content and metadata, not compressed LZW bytes, file
  sizes, or compression ratios.  Do not replace gold images to conceal
  a codec-induced pixel difference.

Done when:

- The existing writer passes backend-independent raster and header
  compatibility tests suitable for the giflib replacement.

Validation: run focused writer tests against the current implementation
and reuse existing image tests.  No manual image comparison.

### 12.5. GIF Writer Extension Compatibility Coverage

Source: `encoder()`, `put_extend_blk()`, `extend_blk_len()`, and
`store_item_name()` in `io/encoder.cpp`; the shared `get_*_info()` and
`put_*_info()` helpers in `io/gif_file.cpp`, their existing tests, and
production metadata loading in `io/loadfile.cpp`.

Work:

- Test extensions emitted by the production save path, reusing existing
  payload serializer tests rather than duplicating every field check.
  Keep known fixture expectations as well as round trips through the
  shared readers; a matching reader/writer mistake must not pass unnoticed.
- Cover the main fractal record, resume data, formula/L-system/IFS names
  and flags, iteration ranges, arbitrary-precision parameters, evolution
  state, and orbit settings.  Verify omission when a block is inapplicable.
- Preserve application identifiers, little-endian payload encoding,
  block order, and the final placement of `fractint001`.
- Check empty payload handling and lengths around the 255-byte
  sub-block boundary.  Verify `tot_extend_len` against the actual encoded
  extension layout, including identifiers and terminators.
- Read saved files through `find_fractal_info()` using the shared
  extension readers from 12.2.1, and inspect giflib extension records.
  Preserve the layout required by older readers' backward metadata
  search even though production loading no longer depends on that search.

Done when:

- Application-specific save metadata is covered independently of the
  custom GIF framing and compression implementation.

Validation: run focused writer, extension, and metadata-loader tests.
No manual test; these are deterministic file-format checks.

### 12.6. giflib Writer Operations

Source: new `io/GifWriter.h` and `io/GifWriter.cpp`; giflib output APIs
and extension helpers in `io/gif_file.cpp`; new focused writer tests.

Work:

- Introduce `GifWriter` to own an output handle and its lifecycle.  Its
  operations accept dimensions, palette data, pixel rows, and extension
  blocks assembled by the existing `put_*_info()` helpers.  It must not
  read engine/UI globals or introduce another metadata serializer.
- Use giflib screen/image descriptors, `EGifPutLine()`, and extension
  leader/block/trailer operations.  Do not use `EGifSpew()` or collect an
  entire raster before writing it.
- The existing `put_*_info()` helpers append to giflib's in-memory
  `ExtensionBlocks`; they do not write a file.  Emit those blocks with
  sequential giflib operations, preserving application/continuation
  grouping and terminating each extension once.  Reuse their identifiers
  and chunking rather than splitting raw payloads a second time.
- Preserve the tested GIF89a header and palette fields, including the
  aspect byte.  Keep payload serialization in `gif_file.cpp`; the writer
  owns only file framing, output, and lifecycle.
- Permit row-sized pixel writes, including the two half-row writes used
  for potential output, without embedding pixel-source or disk-video
  traversal policy in the writer.
- Make finish and cleanup explicit.  giflib owns GIF termination and
  handle closure; do not write a second trailer or close a handle twice.
  Report open, row, extension, finalization, and close failures.  Row
  completion permits UI-owned feedback, not keyboard polling or dispatch.
- Test deterministic write failures and early destruction as well as
  successful files.  Keep prompts, progress markers, input polling,
  completion sounds, and file-retention choices out of this class.

Done when:

- A giflib-backed writer can produce and finalize rows and application
  records independently of the synchronous application save loop.

Validation: read produced files with giflib and the existing metadata
loader; test resource lifetime and failure paths.  No manual test.

### 12.7. Production GIF Writer Integration

Source: `save_image()`, `encoder()`, `compress()`, and extension emission
in `io/encoder.cpp`; `GifWriter` from 12.6 and shared `put_*_info()`
helpers in `io/gif_file.cpp`.

Work:

- Route production saves through one `GifWriter`.  Replace manual GIF
  headers, palettes, image descriptors, LZW output, extension framing,
  and termination with its operations.  The caller performs one atomic
  save through file finalization and closure, coordinating feedback per
  completed scanline without yielding to keyboard input.
- Remove keyboard polling, pending-key callbacks, and user-interruption
  returns throughout encoding and its output path.  Do not preserve a
  keep/delete-partial prompt for a keypress during GIF writing.
- Retain existing pixel-source traversal and potential color/low-byte
  ordering while submitting rows to the writer.  Preserve metadata
  assembly from current settings, but serialize it with the existing
  `put_fractal_info()`, `put_formula_info()`, `put_ranges_info()`,
  `put_extended_param_info()`, `put_evolution_info()`, and
  `put_orbits_info()`, plus the resume-data helper from 12.1.1.
- Replace the legacy writer's packed-structure encoding and raw payload
  handling with those shared helpers.  Do not retain that second
  serialization path behind `GifWriter` or copy it into the new class.
- Preserve application identifiers, extension ordering, and accurate
  `tot_extend_len`, calculated from the assembled extension blocks and
  their framing.  Set the final main record's length before serializing
  it.  Let giflib emit the blocks; do not mix raw writes with its output.
- Establish one owner for the output handle.  Remove overlapping
  `fopen()`/`fclose()` ownership.  Return write and close failures before
  a temporary save replaces an original.  Do not introduce an active
  global writer or raw-file bypass.
- Keep filename selection, overwrite rules, temporary-file replacement,
  autosave, and genuine I/O-error results compatible.  Saving a partial
  calculation must still write a complete GIF with its resume metadata;
  this does not justify interruptible encoding or synthetic padding.
- Preserve existing save progress geometry, drawing/restoration, and
  busy-state behavior through UI-owned scanline feedback, not inside
  `GifWriter`.  Capture/write that scanline's pixels before drawing its
  progress marks so feedback cannot corrupt saved data.
- After each completed scanline, draw progress and call `driver_flush()`
  before advancing to the next scanline.  For potential output, preserve
  the color/low-byte grouping and advance progress once the stored row
  is complete.  Do not add a disk flush or input poll to this display
  update.  Restore progress marks and flush on completion or failure.
- Do not label the synchronous whole-file save as a temporary drain or
  treat scanline feedback as permission to poll input.  Mark only any
  remaining misplaced prompt/notification bridges for removal in Image
  Saving UI Decisions; the atomic command boundary is permanent.
- Leave unused custom compression code for the following removal slice;
  do not retain it as a runtime fallback.  Retain `s_outfile` only if
  those unused definitions still need it to compile, mark it temporary,
  and remove it in 12.8; it must not own a production output file.
- Verify with pending input and mock drivers that all pixels and metadata
  are written without keyboard calls.  Cover success, write failure,
  close failure, autosave, and saving a resumable calculation.
- Test scanline-write, progress-draw, and display-flush ordering, including
  potential output and progress restoration on success/failure.  Check
  that progress cannot change the encoded pixels or consume pending keys.

Done when:

- Every production image save uses giflib while retaining the tested
  raster, metadata, file-management behavior, and per-scanline on-screen
  progress with a display flush, without polling input or returning a
  keyboard-interruption status from encoding.

Validation: run 12.4/12.5 coverage, writer failure/lifetime tests, existing
save/autosave coverage, and the full `rt-default` workflow.  No manual
image comparison; add deterministic tests for uncovered changed paths.

### 12.8. GIF Custom Encoder Removal

Source: unused compressor state and `compress()`, `output()`,
`cl_block()`, `char_out()`, and `flush_char()` in `io/encoder.cpp`;
obsolete framing helpers and tests; `hc/src/help.src`.

Work:

- Remove the unused custom LZW encoder, hash/code tables, bit packing,
  byte accumulator, `s_outfile`, and obsolete GIF framing helpers and
  declarations.
- Remove `write_gif_uint16()` and its implementation-specific tests if
  it has no remaining callers; retain dimension-boundary coverage at
  `GifWriter` and production save boundaries.
- Keep the shared extension serializers in `gif_file.cpp` and helpers
  still required for metadata lengths.  Remove the superseded packed
  structure encode/decode helpers, tests, and registrations only after
  verifying that no callers remain on either the read or write path.
- Audit `g_block` after the metadata-reader conversion and remove it if
  unused.  Do not retain it merely because the old loader once used it,
  or delete application data that still has another caller.
- Remove stale custom-writer references and migration bridges that are
  no longer called.  Keep the atomic save command and UI-owned scanline
  feedback; later UI cleanup must not reintroduce keyboard polling.
  Do not leave test-only compressor facades, obsolete interruption flags,
  partial-encoding prompts, or key-polling test expectations.
- Add a current-release code-cleanup changelog item covering the giflib
  reader/writer conversion.  Do not alter historical release notes.

Done when:

- Neither production GIF reading nor writing uses a custom LZW codec.
  Both use the existing shared extension readers/writers, with no
  duplicate payload codec or keyboard polling.  Whole-file I/O is atomic;
  metadata, resumable images, and reduction features remain supported.
- This completes the reader/writer conversion requested by
  [issue #270][270].

Validation: run focused writer/restore/metadata tests and the full
`rt-default` workflow.  No manual test.

[270]: https://github.com/LegalizeAdulthood/iterated-dynamics/issues/270

### 13. Restored Image Sound Controller

Source: scanlines from `GifRestore` and `RestoredImageOutput`.
UI owner: the sound-restoration branch in `ui/big_while_loop.cpp`.

Work:

- Have the UI owner read one scanline, process its pixel/sound output,
  then request the next until the entire GIF is loaded and closed.  Keep
  the current line valid until its output completes; do not buffer the
  full image or add a second playback traversal after file closure.
- Submit the current line's pixel-value samples from
  `RestoredImageOutput`.  Keep palette-index frequency mapping in the
  adapter and the existing pixel order; scanline feedback does not mean
  reducing a line to one tone.
- Have UI advance pixel output as playback permits, servicing `update()`
  while presenting and waiting.  Retain a refused pixel until accepted,
  then plot it exactly once before advancing.  Keep sound waits outside
  the codec and pixel-output operation.
- Complete this scanline loop without keyboard polling, key consumption,
  or dispatch, including while waiting for sound readiness.  The file can
  remain open during sound feedback; loading is still one atomic command
  with respect to input.  Finish with normal release, not mute.
- Remove `RestoredImageOutput::pending_color()` and the temporary pixel
  drains from the sound path, plus `sleep_orbit_delay()`, `write_sound()`,
  and direct mute calls.  Submit the retained pixel's sample inside
  `RestoredImageOutput::iterate()` before plotting it.
  Keep any remaining UI-ownership compatibility paths separate from the
  permanent atomic file operation.

Done when:

- Sound restoration processes each decoded scanline while loading, with
  plotting and sound coordinated and refused pixels retained until
  accepted.  The codec and pixel-output operation do not wait on playback;
  their UI owner does not poll input during the atomic load.

Validation: cover sample refusal, line continuation, read/output failure,
and legacy sound-file records with fake-port/controller tests.  Verify
each line's pixel/sound output completes before the next decode, no
sample or plot is repeated, and pending input neither interrupts loading
nor is consumed.  Exercise sound-restore selection through its UI entry
as well as the traversal object.  Do not repeat image-restoration tests
manually.

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
  output paths.  Remaining GIF UI-ownership bridges are handled by Image
  Loading UI Decisions, without making file I/O incremental again.
  Do not add a global forwarding API around `SoundOutput`.
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
  before invoking atomic GIF reading or subsequent render operations.
- Preserve cancellation of setup dialogs, batch defaults, and prior-image
  state on setup/read failure.  There is no mid-file keyboard cancellation.
- Remove remaining UI calls and ownership bridges around whole-file
  reading.  Do not replace the atomic operation with a keyboard-polling
  controller around decoder records or rows.
- Coordinate existing pixel/transformation output and optional sound for
  each decoded scanline before requesting the next.  Keep that feedback
  outside the codec, but within the atomic load command; do not require
  full-image retention or post-close playback.  Split distinct consumer
  conversions if needed without reintroducing keyboard polling.

Done when:

- Image-loading code does not invoke `get_video_mode()`,
  `get_3d_params()`, or modal diagnostics from inside decoding setup.
- The UI completes scanline-based loading and feedback as one atomic
  command, without load-time input polling or a second image traversal.

Validation: cover cancelled selection, invalid metadata, and successful
load through existing codec tests and focused UI-choice tests.

### 43. Image Saving UI Decisions

Source: remaining UI decisions around `save_image()`/`encoder()` in
`io/encoder.cpp` and the giflib-backed `GifWriter` from 12.6-12.8;
UI owners: manual save and autosave paths in `ui/big_while_loop.cpp`.

Work:

- Return completed-write or I/O-failure results to the UI owner before
  displaying diagnostics or making any applicable file-retention choice.
  Do not recreate the removed keyboard-interrupted save/partial prompt.
- Let UI own saved/error messages, completion sounds, and any
  save-then-exit decision.
- Keep save progress drawing/restoration and busy-state presentation in
  UI.  Retain the scanline feedback established by the giflib conversion:
  after a scanline is written, update progress and call `driver_flush()`
  before writing the next.  Flush restored feedback on completion or
  failure as well; a display flush must not poll or dispatch input.
- Remove remaining marked prompt/notification bridges, not the permanent
  synchronous command or its scanline feedback.  Keep pixel-source
  traversal and encoding below UI; no per-row disk flush is required.
- Keep encoding, file replacement mechanics, and resource cleanup in I/O;
  the selected UI decision determines when those mechanics run.

Done when:

- Encoding and save-file operations cannot open prompts or terminate the
  application.  Whole-file writing remains atomic and reports real I/O
  failures; it cannot be interrupted by pending keyboard input.

Validation: exercise saving incomplete/resumable calculations, overwrite
and failure cleanup for new/existing files, write/close failures, and
autosave completion.  Use codec tests for file contents and pending-key
behavior and mock-driver tests for per-scanline progress/flush ordering
instead of manual image comparisons.

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
  sound wait from synchronous compatibility line callbacks.  Bounded
  file traversal currently exists through `GifRestore`; the giflib
  slices replace polling-oriented file traversal with atomic loading.
  The UI owner coordinates pixel/sound feedback on each decoded scanline
  during that command, without codec-side waits or keyboard polling.
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
  The giflib slices remove codec polling and keyboard-interrupted saves;
  later UI cleanup moves the remaining surrounding decisions outward.
- `libid/engine/cmdfiles.cpp`: `arg_error()` calls UI through
  `s_stop_msg` and `s_goodbye`; `init_msg()` displays startup errors and
  schedules `do_pause()`.  Other command branches generate UI output or
  exit.  These are live behaviors, not merely comments about old code.

The numbered slices cover all these ownership gaps.  The source audit
must trace UI helper and dispatch-pointer calls as well as raw driver
calls; an empty raw-key search alone does not establish inversion.

The `OrbitPlot` wait loop, delay state, and sound mapping have been
removed.  It submits numerical samples through `g_sound`; standard and
FrothyBasin UI controllers own sound servicing and session completion.

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
