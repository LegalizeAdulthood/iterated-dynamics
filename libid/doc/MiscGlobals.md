<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Miscellaneous Global Variables

Inventory of public `g_*` declarations in `libid/include/misc`.
`misc` is a collection of support responsibilities, not a coherent
architectural layer.  Its globals currently cover debugging, driver
selection, compatibility versioning, and stack diagnostics.

## Debug Selection

Header: `debug_flags.h`.

- `g_debug_flag` - selected `DebugFlags` value.

Debug values are numeric modes, not independent feature bits to combine.
Most callers compare against one named value; some test a numeric range
or any nonzero value.  The existence of bitwise helper operators does not
make arbitrary combinations meaningful.

For example:

```cpp
if (g_debug_flag == DebugFlags::FORCE_ARBITRARY_PRECISION_MATH)
{
    // Select the supported arbitrary-precision path.
}
```

The PAR-output precision range is 700 through 719.  The enum name
`FORCE_PRECISION_20_DIGITS` at 720 is its exclusive upper bound, not a
working 20-digit selection.

The current enum and its callers define the supported behavior.
[Debug Flags](../../home/doc/debug-flags.md) documents the individual
values, including formula diagnostics, memory selection, compatibility
overrides, and numerical-path controls.  Do not retain removed flag names
or a second independently maintained behavior table here.

## Active Driver

Header: `Driver.h`.

- `g_driver` - raw `Driver *` identifying the active implementation;
  not a function pointer or ownership handle.

`misc/Driver.cpp` privately keeps the registered driver pointers.
`load_driver()` registers a successfully initialized driver and selects
the first usable one.  `init_drivers()` attempts the supplied set.
`driver_set_video_mode()` changes the active driver when required by the
mode.

`close_drivers()` terminates registered drivers in reverse initialization
order and clears `g_driver`.  It must not be preceded by another
termination of the active driver merely as part of ordinary shutdown.

The interface currently groups:

- Graphics pixels, spans, lines, and graphics-state operations.
- Palette reads and writes.
- Text screens, attributes, cursor placement, and screen stacking.
- Keyboard queue operations.
- Window creation, resizing, and display-mode operations.
- Delays, event servicing, shell access, and diagnostics.
- Completion buzzers and continuous-tone methods.

Inline `driver_*` helpers delegate through `g_driver`.  This broad API
still exposes more capabilities than an individual consumer needs.
Calculation code uses output operations but does not poll keyboard or
mouse input.  UI owns input interpretation and interaction.

Orbit sound calculation instead uses the narrow `engine::SoundOutput`
port.  `ui::DriverSoundOutput` implements driver-backed playback and
optional tone recording.  Merely retaining continuous-tone driver
methods does not imply that every platform supports them.

## Compatibility Version

Header: `version.h`.

- `g_version` - active compatibility version, represented by `Version`
  with major, minor, patch, tweak, and legacy-format fields.

The active compatibility version can differ from the running executable's
release.  `current_id_version()` reports the latter;
`reset_version_to_current()` resets the compatibility setting.
The loaded image's producer version is the separate `io::g_file_version`.

## Stack Diagnostic Anchor

Header: `stack_avail.h`.

- `g_top_of_stack` - stack-address anchor used by `stack_avail()`.

This diagnostic is not the arbitrary-precision scratch allocator.
That allocator and its offset/high-water globals are documented in
[MathGlobals.md](MathGlobals.md).

## Ownership And Future Direction

These declarations are shared state, not thread-local contexts.
A driver registry, debug selection, compatibility version, and stack
anchor have different lifetimes and consumers; they should not be
collected into a single replacement context merely because they reside
in `misc`.

The driver is currently a broad platform adapter rather than a segregated
set of ports.  [hexagonal.md](hexagonal.md) records the proposed
consumer-driven split and the rule that existing platform implementations
can bridge narrower interfaces during migration.
