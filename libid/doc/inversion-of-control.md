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

Setup dialogs use pending settings and commit them only when accepted.
Cancelling setup leaves the active image, display state, and stacked
screen unchanged.  This does not make ordered parameter processing
transactional or roll back settings accepted before a command error.

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

## Remaining Handler And Dispatch Work

The release 1.5 boundary keeps direct input in UI.  Consistent use of the
handler stack and application-owned input delivery are further work:

- Express each active controller or tool's key behavior through scoped
  handlers, including contexts that currently only peek and suspend or
  interpret keys in a private switch.
- Let handlers consume context-specific keys and decide whether they
  request interruption.  Route other commands to application handling
  without requeueing a consumed key in the driver.
- Separate input acquisition from handler dispatch.  The dispatcher
  accepts keys supplied by the application; it does not read the driver
  queue.  Interruption queries observe state rather than pump input.
- Remove independent input loops from controller waits, menus, prompts,
  and tools.  Preserve their interaction state while returning control
  to application input delivery and bounded calculation scheduling.

Preserve existing interaction behavior and useful calculation batches.
Moving polling into another helper is not sufficient if controllers
still decide when application input can be processed.

## Future Event-Driven Work

The application-level input pump supplies the handler stack while driver
polling remains.  GUI-thread event delivery can then replace that pump
without changing context-specific key handling.  This is an application
responsibility, not a requirement that one particular function be named
`id_main`.

The wider UI transition can proceed incrementally:

- Convert `Frame`, `WinText`, and `Plot` to wxWidgets controls while the
  current polling code still pumps GUI events.
- Add menu-bar support, retaining a temporary blocking-screen workaround
  until those interactions are migrated.
- Convert blocking text screens to modal dialogs incrementally.  Start
  with simple numeric input, then form-style prompts, then file-entry
  screens.  Remove the menu-bar workaround when it is no longer needed.
- Use idle-processing callbacks to schedule bounded calculation as a
  bridge to later worker-thread or distributed rendering.
- Plan thread ownership, command delivery, cancellation, and result
  delivery separately.  Active handlers decide command meaning; moving
  calculation to a worker does not make shared interruption flags a
  thread-communication design.
- Build a tool interface for image interaction.  A tool receives mouse,
  keyboard, and modifier-key events, not only `MouseNotification` calls.
- Support a tool stack with a permanent default zoom tool.  Events go to
  the top tool, with optional fall-through to lower tools.
- Allow multiple zoom tools over time, such as current zoom-box behavior,
  XaoS-style continuous zoom, or ManpWIN-style controls.
- Preserve accurate zoom-box area selection as a required interaction.
- Once GUI events supply input throughout, remove the `Driver` keyboard
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
