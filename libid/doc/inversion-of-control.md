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

Cellular exposes initial-generation preparation as calculation state;
its UI controller owns thinking-screen updates and cleanup.  L-system
sizing and drawing use explicit frame stacks and advance incrementally.
Its UI controller owns the thinking screen and interruption in both
phases; sizing and drawing do not poll input.

JIIM's `InverseJulia` calculation owns orbit state and accepts explicit
point and plot inputs.  `InverseJuliaController` in
`libid/ui/inverse_julia.cpp` owns the outer interaction loop, cursor,
coordinate messages, mode preferences, and keyboard and mouse scopes.
`InverseJulia::iterate()` only calculates and plots bounded orbit work.

## Target Shape

The standard escape-time calculation object already has this lifecycle:

```cpp
namespace id::engine
{
class StandardFractal
{
public:
    std::vector<std::string> resume();
    void suspend();
    void finish();
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
- `StandardFractal` exposes separate `select_pixel()`, `selected_pixel()`,
  and `calculate_pixel()` operations.  It owns calculation dispatch and
  retains an unfinished pixel without delivering a sentinel to the pass.
  `iterate()` batches those operations at existing work checkpoints;
  callers may also observe selected coordinates before calculation.
- `StandardPass` routes pixel selection, selected coordinates, and color
  completion through its existing variant without calculation dispatch.
  Synchronous and sticky orbit work retain their incremental paths.  The
  pixel passes expose chunk preparation and completion state, not
  calculation bridges.  Standard and orbit strategy selection stay in
  `StandardFractal`.
- Standard potential initialization returns a storage requirement before
  creating the work list.  The standard UI controller prepares storage
  and reports availability; the calculation retains one-pass selection
  and failure fallback.  Resume reuses its existing storage.  Potential
  high-byte output uses the selected pixel backend, not a driver query.
- Tesseral is a concrete `StandardPass` alternative.  `Tesseral` has
  `iterate()`, `done()`, and `suspend()`, and owns its box stack.
  Initial edge setup, later edge probes, and box filling run through
  bounded, resumable state.  Tesseral suspend records the active box and
  restarts initial edge setup if interruption happens during that phase.
  Selection retains probe orientation, coordinates, and accumulated color
  until completion.  Existing edge, box, and fill chunks are retained
  through the common `StandardFractal` pixel boundary.
- The `OnePass` and `TwoPass` paths share `ScanlinePass`; they preserve
  scan position in the concrete pass object and yield standard pixel work
  back to the UI.  Their `suspend()` records resume work at the retained
  cursor.  Standard pixel kernels no longer poll for interruption;
  optimized Mandelbrot and general pixel work yield at existing work
  checkpoints, while Lyapunov returns between scanline pixels.  They
  expose separate `select_pixel()`, `selected_pixel()`, and
  `complete_pixel(color)` operations.  Selection retains coordinates
  without calculating or plotting; completion advances traversal and
  copies sparse first-pass results.  `StandardFractal` performs batched
  calculation at the original yield checkpoints.
- `BoundaryTrace` is a concrete `StandardPass` alternative.  It owns the
  outer scan cursor, outline tracing state, region filling state, and
  suspend path.  Selection retains scan or outline coordinates until
  color completion; cached lookups and region fills do not request orbit
  calculation.  Scan and outline chunk counters persist across requests.
  `StandardFractal` performs calculation and retains the scan and outline
  yield checkpoints.  Invalid options still invoke a modal `stop_msg()`
  from pass initialization.
- `Diffusion` owns its counter, tile traversal, and fill state.  Its
  explicit phases advance in bounded counter chunks and suspend through
  `StandardPass`.  Selection retains the active tile coordinate until
  color completion; completion plots the point or inferred block before
  advancing the tile and counter.  `StandardFractal` keeps calculation
  batched in the existing 512-counter chunks.  Saved resume work
  still restarts the retained counter's tile group.
- `SolidGuess` owns first-pass row traversal, remaining pass traversal,
  `guess_row()` block state, and row repaint state.  Its explicit phases
  yield at row boundaries without polling keyboard input.  Seed rows and
  block probes expose selection and color completion, retaining pending
  coordinates, neighboring colors, and repair continuation.  Completed
  colors drive guessing, prefix updates, and repainting once.  Selection
  does not calculate pixels.  `StandardFractal` batches calculations at
  the existing row boundaries.
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
  `StandardPass` traversal.  `StandardPass` owns all pixel traversal.
  `PertEngine` always computes the requested pixel, using cached reference
  orbits.  On reference failure it selects a closer cached reference or
  adds one at that pixel.  No deferred-pixel queue or retry traversal
  remains.  BoundaryTrace can use perturbation and receives the same
  nonzero palette color that is stored in its image cache.
- `CommandResult` already separates menu command effects from the
  `MainState` application-phase switch.  Some effects still travel through
  `g_calc_status` and `MainContext::more_keys` side effects.
- `engine_timer` and benchmarking machinery have been removed.  Explicit
  engine timing still owns total image calculation time, including
  suspension and completion accounting through `g_calc_time`.
- `ShowDotController` owns marker geometry, saved pixels, symmetry
  copies, restoration, and delay in the standard and Lyapunov UI
  controllers.  It restores the image before calculation or traversal
  continues, and does not repeat marker display for an unfinished pixel.
  Zero-delay work retains calculation batches.  Engine code has no marker
  display bridge, dispatch wrapper, or marker-specific work scheduling.
- `Orbit2D`, `Orbit3D`, and `Dynamic2D` submit numerical samples through
  `g_sound`.  Their UI controllers service playback and mute on
  interruption.  Nested producer scopes borrow the same output; the UI
  render-operation owner finishes it once across evolution cells and
  stereo views.  Engine teardown no longer closes sound.  Pending points
  retain calculation state without changing ready-output polling cadence.
- Legacy speaker/file routines are gone.  Only `ui/DriverSoundOutput.cpp`
  calls continuous-tone driver methods.  Orbit cleanup mutes the bound
  output port without closing it; sound parameters and completion-buzzer
  settings retain their existing behavior.
- `engine/spindac.cpp` rotates and writes the palette without delaying.
  `ui/rotate.cpp` owns delay after cycling and single-step operations;
  ordinary refreshes and pause-border updates do not acquire that delay.
- `ui/stop_msg.cpp` owns the startup-error display's ten-second hold.
  Legacy engine wait helpers and the image-setup timer reset are gone.
- Standard image setup returns recoverable log-map warnings and throws
  on required-table allocation failure.  BoundaryTrace validation throws
  without polling or completing an invalid pass.  The UI reports these
  diagnostics; failed standard and Lyapunov operations discard work and
  restore dispatch.  Invalid batch renders report failure without saving.
  The temporary `ui/calculate_image.cpp` boundary remains for slice 52.
- `ifs_load()` throws diagnostics without UI, closes rejected files,
  and publishes coefficients only after validating the definition.
  Selection reports the failure once and retries locally; rendering
  reports it through the outer UI diagnostic boundary.  IFS lookup uses
  non-interactive `find_file_item()`.  Entry lookup and scanning return
  diagnostics; UI selection and preview callers own their presentation.
- `lsystem_load()` throws parser and lookup diagnostics without UI and
  closes definition files on failure.  A failed load clears cached rules
  and the axiom, and sizing refuses to start without a valid definition.
  Selection reports errors and retries; render failures use the outer UI
  diagnostic boundary without starting sizing or drawing.
- Formula lookup, parameter inspection, prescan, and compilation throw
  diagnostics without UI.  Source lines, expressions, and carets remain
  in parser errors; recoverable warnings are returned to the caller.
  Failed compilation clears the program and leaves safe dispatch in
  place.  Selection displays diagnostics and retries locally; parameter
  inspection and render setup report through their UI owners.  Open
  definition and debug files close on early failure.
- `Lyapunov::resume()` returns the unsupported-inside warning to its UI
  controller before iteration.  Per-image setup still applies the same
  numeric-color fallback after legacy option normalization.  Julibrot's
  low-color check throws before initialization; the outer UI diagnostic
  boundary reports the failure without starting the 4D renderer.
- Palette loaders return diagnostics without UI calls.  Missing files
  preserve palette data; malformed maps retain their valid prefix and
  fill the remainder gray, with the same compatibility conversion.
  Selection, command input, and render controllers report diagnostics.
  Julibrot and stereo controllers own their palette preparation;
  FrothyBasin reports an optional-map failure and continues with the
  prior palette.  Palette-selection failures retry in their UI context.
- `geometry/line3d.cpp` returns scanline debug warnings and reports file
  failures without modal UI.  The restore adapter owns acknowledgement,
  cleanup before failure display, and batch abandonment.  Targa setup
  returns diagnostics; image setup retains palette-rendering fallback.
  Rejected overlays close their sources and cannot trigger replacement
  of the original without a completed temporary copy.  File preparation
  returns a Targa storage request before rendering.  Restore and image UI
  controllers prepare backing storage; true-color setup completes with
  that outcome.  Mapping-line status belongs to the restore adapter.
- `math/biginit.cpp` reports precision and arena exhaustion through
  `PrecisionError`, without input or application shutdown.  Rejected
  precision requests preserve the active arena; failed stack requests
  leave its allocation state unchanged.  Render cleanup runs before the
  failure reaches `id_main()`, which owns the existing fatal diagnostic
  and batch exit status.  Freed arenas can be initialized again.
- `engine/fractalb.cpp` formats precision debug text without modal UI.
  Debug checks collect snapshots before returning to their callers;
  `id_main()`, image setup, and the standard render controller own their
  display and Escape-to-exit decision.  The conversion round-trip check
  restores the image corners before UI inspection, including on failure.
- `misc/memory.cpp` reports resource and backing-file failures through
  `MemoryError`; invalid requests and transfers throw `std::logic_error`.
  It owns allocation, backing paths, and cleanup without UI callbacks.
  `exit_check()` returns diagnostics after releasing resources; shutdown
  displays them.  Storage setup in `ui/diskvid.cpp` owns allocation
  retry/cancel and debug statistics directly; the temporary
  `request_memory()` helper is gone.  Disk cleanup releases resources
  before propagating a transfer failure.  Targa allocations include cache
  padding after the header, preserving bounded backing-store transfers.
- `engine/pixel.cpp` owns pixel/span access, coordinate offsets, color
  masking, and the selected pixel functions.  UI owns activation of the
  driver, disk, or null target.  Spans use those pixel functions directly,
  without separate initialization.  `io/decoder.cpp` owns `out_line()`
  and its row counter.  Calculation and codec callers no longer include
  `ui/video.h` for pixel access.  Disk targets select I/O pixel operations
  directly, without per-pixel UI progress callbacks.
- `io/disk_storage.cpp` owns cache state, packed pixel and byte access,
  backing handles, Targa headers, and cleanup.  Backend operations have
  no UI or driver calls; they report read/write progress as data and
  propagate failures.  Failed setup releases backing resources before
  returning to UI.  Active Targa backing paths are retained independently
  of the next requested filename.  Pixel and Targa callers use the I/O
  operations directly.  UI controllers service progress at existing work
  checkpoints, retaining the status throttle without adding yields or
  input polling.  UI storage preparation owns allocation retry/cancel;
  the temporary setup bridges are gone.  Prepared Targa requests carry
  the file path, header, and dimensions without invoking storage UI.
- Plasma construction normalizes configuration without allocating storage
  or plotting pixels.  `NonStandardFractal` selects and owns the object;
  its UI controller prepares requested potential storage and reports
  availability through `complete_potential_storage()`.  Calculation
  retains initialization, border handling, 8-bit fallback, and saved
  potential-state restoration; pixel output identifies disk targets
  without querying the driver.
- Inverse-Julia queue initialization accepts prepared backing memory.
  Both requesting UI controllers own disk-video rejection, smaller-queue
  retries, cancellation, and fallback.  Standalone setup separates
  parameter normalization from queue seeding; calculation code retains
  byte transfers, queue indices, orbit math, and cleanup without prompts.
- Potential-image restoration prepares storage once in its existing UI
  path, retaining setup-failure fallback there.  `pot_line()` requires
  prepared storage and writes high/low planes with the existing offsets;
  it neither allocates storage nor queries the display driver.  UI owns
  output-target selection and storage cleanup.
- `ui/restore_image.cpp` drives `GifRestore` scanlines and services sound
  through `SoundSession` with atomic input policy.  `RestoredImageOutput`
  submits each pixel value through `g_sound`, retaining refused pixels
  and plotting accepted pixels once.  Each line's playback completes
  before the next decode; loading and normal completion do not poll input
  or mute.  Pixel helpers remain in `io/gifview.cpp`.

The fresh reverse-call audit also found these live paths:

- `calc_fract()` calls UI render controllers directly.
  `NonStandardFractal::render_legacy()` also reaches UI render loops
  through dispatch function pointers stored in `FractalSpecific`.
  Per-image callbacks perform setup only; their renderer-call bridge
  and nested timer brackets have been removed.
- `ifs_type()` selects UI renderers, while `orbit3d_type()` and the IFS3D
  path pass UI render functions to `funny_glasses_call()`.  That function
  also invokes photographer prompts between stereo images.
- Command validation can still invoke modal `stop_msg()` or application
  shutdown through `goodbye()`.
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
true-color setup and teardown, parser setup, log-map setup
and cleanup, orbit display defaults, inversion setup, resume cleanup,
calculation-time accounting, work-area cleanup, and disk image cleanup.

`StandardFractal` and the legacy `NonStandardFractal` use that shared
implementation directly.  Lyapunov uses it through its contained standard
calculation.  Standard resume performs common setup; bounded traversal
retains its existing return points.  Suspension, completion, and failure
cleanup finalize each elapsed interval once.  UI controllers acknowledge
setup warnings and prepare requested storage without owning calculation
setup, timing, or cleanup.

The concrete render operation objects are:

- `engine::StandardFractal`: owns standard escape-time traversal,
  `StandardPass`, standard pixel state, perturbation, pass dispatch,
  standard orbit-plot requests, and standard suspend/resume state.
- `fractals::Lyapunov`: owns Lyapunov render state by containing an
  `engine::StandardFractal`; Lyapunov-specific per-image setup remains in
  `lyapunov_per_image()` below the dispatch layer.
- `engine::NonStandardFractal`: owns non-standard dispatch setup,
  symmetry defaults, plot defaults, `per_image()` invocation, and final
  status interpretation.  It shares common image setup, timing, and
  cleanup, but its marked dispatch still enters whole UI render loops.
  Its bounded operation router owns bifurcation, Popcorn, standalone
  Diffusion, TestPoint, Ant, Cellular, Plasma, L-system, and FrothyBasin
  state without renderer callbacks.  Their UI controllers advance
  selected objects; setup, resume, and finish stay below UI.  L-system
  definition loading, startup, and terminal interpretation stay below
  UI; its controller retains thinking-screen handling and the drawing
  poll throttle.  FrothyBasin retains its orbit and pixel checkpoints,
  palette selection, and resume coordinates below UI; its controller
  owns palette diagnostics, overlay interaction, and sound servicing.
  The completed-image orbit tool uses setup-only FrothyBasin callbacks,
  independently of the selected image calculation's lifetime.
  Plasma's controller reports storage availability before engine
  initialization.  Cellular retains generation-sized preparation and
  next-screen initialization below UI; its thinking-screen display stays
  in the controller.  Popcorn's Julia setup and symmetry selection also
  stay below UI.  Ant's Ctrl+A entry selects an operation on the existing
  image, without restarting common image setup, timing, or resource
  cleanup.
  Per-image callbacks no longer invoke renderers.  IFS has no per-image
  callback; the marked legacy entry calls its renderer once without
  applying standard symmetry, and stereo skips absent second-eye setup.
  Bounded operations cannot enter that legacy rendering entry.

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
still contains UI renderer pointers used by the marked legacy entry;
IFS and 3D front ends call UI controllers from `fractals/lorenz.cpp`.
Per-image callbacks no longer invoke those renderers.  A top-level-only
change does not eliminate the remaining nested UI calls.

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

- `libid/fractals/lorenz.cpp`: IFS and 3D front ends invoke UI renderers;
  stereo orchestration invokes photographer prompts between images.
- Calculation setup still invokes `stop_msg()` and sometimes `goodbye()`.
  These are input and application-control leaks even when reachable only
  on failure or in debug builds.  Memory operations and command
  diagnostics now report failures without UI or shutdown callbacks.

Command parsing returns diagnostics, generation requests, and the need
for startup file-entry selection.  UI callers own action execution,
selection cancellation, device initialization, and application exits.
Image-save feedback and policy live in `ui/save_image.cpp`, which drives
scanline encoding through `io/ImageSave`.  Image-load setup choices live
in `ui/restore_image.cpp`.  Video selection, 3D settings, and Tab
information use pending values; the active image and stacked screen
remain unchanged when setup is cancelled.

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

Validation has three complementary layers: image or semantic tests protect
calculation, autokey scripts protect real application interaction, and
controller tests cover precise transitions and injected failures.  A
grep-based test is not a substitute for exercising a callback path.
Source searches remain audit tools, not repository tests.

## Validation Audit

Existing coverage does not imply coverage of live interaction:

- `tests/cmake/AddImageTest.cmake` runs images with `batch=yes` and
  `sound=off`.  Pass, cellular, orbit, IFS, and restoration fixtures
  protect output, not calculation input, transient markers, or playback.
  `ImageTest.rotate` tests geometric rotation, not palette cycling.
- `tests/autokey/CMakeLists.txt` covers parameter/MIG generation, 3D-look
  Targa output, random-dot stereograms, palette cycling, standard
  one/two-pass interruption, and arbitrary-precision pass switching.
  Standard scripts save a resumable checkpoint, invoke `x` and Esc, then
  compare completed pixels exactly with an uninterrupted control for
  optimized and general Mandelbrot paths.
  Palette scripts verify cycling-context saves, single stepping, failed
  map selection, malformed-map warnings, valid-map retry, and return to
  the image.  Pass-switch scripts select BoundaryTrace and
  solid guessing through `x`, verify saved pass and precision metadata,
  and compare pixels exactly with fresh renders for `perturbation=auto`
  and `no`.  The show-dot script uses `ROWWAIT` to cross row boundaries,
  saves interrupted checkpoints around menu cancellation, and verifies
  the Maximum Iterations restart against a fresh render.  These scripts
  do not exercise the stereo photographer prompt.
  The cellular script delivers its interrupt only while the preparation
  thinking screen is active, saves a non-resumable checkpoint, then
  starts generation-zero rendering and compares it with a fresh image.
  The L-system script interrupts the sizing thinking screen, returns
  through the outer menu, and starts a completed drawing at a new order.
  Its validation script rejects an invalid definition, acknowledges the
  error, selects a valid definition, and verifies completed drawing.
  Cancelling another invalid selection preserves that completed image.
  Batch checks require one diagnostic and no saved image for rejected
  definitions.  These checks use metadata and blank/nonblank output,
  not drift-sensitive line goldens.
  The IFS script acknowledges a missing definition, rejects malformed
  selection, chooses a valid definition, and preserves the completed
  image when another invalid selection is cancelled.  Batch checks
  require one diagnostic and no saved image for each rejected definition.
  It verifies metadata and blank/nonblank output, not rasterized lines.
  The formula script acknowledges a missing definition, rejects invalid
  syntax, selects a valid formula, and preserves the completed image
  when another invalid selection is cancelled.  Batch checks reject
  missing, malformed, and empty formulas without saving an image.
  The Lyapunov setup script dismisses the inside-method warning with a
  key that would otherwise open options, verifies completed rendering
  with the substituted numeric color, then changes the inside color and
  compares the restarted image with a fresh calculation.
  The JIIM script checks picker movement and exact-point selection through
  saved Julia parameters, then reopens the orbit window around a menu and
  verifies restored Mandelbrot pixels.  It is the baseline for moving
  the temporary tool owner into UI, not a native cursor presentation test.
- Keyboard-handler tests cover dispatch, interruption flags, and key
  requeueing.  Standard controller tests also cover mid-orbit suspension,
  continuation, scanline resume positions, and perturbation/Lyapunov gates.
  Palette controller tests verify speed and pause state without timing,
  plus file-selection failure, acknowledgement, and retry.
  Loader tests verify missing-file preservation, partial malformed-map
  loading, returned warnings, and legacy quantization without a driver.
  Thinking-screen tests cover text, spinner updates, and active state.
  Cellular controller tests cover preparation, transition to pixel
  output, exactly-once cleanup, interruption, and injected failure.
  L-system calculation tests cover bounded sizing, suspension, and
  transition to drawing without UI calls.  Parser tests cover missing
  definitions, malformed commands, missing axiom and angle operands,
  invalid angles, stale-definition cleanup, and successful reload after
  failure.  Controller tests cover thinking-screen entry, updates,
  exactly-once cleanup, interruption, initialization failure, and injected
  calculation/presentation failure.
- Formula parser tests run without a UI driver.  They cover source and
  caret diagnostics, syntax and jump failures, recoverable symmetry
  warnings, file cleanup, failed-program cleanup, successful recompilation,
  and parameter inspection.
- Lyapunov and Julibrot setup tests run without a UI driver.  They cover
  supported options, warning delivery, legacy normalization order, and
  rejection before Julibrot initialization.  Current display and disk
  drivers accept only 256-color modes, so Julibrot's low-color failure is
  covered directly rather than through an unreachable autokey path.
- Perturbation tests cover reference failure, cached-reference reuse,
  caller-selected pixel order, deep-zoom BoundaryTrace completion, and
  palette-color consistency.  They also verify BoundaryTrace strategy
  selection.  Pass-switch tests verify automatic strategy selection,
  disabled perturbation, fresh reference caches, dispatcher restoration,
  and big-number stack cleanup between traversals.
  A separate numerical follow-up remains: with
  `center-mag=-2/0/1e20 maxiter=300 periodicity=0 inside=7`, automatic
  perturbation produced a uniform image while `perturbation=no` did not.
  The reference orbit lands exactly on the default bailout boundary.
  The pass-switch fixture instead uses `center-mag=0/1/1e20`; it does not
  cover that numerical edge case.
- `TestStereo` covers random-line generation, not PHOTO-mode decisions.
  Add autokey prompt/save/continue/cancel coverage and controller tests for
  eye sequencing and cleanup in the stereo controller slice.
- `TestShowDotController` covers exact point and triangle restoration,
  symmetry copies, unfinished pixels, zero-delay batches, menu suspension,
  and restart.  Autokey verifies row progress and checkpoint contents;
  neither establishes transient native marker presentation.
- Mock drivers and timer callbacks do not exercise a real window's event
  queue, physical mouse tracking, or visible refresh cadence.  Current
  Win32 and X11 drivers do not support continuous tone playback; require
  fake-port, mock-driver, and tone-log tests, not audible manual checks.

The validation requirements in each slice include tests still to be
implemented, not a claim that those tests already exist.  In particular,
automate menu results, preparation-screen lifetime, pass switching, and
failure cleanup rather than assigning them unspecified manual checks.

Manual checks are reserved for JIIM cursor presentation, show-dot
presentation, and native input during busy rendering.  Report any
still-required check when completing the corresponding slice.
If a slice adds automation for that same observation, remove the manual
requirement.  Do not repeat it for an unchanged caller or each fractal.

### Autokey Regression Rules

- Use `tests/autokey` for real application journeys through the driver-key
  interface, active handlers, menus, and render controllers.  Establish
  the relevant baseline script before changing its control flow, then
  retain it as a regression.  Record known defects separately; a test
  must not silently redefine the intended behavior around an old bug.
- Trace the actual context and key bindings before writing a script.
  Do not assume a control supported by one fractal applies to another.
  Reuse scenarios across slices; add cases for distinct interaction paths,
  not every fractal sharing the same controller or unchanged image test.
- Assert meaningful outcomes: selected parameters, generated files,
  completion, return to the outer UI, and relevant intermediate state.
  A correct final image alone does not prove a menu opened during work;
  its keys might have been processed only after calculation completed.
- For an in-progress interruption claim, observe the active calculation
  or preparation phase when the key is handled, the menu/cancel result,
  and the subsequent continuation or restart.  Use controller tests for
  exact yield points, resume bookkeeping, and handler lifetime as well.
- `CALCWAIT` waits for completion.  `THINKWAIT <key>` emits a character or
  named key only while a thinking screen is active; it does not assert
  arbitrary calculation phases.  If another intermediate observation is
  unavailable, add narrowly scoped UI/harness test support in a separate
  prerequisite commit, or keep that assertion in a controller test and
  limit the script's claim accordingly.
- Prefer `CALCWAIT` for completion.  Fixed `WAIT` delays, a large workload,
  or a slow CI machine do not prove that input arrived during work.
  Give scripts bounded process timeouts so lost keys or hangs fail.
  Do not increase polling or yield frequency to make a script pass.
- Use isolated test homes, explicit save locations, deterministic inputs,
  and fresh artifacts.  Compare resumed output with an uninterrupted
  control where pixels are stable, reusing existing golds when suitable.
  Use semantic results for L-systems and other drift-sensitive types;
  do not add unstable image golds to test keyboard behavior.
- Scripts exercise injected keys, not native keyboard delivery, mouse
  tracking, blinking, refresh cadence, or responsiveness.  Keep the
  narrowly scoped manual observations above.  Do not assert exact poll
  counts, elapsed render times, or the old location of an input check.
- Keep calculation/codec tests and injected-error controller tests where
  a script cannot reliably exercise resource exhaustion or cleanup.
  GIF load/save remains atomic; autokey may test surrounding choices,
  not introduce or expect mid-file keyboard cancellation.

## Implementation Slices

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

Validation: add autokey journeys selecting 2D and 3D IFS through the UI,
entering/leaving a render-time menu, and completing/saving afterward.
Observe interruption before claiming it and use deterministic seeds with
existing rendering fixtures.  Retain operation-selection tests.  No
manual image comparison.

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

Validation: add autokey scripts through actual PHOTO-mode entry for the
between-eye prompt, save, continue, cancel, and return to the main UI.
Verify prompt/eye sequencing and resulting saved artifacts, with controller
tests for exact phase transitions, display restoration, and failure
cleanup.
Retain stereo images.  Current random-line and 3D-look scripts do not cover
this prompt; no duplicate manual checklist.

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
completion in engine tests.  Reuse family autokey scripts established by
earlier slices; add a journey for any newly selected keyboard context not
yet covered.  Verify selection, return from menus, and supported completion
or continuation.  Reuse orbit, IFS, and standard-4D rendering fixtures.

### 52. Outer UI Render Operation Control

Source: `calc_fract()`, the render branches in `ui/big_while_loop.cpp`,
`ui/calculate_image.cpp`, and standard, Lyapunov, and non-standard UI
controllers.

Work:

- Have the outer UI render context obtain the engine-selected operation,
  initialize it, poll input, advance bounded work, and finish or suspend.
- Retain application-phase handling and command-effect mapping at their
  existing boundary.  Preserve render completion, save, and exit in batch
  mode, including user interruption while batch rendering.
- Remove engine calls to `standard_fractal()`, `lyapunov_fractal()`, and
  `non_standard_fractal()`, then remove obsolete `calc_fract()` dispatch
  scaffolding.  Do not duplicate renderer classification in UI.
- Remove the temporary `calculate_image()` diagnostic bridge.  Preserve
  setup warnings, fatal-error cleanup, and batch failure status while
  the outer UI context invokes the engine operation directly.
- Reuse UI contexts for presentation while the engine operation owns
  calculation state, timing, and resource cleanup.

Done when:

- Rendering begins from UI/application coordination; engine code never
  selects or invokes a UI controller.
- No new wrapper retains the old nested engine/UI render call chain.

Validation: run the full `rt-default` workflow and cover batch completion,
save, and exit with process tests, plus autokey for supported batch-key
interruption.  Reuse and extend autokey workflows for menu cancellation,
fractal/parameter changes that regenerate an image, and display-only
toggles through the outer application phases.  Assert intermediate phase
and command effects as well as final results.  Retain controller tests for
precise phase mapping, handler lifetime, and cleanup.  Do not substitute
fixed waits or a manual retest of every menu or renderer.

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

- `libid/fractals/lorenz.cpp`: `funny_glasses_call()` enters a blocking
  photographer prompt and second-image modal message.  IFS and orbit
  front ends also invoke complete UI render loops.
- `libid/engine/calcfrac.cpp`, `engine/NonStandardFractal.cpp`,
  and remaining renderer table entries in
  `fractals/fractalp.cpp`: reach UI render loops through direct calls or
  dispatch pointers.  Per-image setup no longer enters renderers;
  Cellular is already an engine-owned incremental operation.
- GIF codec operations do not poll input or support keyboard-interrupted
  saves.  Image-save progress, notifications, and exit policy are
  UI-owned; `io/ImageSave` owns encoding and file cleanup.  Image-load
  setup choices and cancellation are UI-owned and do not commit pending
  image parameters.

Command parsing reports errors, warnings, generation actions, and startup
selection requests without UI calls.  The command UI owns their effects;
MIG failures unwind to it rather than exiting from I/O code.

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
- Autokey regressions protect changed keyboard journeys through the real
  application.  In-progress claims have intermediate evidence; controller
  tests cover precise transitions and failure cleanup, while native input
  and transient presentation retain their narrow manual observations.
- Calculation code reports interruption or decisions without owning input.
- Current behavior is preserved while longer-term architecture stays out
  of scope.
