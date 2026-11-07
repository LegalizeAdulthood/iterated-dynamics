<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Orbit Delay Timer

## Scope

This note describes the current `orbitdelay` implementation, not a settled
replacement for DOS display semantics.  Exact microsecond compatibility
is no longer the display goal.  Visible orbit plotting, responsive input,
and coordinated sound output are the useful behaviors.

The broader policy review remains in
[inversion-of-control.md](inversion-of-control.md).  Sound-port ownership
and interactive backpressure are described in [sound.md](sound.md).
The implementation is the source of truth.

## Units And Distinct Uses

The retained numeric unit is 1/10000 second:

```text
orbitdelay=1   -> 100 microseconds
orbitdelay=139 -> 13.9 milliseconds

1 / 72 seconds = 13.888... milliseconds
ceil(10000 / 72) = 139 delay units
```

That scale does not imply one common policy for every caller:

- Standard and FrothyBasin running overlays skip display waits for values
  below 139.  Values of 139 or more are passed to the orbit delay timer.
- `ShowDotController` waits for every positive value, without the overlay
  threshold.  It uses a separate sleep path.
- `AntController` applies its own batch delay and single-step controls.
- `DriverSoundOutput` uses the unrounded value as tone duration.  The
  overlay display threshold does not apply to sound.

Consequently, "small values run at full speed" describes the overlay
display wait only, not show-dot, ant, or audible sound pacing.  A value
alone does not guarantee a particular image time or visible frame rate.

## Running Overlay Policy

[standard_orbit_plot.cpp](../../ui/standard_orbit_plot.cpp) implements the
current policy:

- `display_orbit_delay_units()` maps values below 139 to zero and returns
  larger values unchanged.
- `pace_orbit_display()` calls the platform timer only for a positive,
  above-threshold delay.
- Its input dispatcher uses `steady_clock` to limit dispatch attempts to
  one per 13.9 milliseconds, including zero and below-threshold delays.
- `reset_orbit_delay()` resets both the timer deadline and the next input
  dispatch time.

[StandardFractalController](../../ui/StandardFractalController.cpp) and
[FrothyBasinController](../../ui/FrothyBasinController.cpp) invoke this
helper after calculation reports displayed overlay output.  They advance
calculation and handle interruption outside the calculation loops. Other
controller polling paths are not universally limited to 72 Hz.

The 72 Hz constant is not monitor refresh detection, vertical
synchronization, or a display-flush scheduler.  It does not batch points
into frames or retain each point for one frame.  Above-threshold pacing
still occurs after individual reported overlay output, so it can make
rendering extremely slow.

[OrbitPlot](../engine/OrbitPlot.cpp) plots overlay pixels and saves their
previous colors.  Points remain until calculation scrubs the overlay;
the timer neither removes points nor owns their lifetime.  Plotting and
scrubbing remain engine work.  Input and presentation waits remain UI
work.

## Timer Interface

[OrbitDelayTimer.h](../../ui/include/ui/OrbitDelayTimer.h) declares:

```cpp
using OrbitDelayInputPendingFn = bool();

constexpr unsigned long ORBIT_DELAY_UNIT_MICROSECONDS = 100;

void reset_orbit_delay_timer();
bool wait_for_orbit_delay(
    unsigned long delay_100us, OrbitDelayInputPendingFn &input_pending);
```

The callback is required, not nullable.  It gives the caller a chance to
process input; returning true interrupts the pending wait.  The timer
returns whether that callback interrupted it, not whether rendering was
cancelled.  Keyboard handlers and controllers decide interruption policy.

All current implementations share these semantics:

1. Read the platform clock.  After reset, initialize the deadline to now.
2. Wait for the deadline established by the previous call, unless the
   callback interrupts the wait.
3. Set the next deadline to the current time plus the supplied delay.

The first call after reset therefore arms a delay without waiting.
Changing the supplied delay affects the next deadline, not the interval
already armed.  Passing zero does not itself discard a previous pending
deadline; reset is the explicit way to discard timer state.  Calculation
time between calls counts toward the interval, and deadlines are not
accumulated into an overdue catch-up schedule.

While more than 500 microseconds remain, implementations check the input
callback and wait in bounded chunks.  The final 500 microseconds use a
busy spin without input callbacks.  This retains high-resolution behavior
from the earlier compatibility work; it has not been simplified away.

Timer deadlines and platform initialization data are file-scope state.
There is one shared timer, not an independently owned timer per
calculation.  The interface is not a reentrant scheduling service.

## Platform Selection

[libs/ui/CMakeLists.txt](../../ui/CMakeLists.txt) selects the platform
source at configure time, rather than using platform conditionals in the
generic interface:

- Windows: [OrbitDelayTimerWin32.cpp](../../ui/OrbitDelayTimerWin32.cpp).
  Uses `QueryPerformanceCounter()` and `QueryPerformanceFrequency()`.
  Outside the spin window, waits for messages with a one-millisecond
  `MsgWaitForMultipleObjectsEx()` timeout.  The spin uses
  `YieldProcessor()`.
- Linux: [OrbitDelayTimerLinux.cpp](../../ui/OrbitDelayTimerLinux.cpp).
  Uses `clock_gettime(CLOCK_MONOTONIC, ...)` and `nanosleep()` chunks of at
  most one millisecond before the spin window.
- FreeBSD, OpenBSD, and NetBSD:
  [OrbitDelayTimerBSD.cpp](../../ui/OrbitDelayTimerBSD.cpp).  Uses the same
  monotonic clock and bounded `nanosleep()` approach.
- macOS: [OrbitDelayTimerMac.cpp](../../ui/OrbitDelayTimerMac.cpp). Uses
  `mach_absolute_time()` with `mach_timebase_info()` conversion and bounded
  `nanosleep()`, not `std::chrono::steady_clock` as its timer
  implementation.

The platform timer does not itself dispatch GUI messages.  Message
waiting on Windows is not message pumping; input processing is supplied
through the callback.  The overlay caller additionally throttles that
callback's actual keyboard dispatch.

These implementations measure fine deadlines but do not promise exact
wake-up times.  Scheduling and presentation overhead can exceed the
requested interval.  Clock resolution alone does not establish visible
performance.

## Other Delay Paths

### Show-Dot Marker

[ShowDotController](../../ui/ShowDotController.cpp) captures the configured
delay when constructed.  For each new selected pixel it shows the marker,
flushes output, services the driver with `driver_delay(0)`, sleeps for
`delay * 100` microseconds, services the driver again, and restores saved
pixels before calculation proceeds.

This uses `std::this_thread::sleep_for()`, not `OrbitDelayTimer`, and has
no 139-unit threshold.  Native scheduling can make small delays longer
than requested.  It is a distinct presentation path, not proof that the
overlay timer was bypassed incorrectly.

### Ant

[AntController](../../ui/AntController.cpp) initializes its local delay
from the absolute value of `g_orbit_delay`.  A value of one selects
single-step mode instead of a timed delay.  Otherwise, it waits after
completed batches using its own clock and driver-delay loop.  The delay
does not apply separately to each individual ant pixel and does not use the
overlay timer or threshold.

### Sound

[DriverSoundOutput](../../ui/DriverSoundOutput.cpp) captures tone duration
as `g_orbit_delay * 100` microseconds.  When the driver accepts a tone, the
adapter records a deadline and reports not-ready until `update()` performs
normal voice release.  Explicit mute cancels playback.

[SoundSession](../../ui/SoundSession.cpp) services that output and handles
waiting and input in the UI.  Rendering proceeds at the slower of
calculation and accepted sound playback; it must not outrun coordinated
audio.  The engine's sound-port operations do not sleep or poll input. A
driver that declines device playback creates no fictitious playback delay.
Current Win32 and X11 continuous-tone methods return false.

This is implemented driver-backed sound-port behavior, not a future WAV
buffer implementation.  A modern audio library and audio-file output
remain separate decisions.

## Completed Separation And Remaining Decisions

The old `wait_until()` and `usec_clock()` paths are no longer present in
`id::ui`.  Engine and fractal code no longer wait or poll input for orbit
presentation.  The general pixel kernel still uses sound/delay settings to
select a bounded work chunk: 16 iterations with orbit sound and a positive
delay, otherwise 2048.  That is not an engine-side timer.

Do not turn earlier suggestions into implementation requirements:

- Revisit per-point pacing versus frame-based presentation before changing
  orbit display behavior.  The 138-to-139 threshold remains abrupt.
- Decide overlay retention and any future orbit tool-window behavior
  separately from the low-level clock choice.
- Review show-dot, ant, sound, and batch behavior explicitly; they do not
  all follow the same display policy.
- Decide whether the spin window and shared timer state are still useful
  after presentation policy is chosen.  Removing them is not completed
  work or an already selected replacement design.

## Existing Tests

[Timer tests](../../../tests/libs/ui/test_OrbitDelayTimer.cpp) are
platform-neutral and compiled when CMake selects a supported timer:

- An armed 50-millisecond delay takes at least 40 milliseconds and invokes
  the input callback.  This is a conservative lower-bound test, not an
  exact-duration guarantee.
- A true input callback interrupts an armed long delay.
- A short armed delay does not invoke input during the spin window.
- Parameterized cases check zero, below-threshold, and at-threshold
  overlay delay mapping.

Separate show-dot and sound-adapter tests cover their own state and
timing behavior.  These tests do not establish a real display refresh
rate, frame retention, audible output on unsupported drivers, or DOSBox
performance parity.
