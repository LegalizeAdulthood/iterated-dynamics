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

The release 1.5 input boundary is implemented.  The descriptions below
record current ownership and behavior, including remaining presentation
and backend coupling.  They are not instructions to restore an earlier
design where implementation has superseded it.

The standard escape-time renderer follows the same shape as the other
calculation types split into UI coordination code and calculation objects.

## Existing Shape

These files show the calculation-object and UI-controller structure.
The audit below records the implemented boundary and its current limits:

- `libid/ui/AntController.cpp` drives selected Ant work in
  `libid/fractals/Ant.cpp`.
- `libid/ui/BifurcationController.cpp` drives
  `libid/fractals/Bifurcation.cpp`.
- `libid/ui/CellularController.cpp` drives selected Cellular work in
  `libid/fractals/Cellular.cpp`.
- `libid/ui/DiffusionController.cpp` drives `libid/fractals/Diffusion.cpp`.
- `libid/ui/OrbitController.cpp` drives selected 2D orbit and Dynamic2D
  work in `libid/fractals/lorenz.cpp`.
- `libid/ui/IFS2DController.cpp` drives selected 2D IFS work in
  `libid/fractals/lorenz.cpp`.
- `libid/ui/StereoController.cpp` drives selected 3D IFS and orbit work
  owned by `libid/engine/NonStandardFractal.cpp`.
- `libid/ui/PlasmaController.cpp` drives `libid/fractals/Plasma.cpp`.
- `libid/ui/Standard4DController.cpp` drives
  `libid/fractals/julibrot.cpp`.
- `libid/ui/LSystemController.cpp` drives selected sizing and drawing
  work in `libid/fractals/lsystem.cpp`.
- `libid/ui/PopcornController.cpp` drives selected image work in
  `libid/fractals/popcorn.cpp`.
- `libid/ui/FrothyBasinController.cpp` drives selected pixel and overlay
  work in `libid/fractals/FrothyBasin.cpp`.
- `libid/ui/TestPointController.cpp` drives `libid/fractals/TestPoint.cpp`.

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

## Standard Render Structure

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

Pixel traversal passes do not compute the orbit.  They expose
`select_pixel()`, `selected_pixel()`, and `complete_pixel(color)`;
`StandardFractal` computes the selected pixel and supplies its color.
Communication still uses globals such as `g_row`, `g_col`, `g_color`, and
`g_plot`.  Removing those globals is separate work.

`SynchronousOrbit` and `StickyOrbits` are different: they advance orbit
work directly.  `StandardPass::iterate(PertEngine *)` routes their bounded
work and supplies the selected perturbation strategy when active.  They
do not use the pixel-pass selection/completion contract.

The non-virtual `StandardPass` router holds a `std::variant` of concrete
pass classes.  It routes chunk preparation, selection, pixel completion,
orbit iteration, status, suspension, and resume storage to the active
alternative.

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

Perturbation is an alternate orbit strategy selected and owned by
`StandardFractal`, separately from traversal.  Pixel passes receive its
completed colors; synchronous and sticky orbit work receive the strategy
through their iteration interface.  `passes=p` remains compatibility
syntax selecting perturbation with solid-guess traversal, not a separate
perturbation pass.

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

## UI Interaction Design

Keyboard, mouse, command-line, parameter-file, and batch input are command
sources.  UI coordination interprets interaction and advances calculation;
calculation does not ask for input.  Rendering requires an output target
with defined image dimensions, after which default engine settings are
sufficient to produce an image.

Command effects and application phase are separate concepts.
`CommandResult` describes what an interaction changed or requested;
application coordination maps that result to phase transitions and work.
Menu handlers do not choose application phases.  Distinguish:

- Fractal selection, which resets type-specific state, from a full reset,
  which also restores the default fractal type.
- Calculation changes, such as corners, passes, bailout, or iteration
  limits, from presentation-only changes such as palette edits.
- Render suspension and resumption from starting a replacement image.
- Local interaction, cancelled dialogs, and ignored input from requests
  to load, save, change the display target, or exit.

Parameter processing shares syntax but separates handler ownership.
Decode each command once; UI handles UI-owned names and delegates only
unrecognized names to engine handlers.  Recognition followed by rejection
does not permit delegation.  A mixed parameter has one owner, which may
configure lower-level settings through their public interfaces.

Process one ordered stream, including parameters read from `@` sources.
Partition handlers, not execution into separate UI and engine passes.
Apply each command before advancing; preserve reset scope, compatibility
behavior, accepted settings before an error, and source-specific failure
rules.  Engine source traversal exposes commands and aggregates outcomes;
UI drives routing and owns diagnostic presentation and requested external
actions.  Engine traversal must not call UI handlers.

The engine selects the calculation strategy and owns image setup,
iteration state, traversal, plotting, suspension/resumption, total image
timing, and cleanup.  Its operations perform bounded work and return to
the controller.  A wrapper whose first step renders an entire image does
not provide that interaction boundary.

The UI starts, advances, suspends, or resumes the selected calculation.
It owns active keyboard-handler scopes, mouse interaction, markers,
diagnostics, storage preparation, presentation delays, and sound-output
servicing.  It does not classify fractal algorithms or perform their
rendering.  Preserve useful calculation batches rather than synchronizing
on every pixel or orbit point.

These responsibilities apply throughout setup, normal work, completion,
and failure.  See [hexagonal.md](hexagonal.md) for dependency boundaries
and [sound.md](sound.md) for coordinated output and backpressure.

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

The giflib-based codecs implement this boundary.  Scanline feedback is
part of the run-to-completion command; it is not a reason to restore the
old keyboard polling.

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

No multimedia library has been selected.  `ui::DriverSoundOutput`
optionally records each accepted tone under `OSF_MIDI` before attempting
driver playback.  These sequential behaviors are one implementation, not
two port implementations or a composite.  Its non-blocking `update()`
releases a due tone; explicit mute resets playback immediately.

Do not promise audible continuous sound on drivers that currently return
`false`.  Preserve optional recording regardless of driver capability.
Future audio-file-only output, new audio libraries, WAV encoding, and
changed sound mappings are separate work.

## Orbit Display And Delay Policy

See [orbit-delay-timer.md](orbit-delay-timer.md) for timing behavior and
[sound.md](sound.md) for coordinated audio output.

Temporary orbit overlays and image-producing orbit plots are distinct.
Popcorn and PopcornJul use screen pixels as seeds and plot iterated points
elsewhere in the image, rather than coloring each seed by orbit length.
Their output is image content, not a temporary overlay or an
orbit-inspection feature.

A possible future orbit tool window would use an interactive cursor in
the main image to select a point for orbit calculation.  It could animate,
pause, single-step, or play sound independently of image rendering.
In that design, `showorbit=yes` could become compatibility syntax for
enabling the tool instead of coupling overlay animation to image
generation.  This is a future direction, not a selected replacement for
the existing display policy.

## Current Presentation Ownership

Calculation setup, memory operations, and command processing report
diagnostics without modal UI or shutdown callbacks.  Standard and
Lyapunov show-dot presentation belongs to `ShowDotController`.
FrothyBasin's simple marker and the general pixel kernel's legacy
sound/delay-dependent chunk choice remain as described above.  Neither
performs keyboard polling or a delay inside calculation.

Command parsing returns diagnostics, generation requests, and the need
for startup file-entry selection.  UI callers own action execution,
selection cancellation, device initialization, and application exits.
Image-save feedback and policy live in `ui/save_image.cpp`, which drives
scanline encoding through `io/ImageSave`.  Image-load setup choices live
in `ui/restore_image.cpp`.  Video selection, 3D settings, and Tab
information use pending values; the active image and stacked screen
remain unchanged when setup is cancelled.

The engine and fractal classes expose bounded rendering facts such as
"overlay point was drawn" or "scrub restored
pixels"; the UI controller decides whether that causes display delay or
input handling.  Sound samples go directly to `SoundOutput`; they are
not another reason to add UI-consumed flags to calculation objects.

Any future presentation-cleanup slice must move behavior to a named UI
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
  The 2D and 3D IFS interruption scripts select the definition and type
  through the command UI, deliver `x` after 512 IFS iterations during
  active rendering, cancel the menu, and save a non-resumable checkpoint.
  Changing the iteration limit restarts calculation; saved parameters,
  completion state, and pixels are checked against a fresh seeded image.
  IFS does not support calculation resume.
  The PHOTO-mode IFS and Lorenz 3D scripts remain interaction regressions.
  They reach the first completed eye, acknowledge its notice, save from
  the photographer handler, and continue to the right eye.  Separate
  journeys complete or interrupt that eye after 512 iterations, cancel
  the options screen, and return to image commands.  Checkpoints verify
  type, PHOTO mode, iteration limit, status, dimensions, and nonblank
  output.  They do not compare drift-sensitive orbit pixels.
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
  verifies restored Mandelbrot pixels.  It protects the UI-owned tool's
  behavior, not native cursor presentation.
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
- Perturbation tests cover reference failure and replacement without
  retained history, caller-selected pixel order, bounded reference and
  pixel increments, deep-zoom BoundaryTrace completion, and palette-color
  consistency.  They also verify BoundaryTrace strategy selection.
  Pass-switch tests verify automatic strategy selection, disabled
  perturbation, fresh reference state, dispatcher restoration, and
  big-number stack cleanup between traversals.  Current precision tests
  cover bailout equality and tiny offsets on either side of the boundary,
  including Mandelbrot near `-2`.  Coloring, periodicity, inversion,
  synchronous-orbit, sticky-orbit, and output-backpressure regressions
  cover the later perturbation extensions.
- Non-standard operation tests cover 2D orbit, dynamic, inverse-Julia,
  and Standard4D selection, setup, completion, suspension, and queue
  ownership.  Autokey menu-return journeys compare Henon, dynamic,
  MandelCloud, inverse Julia, and Julibrot output with uninterrupted
  renders.  Non-resumable inverse Julia and Julibrot explicitly restart
  with Enter after cancelling the menu; resumable types continue.
- `TestStereo` covers random-line generation.  PHOTO-mode autokey tests
  cover calculated-view save/continue/interruption and restored-view
  save/advance with Enter or Escape.  `TestStereoView` and
  `TestStereoRestore` cover eye selection, separate GIF decodes, failure
  status, shared cleanup, and full-screen restoration without input
  polling during decode.
- `TestShowDotController` covers exact point and triangle restoration,
  symmetry copies, unfinished pixels, zero-delay batches, menu suspension,
  and restart.  Autokey verifies row progress and checkpoint contents;
  neither establishes transient native marker presentation.
- Mock drivers and timer callbacks do not exercise a real window's event
  queue, physical mouse tracking, or visible refresh cadence.  Current
  Win32 and X11 drivers do not support continuous tone playback; require
  fake-port, mock-driver, and tone-log tests, not audible manual checks.

The coverage described above is implemented.  Future slices should
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
  arbitrary calculation phases.  `ITERWAIT <iteration> <key>` requires
  active calculation and the type-specific `g_color_iter` threshold;
  IFS increments that counter for each generated point.  Do not assume
  it counts image progress for other types.  If another intermediate
  observation is unavailable, add narrowly scoped UI/harness support in
  a separate prerequisite commit, or keep that assertion in a controller
  test and limit the script's claim accordingly.
- Prefer `CALCWAIT` for completion.  Fixed `WAIT` delays, a large workload,
  or a slow CI machine do not prove that input arrived during work.
  Give scripts bounded process timeouts so lost keys or hangs fail.
- `FAST` disables autokey's visual typing delay for the active script,
  including after `WAIT` or `CALCWAIT`.  Use it before phase-gated input
  so typing delays cannot defer an interruption until work has finished.
  It returns no key; a directive at a modal boundary also ends keyahead
  flushing before the script acknowledges the notice.  Normal scripts
  retain their existing typing delays unless they request `FAST`.
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

## Current Engine Boundary

The synchronous engine-to-UI render dispatch has been removed.
Non-standard operation selection and per-image setup no longer enter
renderers or UI.  Engine/fractal sources and headers no longer include
UI headers.  Parameter ownership and defaults are separated:

- `VideoInfo` is key-free.  UI owns the separate `g_video_mode_keys`
  associations, the selected-entry binding, key lookup, key-name parsing,
  and binding-name formatting.  Engine retains disk/display mode
  validation with no UI key dependency.  UI owns the `video` handler and
  pending request `g_init_mode`, including its storage and
  initialization/completion policy.
- Stereo calculation inputs and `CalibrationBars` are geometry-owned.
  Automatic conversion and texture-filename reuse remain UI policy.
  UI owns both the `rds` and `rds-texture` handlers; engine parameter
  handling no longer depends on UI stereo policy.
- UI owns `exitnoask`, `g_escape_exit`, and combined fractal-default
  initialization for startup, fallback, and explicit `reset` commands.
  Engine owns its defaults and reset-version validation/application.
  Accepted resets take effect inline, once, before the next command.
- GIF evolution records and comparisons are I/O-owned; public GIF metadata
  headers no longer import UI.  `io/loadfile.cpp` explicitly includes
  `ui/evolve.h` for its existing application of evolution runtime state.

GIF codec operations do not poll input or support keyboard-interrupted
saves.  Image-save progress, notifications, and exit policy are UI-owned;
`io/ImageSave` owns encoding and file cleanup.  Image-load setup choices
and cancellation are UI-owned and do not commit pending image parameters.

Command parsing reports errors, warnings, generation actions, and startup
selection requests without UI calls.  The command UI owns their effects;
MIG failures unwind to it rather than exiting from I/O code.

The implementation slices for the release 1.5 boundary are complete.
Future source audits must still trace UI helper and dispatch-pointer
calls as well as raw driver calls; an empty raw-key search alone does not
establish inversion.

This is not a claim that every non-UI module is free of UI state or
headers.  Current backend dependencies include:

- `geometry/line3d.cpp` includes `ui/big_while_loop.h` to register its
  output-cleanup function in `g_out_line_cleanup`.
- `io/encoder.cpp` and `io/ImageSave.cpp` include the big-loop and
  evolution headers.  `ImageSave` reads UI-owned evolution state when
  constructing metadata.
- `io/loadfile.cpp` includes evolution and parameter-file UI headers;
  applying saved extensions restores evolution runtime state and genes.
- `io/load_config.cpp` uses UI key-name parsing and writes the separate
  UI-owned video-mode bindings while loading the mode catalog.
- `io/save_timer.cpp` uses UI-owned `g_finish_row` and `read_ticker()` for
  autosave scheduling.

These dependencies do not poll keyboard or mouse input.  They are
distinct from the completed engine/fractal input boundary and from any
future cleanup of metadata, configuration, or autosave ownership.

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
- Render controllers already expose bounded stepping.  Their input waits
  and modal interactions still need event-driven phases before the top
  loop can be the sole driver-key poller.

Simple render controllers that already wrap calculation objects but
still poll the driver queue directly:

- `libid/ui/BifurcationController.cpp`
- `libid/ui/CellularController.cpp`
- `libid/ui/IFS2DController.cpp`
- `libid/ui/LSystemController.cpp`
- `libid/ui/PlasmaController.cpp`
- `libid/ui/Standard4DController.cpp`
- `libid/ui/TestPointController.cpp`

Controllers with bespoke interaction or partial handler-stack use that
still read or write the driver queue directly:

- `libid/ui/AntController.cpp`
- `libid/ui/inverse_julia.cpp`
- `libid/ui/ImageRenderContext.cpp`
- `libid/ui/stereo.cpp`

Handler-based render controllers that still rely on
`KeyboardHandler.cpp` polling internally:

- `libid/ui/DiffusionController.cpp`
- `libid/ui/FrothyBasinController.cpp`
- `libid/ui/InverseJuliaFractalController.cpp`
- `libid/ui/LyapunovController.cpp`
- `libid/ui/OrbitController.cpp`
- `libid/ui/PopcornController.cpp`
- `libid/ui/SoundSession.cpp`
- `libid/ui/StandardFractalController.cpp`
- `libid/ui/standard_orbit_plot.cpp`
- `libid/ui/StereoController.cpp`

Blocking text screens, modal prompts, and utility UI paths also poll
directly.  They can remain for the release 1.5 directory boundary, but
must become application phases or dialogs before `id_main` can be the
only keyboard poller:

- `libid/ui/big_while_loop.cpp`
- `libid/ui/command_diagnostics.cpp`
- `libid/ui/editpal.cpp`
- `libid/ui/file_get_window.cpp`
- `libid/ui/flip_image.cpp`
- `libid/ui/full_screen_choice.cpp`
- `libid/ui/full_screen_prompt.cpp`
- `libid/ui/get_key_no_help.cpp`
- `libid/ui/help.cpp`
- `libid/ui/intro.cpp`
- `libid/ui/input_field.cpp`
- `libid/ui/main_menu.cpp`
- `libid/ui/main_menu_switch.cpp`
- `libid/ui/rotate.cpp`
- `libid/ui/slideshw.cpp`
- `libid/ui/starfield.cpp`
- `libid/ui/stop_msg.cpp`
- `libid/ui/temp_msg.cpp`
- `libid/ui/thinking.cpp`
- `libid/ui/zoom.cpp`

## Future Event-Driven Work

The following ideas are outside the completed release 1.5 input boundary.
Input polling is UI-owned, but the application is not yet event-driven.

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

## Release 1.5 Boundary Checks

- Top-level image rendering is requested through `libid/ui`, not through
  an engine function that selects UI control paths.
- Standard escape-time rendering is driven by a UI polling loop around
  `StandardFractal`.
- `StandardFractal` owns standard renderer iteration, mode dispatch, and
  resume state.
- `libid/engine` and `libid/fractals` do not include headers from
  `libid/ui`.
- This keyboard-input audit returns no matches:

```text
rg "driver_(get_key|key_pressed|wait_key_pressed|unget_key|key_cursor)" \
  libid/engine libid/fractals libid/geometry libid/io libid/math \
  libid/misc
```

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
- Standard and Lyapunov show-dot marker state belongs to UI, with no
  temporary marker bridge in their calculation paths.  FrothyBasin still
  draws its simple marker in calculation; it does not poll or wait there.
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
