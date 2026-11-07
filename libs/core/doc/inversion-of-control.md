<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Rendering Interaction Design And Remaining Work

Input and presentation code decide when to request another bounded unit
of calculation.  Engine and fractal code own calculation state,
lifecycle, timing, setup, and cleanup.  This document describes that
interaction design and the remaining handler-stack and event-driven work.
Dependency direction and output-port ownership are described in
[hexagonal.md](hexagonal.md).

## Traversal And Orbit Calculation

Pixel traversal determines which pixels require orbit calculation and
which can be filled from surrounding results.  It owns traversal and fill
state, independently of the fractal's orbit calculation.

The image calculation owns the selected traversal and orbit strategy.
It retains an unfinished pixel across bounded increments and delivers a
completed color to traversal.  Orbit-producing modes can instead advance
orbit work directly; they need not use a pixel selection/completion
contract.

Perturbation is an alternate orbit strategy, not a traversal algorithm.
`passes=p` remains compatibility syntax selecting perturbation with
solid-guess traversal, not a separate perturbation pass.

## Keyboard Handler Stack

Each active UI context pushes a keyboard handler and pops it when the
context ends.  Nested contexts take precedence without coupling their
key handling to one another.

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

Dispatch offers each supplied key to handlers from top to bottom.  The
first handler returning `true` consumes the key and stops propagation.
If none consumes it, the key is discarded.

`handle_key()` reports consumption only.  Each handler decides whether a
key requests calculation interruption and sets UI-owned state accordingly.
The caller does not infer interruption from the key or the return value.
A context can therefore consume an orbit-display toggle without stopping
calculation, while an outer context handles application commands.

Input acquisition is separate from this dispatch contract; its remaining
centralization work is described below.

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

Reading or writing a GIF is one atomic, run-to-completion I/O command.
Atomic means non-interruptible by keyboard input, not feedback-free or
transactional filesystem replacement.

- Load and save use giflib's scanline APIs.  Pending keys remain for the
  outer input loop; neither codecs nor scanline feedback dispatch input
  or enter modal UI during the operation.
- During loading, the UI owner coordinates pixel output and optional
  sound for each decoded scanline before requesting the next.  Audio
  servicing follows [sound.md](sound.md), without keyboard interruption.
- During saving, the UI owner updates progress after each completed
  scanline and flushes the display before writing the next.  This is not
  a per-scanline disk flush.  Feedback restoration must not alter pixels
  that have not yet been saved.
- Codec work, pixel traversal, and image output stay below UI.  Codecs do
  not service audio or invoke UI feedback callbacks.
- Mode selection, overwrite decisions, busy-state setup and cleanup,
  error reporting, and completion notifications surround the file
  operation rather than interrupting scanline processing.
- I/O failures still require reporting, resource cleanup, and the
  existing temporary-file replacement rules.  Non-interruptibility does
  not suppress read/write errors or introduce keep-partial choices for
  keyboard interruption.
- Saving an incompletely rendered image retains its resume metadata.
  An interrupted calculation is not an interrupted GIF write.

## [Sound Output Port](sound.md)

The sound design defines numerical output, dependency injection, session
ownership, playback backpressure, voice release, and mute semantics.

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

To make handler-stack use consistent and input delivery application-owned:

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

The target is an application-level input pump supplying the handler stack
while driver polling remains.  GUI-thread event delivery can then replace
that pump without changing context-specific key handling.

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
