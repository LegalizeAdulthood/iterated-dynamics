<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Orbit Delay Timer

This note records the timing behavior behind `orbitdelay`.  Exact
microsecond compatibility with DOS FRACTINT is no longer the display goal.
The useful behavior is visible orbit plotting and responsive input.

## User-Visible Meaning

FRACTINT documented `orbitdelay` in 1/10000 second units.

```text
orbitdelay=1 -> 100 microseconds
```

Id keeps that numeric scale for values large enough to create visible
display pacing.  For orbit display:

- `orbitdelay=0` means no display pacing.
- `orbitdelay=1` through `orbitdelay=138` run as fast as practical.
- `orbitdelay=139` is the first value that requests visible pacing.
- `orbitdelay=139` and higher are interpreted in 1/10000 second units per
  displayed orbit point.

The threshold comes from a 72 Hz display refresh rate:

```text
1 / 72 seconds = 13.888... milliseconds
139 * 100 microseconds = 13.9 milliseconds
```

Input polling during orbit display is also throttled to that display
cadence.  Polling the keyboard more frequently than visible presentation
does not improve interaction and can dominate the cost of plotting.

Sound uses `orbitdelay` differently.  It is a tone-duration control, not a
reason for rendering to block.  When sound output is restored, the value
should be applied while constructing or scheduling audio output, such as a
WAV buffer, instead of sleeping inside the renderer.

## Historical FRACTINT Behavior

DOS FRACTINT used a microsecond timer and a busy deadline loop.

The relevant shape was:

```text
wait_until(index, orbit_delay):
    while usec_clock() < next_time[index]:
        if keypressed():
            break
    next_time[index] = now + orbit_delay * 100
```

The DOS `usec_clock()` implementation read the BIOS tick count and the
8253 timer.  It did not yield the thread while waiting.  That made small
values such as `orbitdelay=1` behave like a CPU-burning 100 us deadline
wait.

That behavior was useful on old hardware, but it is not a good target for
modern windowed display.  Modern GUI presentation, message pumping, and
input dispatch overhead can cost more than those tiny delays.

## Current Id Behavior

The UI orbit display path in `libid/ui/standard_orbit_plot.cpp` applies
the display policy:

- convert `orbitdelay` to zero when it is below the 72 Hz threshold;
- use the orbit delay timer for values at or above that threshold;
- dispatch pending keyboard input at most once per 72 Hz interval while
  orbit display pacing is active.

The platform timer implementations are selected by CMake:

- Windows uses `QueryPerformanceCounter()` for deadline measurement.
- Linux uses `clock_gettime(CLOCK_MONOTONIC, ...)`.
- BSD uses `clock_gettime(CLOCK_MONOTONIC, ...)`.
- macOS uses `std::chrono::steady_clock`.

The current implementations still include high-resolution deadline support
from the earlier compatibility attempt.  With the new display semantics,
busy-spin precision is no longer important for orbit display and can be
simplified later.

Older delay paths remain until the remaining inversion-of-control slices
remove engine-side waiting.

## Timing Goals

Orbit display pacing should preserve these properties:

- small positive values run at full practical display speed;
- `orbitdelay=139` is the first deliberate visible display delay;
- larger values scale in the documented 1/10000 second units;
- pending keyboard input is noticed at display-refresh cadence;
- display pacing policy remains in `libid/ui`;
- fractal calculation code does not sleep or poll input for display;
- sound timing is handled by audio output code, not render blocking.

## Remaining Work

- Remove busy-spin behavior from orbit display timer implementations once
  all callers follow the 72 Hz display policy.
- Remove `wait_until()` from engine and fractal rendering paths.
- Keep sound timing separate from orbit display pacing when sound output is
  restored.
