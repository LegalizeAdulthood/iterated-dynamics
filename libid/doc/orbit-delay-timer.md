<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Orbit Delay Timer

This note records the timing behavior behind `orbitdelay` and the
possible directions for making it closer to DOS FRACTINT.  It does not
choose an implementation.

## User-Visible Meaning

The `orbitdelay` parameter is documented by FRACTINT as a delay in
1/10000 second units.

```text
orbitdelay=1 -> 100 microseconds -> 100000 nanoseconds
```

The delay is applied per displayed orbit point, not once for the entire
orbit.  During `showorbit=yes`, that can mean many small delays per
pixel.

## FRACTINT Behavior

DOS FRACTINT uses a microsecond timer and a busy deadline loop.

The relevant shape is:

```text
wait_until(index, orbit_delay):
    while usec_clock() < next_time[index]:
        if keypressed():
            break
    next_time[index] = now + orbit_delay * 100
```

The DOS `usec_clock()` implementation reads the BIOS tick count and the
8253 timer.  It does not yield the thread while waiting.  That makes
small values such as `orbitdelay=1` behave like a CPU-burning 100 us
deadline wait.

FRACTINT also retains an older calibrated delay-loop implementation
behind `debug=4020`.  The default code path is the microsecond timer.

## Current Id Behavior

Id currently has two related delay paths.

The UI orbit display path in `libid/ui/standard_orbit_plot.cpp` uses:

```text
std::chrono::steady_clock
std::this_thread::sleep_for()
dispatch_pending_keyboard_input()
```

The older engine helper in `libid/engine/wait_until.cpp` uses:

```text
std::chrono::steady_clock
std::this_thread::sleep_for()
driver_key_pressed()
```

The driver is used only for input polling in the older path.  The timing
itself is not driver-owned.

On Windows, `sleep_for(100us)` should not be treated as a precise
100 us delay.  It yields to the OS scheduler and resumes after timer and
scheduling latency.  The actual wait can be closer to milliseconds than
microseconds.  Because orbit delay is applied per displayed point, that
rounding can multiply into very large render-time differences.

POSIX platforms can show the same class of problem with `nanosleep()` or
`clock_nanosleep()`: the argument may be tiny, but the actual wake-up is
still scheduler-limited.

## Timing Goals

Any replacement should preserve these properties:

- `orbitdelay=1` means a nominal 100 us cadence.
- pending keyboard input wakes the delay promptly.
- delay is applied per displayed orbit point.
- larger delays do not needlessly burn CPU.
- the timing mechanism is isolated from fractal calculation code.
- platform-specific timing code is not buried in renderer control flow.

## Platform Clocks

Windows:

- `QueryPerformanceCounter()` is the usual high-resolution monotonic
  clock for deadline measurement.
- `CreateWaitableTimerEx()` with
  `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION` may improve sleeping waits, but
  wake-up still depends on scheduler behavior.
- `timeBeginPeriod(1)` can improve coarse timer resolution, but it does
  not make 100 us sleeps reliable and has system-wide cost.

Linux:

- `clock_gettime(CLOCK_MONOTONIC, ...)` is the portable monotonic clock.
- `CLOCK_MONOTONIC_RAW` can avoid some clock adjustments, but is less
  portable.
- `clock_nanosleep()` can sleep until an absolute deadline, but wake-up
  is still scheduler-limited.

BSD:

- `clock_gettime(CLOCK_MONOTONIC, ...)` is the portable monotonic clock.
- `CLOCK_MONOTONIC_FAST` or similar variants may exist on specific BSDs,
  but they are not portable across all BSD targets.
- `nanosleep()` or `clock_nanosleep()` availability and precision vary by
  platform.

macOS:

- `std::chrono::steady_clock` is typically backed by a monotonic system
  clock suitable for measuring deadlines.
- Native code can also use Mach absolute time APIs if a platform layer
  needs explicit control.
- Sleeping for sub-millisecond intervals is still scheduler-limited.

## Alternative Designs

### Pure Sleep

Use `sleep_for()`, `nanosleep()`, waitable timers, or similar APIs for
every delay.

Advantages:

- simple
- low CPU use
- friendly to multitasking and battery life

Disadvantages:

- poor match for DOS FRACTINT at small delays
- `orbitdelay=1` can become millisecond-scale
- performance differs strongly by OS and timer settings

### Pure Busy Deadline

Measure time with the platform high-resolution monotonic clock and spin
until the deadline, polling input while waiting.

Advantages:

- closest match to DOS FRACTINT
- good behavior for `orbitdelay=1`
- avoids scheduler oversleep

Disadvantages:

- burns CPU while waiting
- bad for long delays
- can affect responsiveness of other processes

### Hybrid Sleep And Spin

Sleep or yield while far from the deadline, then busy-wait for the final
short interval.

Example policy:

```text
if remaining > spin_window:
    sleep or yield briefly
else:
    poll input and busy-wait
```

Advantages:

- keeps small delays close to DOS behavior
- avoids burning CPU for large delays
- can be tuned per platform

Disadvantages:

- more complicated
- requires choosing a spin window
- platform differences still matter

### Driver Or Platform Timing Service

Move deadline timing behind a small platform service, possibly adjacent
to the driver layer.

Possible interface:

```text
reset_orbit_delay_timer()
wait_for_orbit_delay(units_100us, poll_input_callback)
```

Advantages:

- isolates platform-specific timing
- keeps UI render controllers readable
- gives Windows, Linux, BSD, and macOS separate implementations if needed
- preserves the current direction where UI owns input policy

Disadvantages:

- adds an abstraction
- still needs policy decisions for sleep, spin, and input polling

## Open Questions

- Should the spin window be fixed or platform-specific?
- Should small delays always busy-wait, or only `orbitdelay` values below
  a threshold?
- Should the platform timing service be part of the driver layer or a
  separate OS timing layer?
- Should the older engine `wait_until()` path be migrated to the same
  timing service before or after remaining orbit-display inversion work?
- Should sound delay and orbit-display delay share the same timer service
  or keep separate policies?
