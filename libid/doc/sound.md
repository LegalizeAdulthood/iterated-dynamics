<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Sound Output Port

Sound is another output of calculation, alongside plotted pixels.  The
engine writes numerical samples to an abstract `SoundOutput` supplied by
the application.  It does not call a UI controller to make sound or pass
coordinates back through the calculation call chain solely for audio.

This is a durable design description, not an implementation checklist.
Implementation slices belong in
[inversion-of-control.md](inversion-of-control.md).

## Ownership

The port declaration belongs in `libid/include/engine/SoundOutput.h`.
It has no UI, driver, audio-device, or multimedia-library dependency.
Concrete implementations may be virtual; the non-virtual `StandardPass`
router is unrelated to this output-port interface.

The application owns the output session and injects its configured
implementation into one active output binding.  Calculation producers
and the UI use the same shared instance.  Producers do not each open or
close the same output file.

Responsibilities are separated as follows:

- Calculation owns orbit math, traversal, plotting, resumable state, and
  submission of numerical output at the corresponding calculation site.
- The output implementation owns channel selection, frequency mapping,
  sound configuration, device access, file encoding, audio timing, and
  normal voice release.
- The UI controller owns keyboard and mouse handling, presentation waits,
  interruption, and coordination between calculation and playback.  It
  services the output implementation through non-blocking `update()`.
- The session owner finishes or cancels output and reports failures.
  Calculation and output implementations do not open modal UI or exit.

Calling an output port is not calling back into UI control flow.  An
implementation must nevertheless not hide a polling loop or a playback
wait inside a port method.

### Dependency Injection

Use `SoundOutputPtr`, an alias for `std::shared_ptr<SoundOutput>`, for
shared ownership.  The application installs its selected instance in
`g_sound`, the active output binding, analogous to the existing
plot destination.  Producers call the port directly at their output
sites; the UI services that same instance with `update()`, `mute()`, and
`finish()`.

Injection occurs at this binding, not through constructor parameters on
every calculation object or parameters passed down calculation call
chains.  Do not add forwarding functions, per-pass sound fields, or
producer-specific plumbing merely to reach the output implementation.
Calculation depends only on the abstract port, never its concrete UI or
driver implementation.

Initialize the binding with `NullSoundOutput`, not a null pointer.  The
UI session owner retains a `SoundOutputPtr` throughout the render
operation, and the binding remains the same for that operation.  Shared
ownership protects lifetime; it does not by itself finish playback or
close the output session, which remain explicit owner responsibilities.

Tests inject a recording or fake implementation by replacing the active
binding in a scope and restoring it afterward.  Do not add a separate
test-only injection path or a new singleton/service lookup framework.
This binding does not introduce worker threads or synchronization for
every sample.

### Polling UI Session

`ui::SoundSession` selects the configured output, retains shared
ownership, installs it in `g_sound`, and restores the previous binding
on scope exit.  An owning render controller creates the session; nested
controllers use the installed binding without independently finishing it.
An application-supplied implementation uses the same session and binding.

The controller calls `wait_until_ready()` between bounded calculation
increments.  It services `update()` even without a new sample.  Ready
output adds no input polling or delay; backpressure waits through the
existing keyboard-handler context.  Calculation and plotting remain
outside the session.

Explicit `finish()` services playback until normal release and tails
complete.  Interruption or explicit `cancel()` mutes before finalizing;
output errors propagate to the owner.  An abandoned scope only mutes as
a safety measure.  Normal completion and cancellation remain explicit so
file-finalization failures can be reported outside a destructor.

## SoundOutput Interface

`SoundOutput` has a virtual destructor and a small output/lifecycle
contract:

```cpp
#include <memory>

namespace id::engine
{

struct SoundSample
{
    double x{};
    double y{};
    double z{};
};

class SoundOutput
{
public:
    virtual ~SoundOutput() = default;

    virtual bool play(const SoundSample &sample) = 0;
    virtual void update() = 0;
    virtual bool ready() const = 0;
    virtual void begin_orbit() = 0;
    virtual void mute() = 0;
    virtual bool finish() = 0;
};

using SoundOutputPtr = std::shared_ptr<SoundOutput>;

extern SoundOutputPtr g_sound;

} // namespace id::engine
```

`SoundSample` carries numerical source channels; unused channels are zero.
The configured implementation selects and maps those channels.  The
operations have the following semantics:

- `play(sample)` accepts one numerical sample and returns `true`,
  or returns `false` without accepting it when playback cannot advance.
- `update()` services due playback work, releases voices whose duration
  has expired, and updates readiness/completion state.  It returns
  promptly without waiting for a future deadline or handling input.
- `ready()` reports whether another coordinated output unit can be
  accepted.  It is a query, not a hidden playback servicing operation.
- `begin_orbit()` marks an orbit boundary in the output stream.  The
  initial implementation uses it for optional legacy time records.  It
  is called once per boundary, not again when a sample is retried.
- `mute()` immediately silences all voices, cancels pending audible
  playback, and resets voice bookkeeping on interruption.  Already
  accepted file records remain intact; muting does not close that file.
- `finish()` returns `false` while accepted playback still needs time,
  including normal release tails.  The controller keeps calling
  `update()` until completion; `finish()` then finalizes the session and
  returns `true`.  It does not mute to force normal completion.
  Repeated checks must not duplicate records or close resources twice.

None of these methods intentionally sleeps, dispatches input, or enters
another rendering loop.  Ordinary file I/O can report an error, but the
caller decides how to present it.  Invalid lifecycle use is a programming
error reported with `std::logic_error`, not an assertion.

### Voice Release, Mute, And Completion

Normal voice release and immediate silence are different operations.
When a note's duration expires, its implementation releases that voice
according to the backend's normal envelope behavior.  Other voices and
release tails may continue.  This is the role of DOS `soundoff()`, not
the all-voices reset performed by `mute()`.

Voice release is internal playback lifecycle, not another public port
operation for calculation to call.  The UI services `update()`; the
implementation decides which voices are due for release.  Repeated
updates must not release the same voice twice.

`mute()` is the explicit interruption/cancellation path.  It must discard
stale playback deadlines so that subsequent updates cannot release a new
voice on behalf of an old one.  File records already accepted remain
unchanged.  Muting does not permanently disable later sample playback.

Normal `finish()` allows accepted notes and release tails to complete
before resources close.  Cancellation may mute first and then finalize
without draining cancelled playback.  Neither path needs a playback wait
inside a port method.

### Samples And Mapping

Samples contain numerical source values, not speaker commands, delays,
UI flags, or requests that a controller consume a sound notification.
Use simple numeric channel data rather than an audio-library type or a
generic notification framework.

Bind the appropriate mapping when configuring a producer's sink.  The
initial implementation must accommodate the existing sources:

- Overlay plotting supplies screen coordinates at its plotting site.
  The adapter can preserve the current X, Y, Z, and debug-scaled formulas.
- Sticky orbits supply orbit samples; their initial mapping preserves
  the existing constant base-frequency output.
- Two-dimensional, three-dimensional, and dynamic orbit renderers supply
  their numerical orbit or transformed values rather than `m_sound_var`.
- Image restoration supplies each scanline's pixel values for the
  existing palette-index-to-frequency mapping.  Process pixel/sound
  output for that scanline before decoding the next; the GIF remains
  open during feedback.  A scanline does not become a single tone.

Source mappings are adapter policy, not reasons to bubble plotted
coordinates through `StandardPass` and `StandardFractal`.  A no-op sink
does not require calculation to test sound-mode flags at every point.

The first migration preserves the existing sample sources and mappings.
Longer term, sound need not depend on whether an overlay is enabled or a
point lies on screen.  Expanding those sites is a separate behavior
change, not an implicit consequence of adding the port.

## Interactive Semantics

Interactive rendering and playback are coordinated views of the same
sequence.  They advance at the slower of the calculation and audio rates.
Rendering must not race ahead while the user hears an older sequence.

GIF loading uses scanline-based decoding and feedback in one atomic,
run-to-completion command.  The UI owner reads a scanline and coordinates
its pixel output and sound before requesting the next.  It services
`update()` and playback readiness outside the codec, but does not poll or
dispatch keyboard input, including during playback waits.  Retain the
current line and pixel until accepted and plotted; no full-image buffer,
second traversal, or playback stage after file closure is required.

The calculation object computes a candidate output unit.  If the sink
cannot accept its sample, the object retains that calculation state and
returns to its controller before committing the associated plot.  After
acceptance, it commits that plot exactly once.  Retrying cannot duplicate
an orbit iteration, pixel write, file record, or tone.

The UI controller calls `update()`, checks output readiness, and advances
bounded work when the sink permits it.  It continues servicing `update()`
while waiting for playback and while draining normal completion, not just
when submitting another sample.  It handles input and can suspend
calculation or enter a menu during ordinary rendering, but not during an
atomic GIF load/save command.  It does not perform the engine's plotting.
Calculation code may test output readiness, but it must return on
backpressure rather than spin or wait.  It does not service playback or
release voices itself.

Several points can be handled in one calculation increment while output
is ready.  Do not introduce a key check, display refresh, or cross-thread
synchronization for every sample merely because the interface is virtual.
Silent rendering and output without device playback retain their normal
calculation cadence.

Buffering must be bounded and keep visible plotting coordinated with
audible playback.  Silently dropping or coalescing samples to let
rendering run ahead is not the default interactive policy.  The exact
buffer size and acceptable lead belong to the eventual audio backend.

When calculation is slower, playback follows the available calculated
sequence.  Do not invent samples or continue computing behind a paused
interactive display to keep a device busy.

On normal completion, the controller lets accepted playback finish before
finalizing the session.  On interruption it can mute promptly, preserve
accepted file output, and suspend calculation.  Resumption must not replay
an already accepted sample or commit its plot twice.

## Batch And Silent Output

A batch file sink preserves the numerical sequence and its encoded audio
timing without real-time playback waits.  A minute of future audio-file
output need not take a minute to calculate.  A no-op sink is always ready
and introduces no sound-driven delay.

Batch mode alone does not force file generation.  The interim driver-backed
implementation records tones only when `OSF_MIDI` is requested; that flag
does not select a separate file-output implementation.  A future
audio-file-only implementation is an alternative port implementation,
not the optional text recording performed by today's output operation.
Output errors return to the owner without keyboard I/O.

## Initial Implementations

No final sound library, device API, waveform format, or audio-file encoder
has been selected.  This refactor does not choose one or promise to restore
continuous sound on drivers that do not implement it.

### DriverSoundOutput

The initial concrete implementation owns both behaviors of `write_sound()`
as one output operation: optionally record the tone under `OSF_MIDI`, then
attempt driver playback.  These are sequential behaviors, not separate
file and device adapters composed behind another `SoundOutput`.

For each accepted sample, this implementation maps the source values to
one frequency, appends the optional record once, then calls
`driver_sound_on()`.  Check readiness before accepting the sample; a
refused sample has neither a file record nor a playback side effect.
File resources may use a private helper, but that helper does not
implement `SoundOutput` or form an independently selectable sink.

Its UI-serviced `update()` calls `driver_sound_off()` for normal voice
release when a deadline arrives.  Explicit `mute()` instead calls
`driver_mute()` to silence all voices and reset playback state.  These
calls belong to the adapter, not calculation classes.  It must not copy
`write_sound()` and its `wait_until()` loop behind a virtual method.

Where the driver accepts a tone, the adapter records its playback deadline
and reports backpressure until that output unit permits further progress.
The UI waits and processes input outside the adapter.  A driver returning
`false` means the sample is accepted without device playback; it does not
create a fictitious delay or make `play()` reject forever.  The
current Win32 and X11 continuous-tone methods return `false`.

The current driver supplies no audio callback or playback scheduler.
The UI controller must therefore service the adapter's `update()` during
rendering, playback waits, and completion.  A future multimedia backend
may release voices on its audio callback, but `update()` still provides
the non-blocking UI servicing boundary without double-releasing voices.

`orbitdelay` may supply the interim driver's tone-duration setting, using
the current numeric scale.  That is compatibility policy in the adapter,
not a duration field, timer, or pacing flag in the engine.  It does not
settle future multimedia timing or orbit-overlay display policy.

### Optional Legacy Tone Recording

`DriverSoundOutput` preserves `OSF_MIDI`, selected by `orbitsave=s`, during
this migration.  Despite the flag name, the existing output is a text
tone log, not a MIDI file and not a WAV file.  Preserve these behaviors:

- Existing checked save-directory and `sound001.txt` naming behavior.
- Lazy creation when output is first needed.
- One integer frequency per line, in accepted sample order.
- `time=...` records at the existing standard-orbit boundaries, using
  the current millisecond clock convention.
- Resource closure after completion, suspension, or failure as decided by
  the session owner.  Output-path errors remain visible to that owner.

Preserve recording even when the driver does not support continuous
tones.  The same output operation records each accepted sample once and
must not duplicate it when readiness is checked again.  Driver refusal
does not disable recording or turn an accepted sample into a retry.

The implementation maps numerical samples to the same logged frequencies
as the existing producers.  This optional recording is not the final
audio-file design and does not require a MIDI library.

### Other Implementations

- `NullSoundOutput` accepts samples without output or delay.  Select it
  only when neither playback nor tone recording is requested.
- No-op and future audio-file-only implementations have no playback work
  in `update()` and acquire no artificial device delay.
- A recording test implementation captures samples, boundaries, and
  lifecycle calls, and can simulate backpressure and release progression
  through `update()` deterministically.
- A future audio-file implementation can replace driver-backed output
  for batch audio generation after a multimedia library is selected.

There is no separate `InteractiveSoundOutput` composite in the initial
design.  The UI services the selected implementation directly.

Keep completion beeps and application alerts separate from orbit sound.
Replacing `driver_buzzer()` is not required to introduce this port.

## Verification

Contract tests should cover acceptance, rejection without side effects,
ordering, servicing, completion, interruption, and resource lifetime.
A fake clock and mock driver can exercise coordinated playback without
wall-clock sleep or an actual speaker.  Test normal release separately
from immediate mute, including repeated updates, release tails, and mute
followed by new playback without stale deadlines.  Normal completion
must not substitute mute for release.  Driver refusal must remain a fast
path without suppressing optional tone recording.

Producer tests should prove that backpressure retains calculation state
and neither skips nor repeats plotting or samples.  Use existing image
tests for rendered results.  Driver-backed output tests also cover legacy
naming, frequency records, boundary records, failure, and closure, with
recording enabled and disabled and device playback accepted or refused.
Verify recording precedes the device attempt and rejected samples leave
no record.  Exact clock readings are not golden test data.

Selecting a multimedia library, implementing PCM/WAV output, and changing
sonification mappings or overlay-delay semantics remain separate work.
